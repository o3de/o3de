/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <Tests/AZTestShared/Utils/Utils.h>
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

        class ArchiveTest :
            public ::testing::Test
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
                return "Archive";
            }

            void CreateArchiveFolder( QString archiveFolderName, QStringList fileList )
            {
                QDir tempPath = QDir(m_tempDir.path()).filePath(archiveFolderName);

                for (const auto& thisFile : fileList)
                {
                    QString absoluteTestFilePath = tempPath.absoluteFilePath(thisFile);
                    EXPECT_TRUE(CreateDummyFile(absoluteTestFilePath));
                }
            }

            void CreateArchiveFolder()
            {
                CreateArchiveFolder(GetArchiveFolderName(), CreateArchiveFileList());
            }

            QString GetArchivePath()
            {
                return  QDir(m_tempDir.path()).filePath("TestArchive.pak");
            }

            QString GetArchiveFolder()
            {
                return QDir(m_tempDir.path()).filePath(GetArchiveFolderName());
            }

            bool CreateArchive()
            {
                bool createResult{ false };
                AzToolsFramework::ArchiveCommandsBus::BroadcastResult(createResult, &AzToolsFramework::ArchiveCommandsBus::Events::CreateArchiveBlocking, GetArchivePath().toStdString().c_str(), GetArchiveFolder().toStdString().c_str());
                return createResult;
            }

            void SetUp() override
            {
                m_app.reset(aznew ToolsTestApplication("ArchiveTest"));
                m_app->Start(AzFramework::Application::Descriptor());
                // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
                // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
                // in the unit tests.
                AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
            }

            void TearDown() override
            {
                m_app->Stop();
                m_app.reset();
            }

            AZStd::unique_ptr<ToolsTestApplication> m_app;
            QTemporaryDir m_tempDir {QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("ArchiveTests-")};
        };

#if AZ_TRAIT_DISABLE_FAILED_ARCHIVE_TESTS
        TEST_F(ArchiveTest, DISABLED_CreateArchiveBlocking_FilesAtThreeDepths_ArchiveCreated)
#else
        TEST_F(ArchiveTest, CreateArchiveBlocking_FilesAtThreeDepths_ArchiveCreated)
#endif // AZ_TRAIT_DISABLE_FAILED_ARCHIVE_TESTS
        {
            EXPECT_TRUE(m_tempDir.isValid());
            CreateArchiveFolder();

            bool createResult = CreateArchive();

            EXPECT_EQ(createResult, true);
        }

#if AZ_TRAIT_DISABLE_FAILED_ARCHIVE_TESTS
        TEST_F(ArchiveTest, DISABLED_ListFilesInArchiveBlocking_FilesAtThreeDepths_FilesFound)
#else
        TEST_F(ArchiveTest, ListFilesInArchiveBlocking_FilesAtThreeDepths_FilesFound)
#endif // AZ_TRAIT_DISABLE_FAILED_ARCHIVE_TESTS
        {
            EXPECT_TRUE(m_tempDir.isValid());
            CreateArchiveFolder();
            
            EXPECT_EQ(CreateArchive(), true);

            AZStd::vector<AZStd::string> fileList;
            bool listResult{ false };
            AzToolsFramework::ArchiveCommandsBus::BroadcastResult(listResult, &AzToolsFramework::ArchiveCommandsBus::Events::ListFilesInArchiveBlocking, GetArchivePath().toStdString().c_str(), fileList);

            EXPECT_EQ(fileList.size(), 6);
        }

#if AZ_TRAIT_DISABLE_FAILED_ARCHIVE_TESTS
        TEST_F(ArchiveTest, DISABLED_CreateDeltaCatalog_AssetsNotRegistered_Failure)
#else
        TEST_F(ArchiveTest, CreateDeltaCatalog_AssetsNotRegistered_Failure)
#endif // AZ_TRAIT_DISABLE_FAILED_ARCHIVE_TESTS
        {
            QStringList fileList = CreateArchiveFileList();

            CreateArchiveFolder(GetArchiveFolderName(), fileList);

            bool createResult = CreateArchive();

            EXPECT_EQ(createResult, true);

            bool catalogCreated{ true };
            AZ::Test::AssertAbsorber assertAbsorber;
            AzToolsFramework::AssetBundleCommandsBus::BroadcastResult(catalogCreated, &AzToolsFramework::AssetBundleCommandsBus::Events::CreateDeltaCatalog, GetArchivePath().toStdString().c_str(), true);

            EXPECT_EQ(catalogCreated, false);
        }

#if AZ_TRAIT_DISABLE_FAILED_ARCHIVE_TESTS
        TEST_F(ArchiveTest, DISABLED_CreateDeltaCatalog_ArchiveWithoutCatalogAssetsRegistered_Success)
#else
        TEST_F(ArchiveTest, CreateDeltaCatalog_ArchiveWithoutCatalogAssetsRegistered_Success)
#endif // AZ_TRAIT_DISABLE_FAILED_ARCHIVE_TESTS
        {
            QStringList fileList = CreateArchiveFileList();

            CreateArchiveFolder(GetArchiveFolderName(), fileList);

            bool createResult = CreateArchive();

            EXPECT_EQ(createResult, true);

            for (const auto& thisPath : fileList)
            {
                AZ::Data::AssetInfo newInfo;
                newInfo.m_relativePath = thisPath.toStdString().c_str();
                newInfo.m_assetType = AZ::Uuid::CreateRandom();
                newInfo.m_sizeBytes = 100; // Arbitrary
                AZ::Data::AssetId generatedID(AZ::Uuid::CreateRandom());
                newInfo.m_assetId = generatedID;

                AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, generatedID, newInfo);
            }

            bool catalogCreated{ false };
            AzToolsFramework::AssetBundleCommandsBus::BroadcastResult(catalogCreated, &AzToolsFramework::AssetBundleCommandsBus::Events::CreateDeltaCatalog, GetArchivePath().toStdString().c_str(), true);
            EXPECT_EQ(catalogCreated, true);
        }
    }

}
