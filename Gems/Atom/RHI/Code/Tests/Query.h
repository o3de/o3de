/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/RHI/Query.h>
#include <Atom/RHI/QueryPool.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class Query
        : public AZ::RHI::Query
    {
    public:
        AZ_CLASS_ALLOCATOR(Query, AZ::SystemAllocator, 0);
        
    private:
        AZ::RHI::ResultCode BeginInternal(AZ::RHI::CommandList& commandList, AZ::RHI::QueryControlFlags flags) override;
        AZ::RHI::ResultCode EndInternal(AZ::RHI::CommandList& commandList) override;
        AZ::RHI::ResultCode WriteTimestampInternal(AZ::RHI::CommandList& commandList) override;
    };

    class QueryPool
        : public AZ::RHI::QueryPool
    {
    public:
        AZ_CLASS_ALLOCATOR(QueryPool, AZ::SystemAllocator, 0);

        AZStd::vector<AZ::RHI::Interval> m_calledIntervals;

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device& device, const AZ::RHI::QueryPoolDescriptor& descriptor) override;
        AZ::RHI::ResultCode InitQueryInternal(AZ::RHI::Query& query) override;
        AZ::RHI::ResultCode GetResultsInternal(uint32_t startIndex, uint32_t queryCount, uint64_t* results, uint32_t resultsCount, AZ::RHI::QueryResultFlagBits flags) override;
    };
}
