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
            public UnitTest::LeakDetectionFixture
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
                AZ::ComponentApplication::StartupParameters startupParameters;
                startupParameters.m_loadSettingsRegistry = false;
                m_app->Start(AzFramework::Application::Descriptor(), startupParameters);
                // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
                // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
                // in the unit tests.
                AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

                if (auto fileIoBase = AZ::IO::FileIOBase::GetInstance(); fileIoBase != nullptr)
                {
                    QDir cacheFolder(m_tempDir.GetDirectory());
                    // set the product tree folder to somewhere besides the root temp dir.
                    // This is to avoid error spam - if you try to write to the Cache folder or a subfolder,
                    // AZ::IO will issue an error, since the cache is supposed to be read-only.
                    // here we set it to (tempFolder)/Cache subfolder so that if you want a folder in your test to act like
                    // the read-only cache folder, you can use that folder, but otherwise, all other folders are fair game to
                    // use for your tests without triggering the "you cannot write to the cache" error.
                    fileIoBase->SetAlias("@products@", cacheFolder.absoluteFilePath("Cache").toUtf8().constData());
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

            bool createResult = CreateArchive();

            EXPECT_TRUE(createResult);
        }

        TEST_F(ArchiveComponentTest, ListFilesInArchive_FilesAtThreeDepths_FilesFound)
        {
            CreateArchiveFolder();

            EXPECT_EQ(CreateArchive(), true);

            AZStd::vector<AZStd::string> fileList;
            bool listResult{ false };
            AzToolsFramework::ArchiveCommandsBus::BroadcastResult(listResult,
                &AzToolsFramework::ArchiveCommandsBus::Events::ListFilesInArchive,
                GetArchivePath().toUtf8().constData(), fileList);

            EXPECT_TRUE(listResult);
            EXPECT_EQ(fileList.size(), 6);
        }

        TEST_F(ArchiveComponentTest, CreateDeltaCatalog_AssetsNotRegistered_Failure)
        {
            QStringList fileList = CreateArchiveFileList();

            CreateArchiveFolder(GetArchiveFolderName(), fileList);
            bool createResult = CreateArchive();

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

            std::future<bool> addResult;
            AzToolsFramework::ArchiveCommandsBus::BroadcastResult(
                addResult, &AzToolsFramework::ArchiveCommandsBus::Events::AddFilesToArchive, GetArchivePath().toUtf8().constData(),
                GetArchiveFolder().toUtf8().constData(), listFile.toUtf8().constData());
            bool result = addResult.get();

            EXPECT_TRUE(result);
        }

        TEST_F(ArchiveComponentTest, ExtractArchive_AllFiles_Success)
        {
            CreateArchiveFolder();
            bool createResult = CreateArchive();
            EXPECT_TRUE(createResult);

            std::future<bool> extractResult;
            AzToolsFramework::ArchiveCommandsBus::BroadcastResult(
                extractResult, &AzToolsFramework::ArchiveCommandsBus::Events::ExtractArchive, GetArchivePath().toUtf8().constData(),
                GetExtractFolder().toUtf8().constData());
            bool result = extractResult.get();

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

            bool createResult = CreateArchive();

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

            AZ_TEST_START_TRACE_SUPPRESSION;
            bool catalogCreated{ false };
            AzToolsFramework::AssetBundleCommandsBus::BroadcastResult(catalogCreated, &AzToolsFramework::AssetBundleCommandsBus::Events::CreateDeltaCatalog, GetArchivePath().toUtf8().constData(), true);
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT; // the above raises at least one complaint, but is os specific, since it creates a file in the cache (and then deletes it)

            EXPECT_EQ(catalogCreated, true);
        }


        TEST_F(ArchiveComponentTest, SUITE_periodic_ArchiveAsyncMemoryCorruptionTest)
        {
            // simulate the way the Asset Processor might create many archives asynchronously, overlapping.
            // The general pattern the AP uses is that NCPUs threads are created, and each thread could be creating an archive
            // at the same time.  Each thread is operating on its own temp directory, and calls two APIs:
            // CreateArchive (every time), and then AddFileToArchive (some of the time).
            // There is always a file in the archive, but not always one in the extra API call.
            // to simulate this, we're going to start 8 threads
            // those 8 threads will continuously create files in a folder, then archive them, then add additional files to that archive.

            const int numThreads = 8;
            const int numIterationsPerThread = 100; // takes about 20sec in debug on good HW with ASAN, much faster in profile.

            auto threadFn =
                [this](int threadIndex, int iterations)
            {
                const int numDummyFiles = 5;

                for (int iteration = 0; iteration < iterations; ++iteration)
                {
                    // create a temp folder and then 5 dummy files in that folder to represent the files that will be archived
                    // tempfolder/archive_n_n = folder containing files to archive initially, in the "CreateArchive" API call.
                    // tempfolder/extra_n_n = folder containing files to add to archive afterwards, in the "AddFilesToArchive" API call.
                    // tempfolder/TestArchive_n_n.zip = archive output file.
                    // tempfolder/extra_n_n/filelist.txt = list of files to add to archive in the "AddFilesToArchive" call.
                    // we do not attempt to read the archive back, this is just a thrash test.
                    QString folderName = QString("archive%1_%2").arg(threadIndex).arg(iteration);
                    QString extraFolderName = QString("extra%1_%2").arg(threadIndex).arg(iteration);
                    QString archivePath = QDir(m_tempDir.GetDirectory()).filePath(QString("TestArchive%1_%2.zip").arg(threadIndex).arg(iteration));
                    QString folderPath = QDir(m_tempDir.GetDirectory()).filePath(folderName);
                    QString extraFolderPath = QDir(m_tempDir.GetDirectory()).filePath(extraFolderName);
                    QString dummyFileContent;

                    for (int fileToArchive = 0; fileToArchive < numDummyFiles; ++fileToArchive)
                    {
                        QString filePath = QDir(folderPath).filePath(QString("file%1.txt").arg(fileToArchive));
                        QString extraFileName = QString("extrafile%1.txt").arg(fileToArchive);
                        QString extraFilePath = QDir(extraFolderPath).filePath(extraFileName);
                        CreateDummyFile(filePath, QString(1024 * iteration, QChar('C')));
                        CreateDummyFile(extraFilePath, QString(1024 * iteration, QChar('C')));
                        dummyFileContent.append(QDir::toNativeSeparators(extraFileName));
                        dummyFileContent.append("\n");
                    }
                    QString fileListPath = QDir(extraFolderPath).filePath("filelist.txt");
                    CreateDummyFile(fileListPath, dummyFileContent);

                    std::future<bool> createResult;
                    AzToolsFramework::ArchiveCommandsBus::BroadcastResult(
                        createResult,
                        &AzToolsFramework::ArchiveCommandsBus::Events::CreateArchive,
                        archivePath.toUtf8().constData(),
                        folderPath.toUtf8().constData());
                    EXPECT_TRUE(createResult.valid() ? createResult.get() : false);

                    std::future<bool> addResult;
                    AzToolsFramework::ArchiveCommandsBus::BroadcastResult(
                        addResult,
                        &AzToolsFramework::ArchiveCommandsBus::Events::AddFilesToArchive,
                        archivePath.toUtf8().constData(),
                        extraFolderPath.toUtf8().constData(),
                        fileListPath.toUtf8().constData());

                    EXPECT_TRUE(addResult.valid() ? addResult.get() : false);
                }
            };

            // spawn 8 threads to do the above and then wait for all of them to complete.
            AZStd::vector<AZStd::thread> threads;

            for (int i = 0; i < numThreads; ++i)
            {
                threads.emplace_back(threadFn, i, numIterationsPerThread);
            }

            for (int i = 0; i < numThreads; ++i)
            {
                if (threads[i].joinable())
                {
                    threads[i].join();
                }
            }
        }

    }

}
