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
