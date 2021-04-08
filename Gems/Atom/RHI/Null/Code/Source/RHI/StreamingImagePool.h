
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

namespace AZ
{
    namespace Null
    {
        class StreamingImagePool final
            : public RHI::StreamingImagePool
        {
            using Base = RHI::StreamingImagePool;
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(StreamingImagePool, "{15688218-739F-40B2-9753-70AD2A432C3A}", Base);
            static RHI::Ptr<StreamingImagePool> Create();
            
        private:
            StreamingImagePool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::StreamingImagePool
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::StreamingImagePoolDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitImageInternal([[maybe_unused]] const RHI::StreamingImageInitRequest& request) override { return RHI::ResultCode::Success;}
            RHI::ResultCode ExpandImageInternal([[maybe_unused]] const RHI::StreamingImageExpandRequest& request) override { return RHI::ResultCode::Success;}
            RHI::ResultCode TrimImageInternal([[maybe_unused]] RHI::Image& image, [[maybe_unused]] uint32_t targetMipLevel) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::ResourcePool
            void ShutdownInternal() override {}
            void ShutdownResourceInternal([[maybe_unused]] RHI::Resource& resourceBase) override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
