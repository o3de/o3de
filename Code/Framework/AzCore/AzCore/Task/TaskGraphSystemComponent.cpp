/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Task/TaskGraphSystemComponent.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    void TaskGraphSystemComponent::Activate()
    {
        AZ_Assert(m_taskExecutor == nullptr, "Error multiple activation of the TaskGraphSystemComponent");

        m_taskExecutor = aznew TaskExecutor();
        m_taskExecutor->SetInstance(m_taskExecutor);
    }

    void TaskGraphSystemComponent::Deactivate()
    {
    }

    void TaskGraphSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("TaskExecutorService"));
    }

    void TaskGraphSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("TaskExecutorService"));
    }

    void TaskGraphSystemComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void TaskGraphSystemComponent::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<TaskGraphSystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }
} // namespace AZ
