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
    namespace DX12
    {
        void FrameGraphExecuteGroupBase::SetDevice(Device& device)
        {
            m_device = &device;
        }

        Device& FrameGraphExecuteGroupBase::GetDevice() const
        {
            return *m_device;
        }

        ExecuteWorkRequest&& FrameGraphExecuteGroupBase::MakeWorkRequest()
        {
#if defined(AZ_ENABLE_TRACING)
            if (AZ::RHI::Validation::IsEnabled())
            {
                for (CommandList* commandList : m_workRequest.m_commandLists)
                {
                    AZ_Assert(commandList && commandList->IsRecording() == false, "Command list not valid.");
                }
            }
#endif

            return AZStd::move(m_workRequest);
        }

        RHI::HardwareQueueClass FrameGraphExecuteGroupBase::GetHardwareQueueClass() const
        {
            return m_hardwareQueueClass;
        }

        CommandList* FrameGraphExecuteGroupBase::AcquireCommandList() const
        {
            return m_device->AcquireCommandList(m_hardwareQueueClass);
        }
    }
}
