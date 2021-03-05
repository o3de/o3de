
/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <Atom/RHI/StreamingImagePool.h>
#include <AzCore/std/parallel/atomic.h>
#include <RHI/Image.h>

namespace AZ
{
    namespace Metal
    {
        class StreamingImagePoolResolver;
        
        class StreamingImagePool final
            : public RHI::StreamingImagePool
        {
            using Base = RHI::StreamingImagePool;
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(StreamingImagePool, "{B5AA610C-0EA9-4077-9537-3E5D31646BC4}", Base);
            static RHI::Ptr<StreamingImagePool> Create();
            
            Device& GetDevice() const;
            StreamingImagePoolResolver* GetResolver();
        private:
            StreamingImagePool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::StreamingImagePool
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::StreamingImagePoolDescriptor& descriptor) override;
            RHI::ResultCode InitImageInternal(const RHI::StreamingImageInitRequest& request) override;
            RHI::ResultCode ExpandImageInternal(const RHI::StreamingImageExpandRequest& request) override;
            RHI::ResultCode TrimImageInternal(RHI::Image& image, uint32_t targetMipLevel) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::ResourcePool
            void ShutdownInternal() override;
            void ShutdownResourceInternal(RHI::Resource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
