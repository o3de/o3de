/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Metrics/EventLoggerUtils.h>

namespace AZ::Metrics
{
    inline namespace Utility
    {
        void Flush(EventLoggerId eventLoggerId, IEventLoggerFactory* eventLoggerFactory)
        {
            // Use the global registered AZ::Interface instance of the eventLogger if nullptr is supplied
            if (eventLoggerFactory == nullptr)
            {
                if (eventLoggerFactory = AZ::Interface<IEventLoggerFactory>::Get();
                    eventLoggerFactory == nullptr)
                {
                    // There is no globally registered event logger so nothing can be done
                    return;
                }
            }

            if (auto eventLogger = eventLoggerFactory->FindEventLogger(eventLoggerId);
                eventLogger != nullptr)
            {
                eventLogger->Flush();
            }
        }

        auto RecordEvent(EventLoggerId eventLoggerId, EventPhase eventPhase, const EventPhaseArgs& eventPhaseArgs,
            IEventLoggerFactory* eventLoggerFactory) -> IEventLogger::ResultOutcome
        {
            // Use the global registered AZ::Interface instance of the eventLogger if nullptr is supplied
            if (eventLoggerFactory == nullptr)
            {
                if (eventLoggerFactory = AZ::Interface<IEventLoggerFactory>::Get();
                    eventLoggerFactory == nullptr)
                {
                    // There is no globally registered event logger so nothing can be done
                    return AZ::Failure(IEventLogger::ErrorString("Cannot record an event."
                        " There is no globally registered Event Logger Factory with the AZ::Interface<IEventLoggerFactory>"
                        " that can lookup the event logger"));
                }
            }

            auto VisitEventArgs = [eventLoggerId, eventPhase, eventLoggerFactory](auto&& eventArgs) -> IEventLogger::ResultOutcome
            {
                using EventArgsType = AZStd::remove_cvref_t<decltype(eventArgs)>;
                if constexpr (AZStd::same_as<EventArgsType, DurationArgs>)
                {
                    switch (eventPhase)
                    {
                    case EventPhase::DurationBegin:
                        return RecordDurationEventBegin(eventLoggerId, eventArgs, eventLoggerFactory);
                    case EventPhase::DurationEnd:
                        return RecordDurationEventEnd(eventLoggerId, eventArgs, eventLoggerFactory);
                    default:
                        return AZ::Failure(IEventLogger::ErrorString::format("Event phase type '%c' is not compatible with the DurationArgs event structure",
                            static_cast<char>(eventPhase)));
                    }
                }
                else if constexpr (AZStd::same_as<EventArgsType, CompleteArgs>)
                {
                    switch (eventPhase)
                    {
                    case EventPhase::Complete:
                        return RecordCompleteEvent(eventLoggerId, eventArgs, eventLoggerFactory);
                    default:
                        return AZ::Failure(IEventLogger::ErrorString::format("Event phase type '%c' is not compatible with the CompleteArgs event structure",
                            static_cast<char>(eventPhase)));
                    }
                }
                else if constexpr (AZStd::same_as<EventArgsType, InstantArgs>)
                {
                    switch (eventPhase)
                    {
                    case EventPhase::Instant:
                        return RecordInstantEvent(eventLoggerId, eventArgs, eventLoggerFactory);
                    default:
                        return AZ::Failure(IEventLogger::ErrorString::format("Event phase type '%c' is not compatible with the InstantArgs event structure",
                            static_cast<char>(eventPhase)));
                    }
                }
                else if constexpr (AZStd::same_as<EventArgsType, CounterArgs>)
                {
                    switch (eventPhase)
                    {
                    case EventPhase::Counter:
                        return RecordCounterEvent(eventLoggerId, eventArgs, eventLoggerFactory);
                    default:
                        return AZ::Failure(IEventLogger::ErrorString::format("Event phase type '%c' is not compatible with the CounterArgs event structure",
                            static_cast<char>(eventPhase)));
                    }
                }
                else if constexpr (AZStd::same_as<EventArgsType, AsyncArgs>)
                {
                    switch (eventPhase)
                    {
                    case EventPhase::AsyncStart:
                        return RecordAsyncEventStart(eventLoggerId, eventArgs, eventLoggerFactory);
                    case EventPhase::AsyncInstant:
                        return RecordAsyncEventInstant(eventLoggerId, eventArgs, eventLoggerFactory);
                    case EventPhase::AsyncEnd:
                        return RecordAsyncEventEnd(eventLoggerId, eventArgs, eventLoggerFactory);
                    default:
                        return AZ::Failure(IEventLogger::ErrorString::format("Event phase type '%c' is not compatible with the AsyncArgs event structure",
                            static_cast<char>(eventPhase)));
                    }
                }
            };

            return AZStd::visit(VisitEventArgs, eventPhaseArgs);
        }

        auto RecordDurationEventBegin(EventLoggerId eventLoggerId, const DurationArgs& durationArgs,
            IEventLoggerFactory* eventLoggerFactory) -> IEventLogger::ResultOutcome
        {
            // Use the global registered AZ::Interface instance of the eventLogger if nullptr is supplied
            if (eventLoggerFactory == nullptr)
            {
                if (eventLoggerFactory = AZ::Interface<IEventLoggerFactory>::Get();
                    eventLoggerFactory == nullptr)
                {
                    // There is no globally registered event logger so nothing can be done
                    return AZ::Failure(IEventLogger::ErrorString("Cannot record duration begin event."
                        " There is no globally registered Event Logger Factory with the AZ::Interface<IEventLoggerFactory>"));
                }
            }

            if (auto eventLogger = eventLoggerFactory->FindEventLogger(eventLoggerId);
                eventLogger != nullptr)
            {
                return eventLogger->RecordDurationEventBegin(durationArgs);
            }

            return AZ::Failure(IEventLogger::ErrorString::format("EventLogger with Id %u is not registered found with the EventLoggerFactory."
                " Cannot record duration begin event", static_cast<AZ::u32>(eventLoggerId)));
        }

        auto RecordDurationEventEnd(EventLoggerId eventLoggerId, const DurationArgs& durationArgs,
            IEventLoggerFactory* eventLoggerFactory) -> IEventLogger::ResultOutcome
        {
            // Use the global registered AZ::Interface instance of the eventLogger if nullptr is supplied
            if (eventLoggerFactory == nullptr)
            {
                if (eventLoggerFactory = AZ::Interface<IEventLoggerFactory>::Get();
                    eventLoggerFactory == nullptr)
                {
                    // There is no globally registered event logger so nothing can be done
                    return AZ::Failure(IEventLogger::ErrorString("Cannot record duration begin event."
                        " There is no globally registered Event Logger Factory with the AZ::Interface<IEventLoggerFactory>"));
                }
            }

            if (auto eventLogger = eventLoggerFactory->FindEventLogger(eventLoggerId);
                eventLogger != nullptr)
            {
                return eventLogger->RecordDurationEventEnd(durationArgs);
            }

            return AZ::Failure(IEventLogger::ErrorString::format("EventLogger with Id %u is not registered found with the EventLoggerFactory."
                " Cannot record duration end event", static_cast<AZ::u32>(eventLoggerId)));
        }

        auto RecordCompleteEvent(EventLoggerId eventLoggerId, const CompleteArgs& completeArgs,
            IEventLoggerFactory* eventLoggerFactory) -> IEventLogger::ResultOutcome
        {
            // Use the global registered AZ::Interface instance of the eventLogger if nullptr is supplied
            if (eventLoggerFactory == nullptr)
            {
                if (eventLoggerFactory = AZ::Interface<IEventLoggerFactory>::Get();
                    eventLoggerFactory == nullptr)
                {
                    // There is no globally registered event logger so nothing can be done
                    return AZ::Failure(IEventLogger::ErrorString("Cannot record complete event."
                        " There is no globally registered Event Logger Factory with the AZ::Interface<IEventLoggerFactory>"));
                }
            }

            if (auto eventLogger = eventLoggerFactory->FindEventLogger(eventLoggerId);
                eventLogger != nullptr)
            {
                return eventLogger->RecordCompleteEvent(completeArgs);
            }

            return AZ::Failure(IEventLogger::ErrorString::format("EventLogger with Id %u is not registered found with the EventLoggerFactory."
                " Cannot record complete event", static_cast<AZ::u32>(eventLoggerId)));
        }

        auto RecordInstantEvent(EventLoggerId eventLoggerId, const InstantArgs& instantArgs,
            IEventLoggerFactory* eventLoggerFactory) -> IEventLogger::ResultOutcome
        {
            // Use the global registered AZ::Interface instance of the eventLogger if nullptr is supplied
            if (eventLoggerFactory == nullptr)
            {
                if (eventLoggerFactory = AZ::Interface<IEventLoggerFactory>::Get();
                    eventLoggerFactory == nullptr)
                {
                    // There is no globally registered event logger so nothing can be done
                    return AZ::Failure(IEventLogger::ErrorString("Cannot record instant event."
                        " There is no globally registered Event Logger Factory with the AZ::Interface<IEventLoggerFactory>"));
                }
            }

            if (auto eventLogger = eventLoggerFactory->FindEventLogger(eventLoggerId);
                eventLogger != nullptr)
            {
                return eventLogger->RecordInstantEvent(instantArgs);
            }

            return AZ::Failure(IEventLogger::ErrorString::format("EventLogger with Id %u is not registered found with the EventLoggerFactory."
                " Cannot record instant event", static_cast<AZ::u32>(eventLoggerId)));
        }

        auto RecordCounterEvent(EventLoggerId eventLoggerId, const CounterArgs& counterArgs,
            IEventLoggerFactory* eventLoggerFactory) -> IEventLogger::ResultOutcome
        {
            // Use the global registered AZ::Interface instance of the eventLogger if nullptr is supplied
            if (eventLoggerFactory == nullptr)
            {
                if (eventLoggerFactory = AZ::Interface<IEventLoggerFactory>::Get();
                    eventLoggerFactory == nullptr)
                {
                    // There is no globally registered event logger so nothing can be done
                    return AZ::Failure(IEventLogger::ErrorString("Cannot record counter event."
                        " There is no globally registered Event Logger Factory with the AZ::Interface<IEventLoggerFactory>"));
                }
            }

            if (auto eventLogger = eventLoggerFactory->FindEventLogger(eventLoggerId);
                eventLogger != nullptr)
            {
                return eventLogger->RecordCounterEvent(counterArgs);
            }

            return AZ::Failure(IEventLogger::ErrorString::format("EventLogger with Id %u is not registered found with the EventLoggerFactory."
                " Cannot record counter event", static_cast<AZ::u32>(eventLoggerId)));
        }

        auto RecordAsyncEventStart(EventLoggerId eventLoggerId, const AsyncArgs& asyncArgs,
            IEventLoggerFactory* eventLoggerFactory) -> IEventLogger::ResultOutcome
        {
            // Use the global registered AZ::Interface instance of the eventLogger if nullptr is supplied
            if (eventLoggerFactory == nullptr)
            {
                if (eventLoggerFactory = AZ::Interface<IEventLoggerFactory>::Get();
                    eventLoggerFactory == nullptr)
                {
                    // There is no globally registered event logger so nothing can be done
                    return AZ::Failure(IEventLogger::ErrorString("Cannot record async start event."
                        " There is no globally registered Event Logger Factory with the AZ::Interface<IEventLoggerFactory>"));
                }
            }

            if (auto eventLogger = eventLoggerFactory->FindEventLogger(eventLoggerId);
                eventLogger != nullptr)
            {
                return eventLogger->RecordAsyncEventStart(asyncArgs);
            }

            return AZ::Failure(IEventLogger::ErrorString::format("EventLogger with Id %u is not registered found with the EventLoggerFactory."
                " Cannot record async start event", static_cast<AZ::u32>(eventLoggerId)));
        }

        auto RecordAsyncEventInstant(EventLoggerId eventLoggerId, const AsyncArgs& asyncArgs,
            IEventLoggerFactory* eventLoggerFactory) -> IEventLogger::ResultOutcome
        {
            // Use the global registered AZ::Interface instance of the eventLogger if nullptr is supplied
            if (eventLoggerFactory == nullptr)
            {
                if (eventLoggerFactory = AZ::Interface<IEventLoggerFactory>::Get();
                    eventLoggerFactory == nullptr)
                {
                    // There is no globally registered event logger so nothing can be done
                    return AZ::Failure(IEventLogger::ErrorString("Cannot record async instant event."
                        " There is no globally registered Event Logger Factory with the AZ::Interface<IEventLoggerFactory>"));
                }
            }

            if (auto eventLogger = eventLoggerFactory->FindEventLogger(eventLoggerId);
                eventLogger != nullptr)
            {
                return eventLogger->RecordAsyncEventInstant(asyncArgs);
            }

            return AZ::Failure(IEventLogger::ErrorString::format("EventLogger with Id %u is not registered found with the EventLoggerFactory."
                " Cannot record async instant event", static_cast<AZ::u32>(eventLoggerId)));
        }

        auto RecordAsyncEventEnd(EventLoggerId eventLoggerId, const AsyncArgs& asyncArgs,
            IEventLoggerFactory* eventLoggerFactory) -> IEventLogger::ResultOutcome
        {
            // Use the global registered AZ::Interface instance of the eventLogger if nullptr is supplied
            if (eventLoggerFactory == nullptr)
            {
                if (eventLoggerFactory = AZ::Interface<IEventLoggerFactory>::Get();
                    eventLoggerFactory == nullptr)
                {
                    // There is no globally registered event logger so nothing can be done
                    return AZ::Failure(IEventLogger::ErrorString("Cannot record async end event."
                        " There is no globally registered Event Logger Factory with the AZ::Interface<IEventLoggerFactory>"));
                }
            }

            if (auto eventLogger = eventLoggerFactory->FindEventLogger(eventLoggerId);
                eventLogger != nullptr)
            {
                return eventLogger->RecordAsyncEventEnd(asyncArgs);
            }

            return AZ::Failure(IEventLogger::ErrorString::format("EventLogger with Id %u is not registered found with the EventLoggerFactory."
                " Cannot record async end event", static_cast<AZ::u32>(eventLoggerId)));
        }
    } // inline namespace Utility
} // namespace AZ::Metrics
