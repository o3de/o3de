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
#include <AzCore/Serialization/EditContext.h>

// Create a cvar as a central location for experimentation with switching from the Job system to TaskGraph system.
AZ_CVAR(bool, cl_useTaskGraph, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Flag for use of TaskGraph (Note does not disable task graph system)");

namespace AZ
{
    void TaskGraphSystemComponent::Activate()
    {
        AZ_Assert(m_taskExecutor == nullptr, "Error multiple activation of the TaskGraphSystemComponent");

        if (Interface<UseTaskGraphInterface>::Get() == nullptr)
        {
            Interface<UseTaskGraphInterface>::Register(this);
            m_taskExecutor = aznew TaskExecutor();
            m_taskExecutor->SetInstance(m_taskExecutor);
        }
    }

    void TaskGraphSystemComponent::Deactivate()
    {
        if (&TaskExecutor::Instance() == m_taskExecutor) // check that our instance is the global instance (not always true in unit tests)
        {
            m_taskExecutor->SetInstance(nullptr);
        }
        if (m_taskExecutor)
        {
            azdestroy(m_taskExecutor);
            m_taskExecutor = nullptr;
        }
        if (Interface<UseTaskGraphInterface>::Get() == this)
        {
            Interface<UseTaskGraphInterface>::Unregister(this);
        }
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

            if (AZ::EditContext* ec = serializeContext->GetEditContext())
            {
                ec->Class<TaskGraphSystemComponent>
                    ("TaskGraph", "System component to create the default executor")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ;
            }
        }
    }

    bool TaskGraphSystemComponent::UseTaskGraph() const
    {
        return cl_useTaskGraph;
    }
} // namespace AZ
