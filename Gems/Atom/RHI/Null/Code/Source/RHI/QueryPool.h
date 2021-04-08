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

#include <Atom/RHI/QueryPool.h>

namespace AZ
{
    namespace Null
    {
        class QueryPool final
            : public RHI::QueryPool
        {
            using Base = RHI::QueryPool;
        public:
            AZ_RTTI(QueryPool, "{C4057417-0B50-4F7C-9FFA-CB66E937AD6E}", Base);
            AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator, 0);
            virtual ~QueryPool() = default;

            static RHI::Ptr<QueryPool> Create();
             
        private:
            QueryPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::QueryPool
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::QueryPoolDescriptor& descriptor) override { return RHI::ResultCode::Success;}
            RHI::ResultCode InitQueryInternal([[maybe_unused]] RHI::Query& query) override { return RHI::ResultCode::Success;}
            RHI::ResultCode GetResultsInternal([[maybe_unused]] uint32_t startIndex, [[maybe_unused]] uint32_t queryCount, [[maybe_unused]] uint64_t* results, [[maybe_unused]] uint32_t resultsCount, [[maybe_unused]] RHI::QueryResultFlagBits flags) override { return RHI::ResultCode::Success;}
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
