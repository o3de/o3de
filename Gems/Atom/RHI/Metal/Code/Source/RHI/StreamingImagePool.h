
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/SingleDeviceStreamingImagePool.h>
#include <AzCore/std/parallel/atomic.h>
#include <RHI/Image.h>

namespace AZ
{
    namespace Metal
    {
        class StreamingImagePoolResolver;
        
        class StreamingImagePool final
            : public RHI::SingleDeviceStreamingImagePool
        {
            using Base = RHI::SingleDeviceStreamingImagePool;
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator);
            AZ_RTTI(StreamingImagePool, "{B5AA610C-0EA9-4077-9537-3E5D31646BC4}", Base);
            static RHI::Ptr<StreamingImagePool> Create();
            
            Device& GetDevice() const;
            StreamingImagePoolResolver* GetResolver();
        private:
            StreamingImagePool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::SingleDeviceStreamingImagePool
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::StreamingImagePoolDescriptor& descriptor) override;
            RHI::ResultCode InitImageInternal(const RHI::SingleDeviceStreamingImageInitRequest& request) override;
            RHI::ResultCode ExpandImageInternal(const RHI::SingleDeviceStreamingImageExpandRequest& request) override;
            RHI::ResultCode TrimImageInternal(RHI::SingleDeviceImage& image, uint32_t targetMipLevel) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::SingleDeviceResourcePool
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::SingleDeviceResource& resourceBase) override;
            void ComputeFragmentation() const override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
