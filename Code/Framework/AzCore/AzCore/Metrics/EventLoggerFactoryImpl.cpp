/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EventLoggerFactoryImpl.h"
#include <AzCore/Metrics/IEventLogger.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/ranges/ranges_algorithm.h>

namespace AZ::Metrics
{

    void EventLoggerFactoryImpl::EventLoggerDeleter::operator()(IEventLogger* ptr) const
    {
        if (m_delete)
        {
            delete ptr;
        }
    }

    EventLoggerFactoryImpl::EventLoggerFactoryImpl() = default;
    EventLoggerFactoryImpl::~EventLoggerFactoryImpl() = default;

    void EventLoggerFactoryImpl::VisitEventLoggers(const VisitEventLoggerInterfaceCallback& callback) const
    {
        auto VisitInterface = [&callback](const EventLoggerPtr& eventLogger)
        {
            return eventLogger != nullptr ? callback(*eventLogger) : true;
        };

        AZStd::scoped_lock lock(m_eventLoggerMutex);
        AZStd::ranges::all_of(m_eventLoggers, VisitInterface);
    }

    AZ::Outcome<void, AZStd::unique_ptr<IEventLogger>> EventLoggerFactoryImpl::RegisterEventLogger(EventLoggerId eventLoggerId, AZStd::unique_ptr<IEventLogger> eventLogger)
    {
        // Transfer ownership of the event logger to the EventLoggerPtr
        // If registration fails, ownership is transferred back to the input param   
        if (auto registerResult = RegisterEventLoggerImpl(eventLoggerId, EventLoggerPtr{ eventLogger.release() });
            !registerResult.IsSuccess())
        {
            return AZ::Failure(AZStd::unique_ptr<IEventLogger>(registerResult.TakeError().release()));
        }

        return AZ::Success();
    }

    bool EventLoggerFactoryImpl::RegisterEventLogger(EventLoggerId eventLoggerId, IEventLogger& eventLogger)
    {
        // The EventLoggerDeleter does not delete the reference
        return RegisterEventLoggerImpl(eventLoggerId, EventLoggerPtr{ &eventLogger, EventLoggerDeleter{false} }).IsSuccess();
    }

    auto EventLoggerFactoryImpl::RegisterEventLoggerImpl(EventLoggerId eventLoggerId, EventLoggerPtr eventLoggerPtr)
        -> AZ::Outcome<void, EventLoggerPtr>
    {
        if (eventLoggerPtr == nullptr)
        {
            return AZ::Failure(AZStd::move(eventLoggerPtr));
        }

        AZStd::scoped_lock lock(m_eventLoggerMutex);
        auto loggerIdIter = FindEventLoggerIndex(eventLoggerId);
        if (loggerIdIter != m_eventLoggerIndirectSet.end()
            && loggerIdIter->m_index < m_eventLoggers.size())
        {
            // The event logger has been found using the event logger id,
            // so second registration cannot be done
            return AZ::Failure(AZStd::move(eventLoggerPtr));
        }

        // Insert new interface since it is not registered
        m_eventLoggers.emplace_back(AZStd::move(eventLoggerPtr));
        const size_t emplaceIndex = m_eventLoggers.size() - 1;

        // Use UpperBound to find the insertion slot for the new entry 
        auto FindIdIndexEntry = [](const EventLoggerIdIndexEntry& lhs, const EventLoggerIdIndexEntry& rhs)
        {
            return lhs.m_id < rhs.m_id;
        };
        EventLoggerIdIndexEntry newEntry{ eventLoggerId, emplaceIndex };
        m_eventLoggerIndirectSet.insert(AZStd::upper_bound(m_eventLoggerIndirectSet.begin(), m_eventLoggerIndirectSet.end(),
            newEntry, AZStd::move(FindIdIndexEntry)),
            AZStd::move(newEntry));

        return AZ::Success();
    }

    bool EventLoggerFactoryImpl::UnregisterEventLogger(EventLoggerId eventLoggerId)
    {
        AZStd::scoped_lock lock(m_eventLoggerMutex);
        auto loggerIdIter = FindEventLoggerIndex(eventLoggerId);
        if (loggerIdIter != m_eventLoggerIndirectSet.end())
        {
            // Check if the logger id index is within the event loggers vector
            if (loggerIdIter->m_index < m_eventLoggers.size())
            {
                // Remove the EventLogger
                m_eventLoggers.erase(m_eventLoggers.begin() + loggerIdIter->m_index);
            }

            // Remove the Event LoggerId to event logger vector indirection entry as well
            m_eventLoggerIndirectSet.erase(loggerIdIter);
            return true;
        }

        return false;
    }

    IEventLogger* EventLoggerFactoryImpl::FindEventLogger(EventLoggerId eventLoggerId) const
    {
        AZStd::scoped_lock lock(m_eventLoggerMutex);
        auto loggerIdIter = FindEventLoggerIndex(eventLoggerId);
        return loggerIdIter != m_eventLoggerIndirectSet.end() && loggerIdIter->m_index < m_eventLoggers.size()
            ? m_eventLoggers[loggerIdIter->m_index].get() : nullptr;
    }

    bool EventLoggerFactoryImpl::IsRegistered(EventLoggerId eventLoggerId) const
    {
        AZStd::scoped_lock lock(m_eventLoggerMutex);
        auto loggerIdIter = FindEventLoggerIndex(eventLoggerId);
        return loggerIdIter != m_eventLoggerIndirectSet.end() && loggerIdIter->m_index < m_eventLoggers.size();
    }

    // returns iterator containing index into the m_eventLoggers vector that matches the id
    // otherwise returns the end iteator
    auto EventLoggerFactoryImpl::FindEventLoggerIndex(EventLoggerId eventLoggerId) const
        -> typename EventLoggerIndirectSet::const_iterator
    {
        auto FindIdIndexEntry = [](const EventLoggerIdIndexEntry& lhs, const EventLoggerIdIndexEntry& rhs)
        {
            return lhs.m_id < rhs.m_id;
        };

        auto [firstFoundIter, lastFoundIter] = AZStd::equal_range(m_eventLoggerIndirectSet.begin(), m_eventLoggerIndirectSet.end(),
            EventLoggerIdIndexEntry{ eventLoggerId }, FindIdIndexEntry);

        return firstFoundIter != lastFoundIter ? firstFoundIter : m_eventLoggerIndirectSet.end();
    }
}// namespace AZ::Metrics
