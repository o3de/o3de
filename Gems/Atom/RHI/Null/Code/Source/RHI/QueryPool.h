/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceQueryPool.h>

namespace AZ
{
    namespace Null
    {
        class QueryPool final
            : public RHI::DeviceQueryPool
        {
            using Base = RHI::DeviceQueryPool;
        public:
            AZ_RTTI(QueryPool, "{C4057417-0B50-4F7C-9FFA-CB66E937AD6E}", Base);
            AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator);
            virtual ~QueryPool() = default;

            static RHI::Ptr<QueryPool> Create();
             
        private:
            QueryPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceQueryPool
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::QueryPoolDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitQueryInternal([[maybe_unused]] RHI::DeviceQuery& query) override { return RHI::ResultCode::Success;}
            RHI::ResultCode GetResultsInternal([[maybe_unused]] uint32_t startIndex, [[maybe_unused]] uint32_t queryCount, [[maybe_unused]] uint64_t* results, [[maybe_unused]] uint32_t resultsCount, [[maybe_unused]] RHI::QueryResultFlagBits flags) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
