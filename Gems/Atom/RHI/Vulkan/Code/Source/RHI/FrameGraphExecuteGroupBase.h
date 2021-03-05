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

#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
#include <RHI/Scope.h>
#include <RHI/CommandQueue.h>
#include <Atom/RHI/FrameGraphExecuteGroup.h>

namespace AZ
{
    namespace Vulkan
    {
        //! Base class for the Execute group classes.
        //! It encapsulates the common functionality for Execute groups
        //! like the work request that will be used and the hardwareQueueClass for
        //! all scopes in the group.
        class FrameGraphExecuteGroupBase
            : public RHI::FrameGraphExecuteGroup
        {
            using Base = RHI::FrameGraphExecuteGroup;
        public:
            FrameGraphExecuteGroupBase() = default;
            AZ_RTTI(FrameGraphExecuteGroupBase, "{8F39B46F-37D3-4026-AB1A-A78F646F311B}", Base);

            void InitBase(Device& device, const RHI::GraphGroupId& groupId, RHI::HardwareQueueClass hardwareQueueClass);

            const ExecuteWorkRequest& GetWorkRequest() const;

            RHI::HardwareQueueClass GetHardwareQueueClass() const;

            const RHI::GraphGroupId& GetGroupId() const;

            virtual AZStd::array_view<const Scope*> GetScopes() const = 0;

            virtual AZStd::array_view<RHI::Ptr<CommandList>> GetCommandLists() const = 0;

        protected:
            RHI::Ptr<CommandList> AcquireCommandList(VkCommandBufferLevel level) const;

            Device* m_device = nullptr;
            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;
            ExecuteWorkRequest m_workRequest;
            RHI::GraphGroupId m_groupId;
        };
    }
}
