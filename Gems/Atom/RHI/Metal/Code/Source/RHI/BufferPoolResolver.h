/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/ResourcePoolResolver.h>

namespace AZ
{
    namespace Metal
    {
        class MemoryView;

        class BufferPoolResolver final
            : public ResourcePoolResolver
        {
            using Base = RHI::ResourcePoolResolver;

        public:
            AZ_RTTI(BufferPoolResolver, "{ECC51B75-62AD-4C86-8CAB-D6B492BD2340}", Base);
            AZ_CLASS_ALLOCATOR(BufferPoolResolver, AZ::SystemAllocator);

            BufferPoolResolver(Device& device, const RHI::BufferPoolDescriptor& descriptor);

            ///Get a pointer to write a content to upload to GPU.
            void* MapBuffer(const RHI::DeviceBufferMapRequest& request);

            //////////////////////////////////////////////////////////////////////
            ///ResourcePoolResolver
            void Compile() override;
            void Resolve(CommandList& commandList) const override;
            void Deactivate() override;
            void OnResourceShutdown(const RHI::DeviceResource& resource) override;
            //////////////////////////////////////////////////////////////////////

        private:
            struct BufferUploadPacket
            {
                Buffer* m_attachmentBuffer = nullptr;
                RHI::Ptr<Buffer> m_stagingBuffer;
                size_t m_byteOffset = 0;
            };
            
            AZStd::mutex m_uploadPacketsLock;
            AZStd::vector<BufferUploadPacket> m_uploadPackets;
        };
    }
}
