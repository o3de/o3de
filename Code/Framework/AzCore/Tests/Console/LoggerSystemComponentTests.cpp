/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class LoggerSystemComponentTests
        : public LeakDetectionFixture
    {
    public:
        LoggerSystemComponentTests()
            : m_logHandler
            (
                [this](AZ::LogLevel logLevel, const char* message, const char* file, const char* function, int32_t line)
                {
                    LogEventHandler(logLevel, message, file, function, line);
                }
            )
        {
            ;
        }

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_loggerComponent = aznew AZ::LoggerSystemComponent;
            AZ::Interface<AZ::ILogger>::Get()->BindLogHandler(m_logHandler);
            AZ::Interface<AZ::ILogger>::Get()->SetLogLevel(AZ::LogLevel::Trace); // Set to trace by default
        }

        void TearDown() override
        {
            m_logHandler.Disconnect();
            delete m_loggerComponent;
            LeakDetectionFixture::TearDown();
        }

        void LogEventHandler(AZ::LogLevel logLevel, const char* message, const char*, const char*, int32_t)
        {
            m_lastLogLevel = logLevel;
            m_lastLogMessage = message;
            m_lastLogMessage = AZ::StringFunc::TrimWhiteSpace(m_lastLogMessage, true, true);
        }

        AZ::LoggerSystemComponent* m_loggerComponent = nullptr;
        AZ::ILogger::LogEvent::Handler m_logHandler;

        AZ::LogLevel m_lastLogLevel = AZ::LogLevel::Trace;
        AZStd::string m_lastLogMessage;
    };

    TEST_F(LoggerSystemComponentTests, LogTraceTest)
    {
        AZLOG_TRACE("test trace");
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Trace);
        EXPECT_EQ(strcmp("test trace", m_lastLogMessage.c_str()), 0);
    }

    TEST_F(LoggerSystemComponentTests, LogDebugTest)
    {
        AZLOG_DEBUG("test debug");
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Debug);
        EXPECT_EQ(strcmp("test debug", m_lastLogMessage.c_str()), 0);
    }

    TEST_F(LoggerSystemComponentTests, LogInfoTest)
    {
        AZLOG_INFO("test info");
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Info);
        EXPECT_EQ(strcmp("test info", m_lastLogMessage.c_str()), 0);
    }

    TEST_F(LoggerSystemComponentTests, LogNoticeTest)
    {
        AZLOG_NOTICE("test notice");
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Notice);
        EXPECT_EQ(strcmp("test notice", m_lastLogMessage.c_str()), 0);
    }

    TEST_F(LoggerSystemComponentTests, LogWarnTest)
    {
        AZLOG_WARN("test warn");
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Warn);
        EXPECT_EQ(strcmp("test warn", m_lastLogMessage.c_str()), 0);
    }

    TEST_F(LoggerSystemComponentTests, LogErrorTest)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        AZLOG_ERROR("test error");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Error);
        EXPECT_EQ(strcmp("test error", m_lastLogMessage.c_str()), 0);
    }

    TEST_F(LoggerSystemComponentTests, LogFatalTest)
    {
        AZLOG_FATAL("test fatal");
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Fatal);
        EXPECT_EQ(strcmp("test fatal", m_lastLogMessage.c_str()), 0);
    }

    TEST_F(LoggerSystemComponentTests, LogLevelTest)
    {
        AZ::Interface<AZ::ILogger>::Get()->SetLogLevel(AZ::LogLevel::Trace);
        AZLOG_DEBUG("test debug");
        AZ::Interface<AZ::ILogger>::Get()->SetLogLevel(AZ::LogLevel::Debug);
        AZLOG_TRACE("test trace");
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Debug);
        EXPECT_EQ(strcmp("test debug", m_lastLogMessage.c_str()), 0);
    }

    TEST_F(LoggerSystemComponentTests, TaggedLogEnableDisableTest)
    {
        AZ::Interface<AZ::ILogger>::Get()->SetLogLevel(AZ::LogLevel::Trace);

        AZLOG_DEBUG("test debug");
        AZLOG(TEST_TAG, "test tag"); // Tag is not enabled, callback should not get invoked
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Debug);
        EXPECT_EQ(strcmp("test debug", m_lastLogMessage.c_str()), 0);

        AZ::ConsoleCommandContainer arguments{ "TEST_TAG" };
        m_loggerComponent->EnableLog(arguments); // Tag should now be enabled
        AZLOG(TEST_TAG, "test tag"); // Tag is enabled, callback should get invoked
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Notice);
        EXPECT_EQ(strcmp("test tag", m_lastLogMessage.c_str()), 0);

        m_loggerComponent->DisableLog(arguments); // Tag should now be disabled
        AZLOG_DEBUG("test debug");
        AZLOG(TEST_TAG, "test tag"); // Tag is not enabled, callback should not get invoked
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Debug);
        EXPECT_EQ(strcmp("test debug", m_lastLogMessage.c_str()), 0);
    }

    TEST_F(LoggerSystemComponentTests, TaggedLogNamesAndCurrentTagTest)
    {
        AZ::ILogger* testLogger = AZ::Interface<AZ::ILogger>::Get();
        AZStd::string tagSeenByHandler;
        AZ::ILogger::LogEvent::Handler tagHandler(
            [&tagSeenByHandler, testLogger](AZ::LogLevel, const char*, const char*, const char*, int32_t)
            {
                const char* tag = testLogger->GetCurrentTag();
                tagSeenByHandler = tag ? tag : "";
            });
        testLogger->BindLogHandler(tagHandler);

        m_loggerComponent->RegisterTag(AZ::TypeHash32("NAMED_TAG"), "NAMED_TAG");
        m_loggerComponent->EnableLog(AZ::ConsoleCommandContainer{ "ENABLED_TAG" });

        // The handler sees the tag of a tagged message, and no tag for an untagged one.
        AZLOG(ENABLED_TAG, "tagged");
        EXPECT_EQ(tagSeenByHandler, "ENABLED_TAG");

        AZLOG_INFO("untagged");
        EXPECT_EQ(tagSeenByHandler, "");

        // Both the registered tag and the enabled tag are listed, with their enabled state.
        AZStd::unordered_map<AZStd::string, bool> tags;
        testLogger->VisitTags(
            [&tags](const char* tagName, bool enabled)
            {
                tags[tagName] = enabled;
            });
        EXPECT_EQ(tags.size(), size_t{ 2 });
        EXPECT_FALSE(tags["NAMED_TAG"]);
        EXPECT_TRUE(tags["ENABLED_TAG"]);

        tagHandler.Disconnect();
    }

    TEST_F(LoggerSystemComponentTests, TaggedLogToggleTest)
    {
        AZ::Interface<AZ::ILogger>::Get()->SetLogLevel(AZ::LogLevel::Trace);

        AZLOG_DEBUG("test debug");
        AZLOG(TEST_TAG, "test tag"); // Tag is not enabled, callback should not get invoked
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Debug);
        EXPECT_EQ(strcmp("test debug", m_lastLogMessage.c_str()), 0);

        AZ::ConsoleCommandContainer arguments{ "TEST_TAG" };
        m_loggerComponent->ToggleLog(arguments); // Tag should now be enabled
        AZLOG(TEST_TAG, "test tag"); // Tag is enabled, callback should get invoked
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Notice);
        EXPECT_EQ(strcmp("test tag", m_lastLogMessage.c_str()), 0);

        m_loggerComponent->ToggleLog(arguments); // Tag should now be disabled
        AZLOG_DEBUG("test debug");
        AZLOG(TEST_TAG, "test tag"); // Tag is not enabled, callback should not get invoked
        EXPECT_EQ(m_lastLogLevel, AZ::LogLevel::Debug);
        EXPECT_EQ(strcmp("test debug", m_lastLogMessage.c_str()), 0);
    }
}
