/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <SourceControl/PerforceComponent.h>
#include <SourceControl/PerforceConnection.h>
#include <QTemporaryDir>

namespace UnitTest
{
    struct MockPerforceComponent
        : AzToolsFramework::PerforceComponent
    {
        friend struct PerforceComponentFixture;
    };

    struct PerforceComponentFixture
        : LeakDetectionFixture
        , SourceControlTest

    {
        void SetUp() override
        {
            AZ::JobManagerDesc jobDesc;
            AZ::JobManagerThreadDesc threadDesc;
            jobDesc.m_workerThreads.push_back(threadDesc);
            jobDesc.m_workerThreads.push_back(threadDesc);
            jobDesc.m_workerThreads.push_back(threadDesc);
            m_jobManager = aznew AZ::JobManager(jobDesc);
            m_jobContext = aznew AZ::JobContext(*m_jobManager);
            AZ::JobContext::SetGlobalContext(m_jobContext);

            AZ::TickBus::AllowFunctionQueuing(true);

            m_perforceComponent = AZStd::make_unique<MockPerforceComponent>();
            m_perforceComponent->Activate();
            m_perforceComponent->SetConnection(new MockPerforceConnection(m_command));

            EnableSourceControl();
        }

        void TearDown() override
        {
            AZ::TickBus::AllowFunctionQueuing(false);
            AZ::TickBus::ClearQueuedEvents();

            m_perforceComponent->Deactivate();
            m_perforceComponent = nullptr;

            AZ::JobContext::SetGlobalContext(nullptr);
            delete m_jobContext;
            delete m_jobManager;
        }

        AZStd::unique_ptr<MockPerforceComponent> m_perforceComponent;
        AZ::JobManager* m_jobManager = nullptr;
        AZ::JobContext* m_jobContext = nullptr;
    };

    TEST_F(PerforceComponentFixture, TestGetBulkFileInfo_MultipleFiles_Succeeds)
    {
        static constexpr char FileAPath[] = R"(C:\depot\dev\default.font)";
        static constexpr char FileBPath[] = R"(C:\depot\dev\default.xml)";

        m_command.m_fstatResponse =
            R"(... depotFile //depot/dev/default.xml)" "\r\n"
            R"(... clientFile C:\depot\dev\default.xml)" "\r\n"
            R"(... isMapped)" "\r\n"
            R"(... headAction integrate)" "\r\n"
            R"(... headType text)" "\r\n"
            R"(... headTime 1454346715)" "\r\n"
            R"(... headRev 3)" "\r\n"
            R"(... headChange 147109)" "\r\n"
            R"(... headModTime 1452731919)" "\r\n"
            R"(... haveRev 3)" "\r\n"
            "\r\n"
            R"(... depotFile //depot/dev/default.font)" "\r\n"
            R"(... clientFile C:\depot\dev\default.font)" "\r\n"
            R"(... isMapped)" "\r\n"
            R"(... headAction branch)" "\r\n"
            R"(... headType text)" "\r\n"
            R"(... headTime 1479280355)" "\r\n"
            R"(... headRev 1)" "\r\n"
            R"(... headChange 317116)" "\r\n"
            R"(... headModTime 1478804078)" "\r\n"
            R"(... haveRev 1)" "\r\n"
            "\r\n";

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> requestFiles = { FileAPath, FileBPath };

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetBulkFileInfo, requestFiles, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_EQ(fileInfo.size(), 2);

        for (int i = 0; i < 2; ++i)
        {
            ASSERT_EQ(fileInfo[i].m_status, AzToolsFramework::SourceControlStatus::SCS_OpSuccess);
            ASSERT_TRUE(fileInfo[i].IsManaged());
        }
    }

    TEST_F(PerforceComponentFixture, TestGetBulkFileInfo_MissingFile_Succeeds)
    {
        static constexpr char FileAPath[] = R"(C:\depot\dev\does-not-exist.txt)";
        static constexpr char FileBPath[] = R"(C:\depot\dev\does-not-exist-two.txt)";

        m_command.m_fstatErrorResponse =
            R"(C:\depot\dev\does-not-exist.txt - no such file(s).)" "\r\n"
            R"(C:\depot\dev\does-not-exist-two.txt - no such file(s).)" "\r\n"
            "\r\n";

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> requestFiles = { FileAPath, FileBPath };

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetBulkFileInfo, requestFiles, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_EQ(fileInfo.size(), 2);

        for (int i = 0; i < 2; ++i)
        {
            ASSERT_EQ(fileInfo[i].m_status, AzToolsFramework::SourceControlStatus::SCS_OpSuccess);
            ASSERT_EQ(fileInfo[i].m_flags, AzToolsFramework::SourceControlFlags::SCF_Writeable); // Writable should be the only flag
        }
    }

    TEST_F(PerforceComponentFixture, TestGetBulkFileInfo_CompareWithGetFileInfo_ResultMatches)
    {
        static constexpr char FileAPath[] = R"(C:\depot\dev\default.font)";

        static constexpr char FstatResponse[] =
            R"(... depotFile //depot/dev/default.font)" "\r\n"
            R"(... clientFile C:\depot\dev\default.font)" "\r\n"
            R"(... isMapped)" "\r\n"
            R"(... headAction branch)" "\r\n"
            R"(... headType text)" "\r\n"
            R"(... headTime 1479280355)" "\r\n"
            R"(... headRev 1)" "\r\n"
            R"(... headChange 317116)" "\r\n"
            R"(... headModTime 1478804078)" "\r\n"
            R"(... haveRev 1)" "\r\n"
            "\r\n";

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AzToolsFramework::SourceControlFileInfo fileInfoSingle;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto singleCallback = [&callbackSignal, &result, &fileInfoSingle](bool success, AzToolsFramework::SourceControlFileInfo info)
        {
            result = success;
            fileInfoSingle = info;
            callbackSignal.release();
        };

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> requestFiles = { FileAPath };

        m_command.m_fstatResponse = FstatResponse;
        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetBulkFileInfo, requestFiles, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);

        m_command.m_fstatResponse = FstatResponse;
        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetFileInfo, FileAPath, singleCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_FALSE(fileInfo.empty());
        ASSERT_EQ(fileInfoSingle.m_flags, fileInfo[0].m_flags);
    }

    TEST_F(PerforceComponentFixture, Test_ExecuteEditBulk)
    {
        static constexpr char FileAPath[] = R"(C:\depot\dev\does-not-exist.txt)";
        static constexpr char FileBPath[] = R"(C:\depot\dev\default.font)";

        m_command.m_fstatErrorResponse =
            R"(C:\depot\dev\does-not-exist.txt - no such file(s).)" "\r\n"
            "\r\n";

        m_command.m_fstatResponse = 
            R"(... depotFile //depot/dev/default.font)" "\r\n"
            R"(... clientFile C:\depot\dev\default.font)" "\r\n"
            R"(... isMapped)" "\r\n"
            R"(... headAction branch)" "\r\n"
            R"(... headType text)" "\r\n"
            R"(... headTime 1479280355)" "\r\n"
            R"(... headRev 1)" "\r\n"
            R"(... headChange 317116)" "\r\n"
            R"(... headModTime 1478804078)" "\r\n"
            R"(... otherOpen)" "\r\n"
            R"(... haveRev 1)" "\r\n"
            "\r\n";

        bool addCalled = false;
        bool editCalled = false;

        m_command.m_addCallback = [&addCalled]([[maybe_unused]] const AZStd::string& args)
        {
            addCalled = true;
        };

        m_command.m_editCallback = [this, &editCalled]([[maybe_unused]] const AZStd::string& args)
        {
            editCalled = true;

            m_command.m_fstatResponse = 
                R"(... depotFile //depot/dev/does-not-exist.txt)" "\r\n"
                R"(... clientFile C:\depot\dev\does-not-exist.txt)" "\r\n"
                R"(... isMapped)" "\r\n"
                R"(... action add)" "\r\n"
                R"(... change default)" "\r\n"
                R"(... type text)" "\r\n"
                R"(... actionOwner unittest)" "\r\n"
                R"(... workRev 1)" "\r\n"
                "\r\n"
                R"(... depotFile //depot/dev/default.font)" "\r\n"
                R"(... clientFile C:\depot\dev\default.font)" "\r\n"
                R"(... isMapped)" "\r\n"
                R"(... headAction add)" "\r\n"
                R"(... headType text)" "\r\n"
                R"(... headTime 1557439413)" "\r\n"
                R"(... headRev 1)" "\r\n"
                R"(... headChange 902209)" "\r\n"
                R"(... headModTime 1556296348)" "\r\n"
                R"(... haveRev 1)" "\r\n"
                R"(... action edit)" "\r\n"
                R"(... change default)" "\r\n"
                R"(... type text)" "\r\n"
                R"(... actionOwner unittest)" "\r\n"
                R"(... workRev 1)" "\r\n"
                "\r\n";
        };

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> requestFiles = { FileAPath, FileBPath };

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEditBulk, requestFiles, true, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_TRUE(addCalled);
        ASSERT_TRUE(editCalled);
        ASSERT_EQ(fileInfo.size(), 2);

        for (int i = 0; i < 2; ++i)
        {
            ASSERT_EQ(fileInfo[i].m_status, AzToolsFramework::SourceControlStatus::SCS_OpSuccess);
        }
    }

    TEST_F(PerforceComponentFixture, Test_ExecuteEditBulk_CheckedOutByOther_Failure)
    {
        static constexpr char FileBPath[] = R"(C:\depot\dev\default.font)";

        m_command.m_fstatResponse =
            R"(... depotFile //depot/dev/default.font)" "\r\n"
            R"(... clientFile C:\depot\dev\default.font)" "\r\n"
            R"(... isMapped)" "\r\n"
            R"(... headAction branch)" "\r\n"
            R"(... headType text)" "\r\n"
            R"(... headTime 1479280355)" "\r\n"
            R"(... headRev 1)" "\r\n"
            R"(... headChange 317116)" "\r\n"
            R"(... headModTime 1478804078)" "\r\n"
            R"(... otherOpen)" "\r\n"
            R"(... haveRev 1)" "\r\n"
            "\r\n";

        bool addCalled = false;
        bool editCalled = false;

        m_command.m_addCallback = [&addCalled]([[maybe_unused]] const AZStd::string& args)
        {
            addCalled = true;
        };

        m_command.m_editCallback = [&editCalled]([[maybe_unused]] const AZStd::string& args)
        {
            editCalled = true;
        };

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> requestFiles = { FileBPath };

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEditBulk, requestFiles, false, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_FALSE(result);
        ASSERT_FALSE(addCalled);
        ASSERT_FALSE(editCalled);
    }

    bool CreateDummyFile(const QString& fullPathToFile, QString contents = "")
    {
        QFileInfo fi(fullPathToFile);
        QDir fp(fi.path());
        fp.mkpath(".");
        QFile writer(fullPathToFile);
        if (!writer.open(QFile::WriteOnly))
        {
            return false;
        }

        {
            QTextStream ts(&writer);
            ts.setCodec("UTF-8");
            ts << contents;
        }
        return true;
    }

    TEST_F(PerforceComponentFixture, Test_ExecuteEditBulk_Local_Succeeds)
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, false);

        QTemporaryDir tempDir;
        AZStd::string fullPathA = tempDir.filePath("fileA.txt").toUtf8().constData();
        AZStd::string fullPathB = tempDir.filePath("fileB.txt").toUtf8().constData();
        AZStd::vector<const char*> testPaths = { fullPathA.c_str(), fullPathB.c_str() };

        for (const char* path : testPaths)
        {
            ASSERT_TRUE(CreateDummyFile(path));
            ASSERT_TRUE(AZ::IO::SystemFile::Exists(path));

            AZ::IO::SystemFile::SetWritable(path, false);

            ASSERT_FALSE(AZ::IO::SystemFile::IsWritable(path));
        }

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> requestFiles = { testPaths.begin(), testPaths.end() };

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestEditBulk, requestFiles, false, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_EQ(fileInfo.size(), testPaths.size());

        for (int i = 0; i < testPaths.size(); ++i)
        {
            ASSERT_EQ(fileInfo[i].m_status, AzToolsFramework::SourceControlStatus::SCS_OpSuccess);
            ASSERT_TRUE(fileInfo[i].HasFlag(AzToolsFramework::SourceControlFlags::SCF_Writeable));
            ASSERT_TRUE(AZ::IO::SystemFile::IsWritable(testPaths[i]));
        }
    }

    TEST_F(PerforceComponentFixture, Test_ExecuteRenameBulk_Local_Succeeds)
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, false);

        QTemporaryDir tempDir;
        AZStd::string fullPathA = tempDir.filePath("one/two/three/fileA.txt").toUtf8().constData();
        AZStd::string fullPathB = tempDir.filePath("one/two/three/fileB.txt").toUtf8().constData();
        AZStd::vector<const char*> testPaths = { fullPathA.c_str(), fullPathB.c_str() };

        for (const char* path : testPaths)
        {
            ASSERT_TRUE(CreateDummyFile(path));
            ASSERT_TRUE(AZ::IO::SystemFile::Exists(path));
        }

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        auto from = tempDir.filePath("o*e/*o/three/file*.txt");
        auto to = tempDir.filePath("o*e/*o/three/fileRenamed*.png");

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestRenameBulk, from.toUtf8().constData(), to.toUtf8().constData(), bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_EQ(fileInfo.size(), testPaths.size());

        ASSERT_FALSE(AZ::IO::SystemFile::Exists(fullPathA.c_str()));
        ASSERT_FALSE(AZ::IO::SystemFile::Exists(fullPathB.c_str()));

        ASSERT_TRUE(AZ::IO::SystemFile::Exists(tempDir.filePath("one/two/three/fileRenamedA.png").toUtf8().constData()));
        ASSERT_TRUE(AZ::IO::SystemFile::Exists(tempDir.filePath("one/two/three/fileRenamedB.png").toUtf8().constData()));

        for (int i = 0; i < testPaths.size(); ++i)
        {
            ASSERT_EQ(fileInfo[i].m_status, AzToolsFramework::SourceControlStatus::SCS_OpSuccess);
            ASSERT_TRUE(fileInfo[i].HasFlag(AzToolsFramework::SourceControlFlags::SCF_Tracked));
        }
    }

    TEST_F(PerforceComponentFixture, Test_ExecuteRenameBulk_Local_MismatchedWildcards_Fails)
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, false);

        QTemporaryDir tempDir;
        AZStd::string fullPathA = tempDir.filePath("one/two/three/fileA.txt").toUtf8().constData();
        AZStd::string fullPathB = tempDir.filePath("one/two/three/fileB.txt").toUtf8().constData();
        AZStd::vector<const char*> testPaths = { fullPathA.c_str(), fullPathB.c_str() };

        for (const char* path : testPaths)
        {
            ASSERT_TRUE(CreateDummyFile(path));
            ASSERT_TRUE(AZ::IO::SystemFile::Exists(path));
        }

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        auto from = tempDir.filePath("o*e/*o/three/file*.txt");
        auto to = tempDir.filePath("o*e/two/three/fileRenamed*.png");

        AZ_TEST_START_TRACE_SUPPRESSION;
        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestRenameBulk, from.toUtf8().constData(), to.toUtf8().constData(), bulkCallback);

        WaitForSourceControl(callbackSignal);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        ASSERT_FALSE(result);
        ASSERT_EQ(fileInfo.size(), 0);

        ASSERT_TRUE(AZ::IO::SystemFile::Exists(fullPathA.c_str()));
        ASSERT_TRUE(AZ::IO::SystemFile::Exists(fullPathB.c_str()));

        ASSERT_FALSE(AZ::IO::SystemFile::Exists(tempDir.filePath("one/two/three/fileRenamedA.png").toUtf8().constData()));
        ASSERT_FALSE(AZ::IO::SystemFile::Exists(tempDir.filePath("one/two/three/fileRenamedB.png").toUtf8().constData()));
    }

    TEST_F(PerforceComponentFixture, Test_ExecuteDeleteBulk_Local_Succeeds)
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, false);

        QTemporaryDir tempDir;
        AZStd::string fullPathA = tempDir.filePath("one/two/three/fileA.txt").toUtf8().constData();
        AZStd::string fullPathB = tempDir.filePath("one/two/three/fileB.txt").toUtf8().constData();
        AZStd::vector<const char*> testPaths = { fullPathA.c_str(), fullPathB.c_str() };

        for (const char* path : testPaths)
        {
            ASSERT_TRUE(CreateDummyFile(path));
            ASSERT_TRUE(AZ::IO::SystemFile::Exists(path));
        }

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        auto from = tempDir.filePath("o*e/*o/three/file*.txt");

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::RequestDeleteBulk, from.toUtf8().constData(), bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_EQ(fileInfo.size(), testPaths.size());

        ASSERT_FALSE(AZ::IO::SystemFile::Exists(fullPathA.c_str()));
        ASSERT_FALSE(AZ::IO::SystemFile::Exists(fullPathB.c_str()));

        for (int i = 0; i < testPaths.size(); ++i)
        {
            ASSERT_EQ(fileInfo[i].m_status, AzToolsFramework::SourceControlStatus::SCS_OpSuccess);
            ASSERT_FALSE(fileInfo[i].HasFlag(AzToolsFramework::SourceControlFlags::SCF_Tracked));
        }
    }

    TEST_F(PerforceComponentFixture, Test_GetFiles_Succeeds)
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, false);

        QTemporaryDir tempDir;
        AZStd::string fullPathA = tempDir.filePath("one/two/three/fileA.txt").toUtf8().constData();
        AZStd::string fullPathB = tempDir.filePath("one/two/three/fileB.txt").toUtf8().constData();
        AZStd::vector<const char*> testPaths = { fullPathA.c_str(), fullPathB.c_str() };

        for (const char* path : testPaths)
        {
            ASSERT_TRUE(CreateDummyFile(path));
            ASSERT_TRUE(AZ::IO::SystemFile::Exists(path));
        }

        auto result = AzToolsFramework::LocalFileSCComponent::GetFiles(tempDir.filePath("one/tw*/fileA.txt").toUtf8().constData());

        ASSERT_EQ(result.size(), 0);

        result = AzToolsFramework::LocalFileSCComponent::GetFiles(tempDir.filePath("on...").toUtf8().constData());

        ASSERT_EQ(result.size(), 2);
    }

    TEST_F(PerforceComponentFixture, Test_GetFiles_StarWildcardAtEnd_OnlyReturnsFirstFile)
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, false);

        QTemporaryDir tempDir;
        AZStd::string fullPathA = tempDir.filePath("one/file1.txt").toUtf8().constData();
        AZStd::string fullPathB = tempDir.filePath("one/folder/file1.txt").toUtf8().constData();
        AZStd::vector<const char*> testPaths = { fullPathA.c_str(), fullPathB.c_str() };

        for (const char* path : testPaths)
        {
            ASSERT_TRUE(CreateDummyFile(path));
            ASSERT_TRUE(AZ::IO::SystemFile::Exists(path));
        }

        auto result = AzToolsFramework::LocalFileSCComponent::GetFiles(tempDir.filePath("one/f*").toUtf8().constData());

        ASSERT_EQ(result.size(), 1);
    }

    TEST_F(PerforceComponentFixture, Test_GetFiles_MultipleWildcardsAndWildcardAtEnd_Succeeds)
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, false);

        QTemporaryDir tempDir;
        AZStd::string fullPathA = tempDir.filePath("one/two/three/fileA.txt").toUtf8().constData();
        AZStd::string fullPathB = tempDir.filePath("one/two/three/fileB.txt").toUtf8().constData();
        AZStd::vector<const char*> testPaths = { fullPathA.c_str(), fullPathB.c_str() };

        for (const char* path : testPaths)
        {
            ASSERT_TRUE(CreateDummyFile(path));
            ASSERT_TRUE(AZ::IO::SystemFile::Exists(path));
        }

        auto result = AzToolsFramework::LocalFileSCComponent::GetFiles(tempDir.filePath("o*e/tw*/...").toUtf8().constData());

        ASSERT_EQ(result.size(), 2);
    }

    TEST_F(PerforceComponentFixture, Test_GetBulkFileInfo_Wildcard_Succeeds)
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, false);

        QTemporaryDir tempDir;
        AZStd::string fullPathA = tempDir.filePath("one/two/three/fileA.txt").toUtf8().constData();
        AZStd::string fullPathB = tempDir.filePath("one/two/three/fileB.txt").toUtf8().constData();
        AZStd::vector<const char*> testPaths = { fullPathA.c_str(), fullPathB.c_str() };

        for (const char* path : testPaths)
        {
            ASSERT_TRUE(CreateDummyFile(path));
        }

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> paths = { tempDir.filePath("o*e/*o/three/file*.txt").toUtf8().constData() };

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetBulkFileInfo, paths, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_EQ(fileInfo.size(), testPaths.size());

        using namespace AzToolsFramework;

        for (int i = 0; i < testPaths.size(); ++i)
        {
            ASSERT_EQ(fileInfo[i].m_status, SCS_OpSuccess);
            ASSERT_EQ(fileInfo[i].m_flags, SCF_Writeable | SCF_OpenByUser | SCF_Tracked);
        }
    }

    TEST_F(PerforceComponentFixture, Test_GetBulkFileInfo_MultipleFiles_Succeeds)
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(&AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, false);

        QTemporaryDir tempDir;
        AZStd::string fullPathA = tempDir.filePath("one/two/three/fileA.txt").toUtf8().constData();
        AZStd::string fullPathB = tempDir.filePath("one/two/three/fileB.txt").toUtf8().constData();
        AZStd::vector<const char*> testPaths = { fullPathA.c_str(), fullPathB.c_str() };

        for (const char* path : testPaths)
        {
            ASSERT_TRUE(CreateDummyFile(path));
        }

        AZStd::binary_semaphore callbackSignal;

        bool result = false;
        AZStd::vector<AzToolsFramework::SourceControlFileInfo> fileInfo;

        auto bulkCallback = [&callbackSignal, &result, &fileInfo](bool success, AZStd::vector<AzToolsFramework::SourceControlFileInfo> info)
        {
            result = success;
            fileInfo = info;
            callbackSignal.release();
        };

        AZStd::unordered_set<AZStd::string> paths = { fullPathA, fullPathB };

        AzToolsFramework::SourceControlCommandBus::Broadcast(&AzToolsFramework::SourceControlCommandBus::Events::GetBulkFileInfo, paths, bulkCallback);

        WaitForSourceControl(callbackSignal);

        ASSERT_TRUE(result);
        ASSERT_EQ(fileInfo.size(), testPaths.size());

        using namespace AzToolsFramework;

        for (int i = 0; i < testPaths.size(); ++i)
        {
            ASSERT_EQ(fileInfo[i].m_status, SCS_OpSuccess);
            ASSERT_EQ(fileInfo[i].m_flags, SCF_Writeable | SCF_OpenByUser | SCF_Tracked);
        }
    }
}
