/*
 * Scheduler priority management
 *
 * Copyright (C) 2015 Joakim Hernberg
 * Copyright (C) 2015 Sebastian Lackner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#define _GNU_SOURCE  /* for SCHED_BATCH, SCHED_IDLE */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifdef HAVE_SCHED_H
# include <sched.h>
#endif
#ifndef SCHED_RESET_ON_FORK
# define SCHED_RESET_ON_FORK 0x40000000
#endif
#ifndef SCHED_ISO
# define SCHED_ISO 4
#endif
#ifndef SCHED_IDLE
 #define SCHED_IDLE 5
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "thread.h"
#include "process.h"

#if defined(__linux__) && defined(HAVE_SYS_RESOURCE_H) && defined(HAVE_SCHED_H)

static int thread_base_priority_fifo = -1;
static int thread_base_priority_rr = -1;
static BOOL has_sched_iso = FALSE;
static int rlim_nice_max = 20; /* -1 = umlimited */

/* gets the priority value from an environment variable */
static int get_priority( const char *variable, const int min, const int max, const int def )
{
    const char *env;
    int val;

    env = getenv( variable );
    val = env ? atoi(env) : min( max, max( min, def ));

    if (val >= min && val <= max)
        return val;

    if (val == 0)
        fprintf( stderr, "wineserver: not using %s\n", variable );
    else
        fprintf( stderr, "wineserver: %s should be between %d and %d\n", variable, min, max );
    return -1;
}

/* initializes the scheduler */
void init_scheduler( void )
{
    struct rlimit rlim;
    struct sched_param param;
    int min, max, priority;

    /* SCHED_ISO is safe to use, thus we do not need to depend on an environment
     * variable. If it succeeds, we prefer it over SCHED_FIFO and use nice
     * priorities instead. SCHED_ISO provides realtime capabilities and low latency
     * for processes without compromising system stability. Processes running under
     * SCHED_ISO always run with highest priority and fall back to the nice value
     * if their CPU usage stays above a certain threshold during the last 5 seconds
     * averaged across all cores (/proc/sys/kernel/iso_cpu). This is supported by
     * kernels implementing the MuQSS scheduler or its variants (i.e. PDS).
     *
     * The STAGING_RT_* variables are still respected for the base priority of
     * processes but is used in a more safe way now.
     *
     * Using nice also affects IO priorities in the best effort class:
     * io_priority = (cpu_nice + 20) / 5
     */

    /* Let's detect the RLIMIT_NICE for the user running wineserver. The returned
     * rlim struct handles values from 1 to 40, so we need this formula to convert
     * to well-known nice levels:
     *
     * rlim_nice = 20 - rlim_max | with -1 = unlimited
     */
    getrlimit( RLIMIT_NICE, &rlim );
    rlim_nice_max = rlim.rlim_max == -1 ? 40 : rlim.rlim_max;
    if (debug_level)
        fprintf( stderr, "wineserver: detected RLIMIT_NICE = %d\n", 20 - rlim_nice_max );

    /* First, renice wineserver to the maximum nice level possible. This is useful
     * if SCHED_ISO is available as SCHED_ISO may fall back to SCHED_OTHER when
     * excessive CPU usage is detected.
     */
    errno = 0;
    if (setpriority( 0, PRIO_PROCESS, 20 - rlim_nice_max ) == -1)
    {
        fprintf( stderr, "wineserver: failed to change nice value to %d\n",
                 20 - rlim_nice_max );
    }

    /* Set and detect SCHED_ISO support. If SCHED_ISO is supported, we use that
     * mode for wineserver unconditionally because it is a safe choice. Processes
     * scheduled in this mode cannot freeze the system but will be scheduled
     * before other processes.
     */
    memset( &param, 0, sizeof(param) );
    if (sched_setscheduler( 0, SCHED_ISO | SCHED_RESET_ON_FORK, &param ) == -1)
        fprintf( stderr, "wineserver: SCHED_ISO not supported\n" );
    else
        has_sched_iso = TRUE;

    /* Change the wineserver priority to SCHED_FIFO through the classical
     * staging approach if SCHED_ISO is not supported.
     */
    min = sched_get_priority_min( SCHED_FIFO );
    max = sched_get_priority_max( SCHED_FIFO );
    if (min == -1 || max == -1)
    {
        fprintf( stderr, "wineserver: Could not detect SCHED_FIFO: %s\n", strerror(errno));
        goto skip_sched_fifo;
    }

    if (!has_sched_iso && ((priority = get_priority( "STAGING_RT_PRIORITY_SERVER", min, max, max-9 )) != -1))
    {
        memset( &param, 0, sizeof(param) );
        param.sched_priority = priority;
        if (sched_setscheduler( 0, SCHED_FIFO | SCHED_RESET_ON_FORK, &param ) == -1 &&
            sched_setscheduler( 0, SCHED_FIFO, &param ) == -1)
        {
            fprintf( stderr, "wineserver: failed to change priority to SCHED_FIFO/%d\n",
                     param.sched_priority );
        }
        else
        {
            if (debug_level)
                fprintf( stderr, "wineserver: changed priority to SCHED_FIFO/%d\n",
                         param.sched_priority );
        }
    }

    /* determine base priority which will be used for SCHED_FIFO threads */
    thread_base_priority_fifo = get_priority( "STAGING_RT_PRIORITY_BASE", min, max - 31, (min+max)/2 );
    if (thread_base_priority_fifo != -1)
        if (debug_level)
            fprintf( stderr, "wineserver: initialized SCHED_FIFO thread base priority to %d\n",
                     thread_base_priority_fifo );

skip_sched_fifo:
    min = sched_get_priority_min( SCHED_RR );
    max = sched_get_priority_max( SCHED_RR );
    if (min == -1 || max == -1)
    {
        fprintf( stderr, "wineserver: Could not detect SCHED_RR: %s\n", strerror(errno));
        return;
    }

    /* determine base priority which will be used for SCHED_RR threads */
    thread_base_priority_rr = get_priority( "STAGING_RT_PRIORITY_BASE", min, max - 31, (min+max)/2 );
    if (thread_base_priority_rr != -1)
        if (debug_level)
            fprintf( stderr, "wineserver: initialized SCHED_RR thread base priority to %d\n",
                     thread_base_priority_rr );
}

/* sets the scheduler priority of a windows thread */
void set_scheduler_priority( struct thread *thread )
{
    struct sched_param param;
    int policy = SCHED_OTHER;
    int nice = 20; // kernel interface = 20-nice: 40..1 ~ -20..19 (user space)
    int prio = 8, min_prio = 1, max_prio = 15; // Windows base priority and range
    struct process *process = thread->process;

    if (thread->unix_tid == -1) return;

    /* Try to mimic the Windows process priority class with Linux scheduling policies.
     * https://docs.microsoft.com/en-us/windows/desktop/procthread/scheduling-priorities
     *
     * Each priority class starts from a different base priority:
     *
     * Priorities 1-15 shall be dynamic priorities as Linux uses with SCHED_OTHER and nice.
     *
     * Priorities 16-31 shall be static priorities as Linux uses with SCHED_{FIFO,RR}.
     *
     * The thread priority is then just added to the base priority (within the bounds of
     * its class), and the result is used for programming the Linux scheduler priorities.
     *
     * The multimedia realtime classes shall reserve a CPU bandwidth of 20% for other
     * processes. We cannot currently do this correctly here, so we try to use SCHED_ISO
     * instead which runs as realtime process for 70% of CPU usage, and if its usage
     * is above that threshold for more than 5 seconds, it falls back to SCHED_OTHER.
     * We exploit that behavior for priority 15, as this priority is a suitable
     * heuristic in current Wine for multimedia class scheduling. If SCHED_ISO is not
     * available, our decision degrades gracefully.
     *
     * In contrast to the previous implementation, we also do not base the scheduler
     * off the Windows thread priority but solely the Windows process priority class.
     * This seems to be more correct. As stated above, the only exception here is
     * priority 15 as a heuristic for multimedia workloads. Current Wine is lacking
     * properly implemented AVRT and MMCSS functions. Built-in xaudio seems to use
     * thread priorities while native xaudio seems to use AVRT functions. Adding
     * AVRT support is handled in another commit. Thus, native xaudio cannot benefit
     * from this without touching the AVRT implementation.
     */

    if (process->priority == PROCESS_PRIOCLASS_IDLE)
    {
        policy = SCHED_IDLE;
        prio = 4;
    }
    else if (process->priority == PROCESS_PRIOCLASS_BELOW_NORMAL)
    {
        /* This is technically not correct as it changes the timeslice behavior of
         * the process but it is still a good compromise as it gives processes
         * a slight penalty compared to SCHED_OTHER.
         */
        policy = SCHED_BATCH;
        prio = 6;
    }
    else if (process->priority == PROCESS_PRIOCLASS_NORMAL)
    {
        policy = SCHED_OTHER;
        prio = 8;
    }
    else if (process->priority == PROCESS_PRIOCLASS_ABOVE_NORMAL)
    {
#if 0   // DOOM 2016 does not like SCHED_RR here
        /* Prefer SCHED_ISO if it is supported */
        policy = has_sched_iso ? SCHED_ISO : SCHED_RR;
#else
        /* Prefer SCHED_ISO if it is supported */
        policy = has_sched_iso ? SCHED_ISO : SCHED_OTHER;
#endif
        prio = 10;
    }
    else if (process->priority == PROCESS_PRIOCLASS_HIGH)
    {
#if 0   // DOOM 2016 does not like SCHED_RR here
        /* Prefer SCHED_ISO if SCHED_RR is not supported */
        policy = (thread_base_priority_rr == -1) ? SCHED_ISO : SCHED_RR;
#else
        /* Prefer SCHED_ISO if it is supported */
        policy = has_sched_iso ? SCHED_ISO : SCHED_OTHER;
#endif
        prio = 13;
    }
    else if (process->priority == PROCESS_PRIOCLASS_REALTIME)
    {
        policy = (thread_base_priority_fifo == -1) ? SCHED_RR : SCHED_FIFO;
        min_prio = 16;
        prio = 24;
        max_prio = 31;
    }

    /* Prefer SCHED_ISO for LOWRT prio */
    // TODO: This should probably go away when everything can use AVRT properly.
    if ((prio == THREAD_BASE_PRIORITY_LOWRT) && (policy == SCHED_OTHER))
        policy = SCHED_ISO;

    /* Prefer SCHED_BATCH for lowest prio */
    // TODO: This should probably go away at some point.
    if ((thread->priority <= THREAD_BASE_PRIORITY_MIN) && (policy == SCHED_OTHER))
        policy = SCHED_BATCH;

    /* Downgrade SCHED_FIFO to SCHED_RR if it's not supported */
    if ((thread_base_priority_fifo == -1) && (policy == SCHED_FIFO))
        policy = SCHED_RR;

    /* Downgrade SCHED_RR to SCHED_ISO if it's not supported */
    if ((thread_base_priority_rr == -1) && (policy == SCHED_RR))
        policy = SCHED_ISO;

    /* Downgrade SCHED_ISO to SCHED_OTHER if it's not supported */
    if (!has_sched_iso && (policy == SCHED_ISO))
        policy = SCHED_OTHER;

    /* Calculate nice priority from Windows priority */
    if (prio >= 16)
        nice = 20 - max(1, min(rlim_nice_max, 9 + prio));
    else
        nice = 20 - max(1, min(rlim_nice_max, 12 + prio));

    /* Calculate linux static priority parameter */
    switch (policy)
    {
        case SCHED_FIFO:
        {
            prio = thread_base_priority_fifo + prio;
            break;
        }
        case SCHED_RR:
        {
            prio = thread_base_priority_rr + prio;
            break;
        }
        default:
        {
            /* Other schedulers have no concept of static priorities */
            prio = 0;
        }
    }

    if (debug_level)
        fprintf( stderr, "%04x: set_scheduler_priority (tid:%d,class:%d,threadprio:%d,nice:%d,sched:%d/%d)\n",
                 thread->id, thread->unix_tid, process->priority, thread->priority, nice, policy, prio);

    /* According to "man setpriority", only non-realtime schedulers are affected by setpriority().
     * If a process later reverts to SCHED_OTHER it shall see its nice priority untouched, read, it
     * should not be affected by calls to setpriority() while it was scheduled as realtime process.
     * Thus, we need to eventually first set SCHED_OTHER, then adjust the nice value. Otherwise the
     * nice value may be reverted to some previous value upon reverting the policy.
     */
    memset( &param, 0, sizeof(param) );
    param.sched_priority = prio;
    if (sched_setscheduler(thread->unix_tid, policy | SCHED_RESET_ON_FORK, &param) == -1 &&
        sched_setscheduler(thread->unix_tid, policy, &param) == -1)
    {
        static int once = 0;
        if (debug_level || !once++)
            fprintf( stderr, "%04x: failed to change priority to %d/%d: %s\n",
                     thread->id, policy, param.sched_priority, strerror(errno) );
    }
    else
        if (debug_level)
            fprintf( stderr, "%04x: changed priority to %d/%d\n",
                     thread->id, policy, param.sched_priority );

    /* Set the nice level. */
    errno = 0;
    if ((setpriority( PRIO_PROCESS, thread->unix_tid, nice ) == -1) && (errno != 0))
    {
        static int once = 0;
        if (debug_level || !once++)
            fprintf( stderr, "%04x: failed to change nice value to %d: %s\n",
                     thread->id, nice, strerror(errno) );
    }
    else
        if (debug_level)
            fprintf( stderr, "%04x: changed nice value to %d\n",
                     thread->id, getpriority( PRIO_PROCESS, thread->unix_tid ));
}

#else

void init_scheduler( void )
{
}

void set_scheduler_priority( struct thread *thread )
{
}

#endif
