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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Atom/RHI/RayTracingAccelerationStructure.h>

namespace AZ
{
    namespace Null
    {
        class Buffer;

        class RayTracingTlas final
            : public RHI::RayTracingTlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingTlas, AZ::SystemAllocator, 0);

            static RHI::Ptr<RayTracingTlas> Create();
            
            // RHI::RayTracingTlas overrides...
            virtual const RHI::Ptr<RHI::Buffer> GetTlasBuffer() const override { return nullptr; }

        private:
            RayTracingTlas() = default;

            // RHI::RayTracingTlas overrides
            RHI::ResultCode CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::RayTracingTlasDescriptor* descriptor, [[maybe_unused]] const RHI::RayTracingBufferPools& rayTracingBufferPools) override {return RHI::ResultCode::Success;}

        };
    }
}
