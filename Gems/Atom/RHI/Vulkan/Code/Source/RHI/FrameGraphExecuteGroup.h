/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
#include <RHI/Scope.h>

namespace AZ
{
    namespace Vulkan
    {
        //! Base class for the Execute group classes.
        //! It encapsulates the common functionality for Execute groups
        //! like the work request that will be used and the hardwareQueueClass for
        //! all scopes in the group.
        class FrameGraphExecuteGroup
            : public RHI::FrameGraphExecuteGroup
        {
            using Base = RHI::FrameGraphExecuteGroup;
        public:
            FrameGraphExecuteGroup() = default;
            AZ_RTTI(FrameGraphExecuteGroup, "{8F39B46F-37D3-4026-AB1A-A78F646F311B}", Base);

            void InitBase(Device& device, const RHI::GraphGroupId& groupId, RHI::HardwareQueueClass hardwareQueueClass);

            Device& GetDevice() const;

            const ExecuteWorkRequest& GetWorkRequest() const;

            RHI::HardwareQueueClass GetHardwareQueueClass() const;

            const RHI::GraphGroupId& GetGroupId() const;

            virtual AZStd::span<const Scope* const> GetScopes() const = 0;
            virtual AZStd::span<Scope* const> GetScopes() = 0;

            virtual AZStd::span<const RHI::Ptr<CommandList>> GetCommandLists() const = 0;

        protected:
            RHI::Ptr<CommandList> AcquireCommandList(VkCommandBufferLevel level) const;

            Device* m_device = nullptr;
            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;
            ExecuteWorkRequest m_workRequest;
            RHI::GraphGroupId m_groupId;
        };
    }
}
