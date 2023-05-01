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
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Threading/ThreadUtils.h>

 // PERFORMANCE NOTE & TODO
 // Profiling Ros Con demo, Task Graph was 2-3ms slower than Jobs
 // Time for Jobs was ~5.5ms, maxing out at ~6.3ms
 // Task Graph took ~7.7ms, maxing out at ~8.7ms
 // Disabling cl_activateTaskGraph for performance reasons for the time being

// Create a cvar as a central location for experimentation with switching from the Job system to TaskGraph system.
AZ_CVAR(bool, cl_activateTaskGraph, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Flag clients of TaskGraph to switch between jobs/taskgraph (Note does not disable task graph system)");
AZ_CVAR(float, cl_taskGraphThreadsConcurrencyRatio, 1.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "TaskGraph calculate the number of worker threads to spawn by scaling the number of hw threads, value is clamped between 0.0f and 1.0f");
AZ_CVAR(uint32_t, cl_taskGraphThreadsNumReserved, 2, nullptr, AZ::ConsoleFunctorFlags::Null, "TaskGraph number of hardware threads that are reserved for O3DE system threads. Value is clamped between 0 and the number of logical cores in the system");
AZ_CVAR(uint32_t, cl_taskGraphThreadsMinNumber, 2, nullptr, AZ::ConsoleFunctorFlags::Null, "TaskGraph minimum number of worker threads to create after scaling the number of hw threads");
AZ_CVAR(uint32_t, cl_taskGraphThreadsMaxNumber, 0, nullptr, AZ::ConsoleFunctorFlags::Null, "TaskGraph maximum number of worker threads to create after scaling the number of hw threads (0 indicates uncapped)");

static constexpr uint32_t TaskExecutorServiceCrc = AZ_CRC_CE("TaskExecutorService");

namespace AZ
{
    void TaskGraphSystemComponent::Activate()
    {
        AZ_Assert(m_taskExecutor == nullptr, "Error multiple activation of the TaskGraphSystemComponent");

        AZ::ApplicationTypeQuery appType;
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
        if (!appType.IsValid() || (appType.IsTool() && !appType.IsEditor()))
        {
            // Tools generally do not rely on the task graph or high degrees of concurrency. For now, limit concurrency
            // for non-editor tools to conserve memory and thread-spawn overhead.
            cl_taskGraphThreadsMaxNumber = 1;
        }

        if (Interface<TaskGraphActiveInterface>::Get() == nullptr)
        {
        #if (AZ_TRAIT_THREAD_NUM_TASK_GRAPH_WORKER_THREADS)
            const uint32_t numberOfWorkerThreads = AZ_TRAIT_THREAD_NUM_TASK_GRAPH_WORKER_THREADS;
        #else
            const uint32_t numberOfWorkerThreads = Threading::CalcNumWorkerThreads(
                cl_taskGraphThreadsConcurrencyRatio, cl_taskGraphThreadsMinNumber, cl_taskGraphThreadsMaxNumber,
                cl_taskGraphThreadsNumReserved);
        #endif // (AZ_TRAIT_THREAD_NUM_TASK_GRAPH_WORKER_THREADS)
            Interface<TaskGraphActiveInterface>::Register(this); // small window that another thread can try to use taskgraph between this line and the set instance.
            m_taskExecutor = aznew TaskExecutor(numberOfWorkerThreads);
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
                    ;
            }
        }
    }

    bool TaskGraphSystemComponent::IsTaskGraphActive() const
    {
        return cl_activateTaskGraph;
    }
} // namespace AZ
