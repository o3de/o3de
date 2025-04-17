/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Metrics/IEventLogger.h>
#include <AzCore/Metrics/IEventLoggerFactory.h>


namespace AZ::Metrics
{
    // Utility namespace contains functions/class to simpilfy calls to record events
    // It contains functions that can perform a lookup of an EventLogger and a record command in single calls
    inline namespace Utility
    {
        //! Lookup an Event Logger using the Event Logger id if registered with the specified event logger factory
        //! and invoke the Flush() function on it to write any buffered metrics to the underlying stream
        //! @param eventLoggerId Id of the registered event logger to lookup an IEventLogger instance
        //! @param eventLoggerFactory pointer to an EventLoggerFactory to lookup an Event Logger using the @eventLoggerId param
        void Flush(EventLoggerId eventLoggerId, IEventLoggerFactory* eventLoggerFactory = nullptr);

        //! variant containing alternative for each type of event phase arguments
        //! It allows any type of event argument structure defined in the IEventLogger API to be supplied
        using EventPhaseArgs = AZStd::variant<DurationArgs, CompleteArgs, InstantArgs, CounterArgs, AsyncArgs>;

        //! Query an Event Logger from the Event Logger Factory and then records an event based on the event phase
        //! argument
        //! @param eventLoggerId Id of the registered event logger to lookup an IEventLogger instance
        //! @param eventPhase phase enumeration detailing the type of event to record
        //! @param eventPhaseArgs structure containing arguments for the specific phase events to record
        //! @param eventLoggerFactory optional pointer to an EventLoggerFactory to use
        //! if nullptr the AZ::Interface<IEventLoggerFactory> registered instance is used instead
        auto RecordEvent(EventLoggerId eventLoggerId, EventPhase eventPhase, const EventPhaseArgs& eventPhaseArgs,
            IEventLoggerFactory* eventLoggerFactory = nullptr) -> IEventLogger::ResultOutcome;

        //! Query an Event Logger from the Event Logger Factory and then records a begin duration event
        //! @param eventLoggerId Id of the registered event logger to lookup an IEventLogger instance
        //! @param durationArgs structure containing arguments for the duration event
        //! @param eventLoggerFactory optional pointer to an EventLoggerFactory to use
        //! if nullptr the AZ::Interface<IEventLoggerFactory> registered instance is used instead
        auto RecordDurationEventBegin(EventLoggerId eventLoggerId, const DurationArgs& durationArgs,
            IEventLoggerFactory* eventLoggerFactory = nullptr) -> IEventLogger::ResultOutcome;
        //! Query an Event Logger from the Event Logger Factoryand then records an end duration event
        //! @param eventLoggerId Id of the registered event logger to lookup an IEventLogger instance
        //! @param durationArgs structure containing arguments for the duration event
        //! @param eventLoggerFactory optional pointer to an EventLoggerFactory to use
        //! if nullptr the AZ::Interface<IEventLoggerFactory> registered instance is used instead
        auto RecordDurationEventEnd(EventLoggerId eventLoggerId, const DurationArgs& durationArgs,
            IEventLoggerFactory* eventLoggerFactory = nullptr) -> IEventLogger::ResultOutcome;

        //! Records a complete event(encapsulates a beging and end duration event)
        //! Query an Event Logger from the Event Logger Factory and then records a complete event
        //! @param eventLoggerId Id of the registered event logger to lookup an IEventLogger instance
        //! @param completeArgs structure containing arguments for the complete event
        //! @param eventLoggerFactory optional pointer to an EventLoggerFactory to use
        //! if nullptr the AZ::Interface<IEventLoggerFactory> registered instance is used instead
        auto RecordCompleteEvent(EventLoggerId eventLoggerId, const CompleteArgs& completeArgs,
            IEventLoggerFactory* eventLoggerFactory = nullptr) -> IEventLogger::ResultOutcome;

        //! Query an Event Logger from the Event Logger Factory and then records an instant event
        //! @param eventLoggerId Id of the registered event logger to lookup an IEventLogger instance
        //! @param instantArgs structure containing arguments for the instant event
        //! @param eventLoggerFactory optional pointer to an EventLoggerFactory to use
        //! if nullptr the AZ::Interface<IEventLoggerFactory> registered instance is used instead
        auto RecordInstantEvent(EventLoggerId eventLoggerId, const InstantArgs& instantArgs,
            IEventLoggerFactory* eventLoggerFactory = nullptr) -> IEventLogger::ResultOutcome;

        //! Query an Event Logger from the Event Logger Factory and then records a counter event
        //! @param eventLoggerId Id of the registered event logger to lookup an IEventLogger instance
        //! @param counterArgs structure containing arguments for the counter event
        //! @param eventLoggerFactory optional pointer to an EventLoggerFactory to use
        //! if nullptr the AZ::Interface<IEventLoggerFactory> registered instance is used instead
        auto RecordCounterEvent(EventLoggerId eventLoggerId, const CounterArgs& counterArgs,
            IEventLoggerFactory* eventLoggerFactory = nullptr) -> IEventLogger::ResultOutcome;

        //! Query an Event Logger from the Event Logger Factory and then records an async start event
        //! @param eventLoggerId Id of the registered event logger to lookup an IEventLogger instance
        //! @param asyncArgs structure containing arguments for the async start event
        //! @param eventLoggerFactory optional pointer to an EventLoggerFactory to use
        //! if nullptr the AZ::Interface<IEventLoggerFactory> registered instance is used instead
        auto RecordAsyncEventStart(EventLoggerId eventLoggerId, const AsyncArgs& asyncArgs,
            IEventLoggerFactory* eventLoggerFactory = nullptr) -> IEventLogger::ResultOutcome;
        //! Query an Event Logger from the Event Logger Factory and then records an async instant event
        //! @param eventLoggerId Id of the registered event logger to lookup an IEventLogger instance
        //! @param asyncArgs structure containing arguments for the async instant event
        //! @param eventLoggerFactory optional pointer to an EventLoggerFactory to use
        //! if nullptr the AZ::Interface<IEventLoggerFactory> registered instance is used instead
        auto RecordAsyncEventInstant(EventLoggerId eventLoggerId, const AsyncArgs& asyncArgs,
            IEventLoggerFactory* eventLoggerFactory = nullptr) -> IEventLogger::ResultOutcome;
        //! Query an Event Logger from the Event Logger Factory and then records an async end event
        //! @param eventLoggerId Id of the registered event logger to lookup an IEventLogger instance
        //! @param asyncArgs structure containing arguments for the async end event
        //! @param eventLoggerFactory optional pointer to an EventLoggerFactory to use
        //! if nullptr the AZ::Interface<IEventLoggerFactory> registered instance is used instead
        auto RecordAsyncEventEnd(EventLoggerId eventLoggerId, const AsyncArgs& asyncArgs,
            IEventLoggerFactory* eventLoggerFactory = nullptr) -> IEventLogger::ResultOutcome;
    } // inline namespace Utility
} // namespace AZ::Metrics
