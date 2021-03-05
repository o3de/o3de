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
    namespace Vulkan
    {
        class CommandList;

        /**
        *   QueryPool implementation for Vulkan.
        *   It uses the VkQueryPool vulkan object to implement the pool.
        */
        class QueryPool final
            : public RHI::QueryPool
        {
            using Base = RHI::QueryPool;

        public:
            AZ_RTTI(QueryPool, "{46816FA4-3B31-434A-AAE3-037BC889AE73}", Base);
            AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator, 0);
            ~QueryPool() = default;
            static RHI::Ptr<QueryPool> Create();

            VkQueryPool GetNativeQueryPool() const;

            void ResetQueries(CommandList& commandList, const RHI::Interval& interval);

        private:
            QueryPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::QueryPool
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::QueryPoolDescriptor& descriptor) override;
            RHI::ResultCode InitQueryInternal(RHI::Query& query) override;
            RHI::ResultCode GetResultsInternal(uint32_t startIndex, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, RHI::QueryResultFlagBits flags) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::ResultCode BuildNativeQueryPool(Device& device, const RHI::QueryPoolDescriptor& descriptor);
            VkQueryPool m_nativeQueryPool = VK_NULL_HANDLE;
        };
    }
}
