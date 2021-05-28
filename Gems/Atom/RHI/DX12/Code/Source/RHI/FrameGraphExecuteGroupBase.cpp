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
#include "RHI/Atom_RHI_DX12_precompiled.h"
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
