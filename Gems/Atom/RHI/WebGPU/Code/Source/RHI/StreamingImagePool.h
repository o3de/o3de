
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DeviceStreamingImagePool.h>

namespace AZ::WebGPU
{
    class StreamingImagePool final
        : public RHI::DeviceStreamingImagePool
    {
        using Base = RHI::DeviceStreamingImagePool;
    public:
        AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator);
        AZ_RTTI(StreamingImagePool, "{CF13B5BF-2F0B-4D02-9E8E-5964C93B5147}", Base);
        static RHI::Ptr<StreamingImagePool> Create();
            
    private:
        StreamingImagePool() = default;

        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceStreamingImagePool
        RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::StreamingImagePoolDescriptor& descriptor) override;
        RHI::ResultCode InitImageInternal(const RHI::DeviceStreamingImageInitRequest& request) override;
        RHI::ResultCode ExpandImageInternal(const RHI::DeviceStreamingImageExpandRequest& request) override;
        RHI::ResultCode TrimImageInternal(RHI::DeviceImage& image, uint32_t targetMipLevel) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceResourcePool
        void ShutdownInternal() override;
        void ShutdownResourceInternal(RHI::DeviceResource& resourceBase) override;
        void ComputeFragmentation() const override;
        //////////////////////////////////////////////////////////////////////////

        void WaitFinishUploading(const Image& image);
    };
}
