/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
        AZ_CLASS_ALLOCATOR(EventLoggerFactoryImpl, AZ::SystemAllocator, 0);

        EventLoggerFactoryImpl();
        ~EventLoggerFactoryImpl();

        void VisitEventLoggers(const VisitEventLoggerInterfaceCallback&) const override;

        //! Registers an event logger with a standard deleter
        AZ::Outcome<void, AZStd::unique_ptr<IEventLogger>> RegisterEventLogger(EventLoggerId loggerId, AZStd::unique_ptr<IEventLogger>) override;

        //! Registers an event logger with a null deleter
        bool RegisterEventLogger(EventLoggerId loggerId, IEventLogger&) override;

        bool UnregisterEventLogger(EventLoggerId loggerId) override;

        IEventLogger* FindEventLogger(EventLoggerId) const override;

        bool IsRegistered(EventLoggerId) const override;

    private:

        struct EventLoggerDeleter
        {
            void operator()(IEventLogger* ptr) const;
            bool m_delete{ true };
        };
        using EventLoggerPtr = AZStd::unique_ptr<IEventLogger, EventLoggerDeleter>;
        AZ::Outcome<void, EventLoggerPtr> RegisterEventLoggerImpl(EventLoggerId loggerId, EventLoggerPtr);

        struct EventLoggerIdIndexEntry
        {
            EventLoggerId m_id;
            size_t m_index;
        };

        using EventLoggerIndirectSet = AZStd::vector<EventLoggerIdIndexEntry>;
        typename EventLoggerIndirectSet::const_iterator FindEventLoggerIndex(EventLoggerId) const;

        //! Indirect array into the EventLogger Interfaces vector
        AZStd::vector<EventLoggerIdIndexEntry> m_eventLoggerIndirectSet;



        AZStd::vector<EventLoggerPtr> m_eventLoggers;

        //! Protects modifications to the m_eventLoggers range
        //! Doesn't need to be recursive as now inner calls attempts to lock the mutex
        mutable AZStd::mutex m_eventLoggerMutex;
    };
}// namespace AZ::Metrics
