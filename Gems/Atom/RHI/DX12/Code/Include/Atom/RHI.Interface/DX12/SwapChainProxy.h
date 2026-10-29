/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// Windows only. The proxy traffics in IDXGISwapChain4, which does not exist on D3D12 platforms
// that have no DXGI (consoles present through their own path and never create a DXGI swapchain).
#if defined(AZ_PLATFORM_WINDOWS)

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>

// PlatformIncl.h MUST come first; see the comment in RHIDX12Interface.h for why a bare
// <windows.h> (pulled in by <d3d12.h>) breaks any translation unit that later reaches WinSock2.
#include <AzCore/PlatformIncl.h>

#include <d3d12.h>
#include <dxgi1_6.h>

namespace AZ::DX12
{
    //! Lets a gem substitute its own IDXGISwapChain4 implementation for the one Atom creates.
    //!
    //! WHY THIS EXISTS: frame-generation upscalers (AMD FSR3, NVIDIA DLSS-G) do not present the
    //! frames the application renders. They wrap the real swapchain in a proxy that owns the
    //! present timeline, injects interpolated frames between the application's frames and paces
    //! them. That wrapping can only happen where the swapchain is created -- an already-created
    //! swapchain has back buffers the RHI has handed out as RHI::Image resources, so it cannot be
    //! swapped underneath them. Hence a hook here rather than an accessor a gem calls later.
    //!
    //! Register an implementation with AZ::Interface<ISwapChainProxy> BEFORE the render window is
    //! created -- a system component's Init() is early enough, Activate() is not guaranteed to be.
    //! At most one proxy may be registered; a second registration is a programming error.
    //!
    //! Reference counting: ReplaceSwapChain is handed ONE reference, which it owns. If it returns
    //! true it must release (or transfer ownership of) the incoming swapchain and write back a
    //! replacement it also holds one reference to. If it returns false it must leave the incoming
    //! pointer and its reference untouched.
    class ISwapChainProxy
    {
    public:
        AZ_RTTI(ISwapChainProxy, "{3E1D5B24-9A07-4C6F-8B31-6D2F0A7C4E58}");

        virtual ~ISwapChainProxy() = default;

        //! @param presentQueue the ID3D12CommandQueue the RHI presents on.
        //! @param inOutSwapChain in: the swapchain Atom just created; out: the replacement.
        //! @return true if inOutSwapChain was replaced. False leaves Atom using the original.
        virtual bool ReplaceSwapChain(ID3D12CommandQueue* presentQueue, IDXGISwapChain4*& inOutSwapChain) = 0;

        //! Called just before the RHI releases its reference, so the proxy can flush any
        //! outstanding presents and drop cached handles to it.
        virtual void OnSwapChainDestroyed(IDXGISwapChain4* swapChain) = 0;
    };

    using SwapChainProxyInterface = AZ::Interface<ISwapChainProxy>;
} // namespace AZ::DX12

#endif // AZ_PLATFORM_WINDOWS
