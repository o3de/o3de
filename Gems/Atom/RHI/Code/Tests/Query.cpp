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

    RHI::ResultCode QueryPool::InitQueryInternal([[maybe_unused]] RHI::Query& query)
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
