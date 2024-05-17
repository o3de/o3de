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
        class StreamingImagePool;
        
        class StreamingImagePoolResolver final
            : public ResourcePoolResolver
        {
            using Base = RHI::ResourcePoolResolver;
            
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePoolResolver, AZ::SystemAllocator);
            AZ_RTTI(StreamingImagePoolResolver, "{85943BB1-AAE9-47C6-B05A-4B0BFBF1E0A8}", Base);
            
            StreamingImagePoolResolver(Device& device,  [[maybe_unused]]StreamingImagePool* streamingImagePool)
            : ResourcePoolResolver(device)
            {}

            RHI::ResultCode UpdateImage(const RHI::DeviceStreamingImageExpandRequest& request);
            int CalculateMipLevel(int lowestMipLength, int currentMipLength);
            
            void Compile() override;
            void Resolve(CommandList& commandList) const override;
            void Deactivate() override;
        };
    }
}
