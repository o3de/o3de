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

#include <Atom/RHI/BufferPool.h>

namespace AZ
{
    namespace Null
    {
        class BufferPool final
            : public RHI::BufferPool
        {
            using Base = RHI::BufferPool;
        public:
            AZ_RTTI(BufferPool, "{5F02ECA6-135C-4C96-895E-514E273EEFC2}", Base);
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator, 0);
            virtual ~BufferPool() = default;
            
            static RHI::Ptr<BufferPool> Create();
            
        private:
            BufferPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::BufferPool
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::BufferPoolDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            void ShutdownInternal() override {}
            RHI::ResultCode InitBufferInternal([[maybe_unused]] RHI::Buffer& buffer, [[maybe_unused]] const RHI::BufferDescriptor& rhiDescriptor) override{ return RHI::ResultCode::Success;}
            void ShutdownResourceInternal([[maybe_unused]] RHI::Resource& resource) override {}
            RHI::ResultCode OrphanBufferInternal([[maybe_unused]] RHI::Buffer& buffer) override { return RHI::ResultCode::Success;}
            RHI::ResultCode MapBufferInternal([[maybe_unused]] const RHI::BufferMapRequest& mapRequest, [[maybe_unused]] RHI::BufferMapResponse& response) override { return RHI::ResultCode::Unimplemented;}
            void UnmapBufferInternal([[maybe_unused]] RHI::Buffer& buffer) override {}
            RHI::ResultCode StreamBufferInternal([[maybe_unused]] const RHI::BufferStreamRequest& request) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////

        };

    }
}
