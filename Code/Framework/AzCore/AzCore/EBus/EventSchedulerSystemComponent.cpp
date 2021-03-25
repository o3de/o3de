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

#include <AzCore/EBus/EventSchedulerSystemComponent.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Serialization/SerializeContext.h>

AZ_TYPE_SAFE_INTEGRAL_CVARBINDING(TimeMs);

namespace AZ
{
    AZ_CVAR(TimeMs, bg_maxScheduledEventProcessTimeMs, TimeMs{ 0 }, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The maximum number of milliseconds per frame to allow scheduled event execution. 0 means unlimited");

    void EventSchedulerSystemComponent::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<EventSchedulerSystemComponent, Component>()
                ->Version(1);
        }
    }

    void EventSchedulerSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("EventSchedulerService"));
    }

    void EventSchedulerSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("EventSchedulerService"));
    }

    EventSchedulerSystemComponent::EventSchedulerSystemComponent()
    {
        AZ::Interface<IEventScheduler>::Register(this);
        IEventSchedulerRequestBus::Handler::BusConnect();
    }

    EventSchedulerSystemComponent::~EventSchedulerSystemComponent()
    {
        IEventSchedulerRequestBus::Handler::BusDisconnect();
        Interface<IEventScheduler>::Unregister(this);
    }

    void EventSchedulerSystemComponent::Activate()
    {
        TickBus::Handler::BusConnect();
    }

    void EventSchedulerSystemComponent::Deactivate()
    {
        TickBus::Handler::BusDisconnect();
    }

    void EventSchedulerSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        TimeMs startTime = GetElapsedTimeMs();
        bool usingTimeslice = bg_maxScheduledEventProcessTimeMs != TimeMs{ 0 };

        while (!m_queue.empty())
        {
            ScheduledEventHandle* handle = m_queue.top();
            if (handle->GetExecuteTimeMs() > startTime)
            {
                break;
            }
            m_queue.pop();
            m_pendingQueue.push(handle);
        }

        while (!m_pendingQueue.empty())
        {
            if (usingTimeslice && (GetElapsedTimeMs() - startTime > bg_maxScheduledEventProcessTimeMs))
            {
                AZLOG_WARN("Failed to trigger all pending scheduled events, %u events remain on the pending queue", aznumeric_cast<uint32_t>(m_pendingQueue.size()));
                break;
            }
            ScheduledEventHandle* handle = m_pendingQueue.top();
            m_pendingQueue.pop();
            if (!handle->Notify()) // if Notify return false, the event has been deleted and we should delete its handle.
            {
                FreeHandle(handle);
            }
        }
    }

    int EventSchedulerSystemComponent::GetTickOrder()
    {
        // Tick after physics but before rendering
        return AZ::TICK_ATTACHMENT;
    }

    ScheduledEventHandle* EventSchedulerSystemComponent::AddEvent(ScheduledEvent* timedEvent, TimeMs durationMs)
    {
        if (durationMs < TimeMs{ 0 })
        {
            durationMs = TimeMs{ 0 };
        }

        TimeMs currentMilliseconds = GetElapsedTimeMs();
        if (timedEvent->m_handle == nullptr)
        {
            timedEvent->m_handle = AllocateHandle(TimeMs(currentMilliseconds + durationMs), durationMs, timedEvent);
        }
        timedEvent->m_timeInserted = currentMilliseconds;
        m_queue.push(timedEvent->m_handle);
        return timedEvent->m_handle;
    }

    void EventSchedulerSystemComponent::AddCallback(const AZStd::function<void()>& callback, const Name& eventName, TimeMs durationMs)
    {
        if (durationMs < TimeMs{ 0 })
        {
            durationMs = TimeMs{ 0 };
        }

        TimeMs currentMilliseconds = GetElapsedTimeMs();
        ScheduledEvent* timedEvent = AllocateManagedEvent(TimeMs(currentMilliseconds + durationMs), durationMs, callback, eventName);
        timedEvent->m_timeInserted = currentMilliseconds;
        m_queue.push(timedEvent->m_handle);
    }

    AZStd::size_t EventSchedulerSystemComponent::GetHandleCount() const
    {
        return m_handles.size();
    }

    AZStd::size_t EventSchedulerSystemComponent::GetFreeHandleCount() const
    {
        return m_freeHandles.size();
    }

    AZStd::size_t EventSchedulerSystemComponent::GetQueueSize() const
    {
        return m_queue.size();
    }

    void EventSchedulerSystemComponent::DumpStats([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZLOG_INFO("EventSchedulerSystemComponent::HandleCount = %u", aznumeric_cast<uint32_t>(GetHandleCount()));
        AZLOG_INFO("EventSchedulerSystemComponent::FreeHandleCount = %u", aznumeric_cast<uint32_t>(GetFreeHandleCount()));
        AZLOG_INFO("EventSchedulerSystemComponent::QueueSize = %u", aznumeric_cast<uint32_t>(GetQueueSize()));
    }

    ScheduledEventHandle* EventSchedulerSystemComponent::AllocateHandle(TimeMs executeTimeMs, TimeMs durationTimeMs, ScheduledEvent* scheduledEvent)
    {
        ScheduledEventHandle* result = nullptr;
        if (!m_freeHandles.empty())
        {
            result = m_freeHandles.back();
            m_freeHandles.pop_back();
        }
        else
        {
            m_handles.resize(m_handles.size() + 1);
            result = &(m_handles.back());
        }
        *result = ScheduledEventHandle(executeTimeMs, durationTimeMs, scheduledEvent, false);
        return result;
    }

    ScheduledEvent* EventSchedulerSystemComponent::AllocateManagedEvent(TimeMs executeTimeMs, TimeMs durationTimeMs, const AZStd::function<void()>& callback, const Name& eventName)
    {
        ScheduledEvent* result = new ScheduledEvent(callback, eventName);
        ScheduledEventHandle* handle = nullptr;
        if (!m_freeHandles.empty())
        {
            handle = m_freeHandles.back();
            m_freeHandles.pop_back();
        }
        else
        {
            m_handles.resize(m_handles.size() + 1);
            handle = &(m_handles.back());
        }
        *handle = ScheduledEventHandle(executeTimeMs, durationTimeMs, result, true);
        result->m_handle = handle;
        return result;
    }

    void EventSchedulerSystemComponent::FreeHandle(ScheduledEventHandle* handle)
    {
        m_freeHandles.push_back(handle);
    }
}
