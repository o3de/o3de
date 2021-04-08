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

#include <Atom/RHI/RayTracingAccelerationStructure.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Null
    {
        class Buffer;

        class RayTracingBlas final
            : public RHI::RayTracingBlas
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingBlas, AZ::SystemAllocator, 0);

            static RHI::Ptr<RayTracingBlas> Create();

            // RHI::RayTracingBlas overrides...
            virtual bool IsValid() const override { return true; }

        private:
            RayTracingBlas() = default;

            // RHI::RayTracingBlas overrides...
            RHI::ResultCode CreateBuffersInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::RayTracingBlasDescriptor* descriptor, [[maybe_unused]] const RHI::RayTracingBufferPools& rayTracingBufferPools) override {return RHI::ResultCode::Success;}
        };
    }
}
