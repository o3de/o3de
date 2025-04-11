/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Metrics/IEventLoggerFactory.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ::Metrics
{
    class IEventLogger;
    class EventLoggerFactoryImpl final
        : public IEventLoggerFactory
    {
    public:
        AZ_RTTI(EventLoggerFactoryImpl, "{7F44BCD5-D4B9-464C-A54C-ECDF3C2E3802}", IEventLoggerFactory);
        AZ_CLASS_ALLOCATOR(EventLoggerFactoryImpl, AZ::SystemAllocator);

        EventLoggerFactoryImpl();
        ~EventLoggerFactoryImpl();

        void VisitEventLoggers(const VisitEventLoggerInterfaceCallback&) const override;

        //! Registers an event logger with a standard deleter
        AZ::Outcome<void, AZStd::unique_ptr<IEventLogger>> RegisterEventLogger(EventLoggerId loggerId,
            AZStd::unique_ptr<IEventLogger> eventLogger) override;

        //! Registers an event logger with a null deleter
        bool RegisterEventLogger(EventLoggerId loggerId, IEventLogger& eventLogger) override;

        bool UnregisterEventLogger(EventLoggerId loggerId) override;

        [[nodiscard]] IEventLogger* FindEventLogger(EventLoggerId loggerId) const override;

        [[nodiscard]] bool IsRegistered(EventLoggerId loggerId) const override;

        struct EventLoggerDeleter
        {
            EventLoggerDeleter();
            EventLoggerDeleter(bool shouldDelete);
            void operator()(IEventLogger* ptr) const;
            bool m_delete{ true };
        };

    private:
        using EventLoggerPtr = AZStd::unique_ptr<IEventLogger, EventLoggerDeleter>;

        //! Helper function that is used to register an event logger into the event logger array
        //! while taking into account whether the event logger should be owned by this factory
        //! @param loggerId Unique Id of the event logger to register with this factory
        //! @param eventLogger unique_ptr to event logger to register
        //! @return outcome which indicates whether the event logger was registered with the event logger array
        //! On success an empty Success outcome is returned
        //! On failure, the supplied event logger parameter is returned back to the caller
        [[nodiscard]] AZ::Outcome<void, EventLoggerPtr> RegisterEventLoggerImpl(EventLoggerId loggerId, EventLoggerPtr eventlogger);

        struct IdToEventLoggerEntry
        {
            EventLoggerId m_id;
            EventLoggerPtr m_logger;
        };

        using IdToEventLoggerMap = AZStd::vector<IdToEventLoggerEntry>;

        //! Searches within the event logger array for the event logger registered with the specified id
        //! @param loggerId Unique Id of event logger to locate
        //! @return iterator pointing the event logger registered with the specified EventLoggerId
        //! NOTE: It is responsibility of the caller to lock the Event Logger Mutex to protect the search
        typename IdToEventLoggerMap::const_iterator FindEventLoggerImpl(EventLoggerId) const;


        //! Contains the registered event loggers
        //! Sorted to provide O(Log N) search
        IdToEventLoggerMap m_eventLoggers;

        //! Protects modifications to the m_eventLoggers range
        //! Doesn't need to be recursive as now inner calls attempts to lock the mutex
        mutable AZStd::mutex m_eventLoggerMutex;
    };
}// namespace AZ::Metrics
