/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/IO/FileIO.h>
#include <AZTestShared/Utils/Utils.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <AzToolsFramework/AssetBundle/AssetBundleAPI.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextStream>

namespace UnitTest
{
    namespace
    {
        bool CreateDummyFile(const QString& fullPathToFile, const QString& tempStr = {})
        {
            QFileInfo fi(fullPathToFile);
            QDir fp(fi.path());
            fp.mkpath(".");
            QFile writer(fullPathToFile);
            if (!writer.open(QFile::ReadWrite))
            {
                return false;
            }
            {
                QTextStream stream(&writer);
                stream << tempStr << Qt::endl;
            }
            writer.close();
            return true;
        }

        class ArchiveComponentTest :
            public UnitTest::AllocatorsTestFixture
        {

        public:
            QStringList CreateArchiveFileList()
            {
                QStringList returnList;
                returnList.append("basicfile.txt");
                returnList.append("basicfile2.txt");
                returnList.append("testfolder/folderfile.txt");
                returnList.append("testfolder2/sharedfolderfile.txt");
                returnList.append("testfolder2/sharedfolderfile2.txt");
                returnList.append("testfolder3/testfolder4/depthfile.bat");

                return returnList;
            }

            QString GetArchiveFolderName()
            {
                return "archive";
            }

            QString GetExtractFolderName()
            {
                return "extracted";
            }

            void CreateArchiveFolder(QString archiveFolderName, QStringList fileList)
            {
                QDir tempPath = QDir(m_tempDir.GetDirectory()).filePath(archiveFolderName);

                for (const auto& thisFile : fileList)
                {
                    QString absoluteTestFilePath = tempPath.absoluteFilePath(thisFile);
                    EXPECT_TRUE(CreateDummyFile(absoluteTestFilePath));
                }
            }

            QString CreateArchiveListTextFile()
            {
                QString listFilePath = QDir(m_tempDir.GetDirectory()).absoluteFilePath("filelist.txt");
                QString textContent = CreateArchiveFileList().join("\n");
                EXPECT_TRUE(CreateDummyFile(listFilePath, textContent));
                return listFilePath;
            }

            void CreateArchiveFolder()
            {
                CreateArchiveFolder(GetArchiveFolderName(), CreateArchiveFileList());
            }

            QString GetArchivePath()
            {
                return  QDir(m_tempDir.GetDirectory()).filePath("TestArchive.pak");
            }

            QString GetArchiveFolder()
            {
                return QDir(m_tempDir.GetDirectory()).filePath(GetArchiveFolderName());
            }

            QString GetExtractFolder()
            {
                return QDir(m_tempDir.GetDirectory()).filePath(GetExtractFolderName());
            }

            bool CreateArchive()
            {
                std::future<bool> createResult;
                AzToolsFramework::ArchiveCommandsBus::BroadcastResult(createResult,
                    &AzToolsFramework::ArchiveCommandsBus::Events::CreateArchive,
                    GetArchivePath().toUtf8().constData(), GetArchiveFolder().toUtf8().constData());
                bool result = createResult.get();
                return result;
            }

            void SetUp() override
            {
                m_app.reset(aznew ToolsTestApplication("ArchiveComponentTest"));
                m_app->Start(AzFramework::Application::Descriptor());
                // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
                // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
                // in the unit tests.
                AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

                if (auto fileIoBase = AZ::IO::FileIOBase::GetInstance(); fileIoBase != nullptr)
                {
                    fileIoBase->SetAlias("@products@", m_tempDir.GetDirectory());
                }
            }

            void TearDown() override
            {
                m_app->Stop();
                m_app.reset();
            }

            AZStd::unique_ptr<ToolsTestApplication> m_app;
            AZ::Test::ScopedAutoTempDirectory m_tempDir;
        };

        TEST_F(ArchiveComponentTest, CreateArchive_FilesAtThreeDepths_ArchiveCreated)
        {
            CreateArchiveFolder();

            AZ_TEST_START_TRACE_SUPPRESSION;
            bool createResult = CreateArchive();
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

            EXPECT_TRUE(createResult);
        }

        TEST_F(ArchiveComponentTest, ListFilesInArchive_FilesAtThreeDepths_FilesFound)
        {
            CreateArchiveFolder();

            AZ_TEST_START_TRACE_SUPPRESSION;
            EXPECT_EQ(CreateArchive(), true);

            AZStd::vector<AZStd::string> fileList;
            bool listResult{ false };
            AzToolsFramework::ArchiveCommandsBus::BroadcastResult(listResult,
                &AzToolsFramework::ArchiveCommandsBus::Events::ListFilesInArchive,
                GetArchivePath().toUtf8().constData(), fileList);
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

            EXPECT_TRUE(listResult);
            EXPECT_EQ(fileList.size(), 6);
        }

        TEST_F(ArchiveComponentTest, CreateDeltaCatalog_AssetsNotRegistered_Failure)
        {
            QStringList fileList = CreateArchiveFileList();

            CreateArchiveFolder(GetArchiveFolderName(), fileList);
            AZ_TEST_START_TRACE_SUPPRESSION;
            bool createResult = CreateArchive();
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

            EXPECT_EQ(createResult, true);

            bool catalogCreated{ true };
            AZ::Test::AssertAbsorber assertAbsorber;
            AzToolsFramework::AssetBundleCommandsBus::BroadcastResult(catalogCreated,
                &AzToolsFramework::AssetBundleCommandsBus::Events::CreateDeltaCatalog, GetArchivePath().toUtf8().constData(), true);

            EXPECT_EQ(catalogCreated, false);
        }

        TEST_F(ArchiveComponentTest, AddFilesToArchive_FromListFile_Success)
        {
            QString listFile = CreateArchiveListTextFile();
            CreateArchiveFolder(GetArchiveFolderName(), CreateArchiveFileList());

            AZ_TEST_START_TRACE_SUPPRESSION;
            std::future<bool> addResult;
            AzToolsFramework::ArchiveCommandsBus::BroadcastResult(
                addResult, &AzToolsFramework::ArchiveCommandsBus::Events::AddFilesToArchive, GetArchivePath().toUtf8().constData(),
                GetArchiveFolder().toUtf8().constData(), listFile.toUtf8().constData());
            bool result = addResult.get();
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

            EXPECT_TRUE(result);
        }

        TEST_F(ArchiveComponentTest, ExtractArchive_AllFiles_Success)
        {
            CreateArchiveFolder();
            AZ_TEST_START_TRACE_SUPPRESSION;
            bool createResult = CreateArchive();
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
            EXPECT_TRUE(createResult);

            AZ_TEST_START_TRACE_SUPPRESSION;
            std::future<bool> extractResult;
            AzToolsFramework::ArchiveCommandsBus::BroadcastResult(
                extractResult, &AzToolsFramework::ArchiveCommandsBus::Events::ExtractArchive, GetArchivePath().toUtf8().constData(),
                GetExtractFolder().toUtf8().constData());
            bool result = extractResult.get();
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

            EXPECT_TRUE(result);

            QStringList archiveFiles = CreateArchiveFileList();
            for (const auto& file : archiveFiles)
            {
                QString fullFilePath = QDir(GetExtractFolder()).absoluteFilePath(file);
                QFileInfo fi(fullFilePath);
                EXPECT_TRUE(fi.exists());
            }
        }

        TEST_F(ArchiveComponentTest, CreateDeltaCatalog_ArchiveWithoutCatalogAssetsRegistered_Success)
        {
            QStringList fileList = CreateArchiveFileList();

            CreateArchiveFolder(GetArchiveFolderName(), fileList);

            AZ_TEST_START_TRACE_SUPPRESSION;
            bool createResult = CreateArchive();
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;

            EXPECT_EQ(createResult, true);

            for (const auto& thisPath : fileList)
            {
                AZ::Data::AssetInfo newInfo;
                newInfo.m_relativePath = thisPath.toUtf8().constData();
                newInfo.m_assetType = AZ::Uuid::CreateRandom();
                newInfo.m_sizeBytes = 100; // Arbitrary
                AZ::Data::AssetId generatedID(AZ::Uuid::CreateRandom());
                newInfo.m_assetId = generatedID;

                AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, generatedID, newInfo);
            }

            bool catalogCreated{ false };
            AZ_TEST_START_TRACE_SUPPRESSION;
            AzToolsFramework::AssetBundleCommandsBus::BroadcastResult(catalogCreated, &AzToolsFramework::AssetBundleCommandsBus::Events::CreateDeltaCatalog, GetArchivePath().toUtf8().constData(), true);
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT; // produces different counts in different platforms
            EXPECT_EQ(catalogCreated, true);
        }
    }

}
