/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <RHI/ResourcePoolResolver.h>
#include <Atom/RHI/DeviceStreamingImagePool.h>

namespace AZ
{
    namespace Metal
    {
        class ImagePool;
        class Buffer;
        class BufferPool;
    
        class ImagePoolResolver final
            : public ResourcePoolResolver
        {
            using Base = RHI::ResourcePoolResolver;
            
        public:
            AZ_CLASS_ALLOCATOR(ImagePoolResolver, AZ::SystemAllocator);
            AZ_RTTI(ImagePoolResolver, "{85943BB1-AAE9-47C6-B05A-4B0BFBF1E0A8}", Base);
            
            ImagePoolResolver(Device& device);

            RHI::ResultCode UpdateImage(const RHI::DeviceImageUpdateRequest& request, size_t& bytesTransferred);
            int CalculateMipLevel(int lowestMipLength, int currentMipLength);
            
            //////////////////////////////////////////////////////////////////////
            ///ResourcePoolResolver
            void Compile() override;
            void Resolve(CommandList& commandList) const override;
            void Deactivate() override;
            void OnResourceShutdown(const RHI::DeviceResource& resource) override;
            //////////////////////////////////////////////////////////////////////
            
        private:
            
            struct ImageUploadPacket
            {
                Image* m_destinationImage = nullptr;
                RHI::Ptr<Buffer> m_stagingBuffer;
                RHI::DeviceImageSubresourceLayout m_subresourceLayout;
                RHI::ImageSubresource m_subresource;
                RHI::Origin m_offset;
            };
            
            AZStd::mutex m_uploadPacketsLock;
            AZStd::vector<ImageUploadPacket> m_uploadPackets;
        };
    }
}
