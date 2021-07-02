/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
