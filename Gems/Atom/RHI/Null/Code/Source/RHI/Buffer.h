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

#include <Atom/RHI/Buffer.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace Null
    {
        class Buffer final
            : public RHI::Buffer
        {
            using Base = RHI::Buffer;
        public:
            AZ_CLASS_ALLOCATOR(Buffer, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Buffer, "{4D9739D8-8C42-4C3D-8253-C8500EBA2D84}", Base);
            ~Buffer() = default;
            
            static RHI::Ptr<Buffer> Create();
            
        private:
            Buffer() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Resource
            void ReportMemoryUsage([[maybe_unused]] RHI::MemoryStatisticsBuilder& builder) const override {}
            //////////////////////////////////////////////////////////////////////////

        };
    }
}
