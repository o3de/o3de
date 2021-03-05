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
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>

#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace RHI
    {
        class FrameGraphExecuteGroup;
    }

    namespace Vulkan
    {
        class Device;

        //! Base class for the handler classes that manage the execute groups.
        //! Contain the common functionality for all type of handlers like
        //! the execute groups are handled and the work request that will be send
        //! to the command queue.
        class FrameGraphExecuteGroupHandlerBase
        {
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupHandlerBase, AZ::SystemAllocator, 0);

            FrameGraphExecuteGroupHandlerBase() = default;
            virtual ~FrameGraphExecuteGroupHandlerBase() = default;

            RHI::ResultCode Init(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups);
            void End();

            bool IsComplete() const;

        protected:
            virtual RHI::ResultCode InitInternal(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups) = 0;
            virtual void EndInternal() = 0;

            void AddWorkRequest(const ExecuteWorkRequest& workRequest);

            Device* m_device = nullptr;
            ExecuteWorkRequest m_workRequest;
            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;
            AZStd::vector<RHI::FrameGraphExecuteGroup*> m_executeGroups;
        };
    }
}
