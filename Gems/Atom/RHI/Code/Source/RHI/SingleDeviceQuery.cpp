/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/SingleDeviceQuery.h>
#include <Atom/RHI/SingleDeviceQueryPool.h>

namespace AZ::RHI
{
    void SingleDeviceQuery::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
    {
        AZ_UNUSED(builder);
    }

    QueryHandle SingleDeviceQuery::GetHandle() const
    {
        return m_handle;
    }

    const SingleDeviceQueryPool* SingleDeviceQuery::GetQueryPool() const
    {
        return static_cast<const SingleDeviceQueryPool*>(GetPool());
    }

    SingleDeviceQueryPool* SingleDeviceQuery::GetQueryPool()
    {
        return static_cast<SingleDeviceQueryPool*>(GetPool());
    }

    ResultCode SingleDeviceQuery::Begin(CommandList& commandList, QueryControlFlags flags /*= QueryControlFlags::None*/)
    {
        if (Validation::IsEnabled())
        {
            if (m_currentCommandList)
            {
                AZ_Error("RHI", false, "SingleDeviceQuery was never ended");
                return ResultCode::Fail;
            }

            auto poolType = GetQueryPool()->GetDescriptor().m_type;
            if (poolType != QueryType::Occlusion && RHI::CheckBitsAny(flags, QueryControlFlags::PreciseOcclusion))
            {
                AZ_Error("RHI", false, "Precise Occlusion is only available for occlusion type queries");
                return ResultCode::InvalidArgument;
            }

            if (poolType == QueryType::Timestamp)
            {
                AZ_Error("RHI", false, "Begin is not valid for timestamp queries");
                return ResultCode::Fail;
            }
        }
            
        m_currentCommandList = &commandList;
        return BeginInternal(commandList, flags);
    }

    ResultCode SingleDeviceQuery::End(CommandList& commandList)
    {
        if (Validation::IsEnabled())
        {
            if (GetQueryPool()->GetDescriptor().m_type == QueryType::Timestamp)
            {
                AZ_Error("RHI", false, "End operation is not valid for timestamp queries");
                return ResultCode::Fail;
            }

            if (!m_currentCommandList)
            {
                AZ_Error("RHI", false, "SingleDeviceQuery must begin before it can end");
                return ResultCode::Fail;
            }

            // Check that the command list used for begin is the same one.
            if (m_currentCommandList != &commandList)
            {
                AZ_Error("RHI", false, "A different command list was passed when ending the query");
                return ResultCode::InvalidArgument;
            }
        }

        auto result = EndInternal(commandList);
        m_currentCommandList = nullptr;
        return result;
    }

    ResultCode SingleDeviceQuery::WriteTimestamp(CommandList& commandList)
    {
        if (Validation::IsEnabled())
        {
            if (GetQueryPool()->GetDescriptor().m_type != QueryType::Timestamp)
            {
                AZ_Error("RHI", false, "Only timestamp queries support WriteTimestamp");
                return ResultCode::Fail;
            }
        }

        auto result = WriteTimestampInternal(commandList);
        return result;
    }
}
