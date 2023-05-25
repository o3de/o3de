/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QString>
#include <AzCore/Debug/TraceMessageBus.h>
#include <QDir>
#include <AzFramework/IO/LocalFileIO.h>

#include <AzCore/UnitTest/UnitTest.h>
#endif

// ----------------- UTILITY FUNCTIONS --------------------
namespace UnitTestUtils
{
    /** sleep for the minimum amount of time that the file system can store.
    * Different file systems (windows, mac, for example) have differing resolutions that they have for
    * file times.  HFS stores only 'seconds' precision, for example.  so tests that need to wait so that
    * mod times have changed, must wait for this amount of time
    **/
    void SleepForMinimumFileSystemTime();

    //! Create a dummy file using AZ::IO APIs which support mocking
    bool CreateDummyFileAZ(AZ::IO::PathView fullPathToFile, AZStd::string_view contents = "");
    //! Create a dummy file, with optional contents.  Will create directories for it too.
    bool CreateDummyFile(const QString& fullPathToFile, QString contents = "");
    //! This function pumps the Qt event queue until either the varToWatch becomes true or the specified millisecond elapse.
    bool BlockUntil(bool& varToWatch, int millisecondsMax);

    // the Assert Absorber here is used to absorb asserts and errors during unit tests.
    // it only absorbs asserts spawned by this thread;
    class AssertAbsorber : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        AssertAbsorber(bool debugMessages = false) : m_debugMessages{debugMessages}
        {
            m_debugMessages = debugMessages;
            // only absorb asserts when this object is on scope in the thread that this object is on scope in.
            BusConnect();
        }

        void ExpectCheck(const int& numAbsorbed, int expectedAbsorbed, const char* errorType, const AZStd::vector<AZStd::string>& messageList)
        {
            if (numAbsorbed != expectedAbsorbed)
            {
                BusDisconnect();
                AZ_Printf("AssertAbsorber", "Incorrect number of %s absobed:\n\n", errorType);
                for (auto& thisMessage : messageList)
                {
                    AZ_Printf("Absorbed", thisMessage.c_str());
                }
                BusConnect();
            }
            ASSERT_EQ(numAbsorbed, expectedAbsorbed);
        }

        void AssertCheck(const int& numAbsorbed, int expectedAbsorbed, const char* errorType, const AZStd::vector<AZStd::string>& messageList)
        {
            if (numAbsorbed != expectedAbsorbed)
            {
                BusDisconnect();
                AZ_Printf("AssertAbsorber", "Incorrect number of %s absorbed:\n\n", errorType);
                for (auto& thisMessage : messageList)
                {
                    AZ_Printf("Absorbed", thisMessage.c_str());
                }
                BusConnect();
            }
            ASSERT_EQ(numAbsorbed, expectedAbsorbed);
        }

        void ExpectWarnings(int expectValue)
        {
            ExpectCheck(m_numWarningsAbsorbed, expectValue, "warnings", m_warningMessages);
        }

        void ExpectErrors(int expectValue)
        {
            ExpectCheck(m_numErrorsAbsorbed, expectValue, "errors", m_errorMessages);
        }

        void ExpectAsserts(int expectValue)
        {
            ExpectCheck(m_numAssertsAbsorbed, expectValue, "asserts", m_assertMessages);
        }

        void AssertWarnings(int expectValue)
        {
            AssertCheck(m_numWarningsAbsorbed, expectValue, "warnings", m_warningMessages);
        }

        void AssertErrors(int expectValue)
        {
            AssertCheck(m_numErrorsAbsorbed, expectValue, "errors", m_errorMessages);
        }

        void AssertAsserts(int expectValue)
        {
            AssertCheck(m_numAssertsAbsorbed, expectValue, "asserts", m_assertMessages);
        }

        bool OnPreWarning([[maybe_unused]] const char* window, [[maybe_unused]] const char* fileName, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* message) override
        {
            ++m_numWarningsAbsorbed;
            if (m_debugMessages)
            {
                m_warningMessages.push_back(AZStd::string::format("%s\n    File: %s  Line: %d  Func: %s\n", message, fileName, line, func));
            }
            return true;
        }

        bool OnPreAssert([[maybe_unused]] const char* fileName, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* message) override
        {
            // Print out absorbed asserts since asserts are pretty important and accidentally absorbing unintended ones can lead to difficult-to-detect issues
            UnitTest::ColoredPrintf(UnitTest::COLOR_YELLOW, "Absorbed Assert: %s\n", message);

            ++m_numAssertsAbsorbed;
            if (m_debugMessages)
            {
                m_assertMessages.push_back(AZStd::string::format("%s\n    File: %s  Line: %d  Func: %s\n", message, fileName, line, func));
            }
            return true; // I handled this, do not forward it
        }

        bool OnPreError([[maybe_unused]] const char* window, [[maybe_unused]] const char* fileName, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* message) override
        {
            ++m_numErrorsAbsorbed;
            if (m_debugMessages)
            {
                m_errorMessages.push_back(AZStd::string::format("%s\n    File: %s  Line: %d  Func: %s\n", message, fileName, line, func));
            }
            return true; // I handled this, do not forward it
        }

        bool OnPrintf(const char* /*window*/, const char* /*message*/) override
        {
            ++m_numMessagesAbsorbed;
            return true;
        }

        void PrintAbsorbed()
        {
            BusDisconnect();
            AZ_Printf("AssertAbsorber", "Warnings Absorbed:\n");
            for (auto& thisMessage : m_warningMessages)
            {
                AZ_Printf("AbsorbedWarning", thisMessage.c_str());
            }
            AZ_Printf("AssertAbsorber", "Errors Absorbed:\n");
            for (auto& thisMessage : m_errorMessages)
            {
                AZ_Printf("AbsorbedError", thisMessage.c_str());
            }
            AZ_Printf("AssertAbsorber", "Warnings Absorbed:\n");
            for (auto& thisMessage : m_assertMessages)
            {
                AZ_Printf("AbsorbedAssert", thisMessage.c_str());
            }
            BusConnect();
        }

        ~AssertAbsorber()
        {
            BusDisconnect();
        }

        void Clear()
        {
            m_numMessagesAbsorbed = 0;
            m_numWarningsAbsorbed = 0;
            m_numAssertsAbsorbed = 0;
            m_numErrorsAbsorbed = 0;
            m_warningMessages.clear();
            m_errorMessages.clear();
            m_assertMessages.clear();
        }
        AZStd::vector<AZStd::string> m_assertMessages;
        AZStd::vector<AZStd::string> m_warningMessages;
        AZStd::vector<AZStd::string> m_errorMessages;
        int m_numMessagesAbsorbed = 0;
        int m_numWarningsAbsorbed = 0;
        int m_numAssertsAbsorbed = 0;
        int m_numErrorsAbsorbed = 0;
        bool m_debugMessages{ false };
    };

    //! Automatically restore current directory when this leaves scope:
    class ScopedDir
    {
    public:
        ScopedDir() = default;

        ScopedDir(QString newDir)
        {
            Setup(newDir);
        }

        void Setup(QString newDir)
        {
            m_originalDir = QDir::currentPath();
            newDir = QDir::cleanPath(newDir);
            QDir::setCurrent(newDir);

            m_localFileIO = aznew AZ::IO::LocalFileIO();

            m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            if (m_priorFileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
            }

            AZ::IO::FileIOBase::SetInstance(m_localFileIO);

            m_localFileIO->SetAlias("@products@", (newDir + QString("/ALIAS/assets")).toUtf8().constData());
            m_localFileIO->SetAlias("@log@", (newDir + QString("/ALIAS/logs")).toUtf8().constData());
            m_localFileIO->SetAlias("@usercache@", (newDir + QString("/ALIAS/cache")).toUtf8().constData());
            m_localFileIO->SetAlias("@user@", (newDir + QString("/ALIAS/user")).toUtf8().constData());
        }

        ~ScopedDir()
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;
            m_localFileIO = nullptr;
            if (m_priorFileIO)
            {
                AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
            }
            QDir::setCurrent(m_originalDir);
        }

    private:
        QString m_originalDir;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;
    };
}
