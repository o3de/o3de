/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
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

        //! Base class for handler classes that manage frame graph execute groups.
        //! Contains common functionality for all types of handlers including
        //! how execute groups are handled and how work requests are sent
        //! to the command queue.
        class FrameGraphExecuteGroupHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(FrameGraphExecuteGroupHandler, AZ::SystemAllocator);

            FrameGraphExecuteGroupHandler() = default;
            virtual ~FrameGraphExecuteGroupHandler() = default;

            RHI::ResultCode Init(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups);
            void End();

            bool IsComplete() const;
            bool IsExecuted() const;

        protected:
            virtual RHI::ResultCode InitInternal(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups) = 0;
            virtual void EndInternal() = 0;

            void AddWorkRequest(const ExecuteWorkRequest& workRequest);

            Device* m_device = nullptr;
            ExecuteWorkRequest m_workRequest;
            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;
            AZStd::vector<RHI::FrameGraphExecuteGroup*> m_executeGroups;
            bool m_isExecuted = false;
        };
    }
}
