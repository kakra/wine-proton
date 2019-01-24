/*
 * MPEG4 support of Media Foundation
 *
 * Copyright 2019 Jactry Zeng for CodeWeavers
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

#define COBJMACROS

#include <initguid.h>
#include <mfidl.h>

#include "windef.h"
#include "winbase.h"

#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfmp4srcsnk);

DEFINE_GUID(CLSID_MPEG4ByteStreamHandler, 0x271c3902, 0x6095, 0x4c45, 0xa2, 0x2f, 0x20, 0x09, 0x18, 0x16, 0xee, 0x9e);

typedef struct _MPEG4ByteStreamHandlerImpl
{
    IMFByteStreamHandler IMFByteStreamHandler_iface;
    LONG ref;
} MPEG4ByteStreamHandlerImpl;

/* IMFByteStreamHandler implementation of MPEG4 */
static inline MPEG4ByteStreamHandlerImpl *impl_from_IMFByteStreamHandler(IMFByteStreamHandler *iface)
{
    return CONTAINING_RECORD(iface, MPEG4ByteStreamHandlerImpl, IMFByteStreamHandler_iface);
}

static HRESULT WINAPI MPEG4ByteStreamHandlerImpl_QueryInterface(IMFByteStreamHandler *iface, REFIID riid, void **out)
{
    MPEG4ByteStreamHandlerImpl *This = impl_from_IMFByteStreamHandler(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), out);

    if(IsEqualIID(riid, &IID_IMFByteStreamHandler) ||
       IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &This->IMFByteStreamHandler_iface;
    }
    else
    {
        FIXME("(%s, %p)\n", debugstr_guid(riid), out);
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*out);
    return S_OK;
}

static ULONG WINAPI MPEG4ByteStreamHandlerImpl_AddRef(IMFByteStreamHandler *iface)
{
    MPEG4ByteStreamHandlerImpl *This = impl_from_IMFByteStreamHandler(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI MPEG4ByteStreamHandlerImpl_Release(IMFByteStreamHandler *iface)
{
    MPEG4ByteStreamHandlerImpl *This = impl_from_IMFByteStreamHandler(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if(!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI MPEG4ByteStreamHandlerImpl_BeginCreateObject(IMFByteStreamHandler *iface, IMFByteStream *bytestream,
                                                                   LPCWSTR url, DWORD flags, IPropertyStore *props,
                                                                   IUnknown **cancel_cookie, IMFAsyncCallback *callback,
                                                                   IUnknown *state)
{
    MPEG4ByteStreamHandlerImpl *This = impl_from_IMFByteStreamHandler(iface);

    FIXME("(%p)->(%p, %s, %#x, %p, %p, %p, %p): stub\n", This, bytestream, wine_dbgstr_w(url),
          flags, props, cancel_cookie, callback, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI MPEG4ByteStreamHandlerImpl_EndCreateObject(IMFByteStreamHandler *iface, IMFAsyncResult *result,
                                                                 MF_OBJECT_TYPE *object_type, IUnknown **object)
{
    MPEG4ByteStreamHandlerImpl *This = impl_from_IMFByteStreamHandler(iface);

    FIXME("(%p)->(%p, %p, %p): stub\n", This, result, object_type, object);

    return E_NOTIMPL;
}

static HRESULT WINAPI MPEG4ByteStreamHandlerImpl_CancelObjectCreation(IMFByteStreamHandler *iface, IUnknown *cancel_cookie)
{
    MPEG4ByteStreamHandlerImpl *This = impl_from_IMFByteStreamHandler(iface);

    FIXME("(%p)->(%p): stub\n", This, cancel_cookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI MPEG4ByteStreamHandlerImpl_GetMaxNumberOfBytesRequiredForResolution(IMFByteStreamHandler *iface, QWORD *bytes)
{
    MPEG4ByteStreamHandlerImpl *This = impl_from_IMFByteStreamHandler(iface);

    FIXME("(%p)->(%p): stub\n", This, bytes);

    return E_NOTIMPL;
}

static const IMFByteStreamHandlerVtbl MPEG4ByteStreamHandlerImpl_vtbl =
{
    MPEG4ByteStreamHandlerImpl_QueryInterface,
    MPEG4ByteStreamHandlerImpl_AddRef,
    MPEG4ByteStreamHandlerImpl_Release,
    MPEG4ByteStreamHandlerImpl_BeginCreateObject,
    MPEG4ByteStreamHandlerImpl_EndCreateObject,
    MPEG4ByteStreamHandlerImpl_CancelObjectCreation,
    MPEG4ByteStreamHandlerImpl_GetMaxNumberOfBytesRequiredForResolution
};

static HRESULT WINAPI CreateMPEG4ByteStreamHandlerImpl(LPCGUID guid, LPVOID *ppv, LPUNKNOWN outer)
{
    MPEG4ByteStreamHandlerImpl *object;
    HRESULT hr;

    *ppv = NULL;
    if(outer)
        return CLASS_E_NOAGGREGATION;

    object = heap_alloc(sizeof(MPEG4ByteStreamHandlerImpl));
    if(!object)
        return E_OUTOFMEMORY;

    object->IMFByteStreamHandler_iface.lpVtbl = &MPEG4ByteStreamHandlerImpl_vtbl;
    object->ref = 1;

    hr = IMFByteStreamHandler_QueryInterface(&object->IMFByteStreamHandler_iface, guid, ppv);
    IMFByteStreamHandler_Release(&object->IMFByteStreamHandler_iface);
    return hr;
}

typedef struct
{
    IClassFactory IClassFactory_iface;
    HRESULT WINAPI (*fnCreateInstance)(REFIID riid, void **ppv, IUnknown *outer);
} IClassFactoryImpl;


/* IClassFactory implementation */
static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if(ppv == NULL)
        return E_POINTER;

    if(IsEqualGUID(&IID_IUnknown, riid))
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
    else if(IsEqualGUID(&IID_IClassFactory, riid))
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
    else
    {
        FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    *ppv = iface;
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter,
                                                  REFIID riid, void **ppv)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);

    TRACE("(%p, %s, %p)\n", pUnkOuter, wine_dbgstr_guid(riid), ppv);

    return This->fnCreateInstance(riid, ppv, pUnkOuter);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL dolock)
{
    TRACE("(%d)\n", dolock);
    return S_OK;
}

static const IClassFactoryVtbl classfactory_vtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactoryImpl MPEG4ByteStreamHandlerImpl_CF = {{&classfactory_vtbl}, CreateMPEG4ByteStreamHandlerImpl};

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%s, %s, %p)\n", wine_dbgstr_guid(rclsid), wine_dbgstr_guid(riid), ppv);
    if(IsEqualCLSID(rclsid, &CLSID_MPEG4ByteStreamHandler) && IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = &MPEG4ByteStreamHandlerImpl_CF;
        IClassFactory_AddRef((IClassFactory*)*ppv);
        return S_OK;
    }

    WARN("(%s, %s, %p): no interface found.\n", wine_dbgstr_guid(rclsid), wine_dbgstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return FALSE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    TRACE("(%p, %u, %p)\n", instance, reason, reserved);

    switch(reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}
