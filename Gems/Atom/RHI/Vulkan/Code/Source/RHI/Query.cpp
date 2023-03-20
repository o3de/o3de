/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/CommandList.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
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

            static_cast<Device&>(GetDevice())
                .GetContext()
                .CmdBeginQuery(
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
            static_cast<Device&>(GetDevice())
                .GetContext()
                .CmdEndQuery(commandList.GetNativeCommandBuffer(), queryPool->GetNativeQueryPool(), GetHandle().GetIndex());

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode Query::WriteTimestampInternal(RHI::CommandList& commandListBase)
        {
            auto* queryPool = static_cast<const QueryPool*>(GetQueryPool());
            auto& commandList = static_cast<CommandList&>(commandListBase);

            static_cast<Device&>(GetDevice())
                .GetContext()
                .CmdWriteTimestamp(
                    commandList.GetNativeCommandBuffer(),
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    queryPool->GetNativeQueryPool(),
                    GetHandle().GetIndex());
            return RHI::ResultCode::Success;
        }
    }
}
