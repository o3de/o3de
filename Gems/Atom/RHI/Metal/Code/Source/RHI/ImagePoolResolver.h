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
#include <RHI/ResourcePoolResolver.h>
#include <Atom/RHI/StreamingImagePool.h>

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
            AZ_CLASS_ALLOCATOR(ImagePoolResolver, AZ::SystemAllocator, 0);
            AZ_RTTI(ImagePoolResolver, "{85943BB1-AAE9-47C6-B05A-4B0BFBF1E0A8}", Base);
            
            ImagePoolResolver(Device& device);

            RHI::ResultCode UpdateImage(const RHI::ImageUpdateRequest& request, size_t& bytesTransferred);
            int CalculateMipLevel(int lowestMipLength, int currentMipLength);
            
            //////////////////////////////////////////////////////////////////////
            ///ResourcePoolResolver
            void Compile() override;
            void Resolve(CommandList& commandList) const override;
            void Deactivate() override;
            void OnResourceShutdown(const RHI::Resource& resource) override;
            //////////////////////////////////////////////////////////////////////
            
        private:
            
            struct ImageUploadPacket
            {
                Image* m_destinationImage = nullptr;
                RHI::Ptr<Buffer> m_stagingBuffer;
                RHI::ImageSubresourceLayout m_subresourceLayout;
                RHI::ImageSubresource m_subresource;
                RHI::Origin m_offset;
            };
            
            AZStd::mutex m_uploadPacketsLock;
            AZStd::vector<ImageUploadPacket> m_uploadPackets;
        };
    }
}
