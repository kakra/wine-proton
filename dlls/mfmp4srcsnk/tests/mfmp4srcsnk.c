/*
 * Unit test suite for mfmp4srcsnk.
 *
 * Copyright 2019 Jactry Zeng for CodeWeavers
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

#include <initguid.h>
#include <mfidl.h>

#include "wine/test.h"

DEFINE_GUID(CLSID_MPEG4ByteStreamHandler, 0x271c3902, 0x6095, 0x4c45, 0xa2, 0x2f, 0x20, 0x09, 0x18, 0x16, 0xee, 0x9e);

static void test_mpeg4_bytestream_handler(void)
{
    IMFByteStreamHandler *handler;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MPEG4ByteStreamHandler, 0, CLSCTX_INPROC_SERVER,
                          &IID_IMFByteStreamHandler, (void **)&handler);
    ok(hr == S_OK, "CoCreateInstance failed: 0x%08x.\n", hr);

    IMFByteStreamHandler_Release(handler);
}

START_TEST(mfmp4srcsnk)
{
    HMODULE module = LoadLibraryA("mfmp4srcsnk.dll");

    if(!module)
    {
        win_skip("Failed to load mfmp4srcsnk.dll.\n");
        return;
    }

    CoInitialize(NULL);

    test_mpeg4_bytestream_handler();

    CoUninitialize();
}
