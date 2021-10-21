/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <limits>
#include <AzCore/Debug/LocalFileEventLogger.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace AZ::Debug
{
    class LocalFileEventLoggerTest
        : public UnitTest::AllocatorsFixture
    {
    public:
        inline static constexpr EventNameHash MessageId = EventNameHash("Message");
        inline static constexpr const char* LogFileName = "TestLog.azel";
    };

    TEST_F(LocalFileEventLoggerTest, RecordEvent_SingleString_WrittenToLog)
    {
        constexpr const char* message = "Hello world";

        LocalFileEventLogger realLogger;

        AZ::Test::ScopedAutoTempDirectory tempDir;
        auto logFilePath = tempDir.Resolve(LogFileName);

        auto logger = Interface<IEventLogger>::Get();
        ASSERT_NE(logger, nullptr);

        realLogger.Start(logFilePath.c_str());
        logger->RecordStringEvent(MessageId, message);
        realLogger.Stop();

        EventLogReader reader;
        ASSERT_TRUE(reader.ReadLog(logFilePath.c_str()));

        EXPECT_EQ(reader.GetEventName(), PrologEventHash);

        ASSERT_TRUE(reader.Next());
        EXPECT_EQ(reader.GetEventName(), MessageId);
        EXPECT_STREQ(reader.GetString().data(), message);

        EXPECT_FALSE(reader.Next());
    }

    TEST_F(LocalFileEventLoggerTest, RecordEvent_SeveralStrings_WrittenToLog)
    {
        constexpr const char* messages[] = {
            "Hello world",
            "And goodbye",
            "It has been a long and strange journey"
        };

        LocalFileEventLogger realLogger;

        AZ::Test::ScopedAutoTempDirectory tempDir;
        auto logFilePath = tempDir.Resolve(LogFileName);

        auto logger = Interface<IEventLogger>::Get();
        ASSERT_NE(logger, nullptr);

        realLogger.Start(logFilePath.c_str());
        for (auto message : messages)
        {
            logger->RecordStringEvent(MessageId, message);
        }
        realLogger.Stop();

        EventLogReader reader;
        ASSERT_TRUE(reader.ReadLog(logFilePath.c_str()));

        EXPECT_EQ(reader.GetEventName(), PrologEventHash);

        for (auto message : messages)
        {
            ASSERT_TRUE(reader.Next());
            EXPECT_EQ(reader.GetEventName(), MessageId);
            EXPECT_STREQ(reader.GetString().data(), message);
        }

        EXPECT_FALSE(reader.Next());
    }

    TEST_F(LocalFileEventLoggerTest, RecordEvent_StringsFromMultipleThreads_WrittenToLog)
    {
        constexpr const char* messages[] = {
            "Hello world",
            "And goodbye"
        };

        constexpr size_t totalThreads = 4;

        AZ::Test::ScopedAutoTempDirectory tempDir;
        LocalFileEventLogger realLogger;

        auto logFilePath = tempDir.Resolve(LogFileName);

        realLogger.Start(logFilePath.c_str());

        AZStd::atomic_bool startLogging = false;

        AZStd::thread threads[totalThreads];
        for (size_t threadIndex = 0; threadIndex < totalThreads; ++threadIndex)
        {
            AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                      // capture. Newer versions issue unused warning
            threads[threadIndex] = AZStd::thread([&startLogging, &messages]()
            AZ_POP_DISABLE_WARNING
            {
                while (!startLogging)
                {
                    AZStd::this_thread::yield();
                }

                auto logger = Interface<IEventLogger>::Get();
                ASSERT_NE(logger, nullptr);

                for (auto message : messages)
                {
                    logger->RecordStringEvent(MessageId, message);
                }
            });
        }
        startLogging = true;

        for (size_t threadIndex = 0; threadIndex < totalThreads; ++threadIndex)
        {
            threads[threadIndex].join();
        }

        realLogger.Stop();

        EventLogReader reader;
        ASSERT_TRUE(reader.ReadLog(logFilePath.c_str()));

        uint64_t threadIds[totalThreads]{ 0 };
        for (size_t threadIndex = 0; threadIndex < totalThreads; ++threadIndex)
        {
            EXPECT_EQ(reader.GetEventName(), PrologEventHash);
            threadIds[threadIndex] = reader.GetThreadId();

            for (size_t otherThreadIndex = 0; otherThreadIndex < threadIndex; ++otherThreadIndex)
            {
                EXPECT_NE(threadIds[threadIndex], threadIds[otherThreadIndex]);
            }

            for (auto message : messages)
            {
                ASSERT_TRUE(reader.Next());
                EXPECT_EQ(reader.GetEventName(), MessageId);
                EXPECT_STREQ(reader.GetString().data(), message);
            }

            reader.Next();
        }
    }

    TEST_F(LocalFileEventLoggerTest, RecordEvent_BufferGetsWrittenWhenFull_WrittenToLog)
    {
        constexpr EventNameHash largeBlockId = EventNameHash("Large block");
        constexpr size_t largeBlockSizeOffset = 14;
        constexpr size_t largeBlockSize = AZStd::numeric_limits<uint16_t>::max() - largeBlockSizeOffset;
        struct LargeBlock
        {
            char m_block[largeBlockSize];
        };

        constexpr const char* message = "The message after the large block.";

        LocalFileEventLogger realLogger;

        AZ::Test::ScopedAutoTempDirectory tempDir;
        auto logFilePath = tempDir.Resolve(LogFileName);

        auto logger = Interface<IEventLogger>::Get();
        ASSERT_NE(logger, nullptr);

        realLogger.Start(logFilePath.c_str());

        logger->RecordEventBegin<LargeBlock>(largeBlockId);
        logger->RecordEventEnd();

        logger->RecordStringEvent(MessageId, message);
        realLogger.Stop();

        EventLogReader reader;
        ASSERT_TRUE(reader.ReadLog(logFilePath.c_str()));
        EXPECT_EQ(reader.GetEventName(), PrologEventHash);

        ASSERT_TRUE(reader.Next());
        EXPECT_EQ(reader.GetEventName(), largeBlockId);
        EXPECT_EQ(reader.GetEventSize(), largeBlockSize);

        ASSERT_TRUE(reader.Next());
        EXPECT_EQ(reader.GetEventName(), PrologEventHash); // Another prolog as a new cache block started.

        ASSERT_TRUE(reader.Next());
        EXPECT_EQ(reader.GetEventName(), MessageId);
        EXPECT_STREQ(reader.GetString().data(), message);

        EXPECT_FALSE(reader.Next());
    }

    TEST_F(LocalFileEventLoggerTest, Flush_DuringMultipleRecords_FlushDoesNotDeadlock)
    {
        constexpr size_t totalThreads = 8;
        constexpr size_t recordsPerThreadCount = 2000;
        constexpr size_t recordsYieldCount = totalThreads * 1000;

        constexpr const char* message = "This is a threaded message test.";

        LocalFileEventLogger realLogger;

        AZ::Test::ScopedAutoTempDirectory tempDir;
        auto logFilePath = tempDir.Resolve(LogFileName);

        realLogger.Start(logFilePath.c_str());

        AZStd::atomic_int totalRecordsWritten = 0;
        AZStd::atomic_bool startLogging = false;

        AZStd::thread threads[totalThreads];
        for (size_t threadIndex = 0; threadIndex < totalThreads; ++threadIndex)
        {
            AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                      // capture. Newer versions issue unused warning
            threads[threadIndex] = AZStd::thread([&startLogging, &message, &totalRecordsWritten]()
            AZ_POP_DISABLE_WARNING
            {
                AZ_UNUSED(message);

                while (!startLogging)
                {
                    AZStd::this_thread::yield();
                }

                auto logger = Interface<IEventLogger>::Get();
                ASSERT_NE(logger, nullptr);

                for (size_t recordCount = 0; recordCount < recordsPerThreadCount; ++recordCount)
                {
                    logger->RecordStringEvent(MessageId, message);
                    ++totalRecordsWritten;
                }
            });
        }
        startLogging = true;

        while (totalRecordsWritten < recordsYieldCount)
        {
            AZStd::this_thread::yield();
        }

        realLogger.Flush();

        for (size_t threadIndex = 0; threadIndex < totalThreads; ++threadIndex)
        {
            threads[threadIndex].join();
        }

        realLogger.Stop();
    }
} // namespace AZ::Debug
