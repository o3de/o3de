/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/FrameGraphExecuteGroupBase.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        void FrameGraphExecuteGroupBase::InitBase(Device& device, const RHI::GraphGroupId& groupId, RHI::HardwareQueueClass hardwareQueueClass)
        {
            m_device = &device;
            m_groupId = groupId;
            m_hardwareQueueClass = hardwareQueueClass;
        }

        const ExecuteWorkRequest& FrameGraphExecuteGroupBase::GetWorkRequest() const
        {
            return m_workRequest;
        }

        RHI::HardwareQueueClass FrameGraphExecuteGroupBase::GetHardwareQueueClass() const
        {
            return m_hardwareQueueClass;
        }

        const RHI::GraphGroupId& FrameGraphExecuteGroupBase::GetGroupId() const
        {
            return m_groupId;
        }

        RHI::Ptr<CommandList> FrameGraphExecuteGroupBase::AcquireCommandList(VkCommandBufferLevel level) const
        {
            return m_device->AcquireCommandList(m_hardwareQueueClass, level);
        }
    }
}
