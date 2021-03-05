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
#include "Atom_RHI_Vulkan_precompiled.h"
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
