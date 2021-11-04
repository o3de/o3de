/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef UNITTESTRUNNER_H
#define UNITTESTRUNNER_H

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QMetaObject>
#include <QString>
#include <QDebug>
#include <AzCore/Debug/TraceMessageBus.h>
#include <QDir>
#include <QTemporaryDir>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/IO/LocalFileIO.h>
#endif

#include <gtest/gtest.h>
#include <AzCore/UnitTest/UnitTest.h>

//! These macros can be used for checking your unit tests,
//! you can check AssetScannerUnitTest.cpp for usage
#define UNIT_TEST_EXPECT_TRUE(condition) \
    UNIT_TEST_CHECK((condition))

#define UNIT_TEST_EXPECT_FALSE(condition) \
    UNIT_TEST_CHECK(!(condition))

#define UNIT_TEST_CHECK(condition)                                                                          \
    if (!(condition))                                                                                       \
    {                                                                                                       \
        QString failMessage = QString("%1(%2): ---- FAIL: %3").arg(__FILE__).arg(__LINE__).arg(#condition); \
        Q_EMIT UnitTestFailed(failMessage);                                                                 \
        return;                                                                                             \
    }

//! This is the base class.  Derive from this class, implement StartTest
//! and emit UnitTestPassed when done or UnitTestFailed when you fail
//! You must emit either one or the other for the next unit test to start.
class UnitTestRun
    : public QObject
{
    Q_OBJECT
public:
    virtual ~UnitTestRun() {}
    ///implement all your unit tests in this function
    virtual void StartTest() = 0;
    ///Unit tests having higher priority will run first,
    ///negative value means higher priority, default priority is zero
    virtual int UnitTestPriority() const;
    const char* GetName() const;
    void SetName(const char* name);

Q_SIGNALS:
    void UnitTestFailed(QString message);
    void UnitTestPassed();

private:
    const char* m_name = nullptr; // expected to be static memory!
};

// after deriving from this class, place the following somewhere
// REGISTER_UNIT_TEST(YourClassName)

// ----------------- UTILITY FUNCTIONS --------------------
namespace UnitTestUtils
{
    /** sleep for the minimum amount of time that the file system can store.
    * Different file systems (windows, mac, for example) have differing resolutions that they have for
    * file times.  HFS stores only 'seconds' precision, for example.  so tests that need to wait so that
    * mod times have changed, must wait for this amount of time
    **/
    void SleepForMinimumFileSystemTime();

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
            UnitTest::ColoredPrintf(UnitTest::COLOR_YELLOW, message);
            ++m_numWarningsAbsorbed;
            if (m_debugMessages)
            {
                m_warningMessages.push_back(AZStd::string::format("%s\n    File: %s  Line: %d  Func: %s\n", message, fileName, line, func));
            }
            return true;
        }

        bool OnPreAssert([[maybe_unused]] const char* fileName, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* message) override
        {
            UnitTest::ColoredPrintf(UnitTest::COLOR_YELLOW, message);
            ++m_numAssertsAbsorbed;
            if (m_debugMessages)
            {
                m_assertMessages.push_back(AZStd::string::format("%s\n    File: %s  Line: %d  Func: %s\n", message, fileName, line, func));
            }
            return true; // I handled this, do not forward it
        }

        bool OnPreError([[maybe_unused]] const char* window, [[maybe_unused]] const char* fileName, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* message) override
        {
            UnitTest::ColoredPrintf(UnitTest::COLOR_YELLOW, message);
            ++m_numErrorsAbsorbed;
            if (m_debugMessages)
            {
                m_errorMessages.push_back(AZStd::string::format("%s\n    File: %s  Line: %d  Func: %s\n", message, fileName, line, func));
            }
            return true; // I handled this, do not forward it
        }

        bool OnPrintf(const char* /*window*/, const char* message) override
        {
            UnitTest::ColoredPrintf(UnitTest::COLOR_YELLOW, message);
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



// ----------------------------------- IMPLEMENTATION DETAILS ------------------------
class UnitTestRegistry
{
public:
    explicit UnitTestRegistry(const char* name);
    virtual ~UnitTestRegistry() {}
    static UnitTestRegistry* first() { return s_first; }
    UnitTestRegistry* next() const { return m_next; }
    const char* getName() const { return m_name; }

    virtual UnitTestRun* create() = 0;
protected:
    // it forms a static linked-list using the following internal:
    static UnitTestRegistry* s_first;
    UnitTestRegistry* m_next;
    const char* m_name = nullptr; // expected to be static memory
};

template<typename T>
class UnitTestRegistryEntry
    : public UnitTestRegistry
{
public:
    UnitTestRegistryEntry(const char* name)
        : UnitTestRegistry(name) {}
    virtual UnitTestRun* create()
    {
        static_assert(std::is_base_of<UnitTestRun, T>::value, "You must derive from UnitTestRun if you want to use REGISTER_UNIT_TEST");
        T* created = new T();
        created->SetName(m_name);
        return created;
    }
};

/// Derive from UnitTestRun, then put the following macro in your cpp file:
#define REGISTER_UNIT_TEST(classType)                                          \
    UnitTestRegistryEntry<classType> g_autoRegUnitTest##classType(#classType);

#endif // UNITTESTRUNNER_H
