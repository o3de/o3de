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
#include "Atom_RHI_Vulkan_precompiled.h"
#include <RHI/CommandList.h>
#include <RHI/Conversion.h>
#include <RHI/Query.h>
#include <RHI/QueryPool.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<Query> Query::Create()
        {
            return aznew Query();
        }

        RHI::ResultCode Query::BeginInternal(RHI::CommandList& commandListBase, RHI::QueryControlFlags flags)
        {
            auto* queryPool = static_cast<const QueryPool*>(GetQueryPool());
            auto& commandList = static_cast<CommandList&>(commandListBase);
            
            vkCmdBeginQuery(
                commandList.GetNativeCommandBuffer(),
                queryPool->GetNativeQueryPool(),
                GetHandle().GetIndex(),
                ConvertQueryControlFlags(flags));

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Query::EndInternal(RHI::CommandList& commandListBase)
        {
            auto* queryPool = static_cast<const QueryPool*>(GetQueryPool());
            auto& commandList = static_cast<CommandList&>(commandListBase);
            vkCmdEndQuery(
                commandList.GetNativeCommandBuffer(),
                queryPool->GetNativeQueryPool(),
                GetHandle().GetIndex());

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Query::WriteTimestampInternal(RHI::CommandList& commandListBase)
        {
            auto* queryPool = static_cast<const QueryPool*>(GetQueryPool());
            auto& commandList = static_cast<CommandList&>(commandListBase);

            vkCmdWriteTimestamp(
                commandList.GetNativeCommandBuffer(),
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                queryPool->GetNativeQueryPool(),
                GetHandle().GetIndex()
            );
            return RHI::ResultCode::Success;
        }
    }
}