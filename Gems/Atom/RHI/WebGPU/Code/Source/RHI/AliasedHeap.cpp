/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/AliasedHeap.h>
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
        return AZStd::make_unique<NoBarrierAliasingBarrierTracker>(); 
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
        [[maybe_unused]] const RHI::DeviceBufferInitRequest& request, [[maybe_unused]] size_t heapOffset)
    {
        return RHI::ResultCode::Success;
    }
}
