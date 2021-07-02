/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/CommandList.h>
#include <RHI/Conversions.h>
#include <RHI/Query.h>
#include <RHI/QueryPool.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<Query> Query::Create()
        {
            return aznew Query();
        }

        RHI::ResultCode Query::BeginInternal(RHI::CommandList& baseCommandList, RHI::QueryControlFlags flags)
        {
            auto& commandList = static_cast<CommandList&>(baseCommandList);
            auto* queryPool = static_cast<QueryPool*>(GetQueryPool());
            m_currentControlFlags = flags;
            commandList.GetCommandList()->BeginQuery(
                queryPool->GetHeap(), 
                ConvertQueryType(queryPool->GetDescriptor().m_type, m_currentControlFlags), 
                GetHandle().GetIndex());
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Query::EndInternal(RHI::CommandList& baseCommandList)
        {
            auto& commandList = static_cast<CommandList&>(baseCommandList);
            auto* queryPool = static_cast<QueryPool*>(GetQueryPool());
            D3D12_QUERY_TYPE type = ConvertQueryType(queryPool->GetDescriptor().m_type, m_currentControlFlags);
            commandList.GetCommandList()->EndQuery(
                queryPool->GetHeap(), 
                type,
                GetHandle().GetIndex());

            queryPool->OnQueryEnd(*this, type);
            m_currentControlFlags = RHI::QueryControlFlags::None;
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Query::WriteTimestampInternal(RHI::CommandList& commandList)
        {
            // Timestamp queries write using the EndQuery function.
            return EndInternal(commandList);
        }
    }
}
