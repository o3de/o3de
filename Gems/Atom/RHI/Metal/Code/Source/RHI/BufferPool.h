/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/BufferPool.h>
#include <RHI/BufferMemoryAllocator.h>

namespace AZ
{
    namespace Metal
    {
        class BufferPoolResolver;

        class BufferPool final
            : public RHI::BufferPool
        {
            using Base = RHI::BufferPool;
        public:
            AZ_RTTI(BufferPool, "{A0912F67-86AB-47B1-B764-793C939306E7}", Base);
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator, 0);
            virtual ~BufferPool() = default;
            
            static RHI::Ptr<BufferPool> Create();
            
        private:
            BufferPool() = default;

            Device& GetDevice() const;
            
            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::BufferPool
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::BufferPoolDescriptor& descriptor) override;
            void ShutdownInternal() override;
            RHI::ResultCode InitBufferInternal(RHI::Buffer& buffer, const RHI::BufferDescriptor& rhiDescriptor) override;
            void ShutdownResourceInternal(RHI::Resource& resource) override;
            RHI::ResultCode OrphanBufferInternal(RHI::Buffer& buffer) override;
            RHI::ResultCode MapBufferInternal(const RHI::BufferMapRequest& mapRequest, RHI::BufferMapResponse& response) override;
            void UnmapBufferInternal(RHI::Buffer& buffer) override;
            RHI::ResultCode StreamBufferInternal(const RHI::BufferStreamRequest& request) override;
            //////////////////////////////////////////////////////////////////////////
            
            BufferPoolResolver* GetResolver();
            
            BufferMemoryAllocator m_allocator;
        };

    }
}
