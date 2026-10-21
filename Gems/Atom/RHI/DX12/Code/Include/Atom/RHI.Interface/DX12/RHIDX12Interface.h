/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// This header declares ID3D12*/IDXGI* pointers, so it must pull in the D3D12 and DXGI headers
// itself. Without these it only compiles when the including translation unit happens to have
// included them first, which is why including it from a gem (e.g. FSR3) failed with
// "missing ';' before '*'".
//
// PlatformIncl.h MUST come first. <d3d12.h> includes <windows.h>, and a bare <windows.h> pulls in
// the legacy <winsock.h>. Any translation unit that later reaches <WinSock2.h> then fails with a
// wall of C2011/C2375 redefinitions (sockaddr, fd_set, timeval, accept, bind, ...), because the
// two socket headers declare the same symbols with different linkage. PlatformIncl.h defines
// WIN32_LEAN_AND_MEAN (and NOMINMAX) before <windows.h>, which suppresses the legacy header and
// leaves WinSock2.h free to define those symbols once.
#include <AzCore/PlatformIncl.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Device.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/PhysicalDevice.h>

// Base_Platform.h, NOT <d3d12.h> + <dxgi*.h> directly.
//
// The desktop headers are wrong on platforms whose D3D12 comes from a different SDK. Including
// <d3d12.h> alongside a console's own d3d12 header trips that header's own guard --
//     error C1189: D3D12_X cannot be mixed with stock DXGICommon
// followed by a cascade of errors inside dxgi.h. Base_Platform.h resolves through o3de_pal_dir to
// whichever D3D12 header the platform actually uses, which is the same indirection the rest of
// this gem already relies on.
//
// The DXGI includes are dropped rather than replaced: the only DXGI name used here is
// IDXGIAdapter3 in GetPhysicalDeviceNativeHandle below, and a forward declaration is sufficient for
// a returned pointer. Platforms with real DXGI get it transitively; platforms without one
// forward-declare it in their own Base header.
#include <Atom/RHI.Reflect/DX12/Base_Platform.h>

namespace AZ
{
    namespace DX12
    {
        //! It is important to note that the usage of these functions requires care from the user.
        //! This includes:
        //! - Synchronizing with the renderer by waiting on a fence from the FrameGraph before
        //!   starting execution as well as signaling the FrameGraph to continue execution
        //! - Leaving the GPU in a valid state
        //! - Returning resources in a valid state

        //! Provide access to native device handles
        ID3D12Device5* GetDeviceNativeHandle(RHI::Device& device);
        IDXGIAdapter3* GetPhysicalDeviceNativeHandle(const RHI::PhysicalDevice& device);

        //! Provide access to native fence handle and value
        ID3D12Fence* GetFenceNativeHandle(RHI::DeviceFence& fence);
        uint64_t GetFencePendingValue(RHI::DeviceFence& fence);

        //! Provide access to native buffer resource, heap as well as size and offset
        ID3D12Resource* GetBufferResource(RHI::DeviceBuffer& buffer);
        ID3D12Heap* GetBufferHeap(RHI::DeviceBuffer& buffer);
        size_t GetBufferMemoryViewSize(RHI::DeviceBuffer& buffer);
        size_t GetBufferAllocationOffset(RHI::DeviceBuffer& buffer);
        size_t GetBufferHeapOffset(RHI::DeviceBuffer& buffer);

        //! Provide access to native image resource, heap as well as size and offset
        ID3D12Resource* GetImageResource(RHI::DeviceImage& image);
        ID3D12Heap* GetImageHeap(RHI::DeviceImage& image);
        size_t GetImageMemoryViewSize(RHI::DeviceImage& image);
        size_t GetImageAllocationOffset(RHI::DeviceImage& image);
        size_t GetImageHeapOffset(RHI::DeviceImage& image);

        //! Provide access to the native command list for recording into via external libraries
        ID3D12GraphicsCommandList* GetCommandListNativeHandle(RHI::CommandList& commandList);

        //! Provide access to the native graphics queue, which is also the queue the RHI presents on.
        //! Needed by external presentation libraries (frame-generation proxy swapchains) that must
        //! serialize their own submissions against the queue the application presents from.
        ID3D12CommandQueue* GetPresentCommandQueueNativeHandle(RHI::Device& device);
    } // namespace DX12
} // namespace AZ
