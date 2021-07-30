/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/TaskFlowSystemComponent.h>
#include <AzCore/Jobs/taskflow/core/executor.hpp>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    TaskFlowSystemComponent::TaskFlowSystemComponent()
        : m_taskflowExecutor(nullptr)
    {
    }

    void TaskFlowSystemComponent::Activate()
    {
        AZ_Assert(m_taskflowExecutor == nullptr, "TaskFlow Executor should not exist at this time!");

        m_taskflowExecutor = new tf::Executor;

        AZ::Interface<AZ::TaskFlowExecutorInterface>::Register(this);
    }

    void TaskFlowSystemComponent::Deactivate()
    {
        AZ::Interface<AZ::TaskFlowExecutorInterface>::Unregister(this);

        delete m_taskflowExecutor;
        m_taskflowExecutor = nullptr;
    }

    void TaskFlowSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("TaskFlowService"));
    }

    void TaskFlowSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TaskFlowService"));
    }

    void TaskFlowSystemComponent::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<TaskFlowSystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }
} // namespace AZ
