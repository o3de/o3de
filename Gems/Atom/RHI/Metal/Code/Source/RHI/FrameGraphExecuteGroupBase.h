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
