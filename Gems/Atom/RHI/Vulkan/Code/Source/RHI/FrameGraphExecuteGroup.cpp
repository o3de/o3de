/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        void FrameGraphExecuteGroup::InitBase(Device& device, const RHI::GraphGroupId& groupId, RHI::HardwareQueueClass hardwareQueueClass)
        {
            m_device = &device;
            m_groupId = groupId;
            m_hardwareQueueClass = hardwareQueueClass;
        }

        Device& FrameGraphExecuteGroup::GetDevice() const
        {
            return *m_device;
        }

        const ExecuteWorkRequest& FrameGraphExecuteGroup::GetWorkRequest() const
        {
            return m_workRequest;
        }

        RHI::HardwareQueueClass FrameGraphExecuteGroup::GetHardwareQueueClass() const
        {
            return m_hardwareQueueClass;
        }

        const RHI::GraphGroupId& FrameGraphExecuteGroup::GetGroupId() const
        {
            return m_groupId;
        }

        RHI::Ptr<CommandList> FrameGraphExecuteGroup::AcquireCommandList(VkCommandBufferLevel level) const
        {
            return m_device->AcquireCommandList(m_hardwareQueueClass, level);
        }
    }
}
