/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tests/Query.h>

namespace UnitTest
{
    using namespace AZ;

    RHI::ResultCode Query::BeginInternal([[maybe_unused]] RHI::CommandList& commandList, [[maybe_unused]] RHI::QueryControlFlags flags)
    {
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode Query::EndInternal([[maybe_unused]] RHI::CommandList& commandList)
    {
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode Query::WriteTimestampInternal([[maybe_unused]] RHI::CommandList& commandList)
    {
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode QueryPool::InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::QueryPoolDescriptor& descriptor)
    {
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode QueryPool::InitQueryInternal([[maybe_unused]] RHI::DeviceQuery& query)
    {
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode QueryPool::GetResultsInternal(uint32_t startIndex, uint32_t queryCount, uint64_t* results, [[maybe_unused]] uint32_t resultsCount, [[maybe_unused]] RHI::QueryResultFlagBits flags)
    {
        m_calledIntervals.emplace_back(startIndex, startIndex + queryCount - 1);
        for (uint32_t i = 0; i < queryCount; ++i)
        {
            results[i] = startIndex + i;
        }
        return RHI::ResultCode::Success;
    }
}
