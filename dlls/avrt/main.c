/* Avrt dll implementation
 *
 * Copyright (C) 2009 Maarten Lankhorst
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/list.h"
#include "wine/server.h"
#include "avrt.h"

#define AVRT_INVALID_HANDLE 0

enum avrt_task_type
{
    AVRT_TASK_TYPE_NONE,
    AVRT_TASK_TYPE_MMCSS,
};

enum avrt_scheduling_category
{
    AVRT_SC_LOW,
    AVRT_SC_MEDIUM,
    AVRT_SC_HIGH,
};

typedef struct _AVRT_TASK
{
    enum avrt_task_type  type;
    void                *object;
    struct list          entry;
} AVRT_TASK, *PAVRT_TASK;

static struct list avrt_tasks = LIST_INIT(avrt_tasks);

typedef struct _AVRT_TASK_MMCSS
{
    DWORD index;
    DWORD affinity;
    BOOL background_only;
    BYTE background_priority;
    DWORD clock_rate;
    BYTE gpu_priority;
    BYTE priority;
    enum avrt_scheduling_category scheduling_category;
} AVRT_TASK_MMCSS, *PAVRT_TASK_MMCSS;

WINE_DEFAULT_DEBUG_CHANNEL(avrt);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}

/* Handle table functions */
static void *avrt_get_object(HANDLE handle, enum avrt_task_type type)
{
    AVRT_TASK *task = handle;

    if (handle == AVRT_INVALID_HANDLE)
    {
        WARN("Invalid handle %p passed.\n", handle);
        return NULL;
    }

    if (task->type != type)
    {
        WARN("Handle %p is not of type %#x.\n", handle, type);
        return NULL;
    }

    return task->object;
}

static HANDLE avrt_allocate_task(void *object, enum avrt_task_type type)
{
    AVRT_TASK *task = heap_alloc_zero(sizeof(AVRT_TASK));

    if (!task)
    {
        ERR("Failed to allocate handle table memory.\n");
        SetLastError(ERROR_OUTOFMEMORY);
        return AVRT_INVALID_HANDLE;
    }

    task->type = type;
    task->object = object;
    list_add_tail(&avrt_tasks, &task->entry);

    return (HANDLE)task;
}

static void *avrt_free_task(HANDLE handle, enum avrt_task_type type)
{
    AVRT_TASK *task = handle;
    void *object = avrt_get_object(handle, type);

    if (object)
    {
        list_remove(&task->entry);
        heap_free(task);
    }

    return object;
}

HANDLE WINAPI AvSetMmThreadCharacteristicsA(LPCSTR TaskName, LPDWORD TaskIndex)
{
    HANDLE ret;
    LPWSTR str = NULL;

    if (TaskName)
    {
        DWORD len = (lstrlenA(TaskName)+1);
        str = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        if (!str)
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return NULL;
        }
        MultiByteToWideChar(CP_ACP, 0, TaskName, len, str, len);
    }
    ret = AvSetMmThreadCharacteristicsW(str, TaskIndex);
    HeapFree(GetProcessHeap(), 0, str);
    return ret;
}

/***********************************************************************
 *           AvSetMmThreadCharacteristicsW [AVRT.@]
 *
 * Creates a task instance with the specified scheduling characteristic.
 *
 * PARAMS
 *  task_name    [I] A task name as defined in the system profile.
 *  task_index   [O] An index identifying the thread group of this task.
 *
 * RETURNS
 *  Success: AVRT task handle.
 *  Failure: NULL.
 *
 * NOTES
 *  According to patent US7802256, the task index is an allocated index
 *  identifying the threading group of this task. This property should be
 *  inherited by child threads to identify all threads belonging to this
 *  specific task. Adjusting priorities later would affect all threads with
 *  the same task index then. For now, we just ignore this: This grouping is
 *  currently not embedded into wineserver. We will just create an index
 *  value and won't ever inherit it to child threads. If this is needed at
 *  some point, cgroups may be a proper way to group processes for the
 *  linux scheduler and also enable bandwidth reservations and guarantees.
 */
HANDLE WINAPI AvSetMmThreadCharacteristicsW(LPCWSTR task_name, LPDWORD task_index)
{
    AVRT_TASK_MMCSS *object = NULL;
    AVRT_TASK *task = NULL;
    DWORD index = GetCurrentThreadId();
    HANDLE current_thread = GetCurrentThread();
    BOOL existing_task = FALSE;
    NTSTATUS ret;

    FIXME("(%s)->(%p)\n", debugstr_w(task_name), task_index);

    if (!task_name)
    {
        SetLastError(ERROR_INVALID_TASK_NAME);
        return NULL;
    }

    if (!task_index)
    {
        SetLastError(ERROR_INVALID_TASK_INDEX);
        return NULL;
    }

    if (*task_index != 0)
    {
        LIST_FOR_EACH_ENTRY(task, &avrt_tasks, AVRT_TASK, entry)
        {
            object = avrt_get_object(task, AVRT_TASK_TYPE_MMCSS);
            if (object && (object->index == *task_index))
            {
                existing_task = TRUE;
                goto setup_task;
            }
        }

        SetLastError(ERROR_INVALID_TASK_INDEX);
        return NULL;
    }

    object = heap_alloc_zero(sizeof(*object));
    if (!object)
        goto fail;

    object->index = index;

    task = avrt_allocate_task(object, AVRT_TASK_TYPE_MMCSS);
    if (task != AVRT_INVALID_HANDLE)
        goto setup_task;

fail:
    if (!existing_task)
    {
        heap_free(task);
        heap_free(object);
    }
    return NULL;

setup_task:
    FIXME("not using MMCSS (TaskIndex=%d)\n", index);
    SERVER_START_REQ(set_thread_mmcss_priority)
    {
        req->handle = wine_server_obj_handle(current_thread);
        req->mmcss_priority = 23;
        ret = wine_server_call(req);
    }
    SERVER_END_REQ;

    if (ret)
    {
        SetLastError(ret);
        goto fail;
    }

    return task;
}

BOOL WINAPI AvQuerySystemResponsiveness(HANDLE AvrtHandle, ULONG *value)
{
    FIXME("(%p, %p): stub\n", AvrtHandle, value);
    return FALSE;
}

BOOL WINAPI AvRevertMmThreadCharacteristics(HANDLE handle)
{
    DWORD index = GetCurrentThreadId();
    HANDLE current_thread = GetCurrentThread();
    AVRT_TASK_MMCSS *object;
    NTSTATUS ret;

    TRACE("(%p)\n", handle);

    object = avrt_free_task(handle, AVRT_TASK_TYPE_MMCSS);

    if (!object)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (object->index != index)
    {
        SetLastError(ERROR_INVALID_TASK_INDEX);
        return FALSE;
    }

    FIXME("not using MMCSS\n");
    SERVER_START_REQ(set_thread_mmcss_priority)
    {
        req->handle = wine_server_obj_handle(current_thread);
        req->mmcss_priority = 0;
        ret = wine_server_call(req);
    }
    SERVER_END_REQ;

    if (ret)
    {
        SetLastError(ret);
        return FALSE;
    }

    heap_free(object);
    return TRUE;
}

BOOL WINAPI AvSetMmThreadPriority(HANDLE handle, AVRT_PRIORITY prio)
{
    FIXME("(%p, %u)\n", handle, prio);

#if 0
    AVRT_TASK_MMCSS *object;

    object = avrt_get_object(handle, AVRT_TASK_TYPE_MMCSS);

    switch (prio)
    {
        case AVRT_PRIORITY_LOW:
        {
            prio = THREAD_PRIORITY_LOWEST;
            break;
        }
        case AVRT_PRIORITY_NORMAL:
        {
            prio = THREAD_PRIORITY_NORMAL;
            break;
        }
        case AVRT_PRIORITY_HIGH:
        {
            prio = THREAD_PRIORITY_HIGHEST;
            break;
        }
        case AVRT_PRIORITY_CRITICAL:
        {
            prio = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        }
        default:
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    return SetThreadPriority(entry->thread, prio);
#else
    return TRUE;
#endif
}
