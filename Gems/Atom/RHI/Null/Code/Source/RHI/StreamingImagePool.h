
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            void ComputeFragmentation() const override {}
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
