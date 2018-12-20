/*
 * Copyright (C) 2015 Austin English
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "mfidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

/***********************************************************************
 *      MFGetSupportedMimeTypes (mf.@)
 */
HRESULT WINAPI MFGetSupportedMimeTypes(PROPVARIANT *array)
{
    FIXME("(%p) stub\n", array);

    return E_NOTIMPL;
}

/***********************************************************************
 *      MFGetService (mf.@)
 */
HRESULT WINAPI MFGetService(IUnknown *object, REFGUID service, REFIID riid, void **obj)
{
    IMFGetService *gs;
    HRESULT hr;

    TRACE("(%p, %s, %s, %p)\n", object, debugstr_guid(service), debugstr_guid(riid), obj);

    if (!object)
        return E_POINTER;

    if (FAILED(hr = IUnknown_QueryInterface(object, &IID_IMFGetService, (void **)&gs)))
        return hr;

    hr = IMFGetService_GetService(gs, service, riid, obj);
    IMFGetService_Release(gs);
    return hr;
}

typedef struct seqsource
{
    IMFSequencerSource IMFSequencerSource_iface;
    LONG ref;
} seqsource;

static inline seqsource *impl_from_IMFSequencerSource(IMFSequencerSource *iface)
{
    return CONTAINING_RECORD(iface, seqsource, IMFSequencerSource_iface);
}

static HRESULT WINAPI seqsource_QueryInterface(IMFSequencerSource *iface, REFIID riid, void **out)
{
    seqsource *This = impl_from_IMFSequencerSource(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), out);

    if ( IsEqualIID(riid, &IID_IMFSequencerSource) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = &This->IMFSequencerSource_iface;
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

static ULONG WINAPI seqsource_AddRef(IMFSequencerSource *iface)
{
    seqsource *This = impl_from_IMFSequencerSource(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI seqsource_Release(IMFSequencerSource *iface)
{
    seqsource *This = impl_from_IMFSequencerSource(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI seqsource_AppendTopology(IMFSequencerSource *iface, IMFTopology *topology, DWORD flags, MFSequencerElementId *element)
{
    seqsource *This = impl_from_IMFSequencerSource(iface);

    FIXME("%p, %p, %x, %p\n", This, topology, flags, element);

    return E_NOTIMPL;
}

static HRESULT WINAPI seqsource_DeleteTopology(IMFSequencerSource *iface, MFSequencerElementId element)
{
    seqsource *This = impl_from_IMFSequencerSource(iface);

    FIXME("%p, %d\n", This, element);

    return E_NOTIMPL;
}

static HRESULT WINAPI seqsource_GetPresentationContext(IMFSequencerSource *iface, IMFPresentationDescriptor *pd, MFSequencerElementId *id,
        IMFTopology **topology)
{
    seqsource *This = impl_from_IMFSequencerSource(iface);

    FIXME("%p, %p, %p, %p\n", This, pd, id, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI seqsource_UpdateTopology(IMFSequencerSource *iface, MFSequencerElementId id, IMFTopology *topology)
{
    seqsource *This = impl_from_IMFSequencerSource(iface);

    FIXME("%p, %d, %p\n", This, id, topology);

    return E_NOTIMPL;
}

static HRESULT WINAPI seqsource_UpdateTopologyFlags(IMFSequencerSource *iface, MFSequencerElementId id, DWORD flags)
{
    seqsource *This = impl_from_IMFSequencerSource(iface);

    FIXME("%p, %d, %x\n", This, id, flags);

    return E_NOTIMPL;
}

static const IMFSequencerSourceVtbl seqsrc_vtbl =
{
    seqsource_QueryInterface,
    seqsource_AddRef,
    seqsource_Release,
    seqsource_AppendTopology,
    seqsource_DeleteTopology,
    seqsource_GetPresentationContext,
    seqsource_UpdateTopology,
    seqsource_UpdateTopologyFlags
};

/***********************************************************************
 *      MFCreateSequencerSource (mf.@)
 */
HRESULT WINAPI MFCreateSequencerSource(IUnknown *reserved, IMFSequencerSource **sequencer)
{
    seqsource *object;

    TRACE("(%p, %p)\n", reserved, sequencer);

    if (!sequencer)
        return E_POINTER;

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IMFSequencerSource_iface.lpVtbl = &seqsrc_vtbl;
    object->ref = 1;

    *sequencer = &object->IMFSequencerSource_iface;

    return S_OK;
}
