/*
 * Unit test suite for mfasfsrcsnk.
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

DEFINE_GUID(CLSID_ASFByteStreamHandler, 0x41457294, 0x644c, 0x4298, 0xa2, 0x8a, 0xbd, 0x69, 0xf2, 0xc0, 0xcf, 0x3b);

static void test_asf_bytestream_handler(void)
{
    IMFByteStreamHandler *handler;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_ASFByteStreamHandler, 0, CLSCTX_INPROC_SERVER,
                          &IID_IMFByteStreamHandler, (void **)&handler);
    ok(hr == S_OK, "CoCreateInstance failed: 0x%08x.\n", hr);

    IMFByteStreamHandler_Release(handler);
}

START_TEST(mfasfsrcsnk)
{
    HMODULE module = LoadLibraryA("mfasfsrcsnk.dll");

    if(!module)
    {
        win_skip("Failed to load mfasfsrcsnk.dll.\n");
        return;
    }

    CoInitialize(NULL);

    test_asf_bytestream_handler();

    CoUninitialize();
}
