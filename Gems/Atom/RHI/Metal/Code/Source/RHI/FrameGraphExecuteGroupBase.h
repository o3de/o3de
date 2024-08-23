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
        
        class FrameGraphExecuteGroupBase
            : public RHI::FrameGraphExecuteGroup
        {
            using Base = RHI::FrameGraphExecuteGroup;
        public:
            FrameGraphExecuteGroupBase() = default;

            using CallbackFunc = AZStd::function<void(const FrameGraphExecuteGroupBase* groupBase)>;
            void InitBase(
                Device& device,
                const RHI::GraphGroupId& groupId,
                RHI::HardwareQueueClass hardwareQueueClass);

            Device& GetDevice() const;
            ExecuteWorkRequest&& AcquireWorkRequest();

            RHI::HardwareQueueClass GetHardwareQueueClass();
            
            const RHI::GraphGroupId& GetGroupId() const;
            
            //! Set the command buffer that the group will use.
            void SetCommandBuffer(RHI::Ptr<CommandQueueCommandBuffer> commandBuffer);
            
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
            RHI::Ptr<CommandQueueCommandBuffer> m_commandBuffer;
            
        private:
            Device* m_device = nullptr;
            RHI::GraphGroupId m_groupId;
            FrameGraphExecuteGroupHandler* m_handler = nullptr;
            NSAutoreleasePool* m_groupAutoreleasePool = nullptr;
            AZStd::vector<NSAutoreleasePool*> m_contextAutoreleasePools;
        };
    }
}
