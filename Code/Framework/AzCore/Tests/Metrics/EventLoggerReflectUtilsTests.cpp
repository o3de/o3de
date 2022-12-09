/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Metrics/EventLoggerFactoryImpl.h>
#include <AzCore/Metrics/EventLoggerReflectUtils.h>
#include <AzCore/Metrics/JsonTraceEventLogger.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest::EventLoggerReflectUtilsTests
{
    class EventLoggerReflectUtilsFixture
        : public LeakDetectionFixture
    {
    public:
        EventLoggerReflectUtilsFixture()
        {
            // Create an event logger factory
            m_eventLoggerFactory = AZStd::make_unique<AZ::Metrics::EventLoggerFactoryImpl>();

            // Register the event logger factory with the global interface
            AZ::Metrics::EventLoggerFactory::Register(m_eventLoggerFactory.get());

            // Create an byte container stream that allows event logger output to be logged in-memory
            auto metricsStream = AZStd::make_unique<AZ::IO::ByteContainerStream<AZStd::string>>(&m_metricsOutput);
            // Create the trace event logger that logs to the Google Trace Event format
            auto eventLogger = AZStd::make_unique<AZ::Metrics::JsonTraceEventLogger>(AZStd::move(metricsStream));

            // Register the Google Trace Event Logger with the Event Logger Factory
            EXPECT_TRUE(m_eventLoggerFactory->RegisterEventLogger(m_loggerId, AZStd::move(eventLogger)));

            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            AZ::Metrics::Reflect(m_behaviorContext.get());
        }

        ~EventLoggerReflectUtilsFixture()
        {
            AZ::Metrics::EventLoggerFactory::Unregister(m_eventLoggerFactory.get());
        }
    protected:
        AZStd::string m_metricsOutput;
        AZStd::unique_ptr<AZ::Metrics::IEventLoggerFactory> m_eventLoggerFactory;
        AZ::Metrics::EventLoggerId m_loggerId{ 1 };

        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    };

    constexpr const char* EventValueClassName = "EventValue";
    constexpr const char* EventLoggerIdClassName = "EventLoggerId";
    constexpr const char* EventArgsClassName = "EventArgs";
    constexpr const char* EventPhaseClassName = "EventPhase";
    constexpr const char* InstantEventScopeClassName = "InstantEventScope";
    constexpr const char* EventLoggerFactoryClassName = "EventLoggerFactory";

    // EventArgs Member Method Names
    constexpr const char* EventArgsNameMember = "m_name";
    constexpr const char* EventArgsCategoryMember = "m_cat";
    constexpr const char* EventArgsIdMember = "m_id";
    constexpr const char* EventArgsAppendArgMethodName = "AppendArg";

    // EventFactory Member Method Names
    constexpr const char* EventLoggerFactoryIsRegisteredMethodName = "IsRegistered";

    // Global Method names
    constexpr const char* RecordEventMethodName = "RecordEvent";
    constexpr const char* CreateMetricsLoggerMethodName = "CreateMetricsLogger";
    constexpr const char* IsEventLoggerRegisteredMethodName = "IsEventLoggerRegistered";
    constexpr const char* GetGlobalEventLoggerFactoryMethodName = "GetGlobalEventLoggerFactory";

    TEST_F(EventLoggerReflectUtilsFixture, EventLogger_EventDataTypes_AreAccessibleInScript_Succeeds)
    {
        auto classIter = m_behaviorContext->m_classes.find(EventValueClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);

        classIter = m_behaviorContext->m_classes.find(EventLoggerIdClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);

        classIter = m_behaviorContext->m_classes.find(EventArgsClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);

        classIter = m_behaviorContext->m_classes.find(EventPhaseClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);

        classIter = m_behaviorContext->m_classes.find(InstantEventScopeClassName);
        EXPECT_NE(m_behaviorContext->m_classes.end(), classIter);
    }

    TEST_F(EventLoggerReflectUtilsFixture, EventLogger_EventDataTypes_CanBeCreated)
    {
        auto classIter = m_behaviorContext->m_classes.find(EventValueClassName);
        ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
        {
            AZ::BehaviorClass* eventValueClass = classIter->second;
            AZ::BehaviorClass::ScopedBehaviorObject eventValueInst = eventValueClass->CreateWithScope();
            EXPECT_TRUE(eventValueInst.IsValid());
        }

        classIter = m_behaviorContext->m_classes.find(EventLoggerIdClassName);
        ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
        {
            AZ::BehaviorClass* eventLoggerIdClass = classIter->second;
            AZ::BehaviorClass::ScopedBehaviorObject eventLoggerIdInst = eventLoggerIdClass->CreateWithScope();
            EXPECT_TRUE(eventLoggerIdInst.IsValid());
        }

        classIter = m_behaviorContext->m_classes.find(EventArgsClassName);
        ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
        {
            AZ::BehaviorClass* eventArgsClass = classIter->second;
            AZ::BehaviorClass::ScopedBehaviorObject eventArgsInst = eventArgsClass->CreateWithScope();
            EXPECT_TRUE(eventArgsInst.IsValid());
        }

        classIter = m_behaviorContext->m_classes.find(EventPhaseClassName);
        ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
        {
            AZ::BehaviorClass* eventPhaseClass = classIter->second;
            AZ::BehaviorClass::ScopedBehaviorObject eventPhaseInst = eventPhaseClass->CreateWithScope();
            EXPECT_TRUE(eventPhaseInst.IsValid());
        }

        classIter = m_behaviorContext->m_classes.find(InstantEventScopeClassName);
        ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
        {
            AZ::BehaviorClass* instantEventScopeClass = classIter->second;
            AZ::BehaviorClass::ScopedBehaviorObject instantEventScopeInst = instantEventScopeClass->CreateWithScope();
            EXPECT_TRUE(instantEventScopeInst.IsValid());
        }
    }

    TEST_F(EventLoggerReflectUtilsFixture, EventLogger_CanCreateFromIntOrString_Succeeds)
    {
        // Create an EventLoggerId instance in script that maps to Id 1
        auto classIter = m_behaviorContext->m_classes.find(EventLoggerIdClassName);
        ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
        {
            auto methodIter = classIter->second->m_methods.find("CreateIdFromInt");
            ASSERT_NE(classIter->second->m_methods.end(), methodIter);
            AZ::Metrics::EventLoggerId loggerId{ AZStd::numeric_limits<AZStd::underlying_type_t<AZ::Metrics::EventLoggerId>>::max() };
            EXPECT_TRUE(methodIter->second->InvokeResult(loggerId, 1U));
            EXPECT_EQ(1, static_cast<AZ::u32>(loggerId));
        }

        {
            auto methodIter = classIter->second->m_methods.find("CreateIdFromString");
            ASSERT_NE(classIter->second->m_methods.end(), methodIter);
            AZ::Metrics::EventLoggerId loggerId{ AZStd::numeric_limits<AZStd::underlying_type_t<AZ::Metrics::EventLoggerId>>::max() };
            EXPECT_TRUE(methodIter->second->InvokeResult(loggerId, AZStd::string_view("foo")));
            EXPECT_EQ(static_cast<AZ::u32>(AZStd::hash<AZStd::string_view>{}("foo")), static_cast<AZ::u32>(loggerId));
        }
    }

    TEST_F(EventLoggerReflectUtilsFixture, RecordEvent_CanRecordDurationEventInScript_Succeeds)
    {
        const AZ::BehaviorClass* eventArgsClass = m_behaviorContext->FindClassByReflectedName(EventArgsClassName);
        ASSERT_NE(nullptr, eventArgsClass);

        const AZ::BehaviorClass* eventValueClass = m_behaviorContext->FindClassByReflectedName(EventValueClassName);
        ASSERT_NE(nullptr, eventValueClass);

        auto eventArgsScopedObj = eventArgsClass->CreateWithScope();
        EXPECT_TRUE(eventArgsScopedObj.IsValid());

        // Stores arguments for behavior context calls
        AZStd::fixed_vector<AZ::BehaviorArgument, 8> behaviorArguments;

        // Use the bound behavior context properties and methods to configure the ScriptEventArgs
        {
            // Set `ScriptEventArgs::m_name` field
            const AZ::BehaviorMethod* nameSetter = eventArgsClass->FindSetterByReflectedName(EventArgsNameMember);
            ASSERT_NE(nullptr, nameSetter);
            AZStd::string eventName{ "Duration Event" };

            behaviorArguments.emplace_back(&eventArgsScopedObj.m_behaviorObject);
            behaviorArguments.emplace_back(&eventName);
            EXPECT_TRUE(nameSetter->Call(behaviorArguments, nullptr));
        }

        {
            // Set `ScriptEventArgs::m_cat` field
            const AZ::BehaviorMethod* nameSetter = eventArgsClass->FindSetterByReflectedName(EventArgsCategoryMember);
            ASSERT_NE(nullptr, nameSetter);
            behaviorArguments.clear();
            AZStd::string category{ "Test" };

            behaviorArguments.emplace_back(&eventArgsScopedObj.m_behaviorObject);
            behaviorArguments.emplace_back(&category);
            EXPECT_TRUE(nameSetter->Call(behaviorArguments, nullptr));
        }

        {
            // Set `ScriptEventArgs::m_id` field
            const AZ::BehaviorMethod* nameSetter = eventArgsClass->FindSetterByReflectedName(EventArgsIdMember);
            ASSERT_NE(nullptr, nameSetter);
            behaviorArguments.clear();
            AZStd::optional<AZStd::string> eventId{ "0" };

            behaviorArguments.emplace_back(&eventArgsScopedObj.m_behaviorObject);
            behaviorArguments.emplace_back(&eventId);
            EXPECT_TRUE(nameSetter->Call(behaviorArguments, nullptr));
        }

        {
            // Populate the ScriptEventArgs "args" object field
            const AZ::BehaviorMethod* appendArgMethod = eventArgsClass->FindMethodByReflectedName(EventArgsAppendArgMethodName);
            ASSERT_NE(nullptr, appendArgMethod);

            // Create an EventValue instance
            AZ::u64 intValue{ 20 };
            behaviorArguments.clear();
            behaviorArguments.emplace_back(&intValue);
            auto eventValueClassObj = eventValueClass->CreateWithScope(behaviorArguments);
            EXPECT_TRUE(eventValueClassObj.IsValid());

            AZStd::string argName{ "Entity Count" };

            behaviorArguments.clear();
            behaviorArguments.emplace_back(&eventArgsScopedObj.m_behaviorObject);
            behaviorArguments.emplace_back(&argName);
            // The AppendArg function accepts an EventValue instance by value, so it needs to be emplaced
            // with the BehaviorArgumentValueTypeTag
            behaviorArguments.emplace_back(AZ::BehaviorArgumentValueTypeTag, &eventValueClassObj.m_behaviorObject);
            EXPECT_TRUE(appendArgMethod->Call(behaviorArguments, nullptr));

            argName = "Entity Group";
            AZStd::string stringValue{ "Level" };
            behaviorArguments.clear();
            behaviorArguments.emplace_back(&stringValue);
            eventValueClassObj = eventValueClass->CreateWithScope(behaviorArguments);
            EXPECT_TRUE(eventValueClassObj.IsValid());

            // Invoke the AppendArg method this time with a string value
            behaviorArguments.clear();
            behaviorArguments.emplace_back(&eventArgsScopedObj.m_behaviorObject);
            behaviorArguments.emplace_back(&argName);
            behaviorArguments.emplace_back(AZ::BehaviorArgumentValueTypeTag, &eventValueClassObj.m_behaviorObject);
            EXPECT_TRUE(appendArgMethod->Call(behaviorArguments, nullptr));
        }

        // Finally invoke the RecordEvent function
        const AZ::BehaviorMethod* recordEventMethod = m_behaviorContext->FindMethodByReflectedName(RecordEventMethodName);
        ASSERT_NE(nullptr, recordEventMethod);

        constexpr auto durationBeginPhase = AZ::Metrics::EventPhase::DurationBegin;
        behaviorArguments.clear();
        behaviorArguments.emplace_back(&m_loggerId);
        behaviorArguments.emplace_back(&durationBeginPhase);
        behaviorArguments.emplace_back(AZ::BehaviorArgumentValueTypeTag, &eventArgsScopedObj.m_behaviorObject);
        // The return value argument needs storage
        bool returnStorage{};
        AZ::BehaviorArgument returnValue(&returnStorage);
        EXPECT_TRUE(recordEventMethod->Call(behaviorArguments, &returnValue));

        ASSERT_EQ(azrtti_typeid<bool>(), returnValue.m_typeId);
        bool* recordEventResult = returnValue.GetAsUnsafe<bool>();
        ASSERT_NE(nullptr, recordEventResult);
        EXPECT_TRUE(*recordEventResult);

        // Validate that the output contains an "Entity Count and an "Entity Group" argument
        EXPECT_TRUE(m_metricsOutput.contains("Entity Count"));
        EXPECT_TRUE(m_metricsOutput.contains("Entity Group"));
    }

    TEST_F(EventLoggerReflectUtilsFixture, RecordEvent_CanCreateMetricsLoggerInScript_Succeeds)
    {
        const AZ::BehaviorMethod* createLoggerMethod = m_behaviorContext->FindMethodByReflectedName(CreateMetricsLoggerMethodName);
        ASSERT_NE(nullptr, createLoggerMethod);

        constexpr AZ::Metrics::EventLoggerId scriptLoggerId{ 2 };
        AZ::Test::ScopedAutoTempDirectory tempDirectory;
        const auto loggerPath = tempDirectory.GetDirectoryAsFixedMaxPath() / "script_logger.json";
        constexpr AZStd::string_view loggerName = "ScriptLogger";
        bool createLoggerResult{};
        EXPECT_TRUE(createLoggerMethod->InvokeResult(createLoggerResult, scriptLoggerId, loggerPath.c_str(), loggerName));
        EXPECT_TRUE(createLoggerResult);

        // Validate that the event logger created during script is registered with the global event logger factory
        EXPECT_TRUE(m_eventLoggerFactory->IsRegistered(scriptLoggerId));

        // Also validate that the script IsEventLoggerRegistered function succeeds as well
        const AZ::BehaviorMethod* isEventLoggerRegisteredMethod = m_behaviorContext->FindMethodByReflectedName(
            IsEventLoggerRegisteredMethodName);
        ASSERT_NE(nullptr, isEventLoggerRegisteredMethod);
        bool isEventLoggerRegistered{};
        EXPECT_TRUE(isEventLoggerRegisteredMethod->InvokeResult(isEventLoggerRegistered, scriptLoggerId));
        EXPECT_TRUE(isEventLoggerRegistered);

        // Finally validate that the EventFactory IsRegistered member function succeeds as well
        const AZ::BehaviorMethod* getGlobalEventLoggerFactoryMethod = m_behaviorContext->FindMethodByReflectedName(
            GetGlobalEventLoggerFactoryMethodName);
        ASSERT_NE(nullptr, getGlobalEventLoggerFactoryMethod);
        AZ::Metrics::IEventLoggerFactory* eventLoggerFactory{};
        EXPECT_TRUE(getGlobalEventLoggerFactoryMethod->InvokeResult(eventLoggerFactory));

        // This event logger factory should be the same address as the member variable
        ASSERT_EQ(m_eventLoggerFactory.get(), eventLoggerFactory);

        // reset back to false;
        isEventLoggerRegistered = {};
        // Lookup the EventLoggerFactory BehaviorClass in order to lookup the IsRegistered member function
        const AZ::BehaviorClass* eventLoggerFactoryClass = m_behaviorContext->FindClassByReflectedName(EventLoggerFactoryClassName);
        ASSERT_NE(nullptr, eventLoggerFactoryClass);
        const AZ::BehaviorMethod* isRegisteredMethod = eventLoggerFactoryClass->FindMethodByReflectedName(
            EventLoggerFactoryIsRegisteredMethodName);
        ASSERT_NE(nullptr, isRegisteredMethod);
        EXPECT_TRUE(isRegisteredMethod->InvokeResult(isEventLoggerRegistered, eventLoggerFactory, scriptLoggerId));
        EXPECT_TRUE(isEventLoggerRegistered);
    }
} // namespace UnitTest::EventLoggerReflectUtilsTests
