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
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Threading/ThreadUtils.h>

// Create a cvar as a central location for experimentation with switching from the Job system to TaskGraph system.
AZ_CVAR(bool, cl_activateTaskGraph, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Flag clients of TaskGraph to switch between jobs/taskgraph (Note does not disable task graph system)");
AZ_CVAR(float, cl_taskGraphThreadsConcurrencyRatio, 1.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "TaskGraph calculate the number of worker threads to spawn by scaling the number of hw threads, value is clamped between 0.0f and 1.0f");
AZ_CVAR(uint32_t, cl_taskGraphThreadsNumReserved, 2, nullptr, AZ::ConsoleFunctorFlags::Null, "TaskGraph number of hardware threads that are reserved for O3DE system threads. Value is clamped between 0 and the number of logical cores in the system");
AZ_CVAR(uint32_t, cl_taskGraphThreadsMinNumber, 2, nullptr, AZ::ConsoleFunctorFlags::Null, "TaskGraph minimum number of worker threads to create after scaling the number of hw threads");

static constexpr uint32_t TaskExecutorServiceCrc = AZ_CRC_CE("TaskExecutorService");

namespace AZ
{
    void TaskGraphSystemComponent::Activate()
    {
        AZ_Assert(m_taskExecutor == nullptr, "Error multiple activation of the TaskGraphSystemComponent");

        if (Interface<TaskGraphActiveInterface>::Get() == nullptr)
        {
            Interface<TaskGraphActiveInterface>::Register(this); // small window that another thread can try to use taskgraph between this line and the set instance.
            m_taskExecutor = aznew TaskExecutor(Threading::CalcNumWorkerThreads(cl_taskGraphThreadsConcurrencyRatio, cl_taskGraphThreadsMinNumber, cl_taskGraphThreadsNumReserved));
            TaskExecutor::SetInstance(m_taskExecutor);
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
        if (Interface<TaskGraphActiveInterface>::Get() == this)
        {
            Interface<TaskGraphActiveInterface>::Unregister(this);
        }
    }

    void TaskGraphSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(TaskExecutorServiceCrc);
    }

    void TaskGraphSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(TaskExecutorServiceCrc);
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

    bool TaskGraphSystemComponent::IsTaskGraphActive() const
    {
        return cl_activateTaskGraph;
    }
} // namespace AZ
