/*
 * Unit test suite for mfplat.
 *
 * Copyright 2015 Michael Müller
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

#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"

#include "initguid.h"
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mfreadwrite.h"
#include "propvarutil.h"
#include "strsafe.h"

#include "wine/test.h"

static HRESULT (WINAPI *pMFCreateSourceResolver)(IMFSourceResolver **resolver);
static HRESULT (WINAPI *pMFCreateMFByteStreamOnStream)(IStream *stream, IMFByteStream **bytestream);
static HRESULT (WINAPI *pMFCreateMemoryBuffer)(DWORD max_length, IMFMediaBuffer **buffer);

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

DEFINE_GUID(MF_BYTESTREAM_CONTENT_TYPE, 0xfc358289,0x3cb6,0x460c,0xa4,0x24,0xb6,0x68,0x12,0x60,0x37,0x5a);

DEFINE_GUID(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 0xa634a91c, 0x822b, 0x41b9, 0xa4, 0x94, 0x4d, 0xe4, 0x64, 0x36, 0x12, 0xb0);

DEFINE_GUID(MFT_CATEGORY_OTHER, 0x90175d57,0xb7ea,0x4901,0xae,0xb3,0x93,0x3a,0x87,0x47,0x75,0x6f);

DEFINE_GUID(DUMMY_CLSID, 0x12345678,0x1234,0x1234,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19);
DEFINE_GUID(DUMMY_GUID1, 0x12345678,0x1234,0x1234,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21);
DEFINE_GUID(DUMMY_GUID2, 0x12345678,0x1234,0x1234,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22);
DEFINE_GUID(DUMMY_GUID3, 0x12345678,0x1234,0x1234,0x23,0x23,0x23,0x23,0x23,0x23,0x23,0x23);

static const WCHAR mp4file[] = {'t','e','s','t','.','m','p','4',0};

static BOOL is_prewin8(void)
{
    DWORD version = GetVersion();
    DWORD major = (DWORD)(LOBYTE(LOWORD(version)));
    DWORD minor = (DWORD)(HIBYTE(LOWORD(version)));

    if(major < 6)
        return TRUE;
    else if(major == 6 && minor < 2)
        return TRUE;
    else
        return FALSE;
}

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    lstrcatW(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0,
                       NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %d\n",
       wine_dbgstr_w(pathW), GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(res != 0, "couldn't find resource\n");
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource(GetModuleHandleA(NULL), res),
               &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res),
       "couldn't write resource\n" );
    CloseHandle(file);

    return pathW;
}

static BOOL check_clsid(CLSID *clsids, UINT32 count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (IsEqualGUID(&clsids[i], &DUMMY_CLSID))
            return TRUE;
    }
    return FALSE;
}

static void test_register(void)
{
    static WCHAR name[] = {'W','i','n','e',' ','t','e','s','t',0};
    MFT_REGISTER_TYPE_INFO input[] =
    {
        { DUMMY_CLSID, DUMMY_GUID1 }
    };
    MFT_REGISTER_TYPE_INFO output[] =
    {
        { DUMMY_CLSID, DUMMY_GUID2 }
    };
    CLSID *clsids;
    UINT32 count;
    HRESULT ret;

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 1, input, 1, output, NULL);
    if (ret == E_ACCESSDENIED)
    {
        win_skip("Not enough permissions to register a filter\n");
        return;
    }
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

if(0)
{
    /* NULL name crashes on windows */
    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, NULL, 0, 1, input, 1, output, NULL);
    ok(ret == E_INVALIDARG, "got %x\n", ret);
}

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 0, NULL, 0, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 1, NULL, 0, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 0, NULL, 1, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

if(0)
{
    /* NULL clsids/count crashes on windows (vista) */
    count = 0;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, NULL, &count);
    ok(ret == E_POINTER, "Failed to enumerate filters: %x\n", ret);
    ok(count == 0, "Expected count > 0\n");

    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, &clsids, NULL);
    ok(ret == E_POINTER, "Failed to enumerate filters: %x\n", ret);
    ok(count == 0, "Expected count > 0\n");
}

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, input, NULL, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, output, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, input, output, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    /* exchange input and output */
    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, output, input, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(!count, "got %d\n", count);
    ok(clsids == NULL, "Expected clsids == NULL\n");

    ret = MFTUnregister(DUMMY_CLSID);
    ok(ret == S_OK ||
       /* w7pro64 */
       broken(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)), "got %x\n", ret);

    ret = MFTUnregister(DUMMY_CLSID);
    ok(ret == S_OK || broken(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)), "got %x\n", ret);
}

static void test_source_resolver(void)
{
    IMFSourceResolver *resolver, *resolver2;
    IMFByteStream *bytestream;
    IMFAttributes *attributes;
    IMFMediaSource *mediasource;
    IMFPresentationDescriptor *descriptor;
    MF_OBJECT_TYPE obj_type;
    HRESULT hr;
    WCHAR *filename;

    static const WCHAR file_type[] = {'v','i','d','e','o','/','m','p','4',0};

    if (!pMFCreateSourceResolver)
    {
        win_skip("MFCreateSourceResolver() not found\n");
        return;
    }

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = pMFCreateSourceResolver(NULL);
    ok(hr == E_POINTER, "got %#x\n", hr);

    hr = pMFCreateSourceResolver(&resolver);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = pMFCreateSourceResolver(&resolver2);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(resolver != resolver2, "Expected new instance\n");

    IMFSourceResolver_Release(resolver2);

    filename = load_resource(mp4file);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, NULL, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        &obj_type, (IUnknown **)&mediasource);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, bytestream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        NULL, (IUnknown **)&mediasource);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, bytestream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        &obj_type, NULL);
    ok(hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, bytestream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        &obj_type, (IUnknown **)&mediasource);
    todo_wine ok(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE, "got 0x%08x\n", hr);
    if (hr == S_OK) IMFMediaSource_Release(mediasource);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, bytestream, NULL, MF_RESOLUTION_BYTESTREAM, NULL,
        &obj_type, (IUnknown **)&mediasource);
    todo_wine ok(hr == MF_E_UNSUPPORTED_BYTESTREAM_TYPE, "got 0x%08x\n", hr);

    IMFByteStream_Release(bytestream);

    /* We have to create a new bytestream here, because all following
     * calls to CreateObjectFromByteStream will fail. */
    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IUnknown_QueryInterface(bytestream, &IID_IMFAttributes,
                                 (void **)&attributes);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IMFAttributes_SetString(attributes, &MF_BYTESTREAM_CONTENT_TYPE, file_type);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IMFAttributes_Release(attributes);

    hr = IMFSourceResolver_CreateObjectFromByteStream(
        resolver, bytestream, NULL, MF_RESOLUTION_MEDIASOURCE, NULL,
        &obj_type, (IUnknown **)&mediasource);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(mediasource != NULL, "got %p\n", mediasource);
    ok(obj_type == MF_OBJECT_MEDIASOURCE, "got %d\n", obj_type);

    hr = IMFMediaSource_CreatePresentationDescriptor(
        mediasource, &descriptor);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(descriptor != NULL, "got %p\n", descriptor);

    IMFPresentationDescriptor_Release(descriptor);
    IMFMediaSource_Release(mediasource);
    IMFByteStream_Release(bytestream);

    IMFSourceResolver_Release(resolver);

    MFShutdown();

    DeleteFileW(filename);
}

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mfplat.dll");

#define X(f) if (!(p##f = (void*)GetProcAddress(mod, #f))) return;
    X(MFCreateSourceResolver);
    X(MFCreateMFByteStreamOnStream);
    X(MFCreateMemoryBuffer);
#undef X
}

static void test_MFCreateMediaType(void)
{
    HRESULT hr;
    IMFMediaType *mediatype;

    hr = MFStartup(MAKELONG( MF_API_VERSION, 0xdead ), MFSTARTUP_FULL);
    ok(hr == MF_E_BAD_STARTUP_VERSION, "got 0x%08x\n", hr);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

if(0)
{
    /* Crash on Windows Vista/7 */
    hr = MFCreateMediaType(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
}

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);

    IMFMediaType_Release(mediatype);

    MFShutdown();
}

static void test_MFCreateMediaEvent(void)
{
    HRESULT hr;
    IMFMediaEvent *mediaevent;

    MediaEventType type;
    GUID extended_type;
    HRESULT status;
    PROPVARIANT value;

    PropVariantInit(&value);
    value.vt = VT_UNKNOWN;

    hr = MFCreateMediaEvent(MEError, &GUID_NULL, E_FAIL, &value, &mediaevent);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    PropVariantClear(&value);

    hr = IMFMediaEvent_GetType(mediaevent, &type);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(type == MEError, "got %#x\n", type);

    hr = IMFMediaEvent_GetExtendedType(mediaevent, &extended_type);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualGUID(&extended_type, &GUID_NULL), "got %s\n",
       wine_dbgstr_guid(&extended_type));

    hr = IMFMediaEvent_GetStatus(mediaevent, &status);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(status == E_FAIL, "got 0x%08x\n", status);

    PropVariantInit(&value);
    hr = IMFMediaEvent_GetValue(mediaevent, &value);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(value.vt == VT_UNKNOWN, "got %#x\n", value.vt);
    PropVariantClear(&value);

    IMFMediaEvent_Release(mediaevent);

    hr = MFCreateMediaEvent(MEUnknown, &DUMMY_GUID1, S_OK, NULL, &mediaevent);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaEvent_GetType(mediaevent, &type);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(type == MEUnknown, "got %#x\n", type);

    hr = IMFMediaEvent_GetExtendedType(mediaevent, &extended_type);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(IsEqualGUID(&extended_type, &DUMMY_GUID1), "got %s\n",
       wine_dbgstr_guid(&extended_type));

    hr = IMFMediaEvent_GetStatus(mediaevent, &status);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(status == S_OK, "got 0x%08x\n", status);

    PropVariantInit(&value);
    hr = IMFMediaEvent_GetValue(mediaevent, &value);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(value.vt == VT_EMPTY, "got %#x\n", value.vt);
    PropVariantClear(&value);

    IMFMediaEvent_Release(mediaevent);
}

#define CHECK_COUNT(obj,expected) _check_count(obj, expected, __LINE__)
static void _check_count(IMFAttributes* obj, ULONG expected, int line)
{
    UINT32 count = 9999;
    HRESULT hr;
    hr = IMFAttributes_GetCount(obj, &count);
    ok_(__FILE__,line)(hr == S_OK, "IMFAttributes_GetCount failed: 0x%08x.\n", hr);
    ok_(__FILE__,line)(count == expected, "got %d, expected %d.\n", count, expected);
}

static void test_MFCreateAttributes(void)
{
    IMFAttributes *attributes;
    HRESULT hr;
    UINT32 uint32_value, string_length = 0;
    UINT64 uint64_value;
    double double_value;
    const static WCHAR stringW[] = {'W','i','n','e',0};
    WCHAR bufferW[256] = {0};
    WCHAR *allacted_string = NULL;

    hr = MFCreateAttributes( &attributes, 3 );
    ok(hr == S_OK, "got 0x%08x\n", hr);
    CHECK_COUNT(attributes, 0);

    hr = IMFAttributes_SetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 123);
    ok(hr == S_OK, "IMFAttributes_SetUINT32 failed: 0x%08x.\n", hr);
    CHECK_COUNT(attributes, 1);

    uint32_value = 0xdeadbeef;
    hr = IMFAttributes_GetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &uint32_value);
    ok(hr == S_OK, "IMFAttributes_GetUINT32 failed: 0x%08x.\n", hr);
    ok(uint32_value == 123, "got wrong value: %d, expected: 123.\n", uint32_value);

    uint64_value = 0xdeadbeef;
    hr = IMFAttributes_GetUINT64(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &uint64_value);
    ok(hr == MF_E_INVALIDTYPE, "IMFAttributes_GetUINT64 should fail: 0x%08x.\n", hr);
    ok(uint64_value == 0xdeadbeef, "got wrong value: %lld, expected: 0xdeadbeef.\n", uint64_value);

    hr = IMFAttributes_SetUINT64(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 65536);
    ok(hr == S_OK, "IMFAttributes_SetUINT64 failed: 0x%08x.\n", hr);
    CHECK_COUNT(attributes, 1);

    hr = IMFAttributes_GetUINT64(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &uint64_value);
    ok(hr == S_OK, "IMFAttributes_GetUINT64 failed: 0x%08x.\n", hr);
    ok(uint64_value == 65536, "got wrong value: %lld, expected: 65536.\n", uint64_value);

    uint32_value = 0xdeadbeef;
    hr = IMFAttributes_GetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &uint32_value);
    ok(hr == MF_E_INVALIDTYPE, "IMFAttributes_GetUINT32 should fail: 0x%08x.\n", hr);
    ok(uint32_value == 0xdeadbeef, "got wrong value: %d, expected: 0xdeadbeef.\n", uint32_value);

    hr = IMFAttributes_SetDouble(attributes, &GUID_NULL, 22.0);
    ok(hr == S_OK, "IMFAttributes_SetDouble failed: 0x%08x.\n", hr);
    CHECK_COUNT(attributes, 2);

    double_value = 0xdeadbeef;
    hr = IMFAttributes_GetDouble(attributes, &GUID_NULL, &double_value);
    ok(hr == S_OK, "IMFAttributes_GetDouble failed: 0x%08x.\n", hr);
    ok(double_value == 22.0, "got wrong value: %f, expected: 22.0.\n", double_value);

    hr = IMFAttributes_SetString(attributes, &DUMMY_GUID1, stringW);
    ok(hr == S_OK, "IMFAttributes_SetString failed: 0x%08x.\n", hr);
    CHECK_COUNT(attributes, 3);
    hr = IMFAttributes_GetStringLength(attributes, &DUMMY_GUID1, &string_length);
    ok(hr == S_OK, "IMFAttributes_GetStringLength failed: 0x%08x.\n", hr);
    ok(string_length == lstrlenW(stringW), "got wrong string length: %d.\n", string_length);
    string_length = 0xdeadbeef;
    hr = IMFAttributes_GetAllocatedString(attributes, &DUMMY_GUID1, &allacted_string, &string_length);
    ok(hr == S_OK, "IMFAttributes_GetAllocatedString failed: 0x%08x.\n", hr);
    ok(!lstrcmpW(allacted_string, stringW), "got wrong string: \"%s\".\n", wine_dbgstr_w(allacted_string));
    ok(string_length == lstrlenW(stringW), "got wrong string length: %d.\n", string_length);
    CoTaskMemFree(allacted_string);
    string_length = 0xdeadbeef;
    hr = IMFAttributes_GetString(attributes, &DUMMY_GUID1, bufferW, ARRAY_SIZE(bufferW), &string_length);
    ok(hr == S_OK, "IMFAttributes_GetString failed: 0x%08x.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    ok(string_length == lstrlenW(stringW), "got wrong string length: %d.\n", string_length);
    memset(bufferW, 0, sizeof(bufferW));

    hr = IMFAttributes_GetString(attributes, &DUMMY_GUID1, bufferW, ARRAY_SIZE(bufferW), NULL);
    ok(hr == S_OK, "IMFAttributes_GetString failed: 0x%08x.\n", hr);
    ok(!lstrcmpW(bufferW, stringW), "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));
    memset(bufferW, 0, sizeof(bufferW));

    hr = IMFAttributes_GetString(attributes, &DUMMY_GUID1, bufferW, 1, NULL);
    ok(hr == STRSAFE_E_INSUFFICIENT_BUFFER, "IMFAttributes_GetString should fail: 0x%08x.\n", hr);
    ok(!bufferW[0], "got wrong string: \"%s\".\n", wine_dbgstr_w(bufferW));

    string_length = 0xdeadbeef;
    hr = IMFAttributes_GetStringLength(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, &string_length);
    ok(hr == MF_E_INVALIDTYPE, "IMFAttributes_GetStringLength should fail: 0x%08x.\n", hr);
    ok(string_length == 0xdeadbeef, "got wrong string length: %d.\n", string_length);

    IMFAttributes_Release(attributes);
}

static void test_MFCreateMFByteStreamOnStream(void)
{
    IMFByteStream *bytestream;
    IMFByteStream *bytestream2;
    IStream *stream;
    IMFAttributes *attributes = NULL;
    IUnknown *unknown;
    HRESULT hr;
    ULONG ref;

    if(!pMFCreateMFByteStreamOnStream)
    {
        win_skip("MFCreateMFByteStreamOnStream() not found\n");
        return;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = pMFCreateMFByteStreamOnStream(stream, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IUnknown,
                                 (void **)&unknown);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok((void *)unknown == (void *)bytestream, "got %p\n", unknown);
    ref = IUnknown_Release(unknown);
    ok(ref == 1, "got %u\n", ref);

    hr = IUnknown_QueryInterface(unknown, &IID_IMFByteStream,
                                 (void **)&bytestream2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(bytestream2 == bytestream, "got %p\n", bytestream2);
    ref = IMFByteStream_Release(bytestream2);
    ok(ref == 1, "got %u\n", ref);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFAttributes,
                                 (void **)&attributes);
    ok(hr == S_OK ||
       /* w7pro64 */
       broken(hr == E_NOINTERFACE), "got 0x%08x\n", hr);

    if (hr != S_OK)
    {
        win_skip("Can not retrieve IMFAttributes interface from IMFByteStream\n");
        IStream_Release(stream);
        IMFByteStream_Release(bytestream);
        return;
    }

    ok(attributes != NULL, "got NULL\n");

    hr = IMFAttributes_QueryInterface(attributes, &IID_IUnknown,
                                 (void **)&unknown);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok((void *)unknown == (void *)bytestream, "got %p\n", unknown);
    ref = IUnknown_Release(unknown);
    ok(ref == 2, "got %u\n", ref);

    hr = IMFAttributes_QueryInterface(attributes, &IID_IMFByteStream,
                                 (void **)&bytestream2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(bytestream2 == bytestream, "got %p\n", bytestream2);
    ref = IMFByteStream_Release(bytestream2);
    ok(ref == 2, "got %u\n", ref);

    IMFAttributes_Release(attributes);
    IMFByteStream_Release(bytestream);
    IStream_Release(stream);
}

static void test_MFCreateFile(void)
{
    IMFByteStream *bytestream;
    IMFByteStream *bytestream2;
    IMFAttributes *attributes = NULL;
    HRESULT hr;
    WCHAR *filename;

    static const WCHAR newfilename[] = {'n','e','w','.','m','p','4',0};

    filename = load_resource(mp4file);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFByteStream_QueryInterface(bytestream, &IID_IMFAttributes,
                                 (void **)&attributes);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(attributes != NULL, "got NULL\n");
    IMFAttributes_Release(attributes);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream2);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    IMFByteStream_Release(bytestream2);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    hr = MFCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    IMFByteStream_Release(bytestream);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, newfilename, &bytestream);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "got 0x%08x\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_EXIST,
                      MF_FILEFLAGS_NONE, filename, &bytestream);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS), "got 0x%08x\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_EXIST,
                      MF_FILEFLAGS_NONE, newfilename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = MFCreateFile(MF_ACCESSMODE_READ, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, newfilename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_NONE, newfilename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_ALLOW_WRITE_SHARING, newfilename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    IMFByteStream_Release(bytestream);

    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_ALLOW_WRITE_SHARING, newfilename, &bytestream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* Opening the file again fails even though MF_FILEFLAGS_ALLOW_WRITE_SHARING is set. */
    hr = MFCreateFile(MF_ACCESSMODE_WRITE, MF_OPENMODE_FAIL_IF_NOT_EXIST,
                      MF_FILEFLAGS_ALLOW_WRITE_SHARING, newfilename, &bytestream2);
    todo_wine ok(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION), "got 0x%08x\n", hr);
    if (hr == S_OK) IMFByteStream_Release(bytestream2);

    IMFByteStream_Release(bytestream);

    MFShutdown();

    DeleteFileW(filename);
    DeleteFileW(newfilename);
}

static void test_MFCreateMemoryBuffer(void)
{
    IMFMediaBuffer *buffer;
    HRESULT hr;
    DWORD length, max;
    BYTE *data, *data2;

    if(!pMFCreateMemoryBuffer)
    {
        win_skip("MFCreateMemoryBuffer() not found\n");
        return;
    }

    hr = pMFCreateMemoryBuffer(1024, NULL);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "got 0x%08x\n", hr);

    hr = pMFCreateMemoryBuffer(0, &buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    if(buffer)
    {
        hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        ok(length == 0, "got %u\n", length);

        IMFMediaBuffer_Release(buffer);
    }

    hr = pMFCreateMemoryBuffer(1024, &buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_GetMaxLength(buffer, NULL);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_GetMaxLength(buffer, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(length == 1024, "got %u\n", length);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 1025);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_SetCurrentLength(buffer, 10);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, NULL);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_GetCurrentLength(buffer, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(length == 10, "got %u\n", length);

    length = 0;
    max = 0;
    hr = IMFMediaBuffer_Lock(buffer, NULL, &length, &max);
    ok(hr == E_INVALIDARG || hr == E_POINTER, "got 0x%08x\n", hr);
    ok(length == 0, "got %u\n", length);
    ok(max == 0, "got %u\n", length);

    hr = IMFMediaBuffer_Lock(buffer, &data, &max, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(length == 10, "got %u\n", length);
    ok(max == 1024, "got %u\n", max);

    /* Attempt to lock the bufer twice */
    hr = IMFMediaBuffer_Lock(buffer, &data2, &max, &length);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(data == data2, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_Lock(buffer, &data, NULL, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    /* Extra Unlock */
    hr = IMFMediaBuffer_Unlock(buffer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IMFMediaBuffer_Release(buffer);
}

static void test_MFSample(void)
{
    IMFSample *sample;
    HRESULT hr;
    UINT32 count;

    hr = MFCreateSample( &sample );
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFSample_GetBufferCount(sample, &count);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 0, "got %d\n", count);

    IMFSample_Release(sample);
}

static void test_IMFAttributes_item(void)
{
    IMFAttributes *attributes;
    HRESULT hr;
    PROPVARIANT propvar, ret_propvar;
    GUID key;

    hr = MFCreateAttributes(&attributes, 0);
    ok(hr == S_OK, "MFCreateAttributes failed: 0x%08x.\n", hr);

    PropVariantInit(&propvar);
    propvar.vt = MF_ATTRIBUTE_UINT32;
    U(propvar).ulVal = 123;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID1, &propvar);
    ok(hr == S_OK, "IMFAttributes_SetItem failed: 0x%08x.\n", hr);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_UINT32;
    U(ret_propvar).ulVal = 0xdeadbeef;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "IMFAttributes_GetItem failed: 0x%08x.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "got wrong property.\n");
    PropVariantClear(&ret_propvar);

    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_STRING;
    U(ret_propvar).pwszVal = NULL;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "IMFAttributes_GetItem failed: 0x%08x.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "got wrong property.\n");
    PropVariantClear(&ret_propvar);

    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = MF_ATTRIBUTE_UINT64;
    U(propvar).uhVal.QuadPart = 65536;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID1, &propvar);
    ok(hr == S_OK, "IMFAttributes_SetItem failed: 0x%08x.\n", hr);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_UINT32;
    U(ret_propvar).ulVal = 0xdeadbeef;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID1, &ret_propvar);
    ok(hr == S_OK, "IMFAttributes_GetItem failed: 0x%08x.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "got wrong property.\n");
    PropVariantClear(&ret_propvar);
    PropVariantClear(&propvar);

    PropVariantInit(&propvar);
    propvar.vt = VT_I4;
    U(propvar).lVal = 123;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID2, &propvar);
    ok(hr == MF_E_INVALIDTYPE, "IMFAttributes_SetItem should failed: 0x%08x.\n", hr);
    PropVariantInit(&ret_propvar);
    ret_propvar.vt = MF_ATTRIBUTE_UINT32;
    U(ret_propvar).lVal = 0xdeadbeef;
    hr = IMFAttributes_GetItem(attributes, &DUMMY_GUID2, &ret_propvar);
    ok(hr == S_OK, "IMFAttributes_GetItem failed: 0x%08x.\n", hr);
    PropVariantClear(&propvar);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "got wrong property.\n");
    PropVariantClear(&ret_propvar);

    PropVariantInit(&propvar);
    propvar.vt = MF_ATTRIBUTE_UINT32;
    U(propvar).ulVal = 123;
    hr = IMFAttributes_SetItem(attributes, &DUMMY_GUID3, &propvar);
    ok(hr == S_OK, "IMFAttributes_SetItem failed: 0x%08x.\n", hr);

    CHECK_COUNT(attributes, 3);
    hr = IMFAttributes_DeleteItem(attributes, &DUMMY_GUID2);
    ok(hr == S_OK, "IMFAttributes_DeleteItem failed: 0x%08x.\n", hr);
    CHECK_COUNT(attributes, 2);
    if(is_prewin8())
        hr = IMFAttributes_GetItemByIndex(attributes, 1, &key, &ret_propvar);
    else
        hr = IMFAttributes_GetItemByIndex(attributes, 0, &key, &ret_propvar);
    ok(hr == S_OK, "IMFAttributes_GetItemByIndex failed: 0x%08x.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "got wrong property.\n");
    ok(IsEqualIID(&key, &DUMMY_GUID3), "got wrong key: %s.\n", wine_dbgstr_guid(&key));
    PropVariantClear(&ret_propvar);
    PropVariantClear(&propvar);
    propvar.vt = MF_ATTRIBUTE_UINT64;
    U(propvar).uhVal.QuadPart = 65536;
    if(is_prewin8())
        hr = IMFAttributes_GetItemByIndex(attributes, 0, &key, &ret_propvar);
    else
        hr = IMFAttributes_GetItemByIndex(attributes, 1, &key, &ret_propvar);
    ok(hr == S_OK, "IMFAttributes_GetItemByIndex failed: 0x%08x.\n", hr);
    ok(!PropVariantCompareEx(&propvar, &ret_propvar, 0, 0), "got wrong property.\n");
    ok(IsEqualIID(&key, &DUMMY_GUID1), "got wrong key: %s.\n", wine_dbgstr_guid(&key));

    CHECK_COUNT(attributes, 2);
    hr = IMFAttributes_DeleteItem(attributes, &DUMMY_GUID2);
    ok(hr == S_OK, "IMFAttributes_DeleteItem failed: 0x%08x.\n", hr);
    CHECK_COUNT(attributes, 2);

    IMFAttributes_Release(attributes);
}

START_TEST(mfplat)
{
    CoInitialize(NULL);

    init_functions();

    test_register();
    test_MFCreateMediaType();
    test_MFCreateMediaEvent();
    test_MFCreateAttributes();
    test_MFSample();
    test_MFCreateFile();
    test_MFCreateMFByteStreamOnStream();
    test_MFCreateMemoryBuffer();
    test_source_resolver();
    test_IMFAttributes_item();

    CoUninitialize();
}
