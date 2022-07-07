/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <RHI/FrameGraphExecuteGroupBase.h>
#include <RHI/FrameGraphExecuteGroupHandlerBase.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode FrameGraphExecuteGroupHandlerBase::Init(Device& device, const AZStd::vector<RHI::FrameGraphExecuteGroup*>& executeGroups)
        {
            m_device = &device;
            m_executeGroups = executeGroups;
            m_hardwareQueueClass = static_cast<FrameGraphExecuteGroupBase*>(executeGroups.back())->GetHardwareQueueClass();

            return InitInternal(device, executeGroups);
        }

        void FrameGraphExecuteGroupHandlerBase::End()
        {
            EndInternal();
            m_device->GetCommandQueueContext().GetCommandQueue(m_hardwareQueueClass).ExecuteWork(AZStd::move(m_workRequest));
            m_isExecuted = true;
        }

        bool FrameGraphExecuteGroupHandlerBase::IsComplete() const
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

        bool FrameGraphExecuteGroupHandlerBase::IsExecuted() const
        {
            return m_isExecuted;
        }

        template<class T>
        void InsertWorkRequestElements(T& destination, const T& source)
        {
            destination.insert(destination.end(), source.begin(), source.end());
        }

        void FrameGraphExecuteGroupHandlerBase::AddWorkRequest(const ExecuteWorkRequest& workRequest)
        {
            InsertWorkRequestElements(m_workRequest.m_swapChainsToPresent, workRequest.m_swapChainsToPresent);
            InsertWorkRequestElements(m_workRequest.m_semaphoresToWait, workRequest.m_semaphoresToWait);
            InsertWorkRequestElements(m_workRequest.m_semaphoresToSignal, workRequest.m_semaphoresToSignal);
            InsertWorkRequestElements(m_workRequest.m_fencesToSignal, workRequest.m_fencesToSignal);
        }
    }
}
