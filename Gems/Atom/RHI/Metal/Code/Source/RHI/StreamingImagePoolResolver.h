/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <RHI/ResourcePoolResolver.h>
#include <Atom/RHI/StreamingImagePool.h>

namespace AZ
{
    namespace Metal
    {
        class StreamingImagePool;
        
        class StreamingImagePoolResolver final
            : public ResourcePoolResolver
        {
            using Base = RHI::ResourcePoolResolver;
            
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePoolResolver, AZ::SystemAllocator, 0);
            AZ_RTTI(StreamingImagePoolResolver, "{85943BB1-AAE9-47C6-B05A-4B0BFBF1E0A8}", Base);
            
            StreamingImagePoolResolver(Device& device, StreamingImagePool* streamingImagePool)
            : ResourcePoolResolver(device)
            , m_pool{streamingImagePool}
            {}

            RHI::ResultCode UpdateImage(const RHI::StreamingImageExpandRequest& request);
            int CalculateMipLevel(int lowestMipLength, int currentMipLength);
            
            void Compile() override;
            void Resolve(CommandList& commandList) const override;
            void Deactivate() override;
            
        private:
            StreamingImagePool* m_pool = nullptr;
        };
    }
}
