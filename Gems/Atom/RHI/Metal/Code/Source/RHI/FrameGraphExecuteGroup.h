/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <RHI/CommandQueue.h>

namespace AZ
{
    namespace Metal
    {
        class Device;
        class CommandQueueCommandBuffer;
        class FrameGraphExecuteGroupHandler;
        
        class FrameGraphExecuteGroup
            : public RHI::FrameGraphExecuteGroup
        {
            using Base = RHI::FrameGraphExecuteGroup;
        public:
            FrameGraphExecuteGroup() = default;

            using CallbackFunc = AZStd::function<void(const FrameGraphExecuteGroup* groupBase)>;
            void InitBase(
                Device& device,
                const RHI::GraphGroupId& groupId,
                RHI::HardwareQueueClass hardwareQueueClass);

            Device& GetDevice() const;
            ExecuteWorkRequest&& AcquireWorkRequest();

            RHI::HardwareQueueClass GetHardwareQueueClass();

            //! Returns the group id of this FrameGraphExecuteGroup (used for subpass grouping)
            const RHI::GraphGroupId& GetGroupId() const;
            
            //! Set the command buffer that the group will use.
            void SetCommandBuffer(CommandQueueCommandBuffer* commandBuffer);

            //! Set the FrameGraphExecuteGroupHandler that this group belongs.
            void SetHandler(FrameGraphExecuteGroupHandler* handler);
            
            virtual AZStd::span<const Scope* const> GetScopes() const = 0;
            virtual AZStd::span<Scope* const> GetScopes() = 0;
            
        protected:
            void BeginInternal() override;
            void EndInternal() override;
            void BeginContextInternal(
                RHI::FrameGraphExecuteContext& context,
                uint32_t contextIndex) override;
            void EndContextInternal(
                RHI::FrameGraphExecuteContext& context,
                uint32_t contextIndex) override;

            CommandList* AcquireCommandList() const;
            
            //! Go through all the wait fences across all queues and encode them if needed
            void EncodeWaitEvents() const;
            
            ExecuteWorkRequest m_workRequest;
            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;
            CommandQueueCommandBuffer* m_commandBuffer = nullptr;
            
        private:
            Device* m_device = nullptr;
            RHI::GraphGroupId m_groupId;
            FrameGraphExecuteGroupHandler* m_handler = nullptr;
            //! Autorelease pool for the group
            NSAutoreleasePool* m_groupAutoreleasePool = nullptr;
            //! Autorelease pool for each context used (may be a different thread from the group)
            AZStd::vector<NSAutoreleasePool*> m_contextAutoreleasePools;
        };
    }
}
