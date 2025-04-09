/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/intrusive_ptr.h>

// Due to PAL (legal) restrictions, each platform is required to define each <DirectXType>X type internally. To give the platforms flexibility on overlapping types,
// the platforms themselves need to implement the reference counter policies for each base DirectX type.
#define AZ_DX12_REFCOUNTED(DXTypeName) \
    namespace AZStd \
    { \
        template <> \
        struct IntrusivePtrCountPolicy<DXTypeName> \
        { \
            static void add_ref(DXTypeName* p) { p->AddRef(); } \
            static void release(DXTypeName* p) { p->Release(); } \
        }; \
    }

#include <RHI/DX12_Platform.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/containers/vector.h>
#include <Atom/RHI.Reflect/Base.h>

#ifndef IID_GRAPHICS_PPV_ARGS
#define IID_GRAPHICS_PPV_ARGS(ppType) IID_PPV_ARGS(ppType)
#endif

#define MAKE_FOURCC(a, b, c, d) (((uint32_t)(d) << 24) | ((uint32_t)(c) << 16) | ((uint32_t)(b) << 8) | (uint32_t)(a))

namespace AZ
{
    namespace DX12
    {
        template<typename T>
        using DX12Ptr = Microsoft::WRL::ComPtr<T>;

        inline bool operator == (const D3D12_CPU_DESCRIPTOR_HANDLE& l, const D3D12_CPU_DESCRIPTOR_HANDLE& r)
        {
            return l.ptr == r.ptr;
        }

        inline bool operator != (const D3D12_CPU_DESCRIPTOR_HANDLE& l, const D3D12_CPU_DESCRIPTOR_HANDLE& r)
        {
            return l.ptr != r.ptr;
        }

        bool AssertSuccess(HRESULT hr);

        template<class T, class U>
        inline RHI::Ptr<T> DX12ResourceCast(U* resource)
        {
            DX12Ptr<T> newResource;
            resource->QueryInterface(IID_GRAPHICS_PPV_ARGS(newResource.GetAddressOf()));
            return newResource.Get();
        }

        using GpuDescriptorHandle = D3D12_GPU_DESCRIPTOR_HANDLE;
        using GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS;
        using CpuVirtualAddress = uint8_t*;

        DXGI_FORMAT GetBaseFormat(DXGI_FORMAT defaultFormat);
        DXGI_FORMAT GetSRVFormat(DXGI_FORMAT defaultFormat);
        DXGI_FORMAT GetUAVFormat(DXGI_FORMAT defaultFormat);
        DXGI_FORMAT GetDSVFormat(DXGI_FORMAT defaultFormat);
        DXGI_FORMAT GetStencilFormat(DXGI_FORMAT defaultFormat);

        namespace Alignment
        {
            enum
            {
                Buffer = 16,
                Constant = 256,
                Image = 512,
                CommittedBuffer = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT
            };
        }
    }
}
