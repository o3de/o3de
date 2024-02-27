/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <RHI/FrameGraphExecuteGroup.h>
#include <RHI/FrameGraphExecuteGroupHandler.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode FrameGraphExecuteGroupHandler::Init(
            Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups)
        {
            m_device = &device;
            m_executeGroups = executeGroups;
            m_hardwareQueueClass = static_cast<FrameGraphExecuteGroup*>(executeGroups.back())->GetHardwareQueueClass();

            return InitInternal(device, executeGroups);
        }

        void FrameGraphExecuteGroupHandler::End()
        {
            EndInternal();
            CommandQueue* cmdQueue = &m_device->GetCommandQueueContext().GetCommandQueue(m_hardwareQueueClass);
            cmdQueue->ExecuteWork(AZStd::move(m_workRequest));
#if defined(AZ_FORCE_CPU_GPU_INSYNC)
            //Cache the name of the scope we just queued and wait for it to finish on the cpu
            m_device->SetLastExecutingScope(m_workRequest.m_commandList->GetName().GetStringView());
            cmdQueue->FlushCommands();
            cmdQueue->WaitForIdle();
#endif
            m_isExecuted = true;
        }

        bool FrameGraphExecuteGroupHandler::IsComplete() const
        {
            for (const auto& group : m_executeGroups)
            {
                if (!group->IsComplete())
                {
                    return false;
                }
            }

            return true;
        }

        bool FrameGraphExecuteGroupHandler::IsExecuted() const
        {
            return m_isExecuted;
        }

        template<class T>
        void InsertWorkRequestElements(T& destination, const T& source)
        {
            destination.insert(destination.end(), source.begin(), source.end());
        }

        void FrameGraphExecuteGroupHandler::AddWorkRequest(const ExecuteWorkRequest& workRequest)
        {
            InsertWorkRequestElements(m_workRequest.m_swapChainsToPresent, workRequest.m_swapChainsToPresent);
            InsertWorkRequestElements(m_workRequest.m_semaphoresToWait, workRequest.m_semaphoresToWait);
            InsertWorkRequestElements(m_workRequest.m_semaphoresToSignal, workRequest.m_semaphoresToSignal);
            InsertWorkRequestElements(m_workRequest.m_fencesToSignal, workRequest.m_fencesToSignal);
            InsertWorkRequestElements(m_workRequest.m_fencesToWaitFor, workRequest.m_fencesToWaitFor);
        }
    }
}
