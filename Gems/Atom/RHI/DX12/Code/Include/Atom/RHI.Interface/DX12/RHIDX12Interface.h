/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Device.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/PhysicalDevice.h>

#include <AzCore/PlatformIncl.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>

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
    } // namespace DX12
} // namespace AZ
