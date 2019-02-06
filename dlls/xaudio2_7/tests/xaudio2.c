/*
 * Copyright (c) 2015 Andrew Eikum for CodeWeavers
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

#include <windows.h>
#include <math.h>

#define COBJMACROS
#include "wine/test.h"
#include "initguid.h"
#include "xaudio2.h"
#include "xaudio2fx.h"
#include "xapo.h"
#include "xapofx.h"
#include "mmsystem.h"

static BOOL xaudio27;

static HRESULT (WINAPI *pXAudio2Create)(IXAudio2 **, UINT32, XAUDIO2_PROCESSOR) = NULL;
static HRESULT (WINAPI *pCreateAudioVolumeMeter)(IUnknown**) = NULL;

static void test_xapo_creation_legacy(const char *module, unsigned int version)
{
    HANDLE xapofxdll;
    HRESULT hr;
    IUnknown *fx_unk;
    unsigned int i;

    HRESULT (CDECL *pCreateFX)(REFCLSID,IUnknown**) = NULL;

    /* CLSIDs are the same across all versions */
    static const GUID *const_clsids[] = {
        &CLSID_FXEQ27,
        &CLSID_FXMasteringLimiter27,
        &CLSID_FXReverb27,
        &CLSID_FXEcho27,
        /* older versions of xapofx actually have support for new clsids */
        &CLSID_FXEQ,
        &CLSID_FXMasteringLimiter,
        &CLSID_FXReverb,
        &CLSID_FXEcho
    };

    /* different CLSID for each version */
    static const GUID *avm_clsids[] = {
        &CLSID_AudioVolumeMeter20,
        &CLSID_AudioVolumeMeter21,
        &CLSID_AudioVolumeMeter22,
        &CLSID_AudioVolumeMeter23,
        &CLSID_AudioVolumeMeter24,
        &CLSID_AudioVolumeMeter25,
        &CLSID_AudioVolumeMeter26,
        &CLSID_AudioVolumeMeter27
    };

    static const GUID *ar_clsids[] = {
        &CLSID_AudioReverb20,
        &CLSID_AudioReverb21,
        &CLSID_AudioReverb22,
        &CLSID_AudioReverb23,
        &CLSID_AudioReverb24,
        &CLSID_AudioReverb25,
        &CLSID_AudioReverb26,
        &CLSID_AudioReverb27
    };

    xapofxdll = LoadLibraryA(module);
    if(xapofxdll){
        pCreateFX = (void*)GetProcAddress(xapofxdll, "CreateFX");
        ok(pCreateFX != NULL, "%s did not have CreateFX?\n", module);
        if(!pCreateFX){
            FreeLibrary(xapofxdll);
            return;
        }
    }else{
        win_skip("Couldn't load %s\n", module);
        return;
    }

    if(pCreateFX){
        for(i = 0; i < ARRAY_SIZE(const_clsids); ++i){
            hr = pCreateFX(const_clsids[i], &fx_unk);
            ok(hr == S_OK, "%s: CreateFX(%s) failed: %08x\n", module, wine_dbgstr_guid(const_clsids[i]), hr);
            if(SUCCEEDED(hr)){
                IXAPO *xapo;
                hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO27, (void**)&xapo);
                ok(hr == S_OK, "Couldn't get IXAPO27 interface: %08x\n", hr);
                if(SUCCEEDED(hr))
                    IXAPO_Release(xapo);
                IUnknown_Release(fx_unk);
            }

            hr = CoCreateInstance(const_clsids[i], NULL, CLSCTX_INPROC_SERVER,
                    &IID_IUnknown, (void**)&fx_unk);
            ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance should have failed: %08x\n", hr);
            if(SUCCEEDED(hr))
                IUnknown_Release(fx_unk);
        }

        hr = pCreateFX(avm_clsids[version - 20], &fx_unk);
        ok(hr == S_OK, "%s: CreateFX(%s) failed: %08x\n", module, wine_dbgstr_guid(avm_clsids[version - 20]), hr);
        if(SUCCEEDED(hr)){
            IXAPO *xapo;
            hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO27, (void**)&xapo);
            ok(hr == S_OK, "Couldn't get IXAPO27 interface: %08x\n", hr);
            if(SUCCEEDED(hr))
                IXAPO_Release(xapo);
            IUnknown_Release(fx_unk);
        }

        hr = pCreateFX(ar_clsids[version - 20], &fx_unk);
        ok(hr == S_OK, "%s: CreateFX(%s) failed: %08x\n", module, wine_dbgstr_guid(ar_clsids[version - 20]), hr);
        if(SUCCEEDED(hr)){
            IXAPO *xapo;
            hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO27, (void**)&xapo);
            ok(hr == S_OK, "Couldn't get IXAPO27 interface: %08x\n", hr);
            if(SUCCEEDED(hr))
                IXAPO_Release(xapo);
            IUnknown_Release(fx_unk);
        }
    }

    FreeLibrary(xapofxdll);
}

static void test_xapo_creation_modern(const char *module)
{
    HANDLE xaudio2dll;
    HRESULT hr;
    IUnknown *fx_unk;
    unsigned int i;

    HRESULT (CDECL *pCreateFX)(REFCLSID,IUnknown**,void*,UINT32) = NULL;
    HRESULT (WINAPI *pCAVM)(IUnknown**) = NULL;
    HRESULT (WINAPI *pCAR)(IUnknown**) = NULL;

    /* CLSIDs are the same across all versions */
    static const GUID *const_clsids[] = {
        &CLSID_FXEQ27,
        &CLSID_FXMasteringLimiter27,
        &CLSID_FXReverb27,
        &CLSID_FXEcho27,
        &CLSID_FXEQ,
        &CLSID_FXMasteringLimiter,
        &CLSID_FXReverb,
        &CLSID_FXEcho
    };

    xaudio2dll = LoadLibraryA(module);
    if(xaudio2dll){
        pCreateFX = (void*)GetProcAddress(xaudio2dll, "CreateFX");
        ok(pCreateFX != NULL, "%s did not have CreateFX?\n", module);
        if(!pCreateFX){
            FreeLibrary(xaudio2dll);
            return;
        }
    }else{
        win_skip("Couldn't load %s\n", module);
        return;
    }

    if(pCreateFX){
        for(i = 0; i < ARRAY_SIZE(const_clsids); ++i){
            hr = pCreateFX(const_clsids[i], &fx_unk, NULL, 0);
            ok(hr == S_OK, "%s: CreateFX(%s) failed: %08x\n", module, wine_dbgstr_guid(const_clsids[i]), hr);
            if(SUCCEEDED(hr)){
                IXAPO *xapo;
                hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO, (void**)&xapo);
                ok(hr == S_OK, "Couldn't get IXAPO interface: %08x\n", hr);
                if(SUCCEEDED(hr))
                    IXAPO_Release(xapo);
                IUnknown_Release(fx_unk);
            }

            hr = CoCreateInstance(const_clsids[i], NULL, CLSCTX_INPROC_SERVER,
                    &IID_IUnknown, (void**)&fx_unk);
            ok(hr == REGDB_E_CLASSNOTREG, "CoCreateInstance should have failed: %08x\n", hr);
            if(SUCCEEDED(hr))
                IUnknown_Release(fx_unk);
        }

        /* test legacy CLSID */
        hr = pCreateFX(&CLSID_AudioVolumeMeter27, &fx_unk, NULL, 0);
        ok(hr == S_OK, "%s: CreateFX(CLSID_AudioVolumeMeter) failed: %08x\n", module, hr);
        if(SUCCEEDED(hr)){
            IXAPO *xapo;
            hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO, (void**)&xapo);
            ok(hr == S_OK, "Couldn't get IXAPO interface: %08x\n", hr);
            if(SUCCEEDED(hr))
                IXAPO_Release(xapo);
            IUnknown_Release(fx_unk);
        }
    }

    pCAVM = (void*)GetProcAddress(xaudio2dll, "CreateAudioVolumeMeter");
    ok(pCAVM != NULL, "%s did not have CreateAudioVolumeMeter?\n", module);

    hr = pCAVM(&fx_unk);
    ok(hr == S_OK, "CreateAudioVolumeMeter failed: %08x\n", hr);
    if(SUCCEEDED(hr)){
        IXAPO *xapo;
        hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO, (void**)&xapo);
        ok(hr == S_OK, "Couldn't get IXAPO interface: %08x\n", hr);
        if(SUCCEEDED(hr))
            IXAPO_Release(xapo);
        IUnknown_Release(fx_unk);
    }

    pCAR = (void*)GetProcAddress(xaudio2dll, "CreateAudioReverb");
    ok(pCAR != NULL, "%s did not have CreateAudioReverb?\n", module);

    hr = pCAR(&fx_unk);
    ok(hr == S_OK, "CreateAudioReverb failed: %08x\n", hr);
    if(SUCCEEDED(hr)){
        IXAPO *xapo;
        hr = IUnknown_QueryInterface(fx_unk, &IID_IXAPO, (void**)&xapo);
        ok(hr == S_OK, "Couldn't get IXAPO interface: %08x\n", hr);
        if(SUCCEEDED(hr))
            IXAPO_Release(xapo);
        IUnknown_Release(fx_unk);
    }

    FreeLibrary(xaudio2dll);
}

static void test_xapo_creation(void)
{
    test_xapo_creation_legacy("xapofx1_1.dll", 22);
    test_xapo_creation_legacy("xapofx1_2.dll", 23);
    test_xapo_creation_legacy("xapofx1_3.dll", 24);
    test_xapo_creation_legacy("xapofx1_3.dll", 25);
    test_xapo_creation_legacy("xapofx1_4.dll", 26);
    test_xapo_creation_legacy("xapofx1_5.dll", 27);
    test_xapo_creation_modern("xaudio2_8.dll");
}

START_TEST(xaudio2)
{
    HRESULT hr;
    IXAudio27 *xa27 = NULL;
    IXAudio2 *xa = NULL;
    HANDLE xa28dll;

    CoInitialize(NULL);

    xa28dll = LoadLibraryA("xaudio2_8.dll");
    if(xa28dll){
        pXAudio2Create = (void*)GetProcAddress(xa28dll, "XAudio2Create");
        pCreateAudioVolumeMeter = (void*)GetProcAddress(xa28dll, "CreateAudioVolumeMeter");
    }

    test_xapo_creation();

    /* XAudio 2.7 (Jun 2010 DirectX) */
    hr = CoCreateInstance(&CLSID_XAudio27, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXAudio27, (void**)&xa27);
    if(hr == S_OK){
        xaudio27 = TRUE;

        hr = IXAudio27_QueryInterface(xa27, &IID_IXAudio28, (void**) &xa);
        ok(hr != S_OK, "QueryInterface with IID_IXAudio28 on IXAudio27 object returned success. Expected to fail\n");

        IXAudio27_Release(xa27);
    }else
        win_skip("XAudio 2.7 not available\n");

    /* XAudio 2.8 (Win8+) */
    if(pXAudio2Create){
        xaudio27 = FALSE;

        hr = pXAudio2Create(&xa, 0, XAUDIO2_DEFAULT_PROCESSOR);
        ok(hr == S_OK, "XAudio2Create failed: %08x\n", hr);

        hr = IXAudio2_QueryInterface(xa, &IID_IXAudio27, (void**)&xa27);
        ok(hr == E_NOINTERFACE, "XA28 object should support IXAudio27, gave: %08x\n", hr);

        IXAudio2_Release(xa);
    }else
        win_skip("XAudio 2.8 not available\n");

    CoUninitialize();
}
