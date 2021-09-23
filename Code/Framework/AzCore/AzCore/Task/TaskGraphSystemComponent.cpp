/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Task/TaskGraphSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

// Create a cvar as a central location for experimentation with switching from the Job system to TaskGraph system.
AZ_CVAR(bool, cl_useTaskGraph, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Flag for use of TaskGraph (Note does not disable task graph system)");

namespace AZ
{
    void TaskGraphSystemComponent::Activate()
    {
        AZ_Assert(m_taskExecutor == nullptr, "Error multiple activation of the TaskGraphSystemComponent");

        Interface<UseTaskGraphInterface>::Register(this);
        m_taskExecutor = aznew TaskExecutor();
        m_taskExecutor->SetInstance(m_taskExecutor);
    }

    void TaskGraphSystemComponent::Deactivate()
    {
        Interface<UseTaskGraphInterface>::Unregister(this);
    }

    void TaskGraphSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("TaskExecutorService"));
    }

    void TaskGraphSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("TaskExecutorService"));
    }

    void TaskGraphSystemComponent::GetDependentServices([[maybe_unused]] ComponentDescriptor::DependencyArrayType& dependent)
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

    bool TaskGraphSystemComponent::UseTaskGraph() const
    {
        return cl_useTaskGraph;
    }
} // namespace AZ
