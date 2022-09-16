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
    EventLoggerFactoryImpl::EventLoggerDeleter::EventLoggerDeleter() = default;
    EventLoggerFactoryImpl::EventLoggerDeleter::EventLoggerDeleter(bool shouldDelete)
        : m_delete(shouldDelete)
    {}
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
        auto VisitInterface = [&callback](const IdToEventLoggerEntry& eventLoggerEntry)
        {
            return eventLoggerEntry.m_logger != nullptr ? callback(*eventLoggerEntry.m_logger) : true;
        };

        AZStd::scoped_lock lock(m_eventLoggerMutex);
        AZStd::ranges::all_of(m_eventLoggers, VisitInterface);
    }

    AZ::Outcome<void, AZStd::unique_ptr<IEventLogger>> EventLoggerFactoryImpl::RegisterEventLogger(EventLoggerId eventLoggerId, AZStd::unique_ptr<IEventLogger> eventLogger)
    {
        // Transfer ownership to a temporary EventLoggerPtr which is supplied to the RegisterEventLoggerImpl
        // If registration fails, the event logger pointer is returned in the failure outcome
        if (auto registerResult = RegisterEventLoggerImpl(eventLoggerId, EventLoggerPtr{ eventLogger.release() });
            !registerResult.IsSuccess())
        {
            return AZ::Failure(AZStd::unique_ptr<IEventLogger>(registerResult.TakeError().release()));
        }

        // registration succeeded, return a void success outcome
        return AZ::Success();
    }

    bool EventLoggerFactoryImpl::RegisterEventLogger(EventLoggerId eventLoggerId, IEventLogger& eventLogger)
    {
        // Create a temporary EventLoggerPtr with a custom deleter that does NOT delete the eventLogger reference
        // On success, the the EventLoggerPtr is stored in the event logger array and will not the reference to the input eventLogger
        // when the eventLogger is unregistered
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
        auto idLoggerIter = FindEventLoggerImpl(eventLoggerId);
        if (idLoggerIter != m_eventLoggers.end())
        {
            // The event logger has been found using the event logger id,
            // so second registration cannot be done
            return AZ::Failure(AZStd::move(eventLoggerPtr));
        }

        // Use UpperBound to find the insertion slot for the new entry
        auto ProjectionToEventLoggerId = [](const IdToEventLoggerEntry& entry) -> EventLoggerId
        {
            return entry.m_id;
        };

        m_eventLoggers.emplace(AZStd::ranges::upper_bound(m_eventLoggers, eventLoggerId,
            AZStd::ranges::less{}, ProjectionToEventLoggerId),
            IdToEventLoggerEntry{ eventLoggerId, AZStd::move(eventLoggerPtr) });

        return AZ::Success();
    }

    bool EventLoggerFactoryImpl::UnregisterEventLogger(EventLoggerId eventLoggerId)
    {
        AZStd::scoped_lock lock(m_eventLoggerMutex);
        if (auto idLoggerIter = FindEventLoggerImpl(eventLoggerId);
            idLoggerIter != m_eventLoggers.end())
        {
            // Remove the EventLogger
            m_eventLoggers.erase(idLoggerIter);
            return true;
        }

        return false;
    }

    IEventLogger* EventLoggerFactoryImpl::FindEventLogger(EventLoggerId eventLoggerId) const
    {
        AZStd::scoped_lock lock(m_eventLoggerMutex);
        auto idLoggerIter = FindEventLoggerImpl(eventLoggerId);
        return idLoggerIter != m_eventLoggers.end() ? idLoggerIter->m_logger.get() : nullptr;
    }

    bool EventLoggerFactoryImpl::IsRegistered(EventLoggerId eventLoggerId) const
    {
        AZStd::scoped_lock lock(m_eventLoggerMutex);
        auto idLoggerIter = FindEventLoggerImpl(eventLoggerId);
        return idLoggerIter != m_eventLoggers.end();
    }

    // NOTE: The caller should lock the event logger mutex
    // returns iterator to the event logger matching the logger id
    // otherwise a sentinel iterator is returned
    auto EventLoggerFactoryImpl::FindEventLoggerImpl(EventLoggerId eventLoggerId) const
        -> typename IdToEventLoggerMap::const_iterator
    {
        auto ProjectionToEventLoggerId = [](const IdToEventLoggerEntry& entry) -> EventLoggerId
        {
            return entry.m_id;
        };

        auto [firstFoundIter, lastFoundIter] = AZStd::ranges::equal_range(m_eventLoggers,
            eventLoggerId, AZStd::ranges::less{}, ProjectionToEventLoggerId);

        return firstFoundIter != lastFoundIter ? firstFoundIter : m_eventLoggers.end();
    }
}// namespace AZ::Metrics
