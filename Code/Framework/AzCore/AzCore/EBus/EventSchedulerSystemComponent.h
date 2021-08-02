/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/IEventScheduler.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Component/Component.h>
#include <AzCore/EBus/ScheduledEventHandle.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/queue.h>

namespace AZ
{
    //! @struct CompareScheduledEventPtrs
    //! Comparison of scheduled events to add in the priority queue.
    struct CompareScheduledEventPtrs
    {
        bool operator() (const ScheduledEventHandle* lhs, const ScheduledEventHandle* rhs) const
        {
            return (*lhs) > (*rhs);
        }
    };

    //! @struct PrioritizeScheduledEventPtrs
    //! Prioritization operator for scheduled events to add in the priority queue.
    struct PrioritizeScheduledEventPtrs
    {
        bool operator() (const ScheduledEventHandle* lhs, const ScheduledEventHandle* rhs) const
        {
            return (lhs->GetExecuteTimeMs() + lhs->GetDurationTimeMs()) > (rhs->GetExecuteTimeMs() + rhs->GetDurationTimeMs());
        }
    };

    class Name;
    class ScheduledEvent;

    //! @class EventSchedulerSystemComponent
    //! @brief This is scheduled event queue class to run all scheduled events at appropriate intervals.
    class EventSchedulerSystemComponent
        : public Component
        , public TickBus::Handler
        , public IEventSchedulerRequestBus::Handler
    {
    public:
        AZ_COMPONENT(EventSchedulerSystemComponent, "{7D902EAC-A382-4530-8DE2-E7A3D7985DF9}");

        static void Reflect(ReflectContext* context);
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);

        EventSchedulerSystemComponent();
        ~EventSchedulerSystemComponent() override;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! AZ::TickBus::Handler overrides.
        //! @{
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        //! @}

        //! IEventScheduler interface
        //! @{
        ScheduledEventHandle* AddEvent(ScheduledEvent* scheduledEvent, TimeMs durationMs) override;
        void AddCallback(const AZStd::function<void()>& callback, const Name& eventName, TimeMs durationMs) override;
        // @}

        //! EventSchedulerSystemComponent stats
        //! @{
        AZStd::size_t GetHandleCount() const;
        AZStd::size_t GetFreeHandleCount() const;
        AZStd::size_t GetQueueSize() const;
        void DumpStats(const AZ::ConsoleCommandContainer& arguments);
        //! @}

    private:
        ScheduledEventHandle* AllocateHandle();

        //! Allocates a single use event to capture the passed in callback. Event is cleaned up on completion.
        ScheduledEvent* AllocateManagedEvent(const AZStd::function<void()>& callback, const Name& eventName);

        void FreeHandle(ScheduledEventHandle* handle);

        // Bind the DumpStats member function to the console as 'EventSchedulerSystemComponent.DumpStats'
        AZ_CONSOLEFUNC(EventSchedulerSystemComponent, DumpStats, AZ::ConsoleFunctorFlags::Null, "Dump EventSchedulerSystemComponent stats to the console window");

        // Priority queues of scheduled events sorted by execution time
        AZStd::priority_queue<ScheduledEventHandle*, AZStd::vector<ScheduledEventHandle*>, CompareScheduledEventPtrs> m_queue;
        AZStd::priority_queue<ScheduledEventHandle*, AZStd::vector<ScheduledEventHandle*>, PrioritizeScheduledEventPtrs> m_pendingQueue;
        AZStd::deque<ScheduledEvent> m_ownedEvents;
        AZStd::vector<ScheduledEvent*> m_freeEvents;
        AZStd::deque<ScheduledEventHandle> m_handles;
        AZStd::vector<ScheduledEventHandle*> m_freeHandles;
    };
}
