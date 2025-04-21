/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Metrics/EventLoggerFactoryImpl.h>
#include <AzCore/Metrics/EventLoggerReflectUtils.h>
#include <AzCore/Metrics/EventLoggerUtils.h>
#include <AzCore/Metrics/IEventLogger.h>
#include <AzCore/Metrics/IEventLoggerFactory.h>
#include <AzCore/Metrics/JsonTraceEventLogger.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ChronoReflection.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryScriptUtils.h>

namespace AZ::Metrics
{
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(EventPhase);
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(InstantEventScope);

    inline namespace Script
    {
        // Script Event Arguments use container types with persistent storage such as AZStd::string
        // instead of AZStd::string_view for storing string arguments and
        // AZStd::vector instead AZStd::span for storing arrays
        // This persistent storage is needed in script to allow scripters
        // to not deal with lifetime concerns related to the event argument structures
        // The C++ event argument structures located in IEventLogger.h uses non-persistent
        // storage for the EventArg derived structures and expect users to provide a lifetime
        // for fields supplied to the EventArg structure at least as long as the call
        // to the `IEventLogger::Record*Event` call.

        //! Represents a value of a field in the arguments supplied to Record*Event
        //! It is also used to supply arguments to JSON arrays in event arguments
        struct ScriptEventValue
        {
            AZ_TYPE_INFO(ScriptEventValue, "{665521F8-C9C3-440E-86CE-3BD1D6471ECE}");

            //! Variant type which is used to provide storage for arguments until the IEventLogger::Record* functions
            //! can be invoked in C++
            using EventValueStorageVariant = AZStd::variant<AZStd::monostate, AZStd::string, AZStd::vector<EventValue>, AZStd::vector<EventField>>;

            // Need constructors that can initialize a ScriptEventValue for each type of variant
            // in order to use the BehaviorContext::ClassBuilder::Constructor function
            ScriptEventValue() = default;

            // Special case - when the Script Event Value is being copied from another,
            // the default constructor cannot be used, as it would do
            // m_valueStorage = other.m_valueStorage;
            // m_eventValueMirror = other.m_eventValueMirror;
            // The problem is that value storage is a local array of data, as in
            // AZStd::vector<EventValueStorageVariant> m_valueStorage;
            // and the mirror is supposed to basically point at the front of the storage, the first
            // value in it, for types that require storage.  Having it point at the `other` storage
            // risks the `other` being destroyed while this object is still in use
            // So here, we make sure that the mirror value points the local storage, instead of the other.
            ScriptEventValue(const ScriptEventValue& other)
            {
                m_eventValueMirror = other.m_eventValueMirror;
                if (!other.m_valueStorage.empty())
                {
                    m_valueStorage = other.m_valueStorage;
                
                    // we cannot just copy the other's mirror - that would make our mirror point at their internal storage.
                    if (AZStd::holds_alternative<AZStd::string>(m_valueStorage.front()))
                    {
                        m_eventValueMirror = AZStd::get<AZStd::string>(m_valueStorage.front());
                    }
                    else if (AZStd::holds_alternative<AZStd::vector<EventValue>>(m_valueStorage.front()))
                    {
                        m_eventValueMirror = EventArray{ AZStd::get<AZStd::vector<EventValue>>(m_valueStorage.front()) };
                    }
                    else if (AZStd::holds_alternative<AZStd::vector<EventField>>(m_valueStorage.front()))
                    {
                        m_eventValueMirror = EventObject{ AZStd::get<AZStd::vector<EventField>>(m_valueStorage.front()) };
                    }
                }
            }

            //! string instances need storage
            ScriptEventValue(AZStd::string value)
                : m_valueStorage{ EventValueStorageVariant{AZStd::in_place_type<AZStd::string>, AZStd::move(value)} }
                , m_eventValueMirror{ AZStd::get<AZStd::string>(m_valueStorage.front()) }
            {}

            //! Integral types are stored directly in the AZ::Metrics::EventValue instance
            //! so no additional storage is needed
            ScriptEventValue(bool value)
                : m_eventValueMirror{ value }
            {}
            ScriptEventValue(AZ::s64 value)
                : m_eventValueMirror{ value }
            {}
            ScriptEventValue(AZ::u64 value)
                : m_eventValueMirror{ value }
            {}
            ScriptEventValue(double value)
                : m_eventValueMirror{ value }
            {}

            //! array instances need storage
            ScriptEventValue(AZStd::vector<EventValue> value)
                : m_valueStorage{ EventValueStorageVariant{AZStd::in_place_type<AZStd::vector<EventValue>>,  AZStd::move(value)} }
                , m_eventValueMirror{ EventArray{ AZStd::get<AZStd::vector<EventValue>>(m_valueStorage.front())} }
            {
            }
            //! object instances need storage
            ScriptEventValue(AZStd::vector<EventField> value)
                : m_valueStorage{ EventValueStorageVariant{AZStd::in_place_type<AZStd::vector<EventField>>, AZStd::move(value)} }
                , m_eventValueMirror{ EventObject{ AZStd::get<AZStd::vector<EventField>>(m_valueStorage.front())} }
            {
            }

            // Conversion operator for converting to Metrics::EventValue instance
            // This is a helper function to pass the ScriptEventValue to C++
            operator EventValue() const
            {
                return m_eventValueMirror;
            }

        private:

            //! Provides persistent storage for event value arguments of strings, arrays and objects for long enough
            //! to invoke the RecordMetrics function in C++ from script
            AZStd::vector<EventValueStorageVariant> m_valueStorage;

            //! Mirrors the storage for the m_value member into the C++ EventValue variant type for compatibility
            //! with accessing C++ functions which accepts an EventValue type
            EventValue m_eventValueMirror;
        };

        struct ScriptEventArgs
        {
            AZ_TYPE_INFO(ScriptEventArgs, "{D79DEBE1-B271-4F93-9685-F19947E78EC4}");

            //! name of the event
            AZStd::string m_name;
            //! comma separated list of categories associated with the event
            AZStd::string m_cat;
            //! Id to associate with the event
            AZStd::optional<AZStd::string> m_id;

            //! Appends an argument to the "args" field
            void AppendArg(AZStd::string fieldName, ScriptEventValue value)
            {
                m_args.emplace_back(AZStd::move(fieldName), AZStd::move(value));
            }

            //! Callback function to visit each first generation child of the "args" field
            using ArgVisitCallback = AZStd::function<void(AZStd::string_view fieldName, EventValue fieldValue)>;
            void VisitArgs(const ArgVisitCallback& visitCallback)
            {
                for (const auto& [fieldName, fieldValue] : m_args)
                {
                    visitCallback(fieldName, fieldValue);
                }
            }

            //! Specifies the duration of the event in microseconds
            //! For CompleteEvents only
            AZStd::chrono::microseconds m_dur;

            //! Specifies the scope of the event
            //! For InstantEvents only
            InstantEventScope m_scopeKey;

        private:
            //! Arguments used to fill the "args" field for each trace event
            using ScriptField = AZStd::pair<AZStd::string, ScriptEventValue>;

            //! Represents a field that contains a name and EventValue which provides persistent storage
            //! for an EventField type
            AZStd::vector<ScriptField> m_args;
        };

        //! Represents the module to use when importing the event logger metrics functions
        //! This is used in Python automation. ex. `import azlmbr.metrics`
        static constexpr const char* MetricsModuleName = "metrics";
        //! Represents the category to organize the event logger classes and functions
        //! witin the ScriptCanvas NodePalette
        static constexpr const char* MetricsCategory = "Metrics";

        //! Wraps a function to return to create an event logger and register it with the Event Logger Factory
        using SettingsRegistryScriptProxy = AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy;
        static bool CreateMetricsLogger(EventLoggerId loggerId, const char* filePath, AZStd::string_view loggerName,
            SettingsRegistryScriptProxy settingsRegistry = {})
        {
            auto eventLoggerFactory = AZ::Metrics::EventLoggerFactory::Get();
            if (eventLoggerFactory == nullptr)
            {
                // An event logger factory is required to register the created Json Metrics logger
                return false;
            }
            if (!settingsRegistry.IsValid())
            {
                settingsRegistry = SettingsRegistryScriptProxy{ AZ::SettingsRegistry::Get() };
            }

            constexpr AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath;
            auto systemFileStream = AZStd::make_unique<AZ::IO::SystemFileStream>(filePath, openMode);
            if (!systemFileStream->IsOpen())
            {
                return false;
            }

            AZ::Metrics::JsonTraceEventLoggerConfig config{ loggerName, settingsRegistry.m_settingsRegistry.get()};

            auto jsonTraceEventLogger = AZStd::make_unique<AZ::Metrics::JsonTraceEventLogger>(
                AZStd::move(systemFileStream), AZStd::move(config));

            auto registerOutcome = eventLoggerFactory->RegisterEventLogger(loggerId, AZStd::move(jsonTraceEventLogger));

            return registerOutcome.IsSuccess();
        }

        using CreateMetricsLoggerBPOverrides = AZStd::array<AZ::BehaviorParameterOverrides, AZStd::function_traits<decltype(&CreateMetricsLogger)>::arity>;
        static CreateMetricsLoggerBPOverrides GetCreateMetricsLoggerBPOverrides(AZ::BehaviorContext& behaviorContext)
        {
            constexpr size_t settingsRegistryArgIndex = 3;
            static_assert(AZStd::is_same_v<AZStd::function_traits<decltype(&CreateMetricsLogger)>::get_arg_t<settingsRegistryArgIndex>, SettingsRegistryScriptProxy>,
                "The `SettingsRegistryScriptProxy` argument index must be updated for the CreateMetricsLogger function");

            CreateMetricsLoggerBPOverrides behaviorParameterOverrides;
            behaviorParameterOverrides[settingsRegistryArgIndex].m_defaultValue = behaviorContext.MakeDefaultValue(
                SettingsRegistryScriptProxy{});

            return behaviorParameterOverrides;
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
                behaviorContext != nullptr)
            {
                ReflectScript(*behaviorContext);
            }
        }

        static void ReflectEventDataTypes(AZ::BehaviorContext& behaviorContext)
        {
            // Reflect the EventValue type
            // This type is a passthrough type for scripting and is not meant to be used directort
            behaviorContext.Class<EventValue>("_OpaqueEventValue")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Constructor()
                ->Constructor<AZStd::string>()
                ->Constructor<bool>()
                ->Constructor<AZ::s64>()
                ->Constructor<AZ::u64>()
                ->Constructor<double>()
                ;

            // Reflect the EventField type
            // This type is a passthrough type for scripting and is not meant to be used directly
            behaviorContext.Class<EventField>("_OpaqueEventField")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Constructor()
                ->Constructor<AZStd::string_view, EventValue>()
                ;

            // Reflect the ScriptEventValue as the "EventValue" type for scripting
            behaviorContext.Class<ScriptEventValue>("EventValue")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Constructor()
                ->Constructor<AZStd::string>()
                ->Constructor<bool>()
                ->Constructor<AZ::s64>()
                ->Constructor<AZ::u64>()
                ->Constructor<double>()
                ->Constructor<AZStd::vector<EventValue>>()
                ->Constructor<AZStd::vector<EventField>>()
                ;

            // Reflect the EventLoggerId enum class
            behaviorContext.Class<EventLoggerId>("EventLoggerId")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Method("CreateIdFromInt", [](AZ::u32 id) { return EventLoggerId(id); })
                ->Method("CreateIdFromString", [](AZStd::string_view id) {
                    return EventLoggerId(static_cast<AZ::u32>(AZStd::hash<AZStd::string_view>{}(id))); })
                ;

            // Reflect the event phase values
            EventPhaseReflect(behaviorContext);

            // Reflect the Instant Event scope enum values
            InstantEventScopeReflect(behaviorContext);

            // Reflect the ScriptEvent argument wrapper
            behaviorContext.Class<ScriptEventArgs>("EventArgs")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Method("AppendArg", &ScriptEventArgs::AppendArg)
                ->Property("m_name", [](const ScriptEventArgs* self) { return self->m_name; },
                    [](ScriptEventArgs* self, AZStd::string value) { self->m_name = AZStd::move(value); })
                ->Property("m_cat", [](const ScriptEventArgs* self) { return self->m_cat; },
                    [](ScriptEventArgs* self, AZStd::string value) { self->m_cat = AZStd::move(value); })
                ->Property("m_id", [](const ScriptEventArgs* self) { return self->m_id; },
                    [](ScriptEventArgs* self, AZStd::optional<AZStd::string> value) { self->m_id = AZStd::move(value); })
                ->Property("m_duration", [](const ScriptEventArgs* self) { return self->m_dur; },
                    [](ScriptEventArgs* self, AZStd::chrono::microseconds value) { self->m_dur = AZStd::move(value); })
                ->Property("m_scopeKey", [](const ScriptEventArgs* self) { return self->m_scopeKey; },
                    [](ScriptEventArgs* self, InstantEventScope value) { self->m_scopeKey = AZStd::move(value); })
                ;
        }

        static void ReflectCreateMetricsLogger(AZ::BehaviorContext& behaviorContext)
        {
            behaviorContext.Method("CreateMetricsLogger", &CreateMetricsLogger, GetCreateMetricsLoggerBPOverrides(behaviorContext))
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ;
        }

        static void ReflectIsEventLoggerRegistered(AZ::BehaviorContext& behaviorContext)
        {
            behaviorContext.Class<IEventLoggerFactory>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Method("IsRegistered", &IEventLoggerFactory::IsRegistered)
                ;

            behaviorContext.Class<EventLoggerFactoryImpl>("EventLoggerFactory")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ->Constructor<>()
                ->Method("IsRegistered", &EventLoggerFactoryImpl::IsRegistered)
                ;

            // Static method to retrieve the global AZ::Interface<IEventLoggerFactory> instance
            auto GetEventLoggerInterface = []()
            {
                return EventLoggerFactory::Get();
            };
            behaviorContext.Method("GetGlobalEventLoggerFactory", GetEventLoggerInterface)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ;

            auto IsEventLoggerRegistered = [](EventLoggerId eventLoggerId, IEventLoggerFactory* eventLoggerFactory = nullptr)
                -> bool
            {
                if (eventLoggerFactory == nullptr)
                {
                    eventLoggerFactory = EventLoggerFactory::Get();
                }

                return eventLoggerFactory != nullptr && eventLoggerFactory->IsRegistered(eventLoggerId);
            };
            // Add a default argument for the IEventLoggerFactory parameter in the BehaviorCont3ext
            AZStd::array<AZ::BehaviorParameterOverrides, AZStd::function_traits<decltype(IsEventLoggerRegistered)>::arity>
                isEventLoggerRegisteredOverrides;
            constexpr size_t eventLoggerFactoryArgIndex = 1;

            isEventLoggerRegisteredOverrides[eventLoggerFactoryArgIndex].m_defaultValue = behaviorContext.MakeDefaultValue(
                static_cast<IEventLoggerFactory*>(nullptr));

            static_assert(AZStd::is_same_v<AZStd::function_traits<
                decltype(IsEventLoggerRegistered)>::get_arg_t<eventLoggerFactoryArgIndex>, IEventLoggerFactory*>,
                "The `IEventLoggerFactory*` argument index must be updated for the IsEventLoggerRegistered function");

            behaviorContext.Method("IsEventLoggerRegistered", IsEventLoggerRegistered, isEventLoggerRegisteredOverrides)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ;
        }

        static void ReflectRecordEvent(AZ::BehaviorContext& behaviorContext)
        {
            auto RecordEvent = [](EventLoggerId eventLoggerId, EventPhase eventPhase, ScriptEventArgs eventPhaseArgs,
                IEventLoggerFactory* eventLoggerFactory = nullptr)
                -> bool
            {
                if (eventLoggerFactory == nullptr)
                {
                    eventLoggerFactory = EventLoggerFactory::Get();
                }

                IEventLogger::ResultOutcome recordOutcome(AZStd::unexpect, IEventLogger::ResultOutcome::ErrorType::format(
                    R"(Invalid eventPhase type %.*s specified)", AZ_STRING_ARG(ToString(eventPhase))));

                switch (eventPhase)
                {
                case EventPhase::DurationBegin:
                case EventPhase::DurationEnd:
                {
                    DurationArgs durationArgs;
                    durationArgs.m_name = eventPhaseArgs.m_name;
                    durationArgs.m_cat = eventPhaseArgs.m_cat;

                    // The "args" field needs storage in order to last long enough to invoke the RecordEvent function
                    AZStd::vector<EventField> eventFields;
                    auto AppendArgFields = [&eventFields](AZStd::string_view fieldName, EventValue fieldValue)
                    {
                        eventFields.emplace_back(fieldName, fieldValue);
                    };
                    eventPhaseArgs.VisitArgs(AppendArgFields);
                    durationArgs.m_args = eventFields;

                    durationArgs.m_id = eventPhaseArgs.m_id;
                    recordOutcome = AZ::Metrics::Utility::RecordEvent(eventLoggerId, eventPhase, durationArgs, eventLoggerFactory);
                    break;
                }
                case EventPhase::Complete:
                {
                    CompleteArgs completeArgs;
                    completeArgs.m_name = eventPhaseArgs.m_name;
                    completeArgs.m_cat = eventPhaseArgs.m_cat;

                    // The "args" field needs storage in order to last long enough to invoke the RecordEvent function
                    AZStd::vector<EventField> eventFields;
                    auto AppendArgFields = [&eventFields](AZStd::string_view fieldName, EventValue fieldValue)
                    {
                        eventFields.emplace_back(fieldName, fieldValue);
                    };
                    eventPhaseArgs.VisitArgs(AppendArgFields);
                    completeArgs.m_args = eventFields;

                    completeArgs.m_id = eventPhaseArgs.m_id;
                    completeArgs.m_dur = eventPhaseArgs.m_dur;
                    recordOutcome = AZ::Metrics::Utility::RecordEvent(eventLoggerId, eventPhase, completeArgs, eventLoggerFactory);
                    break;
                }
                case EventPhase::Instant:
                {
                    InstantArgs instantArgs;
                    instantArgs.m_name = eventPhaseArgs.m_name;
                    instantArgs.m_cat = eventPhaseArgs.m_cat;

                    // The "args" field needs storage in order to last long enough to invoke the RecordEvent function
                    AZStd::vector<EventField> eventFields;
                    auto AppendArgFields = [&eventFields](AZStd::string_view fieldName, EventValue fieldValue)
                    {
                        eventFields.emplace_back(fieldName, fieldValue);
                    };
                    eventPhaseArgs.VisitArgs(AppendArgFields);
                    instantArgs.m_args = eventFields;

                    instantArgs.m_id = eventPhaseArgs.m_id;
                    instantArgs.m_scope = eventPhaseArgs.m_scopeKey;
                    recordOutcome = AZ::Metrics::Utility::RecordEvent(eventLoggerId, eventPhase, instantArgs, eventLoggerFactory);
                    break;
                }
                case EventPhase::Counter:
                {
                    CounterArgs counterArgs;
                    counterArgs.m_name = eventPhaseArgs.m_name;
                    counterArgs.m_cat = eventPhaseArgs.m_cat;

                    // The "args" field needs storage in order to last long enough to invoke the RecordEvent function
                    AZStd::vector<EventField> eventFields;
                    auto AppendArgFields = [&eventFields](AZStd::string_view fieldName, EventValue fieldValue)
                    {
                        eventFields.emplace_back(fieldName, fieldValue);
                    };
                    eventPhaseArgs.VisitArgs(AppendArgFields);
                    counterArgs.m_args = eventFields;

                    counterArgs.m_id = eventPhaseArgs.m_id;
                    recordOutcome = AZ::Metrics::Utility::RecordEvent(eventLoggerId, eventPhase, counterArgs, eventLoggerFactory);
                    break;
                }
                case EventPhase::AsyncStart:
                case EventPhase::AsyncInstant:
                case EventPhase::AsyncEnd:
                {
                    AsyncArgs asyncArgs;
                    asyncArgs.m_name = eventPhaseArgs.m_name;
                    asyncArgs.m_cat = eventPhaseArgs.m_cat;

                    // The "args" field needs storage in order to last long enough to invoke the RecordEvent function
                    AZStd::vector<EventField> eventFields;
                    auto AppendArgFields = [&eventFields](AZStd::string_view fieldName, EventValue fieldValue)
                    {
                        eventFields.emplace_back(fieldName, fieldValue);
                    };
                    eventPhaseArgs.VisitArgs(AppendArgFields);
                    asyncArgs.m_args = eventFields;

                    asyncArgs.m_id = eventPhaseArgs.m_id.value_or(AZStd::string_view{});
                    recordOutcome = AZ::Metrics::Utility::RecordEvent(eventLoggerId, eventPhase, asyncArgs, eventLoggerFactory);
                    break;
                }
                }
                return recordOutcome.IsSuccess();
            };

            // Add a default argument for the IEventLoggerFactory parameter in the BehaviorCont3ext
            AZStd::array<AZ::BehaviorParameterOverrides, AZStd::function_traits<decltype(RecordEvent)>::arity>
                recordEventOverrides;
            constexpr size_t eventLoggerFactoryArgIndex = 3;
            recordEventOverrides[eventLoggerFactoryArgIndex].m_defaultValue = behaviorContext.MakeDefaultValue(static_cast<IEventLoggerFactory*>(nullptr));
            static_assert(AZStd::is_same_v<AZStd::function_traits<
                decltype(RecordEvent)>::get_arg_t<eventLoggerFactoryArgIndex>, IEventLoggerFactory*>,
                "The `IEventLoggerFactory*` argument index must be updated for the RecordEvent function");

            behaviorContext.Method("RecordEvent", RecordEvent, recordEventOverrides)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, MetricsModuleName)
                ->Attribute(AZ::Script::Attributes::Category, MetricsCategory)
                ;
        }

        void ReflectScript(AZ::BehaviorContext& behaviorContext)
        {
            ReflectEventDataTypes(behaviorContext);
            ReflectCreateMetricsLogger(behaviorContext);
            ReflectIsEventLoggerRegistered(behaviorContext);
            ReflectRecordEvent(behaviorContext);
        }
    } // inline namespace Utility
} // namespace AZ::Metrics
