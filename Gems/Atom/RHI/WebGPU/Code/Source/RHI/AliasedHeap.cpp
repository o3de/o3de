/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/AliasedHeap.h>
#include <RHI/Buffer.h>
#include <RHI/Image.h>
#include <RHI/Device.h>

namespace AZ::WebGPU
{
    RHI::Ptr<AliasedHeap> AliasedHeap::Create()
    {
        return aznew AliasedHeap;
    }

    AZStd::unique_ptr<RHI::AliasingBarrierTracker> AliasedHeap::CreateBarrierTrackerInternal()
    {
        return AZStd::make_unique<NullAliasingBarrierTracker>(); 
    }

    AZStd::unique_ptr<RHI::Allocator> AliasedHeap::CreateAllocatorInternal([[maybe_unused]] const RHI::AliasedHeapDescriptor& descriptor)
    {
        return AZStd::make_unique<NullAllocator>();
    }

    RHI::ResultCode AliasedHeap::InitInternal(
        [[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::AliasedHeapDescriptor& descriptor)
    {
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode AliasedHeap::InitImageInternal(const RHI::DeviceImageInitRequest& request, [[maybe_unused]] size_t heapOffset)
    {
        Image* image = static_cast<Image*>(request.m_image);
        return image->Init(static_cast<Device&>(GetDevice()), request.m_descriptor);
    }

    RHI::ResultCode AliasedHeap::InitBufferInternal(
        const RHI::DeviceBufferInitRequest& request, [[maybe_unused]] size_t heapOffset)
    {
        Buffer* buffer = static_cast<Buffer*>(request.m_buffer);
        return buffer->Init(static_cast<Device&>(GetDevice()), request.m_descriptor);
    }
}
