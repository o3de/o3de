/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Interface/DX12/RHIDX12Interface.h>
#include <RHI/DX12.h>

#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>
#include <RHI/Image.h>
#include <RHI/PhysicalDevice.h>

namespace AZ
{
    namespace DX12
    {
        ID3D12Device5* GetDeviceNativeHandle(RHI::Device& device)
        {
            AZ_Assert(azrtti_cast<Device*>(&device), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            return static_cast<Device&>(device).GetDevice();
        }

        IDXGIAdapter3* GetPhysicalDeviceNativeHandle(const RHI::PhysicalDevice& device)
        {
            AZ_Assert(azrtti_cast<const PhysicalDevice*>(&device), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            return static_cast<const PhysicalDevice*>(&device)->GetAdapter();
        }

        ID3D12Fence* GetFenceNativeHandle(RHI::DeviceFence& fence)
        {
            AZ_Assert(azrtti_cast<FenceImpl*>(&fence), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            return static_cast<FenceImpl&>(fence).Get().Get();
        }

        uint64_t GetFencePendingValue(RHI::DeviceFence& fence)
        {
            AZ_Assert(azrtti_cast<FenceImpl*>(&fence), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            return static_cast<FenceImpl&>(fence).Get().GetPendingValue();
        }

        ID3D12Resource* GetBufferResource(RHI::DeviceBuffer& buffer)
        {
            AZ_Assert(azrtti_cast<Buffer*>(&buffer), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            auto& dx12Buffer = static_cast<Buffer&>(buffer);
            return dx12Buffer.GetMemoryView().GetMemory();
        }

        ID3D12Heap* GetBufferHeap(RHI::DeviceBuffer& buffer)
        {
            AZ_Assert(azrtti_cast<Buffer*>(&buffer), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            auto& dx12Buffer = static_cast<Buffer&>(buffer);
            return dx12Buffer.GetMemoryView().GetHeap();
        }

        size_t GetBufferMemoryViewSize(RHI::DeviceBuffer& buffer)
        {
            AZ_Assert(azrtti_cast<Buffer*>(&buffer), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            auto& dx12Buffer = static_cast<Buffer&>(buffer);
            return dx12Buffer.GetMemoryView().GetSize();
        }

        size_t GetBufferAllocationOffset(RHI::DeviceBuffer& buffer)
        {
            AZ_Assert(azrtti_cast<Buffer*>(&buffer), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            auto& dx12Buffer = static_cast<Buffer&>(buffer);
            return dx12Buffer.GetMemoryView().GetOffset();
        }

        size_t GetBufferHeapOffset(RHI::DeviceBuffer& buffer)
        {
            AZ_Assert(azrtti_cast<Buffer*>(&buffer), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            auto& dx12Buffer = static_cast<Buffer&>(buffer);
            return dx12Buffer.GetMemoryView().GetHeapOffset();
        }

        ID3D12Resource* GetImageResource(RHI::DeviceImage& image)
        {
            AZ_Assert(azrtti_cast<Image*>(&image), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            auto& dx12Image = static_cast<Image&>(image);
            return dx12Image.GetMemoryView().GetMemory();
        }

        ID3D12Heap* GetImageHeap(RHI::DeviceImage& image)
        {
            AZ_Assert(azrtti_cast<Image*>(&image), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            auto& dx12Image = static_cast<Image&>(image);
            return dx12Image.GetMemoryView().GetHeap();
        }

        size_t GetImageMemoryViewSize(RHI::DeviceImage& image)
        {
            AZ_Assert(azrtti_cast<Image*>(&image), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            auto& dx12Image = static_cast<Image&>(image);
            return dx12Image.GetMemoryView().GetSize();
        }

        size_t GetImageAllocationOffset(RHI::DeviceImage& image)
        {
            AZ_Assert(azrtti_cast<Image*>(&image), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            auto& dx12Image = static_cast<Image&>(image);
            return dx12Image.GetMemoryView().GetOffset();
        }

        size_t GetImageHeapOffset(RHI::DeviceImage& image)
        {
            AZ_Assert(azrtti_cast<Image*>(&image), "%s can only be called with a DX12 RHI object", __FUNCTION__);
            auto& dx12Image = static_cast<Image&>(image);
            return dx12Image.GetMemoryView().GetHeapOffset();
        }
    } // namespace DX12
} // namespace AZ
