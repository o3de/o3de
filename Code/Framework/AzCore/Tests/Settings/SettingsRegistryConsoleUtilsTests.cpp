/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryConsoleUtils.h>
#include <AzCore/Settings/SettingsRegistryOriginTracker.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/string/string.h>

namespace SettingsRegistryConsoleUtilsTests
{
    class SettingsRegistryConsoleUtilsFixture
        : public UnitTest::LeakDetectionFixture
    {
    public:

        SettingsRegistryConsoleUtilsFixture()
            : LeakDetectionFixture()
        {
            AZ::NameDictionary::Create();
        }

        ~SettingsRegistryConsoleUtilsFixture()
        {
            AZ::NameDictionary::Destroy();
        }

        void SetUp() override
        {
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            m_originTracker = AZStd::make_unique<AZ::SettingsRegistryOriginTracker>(*m_registry);
            // Store off the old global settings registry to restore after each test
            m_oldSettingsRegistry = AZ::SettingsRegistry::Get();
            if (m_oldSettingsRegistry != nullptr)
            {
                AZ::SettingsRegistry::Unregister(m_oldSettingsRegistry);
            }
            AZ::SettingsRegistry::Register(m_registry.get());
        }

        void TearDown() override
        {
            // Restore the old global settings registry
            AZ::SettingsRegistry::Unregister(m_registry.get());
            if (m_oldSettingsRegistry != nullptr)
            {
                AZ::SettingsRegistry::Register(m_oldSettingsRegistry);
                m_oldSettingsRegistry = {};
            }
            m_originTracker.reset();
            m_registry.reset();
        }

        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
        AZStd::unique_ptr<AZ::SettingsRegistryOriginTracker> m_originTracker;
        AZ::SettingsRegistryInterface* m_oldSettingsRegistry{};
    };

    TEST_F(SettingsRegistryConsoleUtilsFixture, SettingsRegistryCommand_SetValue_Successfully)
    {
        constexpr const char* settingsKey = "/TestKey";
        constexpr const char* expectedValue = "TestValue";
        AZ::Console testConsole(*m_registry);
        AZ::SettingsRegistryConsoleUtils::ConsoleFunctorHandle handle{
            AZ::SettingsRegistryConsoleUtils::RegisterAzConsoleCommands(*m_registry, testConsole) };
        EXPECT_TRUE(testConsole.PerformCommand(AZ::SettingsRegistryConsoleUtils::SettingsRegistrySet, { settingsKey, expectedValue }));

        AZ::SettingsRegistryInterface::FixedValueString settingsValue;
        EXPECT_TRUE(m_registry->Get(settingsValue, settingsKey));
        EXPECT_EQ(expectedValue, settingsValue);
    }

    TEST_F(SettingsRegistryConsoleUtilsFixture, SettingsRegistryCommand_SetValue_DoesNotSetValueAfterHandleDestruction)
    {
        constexpr const char* settingsKey = "/TestKey";
        constexpr const char* expectedValue = "TestValue";
        AZ::Console testConsole(*m_registry);

        // Scopes the console functor handle so that it destructs and unregisters the console functors
        {
            [[maybe_unused]] AZ::SettingsRegistryConsoleUtils::ConsoleFunctorHandle handle{
                AZ::SettingsRegistryConsoleUtils::RegisterAzConsoleCommands(*m_registry, testConsole) };
        }
        EXPECT_FALSE(testConsole.PerformCommand(AZ::SettingsRegistryConsoleUtils::SettingsRegistrySet, { settingsKey, expectedValue }));

        AZ::SettingsRegistryInterface::FixedValueString settingsValue;
        EXPECT_FALSE(m_registry->Get(settingsValue, settingsKey));
        EXPECT_NE(expectedValue, settingsValue);
    }

    TEST_F(SettingsRegistryConsoleUtilsFixture, SettingsRegistryCommand_RemoveKey_RemovesValueSuccessfully)
    {
        constexpr const char* settingsKey = "/TestKey";
        constexpr const char* settingsKey2 = "/TestKey2";
        constexpr const char* expectedValue = R"(TestValue)";
        constexpr const char* expectedValue2 = R"(Hello World)";
        AZ::Console testConsole(*m_registry);
        AZ::SettingsRegistryConsoleUtils::ConsoleFunctorHandle handle{
            AZ::SettingsRegistryConsoleUtils::RegisterAzConsoleCommands(*m_registry, testConsole) };

        // Add settings to settings registry
        EXPECT_TRUE(m_registry->Set(settingsKey, expectedValue));
        EXPECT_TRUE(m_registry->Set(settingsKey2, expectedValue2));
        EXPECT_TRUE(testConsole.PerformCommand(AZ::SettingsRegistryConsoleUtils::SettingsRegistryRemove, { settingsKey, settingsKey2 }));

        AZ::SettingsRegistryInterface::FixedValueString settingsValue;
        EXPECT_FALSE(m_registry->Get(settingsValue, settingsKey));
        EXPECT_FALSE(m_registry->Get(settingsValue, settingsKey2));
    }

    TEST_F(SettingsRegistryConsoleUtilsFixture, SettingsRegistryCommand_RemoveKey_DoesNotRemoveAfterHandleDestruction)
    {
        constexpr const char* settingsKey = "/TestKey";
        constexpr const char* settingsKey2 = "/TestKey2";
        constexpr const char* expectedValue = R"(TestValue)";
        constexpr const char* expectedValue2 = R"(Hello World)";
        AZ::Console testConsole(*m_registry);

        // Add settings to settings registry
        EXPECT_TRUE(m_registry->Set(settingsKey, expectedValue));
        EXPECT_TRUE(m_registry->Set(settingsKey2, expectedValue2));

        AZ::SettingsRegistryInterface::FixedValueString settingsValue;
        // Scopes the ConsoleFunctor Handle to make sure it is destroyed before the second PerformCommand call
        {
            AZ::SettingsRegistryConsoleUtils::ConsoleFunctorHandle handle{
                AZ::SettingsRegistryConsoleUtils::RegisterAzConsoleCommands(*m_registry, testConsole) };
            // @settingsKey2 should be removed by the remove command
            EXPECT_TRUE(testConsole.PerformCommand(AZ::SettingsRegistryConsoleUtils::SettingsRegistryRemove, { settingsKey2 }));
            EXPECT_FALSE(m_registry->Get(settingsValue, settingsKey2));
        }

        // @settingsKey should be remain since the settings registry is no longer registered with the local console
        EXPECT_FALSE(testConsole.PerformCommand(AZ::SettingsRegistryConsoleUtils::SettingsRegistryRemove, { settingsKey }));
        EXPECT_TRUE(m_registry->Get(settingsValue, settingsKey));
        EXPECT_STREQ(expectedValue, settingsValue.c_str());
    }

    TEST_F(SettingsRegistryConsoleUtilsFixture, SettingsRegistryCommand_DumpValue_SuccessfullyDumpsValueToTraceBus)
    {
        constexpr const char* settingsKey = "/TestKey";
        constexpr const char* settingsKey2 = "/TestKey2";
        constexpr const char* expectedValue = R"(TestValue)";
        constexpr const char* expectedValue2 = R"(Hello World)";
        AZ::Console testConsole(*m_registry);

        AZ::SettingsRegistryConsoleUtils::ConsoleFunctorHandle handle{
            AZ::SettingsRegistryConsoleUtils::RegisterAzConsoleCommands(*m_registry, testConsole) };

        // Add settings to settings registry
        EXPECT_TRUE(m_registry->Set(settingsKey, expectedValue));

        EXPECT_TRUE(m_registry->Set(settingsKey2, expectedValue2));

        // Create a TraceMessageBus Handler for capturing the dumping of the settings
        struct SettingsRegistryDumpCommandHandler
            : public AZ::Debug::TraceMessageBus::Handler
        {
            bool OnOutput(const char* window, const char* message) override
            {
                if (window == AZStd::string_view("SettingsRegistry"))
                {
                    bool foundExpectedValue{};
                    AZStd::string_view messageView(message);
                    for (AZStd::string_view expectedValue : m_validValues)
                    {
                        if (messageView.find(expectedValue) != AZStd::string_view::npos)
                        {
                            foundExpectedValue = true;
                            break;
                        }
                    }
                    if (!foundExpectedValue)
                    {
                        AZ::SettingsRegistryInterface::FixedValueString delimitedExpectedValues;
                        AZ::StringFunc::Join(delimitedExpectedValues, m_validValues.begin(), m_validValues.end(), ",");

                        ADD_FAILURE() << "Message:" << message << " Did not contain any of the expected test values: "
                            << delimitedExpectedValues.c_str();
                    }
                    return true;
                }

                return false;
            }
            AZStd::fixed_vector<AZStd::string_view, 2> m_validValues;
        };

        SettingsRegistryDumpCommandHandler traceHandler;
        traceHandler.m_validValues = { expectedValue, expectedValue2 };
        traceHandler.BusConnect();
        EXPECT_TRUE(testConsole.PerformCommand(AZ::SettingsRegistryConsoleUtils::SettingsRegistryDump, { settingsKey }));
        EXPECT_TRUE(testConsole.PerformCommand(AZ::SettingsRegistryConsoleUtils::SettingsRegistryDump, { settingsKey2, settingsKey }));
        traceHandler.BusDisconnect();
    }

    TEST_F(SettingsRegistryConsoleUtilsFixture, SettingsRegistryCommand_DumpAll_DumpsEntireSettingsRegistry)
    {
        constexpr const char* SettingsKey = "TestKey";
        constexpr const char* SettingsKey2 = "TestKey2";
        constexpr const char* ExpectedValue = R"(TestValue)";
        constexpr const char* ExpectedValue2 = R"(Hello World)";
        AZ::Console testConsole(*m_registry);

        AZ::SettingsRegistryConsoleUtils::ConsoleFunctorHandle handle{
            AZ::SettingsRegistryConsoleUtils::RegisterAzConsoleCommands(*m_registry, testConsole) };

        // Add settings to settings registry
        auto jsonPointerPath = AZ::SettingsRegistryInterface::FixedValueString::format("/%s",
            SettingsKey);
        EXPECT_TRUE(m_registry->Set(jsonPointerPath, ExpectedValue));

        // Second Setting to set
        jsonPointerPath = AZ::SettingsRegistryInterface::FixedValueString::format("/%s",
            SettingsKey2);
        EXPECT_TRUE(m_registry->Set(jsonPointerPath, ExpectedValue2));

        // Create a TraceMessageBus Handler for capturing the dumping of the settings
        struct SettingsRegistryDumpAllCommandHandler
            : public AZ::Debug::TraceMessageBus::Handler
        {
            SettingsRegistryDumpAllCommandHandler(const char* settingsKey, const char* settingsKey2,
                const char* expectedValue, const char* expectedValue2)
                : m_settingsKey{ settingsKey }
                , m_settingsKey2{ settingsKey2 }
                , m_expectedValue{ expectedValue }
                , m_expectedValue2{ expectedValue2 }
            {
            }
            bool OnOutput(const char* window, const char* message) override
            {
                if (window == AZStd::string_view("SettingsRegistry"))
                {
                    rapidjson::Document jsonDocument;
                    jsonDocument.Parse(message);
                    // Test for the first key
                    auto memberIterator = jsonDocument.FindMember(m_settingsKey);
                    EXPECT_NE(jsonDocument.MemberEnd(), memberIterator);
                    EXPECT_STREQ(m_expectedValue, memberIterator->value.GetString());

                    // Test for the second key
                    memberIterator = jsonDocument.FindMember(m_settingsKey2);
                    EXPECT_NE(jsonDocument.MemberEnd(), memberIterator);
                    EXPECT_STREQ(m_expectedValue2, memberIterator->value.GetString());
                    return true;
                }

                return false;
            }

            const char* m_settingsKey{};
            const char* m_settingsKey2{};
            const char* m_expectedValue{};
            const char* m_expectedValue2{};
        };

        SettingsRegistryDumpAllCommandHandler traceHandler{ SettingsKey, SettingsKey2, ExpectedValue, ExpectedValue2 };
        traceHandler.BusConnect();
        EXPECT_TRUE(testConsole.PerformCommand(AZ::SettingsRegistryConsoleUtils::SettingsRegistryDumpAll));
        traceHandler.BusDisconnect();
    }

    TEST_F(SettingsRegistryConsoleUtilsFixture, SettingsRegistryCommand_DumpOrigin_Succeeds)
    {
        constexpr const char* SettingsKey = "TestKey";
        constexpr const char* SettingsKey2 = "TestKey2";
        constexpr const char* ExpectedValue = R"(TestValue)";
        constexpr const char* ExpectedValue2 = R"(Hello World)";
        AZ::Console testConsole(*m_registry);

        AZ::SettingsRegistryConsoleUtils::ConsoleFunctorHandle handle{ AZ::SettingsRegistryConsoleUtils::RegisterAzConsoleCommands(
            *m_originTracker, testConsole) };

        // Add settings to settings registry
        auto jsonPointerPath = AZ::SettingsRegistryInterface::FixedValueString::format("/%s", SettingsKey);
        EXPECT_TRUE(m_registry->Set(jsonPointerPath, ExpectedValue));

        // Second Setting to set
        jsonPointerPath = AZ::SettingsRegistryInterface::FixedValueString::format("/%s", SettingsKey2);
        EXPECT_TRUE(m_registry->Set(jsonPointerPath, ExpectedValue2));

        // Create a TraceMessageBus Handler for capturing the dumping of the settings
        struct SettingsRegistryDumpOriginCommandHandler : public AZ::Debug::TraceMessageBus::Handler
        {
            SettingsRegistryDumpOriginCommandHandler(
                const char* settingsKey, const char* settingsKey2, const char* expectedValue, const char* expectedValue2)
                : m_settingsKey{ settingsKey }
                , m_settingsKey2{ settingsKey2 }
                , m_expectedValue{ expectedValue }
                , m_expectedValue2{ expectedValue2 }
            {
            }
            bool OnOutput(const char* window, const char* message) override
            {
                if (window == AZStd::string_view("SettingsRegistry"))
                {
                    // Test for in memory origin
                    AZStd::string_view messageView = message;
                    EXPECT_TRUE(messageView.contains("<in-memory>"));
                }
                return false;
            }

            const char* m_settingsKey{};
            const char* m_settingsKey2{};
            const char* m_expectedValue{};
            const char* m_expectedValue2{};
        };

        SettingsRegistryDumpOriginCommandHandler traceHandler{ SettingsKey, SettingsKey2, ExpectedValue, ExpectedValue2 };
        traceHandler.BusConnect();
        EXPECT_TRUE(testConsole.PerformCommand(AZ::SettingsRegistryConsoleUtils::SettingsRegistryDumpOrigin));
        traceHandler.BusDisconnect();
    }
} 
