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
        class FrameGraphExecuteGroupBase
            : public RHI::FrameGraphExecuteGroup
        {
            using Base = RHI::FrameGraphExecuteGroup;
        public:
            FrameGraphExecuteGroupBase() = default;
            
            void SetDevice(Device& device);
            ExecuteWorkRequest&& AcquireWorkRequest();

            RHI::HardwareQueueClass GetHardwareQueueClass();
            
        protected:
            CommandList* AcquireCommandList() const;
            void FlushAutoreleasePool();
            void CreateAutoreleasePool();
            
            //! Go through all the wait fences across all queues and encode them if needed
            void EncodeWaitEvents();
            
            ExecuteWorkRequest m_workRequest;
            
            //! Command Buffer associated with this group. It will allocate encoders out of the queue related to m_hardwareQueueClass
            CommandQueueCommandBuffer m_commandBuffer;
            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;
        private:
            Device* m_device = nullptr;
            NSAutoreleasePool* m_autoreleasePool = nullptr;
        };
    }
}
