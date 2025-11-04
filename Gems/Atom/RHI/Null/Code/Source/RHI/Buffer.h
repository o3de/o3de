/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBuffer.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace Null
    {
        class Buffer final
            : public RHI::DeviceBuffer
        {
            using Base = RHI::DeviceBuffer;
        public:
            AZ_CLASS_ALLOCATOR(Buffer, AZ::ThreadPoolAllocator);
            AZ_RTTI(Buffer, "{4D9739D8-8C42-4C3D-8253-C8500EBA2D84}", Base);
            ~Buffer() = default;
            
            static RHI::Ptr<Buffer> Create();
            
        private:
            Buffer() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceResource
            void ReportMemoryUsage([[maybe_unused]] RHI::MemoryStatisticsBuilder& builder) const override {}
            //////////////////////////////////////////////////////////////////////////

        };
    }
}
