/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetManager/SourceFileRelocator.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobManagerDesc.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Utils/Utils.h>
#include "AssetProcessorTest.h"
#include "AzToolsFramework/API/AssetDatabaseBus.h"
#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/SourceControl/PerforceConnection.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <utility>

#include <AzTest/AzTest.h>

namespace UnitTests
{
    //using namespace testing;
    using testing::NiceMock;
    using namespace AssetProcessor;
    using namespace AzToolsFramework::AssetDatabase;

    struct MockPerforceComponent
        : AzToolsFramework::PerforceComponent
    {
        friend class SourceFileRelocatorTest;
    };

    class SourceFileRelocatorTestsMockDatabaseLocationListener : public AssetDatabaseRequests::Bus::Handler
    {
    public:
        MOCK_METHOD1(GetAssetDatabaseLocation, bool(AZStd::string&));
    };


    class SourceFileRelocatorTest
        : public ::UnitTest::LeakDetectionFixture
        , public ::UnitTest::SourceControlTest
    {
    public:
        void SetUp() override
        {
            AZ::TickBus::AllowFunctionQueuing(true);

            m_data.reset(new StaticData());

            QDir tempPath(m_tempDir.path());

            m_data->m_connection.reset(new AssetProcessor::AssetDatabaseConnection());
            m_data->m_databaseLocation = tempPath.absoluteFilePath("test_database.sqlite").toUtf8().constData();
            m_data->m_databaseLocationListener.BusConnect();

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

            {
                using namespace testing;

                ON_CALL(m_data->m_databaseLocationListener, GetAssetDatabaseLocation(_))
                    .WillByDefault(
                        DoAll( // set the 0th argument ref (string) to the database location and return true.
                            SetArgReferee<0>(m_data->m_databaseLocation),
                            Return(true)));
            }
            // Initialize the database:
            m_data->m_connection->ClearData(); // this is expected to reset/clear/reopen

            m_data->m_platformConfig.EnablePlatform(AssetBuilderSDK::PlatformInfo("pc", { "desktop" }));

            m_data->m_platformConfig.AddMetaDataType("metadataextension", "metadatatype");
            m_data->m_platformConfig.AddMetaDataType("bar", "foo");
            m_data->m_platformConfig.ReadMetaDataFromSettingsRegistry();

            AZStd::vector<AssetBuilderSDK::PlatformInfo> platforms;
            m_data->m_platformConfig.PopulatePlatformsForScanFolder(platforms);

            m_data->m_scanFolder1 = { tempPath.absoluteFilePath("dev").toUtf8().constData(), "dev", "devKey"};
            m_data->m_scanFolder2 = { tempPath.absoluteFilePath("folder").toUtf8().constData(), "folder", "folderKey"};

            ASSERT_TRUE(m_data->m_connection->SetScanFolder(m_data->m_scanFolder1));
            ASSERT_TRUE(m_data->m_connection->SetScanFolder(m_data->m_scanFolder2));

            ScanFolderInfo scanFolder1(m_data->m_scanFolder1.m_scanFolder.c_str(),
                m_data->m_scanFolder1.m_displayName.c_str(),
                m_data->m_scanFolder1.m_portableKey.c_str(),
                false, true, platforms, 0, m_data->m_scanFolder1.m_scanFolderID);
            m_data->m_platformConfig.AddScanFolder(scanFolder1);

            ScanFolderInfo scanFolder2(m_data->m_scanFolder2.m_scanFolder.c_str(),
                m_data->m_scanFolder2.m_displayName.c_str(),
                m_data->m_scanFolder2.m_portableKey.c_str(),
                false, true, platforms, 0, m_data->m_scanFolder2.m_scanFolderID);
            m_data->m_platformConfig.AddScanFolder(scanFolder2);

            SourceDatabaseEntry sourceFile1{ m_data->m_scanFolder1.m_scanFolderID, "subfolder1/somefile.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile2{ m_data->m_scanFolder1.m_scanFolderID, "subfolder1/otherfile.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile3{ m_data->m_scanFolder2.m_scanFolderID, "otherfile.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile4{ m_data->m_scanFolder2.m_scanFolderID, "a/b/c/d/otherfile.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile5{ m_data->m_scanFolder1.m_scanFolderID, "duplicate/file1.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile6{ m_data->m_scanFolder2.m_scanFolderID, "duplicate/file1.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile7{ m_data->m_scanFolder1.m_scanFolderID, "subfolder2/file.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile8{ m_data->m_scanFolder1.m_scanFolderID, "test.txt", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile9{ m_data->m_scanFolder1.m_scanFolderID, "duplicate/folder/file1.tif", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            SourceDatabaseEntry sourceFile10{ m_data->m_scanFolder1.m_scanFolderID, "folder/file.foo", AZ::Uuid::CreateRandom(), "AnalysisFingerprint" };
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile1));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile2));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile3));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile4));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile5));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile6));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile7));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile8));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile9));
            ASSERT_TRUE(m_data->m_connection->SetSource(sourceFile10));

            SourceFileDependencyEntry dependency1{ AZ::Uuid::CreateRandom(), m_data->m_dependency1Uuid, PathOrUuid("subfolder1/otherfile.tif"), SourceFileDependencyEntry::TypeOfDependency::DEP_SourceToSource, false, "" };
            SourceFileDependencyEntry dependency2{ AZ::Uuid::CreateRandom(), m_data->m_dependency2Uuid, PathOrUuid("otherfile.tif"), SourceFileDependencyEntry::TypeOfDependency::DEP_JobToJob, false, "" };
            ASSERT_TRUE(m_data->m_connection->SetSourceFileDependency(dependency1));
            ASSERT_TRUE(m_data->m_connection->SetSourceFileDependency(dependency2));

            JobDatabaseEntry job1 = { sourceFile1.m_sourceID, "JobKey", 12345, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed,
            1111 };
            JobDatabaseEntry job2 = { sourceFile2.m_sourceID, "JobKey", 2222, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed,
            1111 };
            JobDatabaseEntry job3 = { sourceFile3.m_sourceID, "JobKey", 4444, "pc", AZ::Uuid::CreateRandom(), AzToolsFramework::AssetSystem::JobStatus::Completed,
            1111 };
            ASSERT_TRUE(m_data->m_connection->SetJob(job1));
            ASSERT_TRUE(m_data->m_connection->SetJob(job2));
            ASSERT_TRUE(m_data->m_connection->SetJob(job3));

            const AZ::u32 productSubId = 1;
            ProductDatabaseEntry product1 = { job1.m_jobID, productSubId, "subfolder1/somefile.dds", AZ::Data::AssetType::CreateRandom() };
            ProductDatabaseEntry product2 = { job2.m_jobID, productSubId, "subfolder1/otherfile.dds", AZ::Data::AssetType::CreateRandom() };
            ProductDatabaseEntry product3 = { job3.m_jobID, productSubId, "blah.dds", AZ::Data::AssetType::CreateRandom() };
            ASSERT_TRUE(m_data->m_connection->SetProduct(product1));
            ASSERT_TRUE(m_data->m_connection->SetProduct(product2));
            ASSERT_TRUE(m_data->m_connection->SetProduct(product3));

            ProductDependencyDatabaseEntry productDependency1 = { product1.m_productID, sourceFile2.m_sourceGuid, productSubId, {}, "pc", true };
            ProductDependencyDatabaseEntry productDependency2 = { product2.m_productID, sourceFile3.m_sourceGuid, productSubId, {}, "pc", false };
            ProductDependencyDatabaseEntry productDependency3 = { product1.m_productID, sourceFile4.m_sourceGuid, productSubId, {}, "pc", false };
            ProductDependencyDatabaseEntry productDependency4 = { product2.m_productID, sourceFile4.m_sourceGuid, productSubId, {}, "pc", false };
            ProductDependencyDatabaseEntry productDependency5 = { product3.m_productID, sourceFile4.m_sourceGuid, productSubId, {}, "pc", true };

            ASSERT_TRUE(m_data->m_connection->SetProductDependency(productDependency1));
            ASSERT_TRUE(m_data->m_connection->SetProductDependency(productDependency2));
            ASSERT_TRUE(m_data->m_connection->SetProductDependency(productDependency3));
            ASSERT_TRUE(m_data->m_connection->SetProductDependency(productDependency4));
            ASSERT_TRUE(m_data->m_connection->SetProductDependency(productDependency5));

            QString referenceString = QString(R"(<Class name="Asset" field="Asset" value="id=%1,type={C62C7A87-9C09-4148-A985-12F2C99C0A45},hint={%2}" version="1" type="{77A19D40-8731-4D3C-9041-1B43047366A4}"/>)")
            .arg(AZ::Data::AssetId(sourceFile2.m_sourceGuid, product2.m_subID).ToString<AZStd::string>(AZ::Data::AssetId::SubIdDisplayType::Hex).c_str())
            .arg(sourceFile2.m_sourceName.c_str());

            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/subfolder1/somefile.tif"), referenceString));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/subfolder1/otherfile.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("folder/otherfile.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("folder/a/b/c/d/otherfile.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/duplicate/file1.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("folder/duplicate/file1.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/subfolder2/file.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/duplicate/folder/file1.tif")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/test.txt")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/dummy/foo.metadataextension")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/folder/file.foo")));
            ASSERT_TRUE(UnitTestUtils::CreateDummyFile(tempPath.absoluteFilePath("dev/folder/file.bar")));

            if (AZ::IO::FileIOBase::GetInstance() == nullptr)
            {
                m_localFileIo = AZStd::make_unique<AZ::IO::LocalFileIO>();
                AZ::IO::FileIOBase::SetInstance(m_localFileIo.get());
            }

            m_data->m_reporter = AZStd::make_unique<SourceFileRelocator>(m_data->m_connection, &m_data->m_platformConfig);

            AZ::JobManagerDesc jobDesc;
            AZ::JobManagerThreadDesc threadDesc;
            jobDesc.m_workerThreads.push_back(threadDesc);
            jobDesc.m_workerThreads.push_back(threadDesc);
            jobDesc.m_workerThreads.push_back(threadDesc);
            m_data->m_jobManager = aznew AZ::JobManager(jobDesc);
            m_data->m_jobContext = aznew AZ::JobContext(*m_data->m_jobManager);
            AZ::JobContext::SetGlobalContext(m_data->m_jobContext);

            m_data->m_perforceComponent = AZStd::make_unique<MockPerforceComponent>();
            m_data->m_perforceComponent->Activate();
            m_data->m_perforceComponent->SetConnection(new ::UnitTest::MockPerforceConnection(m_command));
        }

        void TearDown() override
        {
            if (AZ::IO::FileIOBase::GetInstance() == m_localFileIo.get())
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
            }
            m_localFileIo.reset();

            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());

            AZ::JobContext::SetGlobalContext(nullptr);
            delete m_data->m_jobContext;
            delete m_data->m_jobManager;

            m_data->m_perforceComponent->Deactivate();
            m_data->m_databaseLocationListener.BusDisconnect();
            m_data.reset();

            AZ::TickBus::AllowFunctionQueuing(false);
        }

        void TestResultEntries(const SourceFileRelocationContainer& container, AZStd::initializer_list<AZStd::string> expectedEntryDatabaseNames) const
        {
            AZStd::vector<AZStd::string> actualEntryDatabaseNames;

            for(const auto& relocationInfo : container)
            {
                if (relocationInfo.m_sourceEntry.m_sourceID != -1)
                {
                    actualEntryDatabaseNames.push_back(relocationInfo.m_sourceEntry.m_sourceName);
                }
                else
                {
                    // its a meta data file
                    actualEntryDatabaseNames.push_back(relocationInfo.m_oldRelativePath);
                }
            }

            ASSERT_THAT(actualEntryDatabaseNames, testing::UnorderedElementsAreArray(expectedEntryDatabaseNames));
        }

        void TestGetSourcesByPath(const AZStd::string& source, AZStd::initializer_list<AZStd::string> expectedEntryDatabaseNames, bool expectSuccess = true, bool excludeMetaDataFiles = true) const
        {
            SourceFileRelocationContainer relocationContainer;

            QDir tempPath(m_tempDir.path());

            const ScanFolderInfo* info;
            auto result = m_data->m_reporter->GetSourcesByPath(source, relocationContainer, info, excludeMetaDataFiles);
            ASSERT_EQ(result.IsSuccess(), expectSuccess) << (!result.IsSuccess() ? result.GetError().c_str() : "");

            if (expectSuccess)
            {
                TestResultEntries(relocationContainer, expectedEntryDatabaseNames);
            }
        }

        void TestComputeDestination(const ScanFolderDatabaseEntry& scanFolderEntry, AZStd::string_view sourceWithScanFolder, AZStd::string_view source, AZStd::string_view destination, const char* expectedPath, bool expectSuccess = true) const
        {
            SourceFileRelocationContainer entryContainer;

            QDir tempPath(m_tempDir.path());
            const ScanFolderInfo* info = nullptr;
            const ScanFolderInfo* destInfo = nullptr;

            ASSERT_TRUE(m_data->m_reporter->GetSourcesByPath(tempPath.absoluteFilePath(AZStd::string(sourceWithScanFolder).c_str()).toUtf8().constData(), entryContainer, info));

            auto result = m_data->m_reporter->ComputeDestination(entryContainer, m_data->m_platformConfig.GetScanFolderByPath(scanFolderEntry.m_scanFolder.c_str()), source, destination, destInfo);

            ASSERT_EQ(result.IsSuccess(), expectSuccess) << (!result.IsSuccess() ? result.GetError().c_str() : "");

            if (expectSuccess)
            {
                ASSERT_STREQ(entryContainer[0].m_newRelativePath.c_str(), expectedPath);
                ASSERT_TRUE(AZ::StringFunc::StartsWith(entryContainer[0].m_newAbsolutePath, scanFolderEntry.m_scanFolder));
                ASSERT_NE(destInfo, nullptr);
            }
        }

        QString ToAbsolutePath(QString path)
        {
            QDir tempPath(m_tempDir.path());

            return QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath(path);
        }

        void TestMove(QString fromPath, QString toPath, QString expectedSourcePath, QString expectedDestinationPath, QString expectedQueryPath, bool p4Enabled)
        {
            fromPath = ToAbsolutePath(fromPath);
            toPath = ToAbsolutePath(toPath);
            expectedDestinationPath = ToAbsolutePath(expectedDestinationPath);
            expectedQueryPath = ToAbsolutePath(expectedQueryPath);


            QDir tempPath(m_tempDir.path());
            QString absoluteDepotFilePath = tempPath.absoluteFilePath(expectedSourcePath);

            AZStd::string editParams, moveParams;

            m_command.m_fstatResponse =
                AZStd::string::format(R"(... depotFile //depot/%s)", expectedSourcePath.toUtf8().data()) + "\r\n"
                R"(... isMapped)" "\r\n"
                R"(... action edit)" "\r\n"
                R"(... headAction integrate)" "\r\n"
                R"(... headType text)" "\r\n"
                R"(... headTime 1454346715)" "\r\n"
                R"(... headRev 3)" "\r\n"
                R"(... headChange 147109)" "\r\n"
                R"(... headModTime 1452731919)" "\r\n"
                R"(... haveRev 3)" "\r\n";

            m_command.m_fstatResponse.append("... clientFile ")
                .append(absoluteDepotFilePath.toUtf8().constData())
                .append("\r\n\r\n");

            m_command.m_editCallback = [&editParams](AZStd::string params)
            {
                editParams = AZStd::move(params);
            };

            m_command.m_moveCallback = [this, &moveParams, expectedDestinationPath, expectedSourcePath](AZStd::string params)
            {
                moveParams = AZStd::move(params);

                m_command.m_fstatResponse.clear();
                m_command.m_fstatResponse =
                    AZStd::string::format(R"(... depotFile //depot/%s)", expectedSourcePath.toUtf8().data()) + "\r\n"
                    R"(... isMapped)" "\r\n"
                    R"(... action edit)" "\r\n"
                    R"(... headAction integrate)" "\r\n"
                    R"(... headType text)" "\r\n"
                    R"(... headTime 1454346715)" "\r\n"
                    R"(... headRev 3)" "\r\n"
                    R"(... headChange 147109)" "\r\n"
                    R"(... headModTime 1452731919)" "\r\n"
                    R"(... haveRev 3)" "\r\n";

                m_command.m_fstatResponse.append("... clientFile ")
                    .append(expectedDestinationPath.toUtf8().constData())
                    .append("\r\n\r\n");
            };

            auto result = m_data->m_reporter->Move(fromPath.toUtf8().constData(), toPath.toUtf8().constData(), false);

            if (p4Enabled)
            {
                ASSERT_FALSE(editParams.empty());
                ASSERT_FALSE(moveParams.empty());
            }
            ASSERT_TRUE(result.IsSuccess());

            // Check the result report
            RelocationSuccess report = result.TakeValue();
            ASSERT_EQ(report.m_moveFailureCount, 0); // 0 Failures
            ASSERT_EQ(report.m_moveSuccessCount, 1); // Exactly 1 file moved

            // Check the command parameters to make sure the paths are correct
            if (p4Enabled)
            {
                // edit -> we should see p4 edit <fromPath>
                AZStd::vector<AZStd::string> tokens;
                AZ::StringFunc::Tokenize(editParams, tokens, " ");

                ASSERT_EQ(tokens.size(), 4);
                AZ::StringFunc::Strip(tokens[3], "\\\"", true, true, true);
                AZStd::string pathToCheck(fromPath.toUtf8().constData());
                if (fromPath.endsWith("*") && toPath.endsWith("*"))
                {
                    AZ::StringFunc::Replace(pathToCheck, "*", "...");
                }
                ASSERT_STREQ(tokens[3].c_str(), pathToCheck.c_str());

                // move -> we should see p4 move <fromPath> <expectedQueryPath>
                tokens = {};
                AZ::StringFunc::Tokenize(moveParams, tokens, " ");

                ASSERT_EQ(tokens.size(), 5);
                // Remove quotes around the path and any slashes at the end
                AZ::StringFunc::Strip(tokens[3], "\\\"", true, true, true);
                AZ::StringFunc::Strip(tokens[4], "\\\"", true, true, true);
                ASSERT_STREQ(tokens[3].c_str(), pathToCheck.c_str());
                ASSERT_STREQ(tokens[4].c_str(), expectedQueryPath.toUtf8().constData());
            }
        }

        struct StaticData
        {
            // these variables are created during SetUp() and destroyed during TearDown() and thus are always available during tests using this fixture:
            AZStd::string m_databaseLocation;
            NiceMock<SourceFileRelocatorTestsMockDatabaseLocationListener> m_databaseLocationListener;
            AZStd::shared_ptr<AssetProcessor::AssetDatabaseConnection> m_connection;
            PlatformConfiguration m_platformConfig;

            ScanFolderDatabaseEntry m_scanFolder1;
            ScanFolderDatabaseEntry m_scanFolder2;

            FileStatePassthrough m_fileStateCache;

            AZStd::unique_ptr<SourceFileRelocator> m_reporter;
            AZStd::unique_ptr<MockPerforceComponent> m_perforceComponent;

            AZ::Uuid m_dependency1Uuid{ "{2C083160-DD50-459A-9482-CE663F4B558B}" };
            AZ::Uuid m_dependency2Uuid{ "{013BF607-A52A-4D1A-B2F4-AA8222C1BD68}" };

            AZ::JobManager* m_jobManager = nullptr;
            AZ::JobContext* m_jobContext = nullptr;
        };

        QTemporaryDir m_tempDir{ QDir::tempPath() + QLatin1String("/AssetProcessorUnitTest-XXXXXX") };

        AzToolsFramework::UuidUtilComponent m_uuidUtil;
        AzToolsFramework::MetadataManager m_metadataManager;
        AssetProcessor::UuidManager m_uuidManager;
        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_settingsRegistry;

        // we store the above data in a unique_ptr so that its memory can be cleared during TearDown() in one call, before we destroy the memory
        // allocator, reducing the chance of missing or forgetting to destroy one in the future.
        AZStd::unique_ptr<StaticData> m_data;
        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_localFileIo;
    };

    TEST_F(SourceFileRelocatorTest, GetSources_SingleFile_Succeeds)
    {
        TestGetSourcesByPath("subfolder1/somefile.tif", { "subfolder1/somefile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_PrefixedFile_Succeeds)
    {
        TestGetSourcesByPath("otherfile.tif", { "otherfile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_PrefixedAbsFile_Succeeds)
    {
        QDir tempDir(m_tempDir.path());
        TestGetSourcesByPath(tempDir.absoluteFilePath("folder/otherfile.tif").toUtf8().constData(), { "otherfile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_Folder_Fails)
    {
        TestGetSourcesByPath("subfolder1", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_SingleFileWildcard1_Succeeds)
    {
        TestGetSourcesByPath("subfolder1/some*", { "subfolder1/somefile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_NonExistentFile_Fails)
    {
        TestGetSourcesByPath("subfolder1/doesNotExist*.txt", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_ConsecutiveWildcard_Fails)
    {
        TestGetSourcesByPath("subfolder1/**.txt", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_SingleFileWildcard2_Succeeds)
    {
        TestGetSourcesByPath("subfolder1/some*.tif", { "subfolder1/somefile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_MultipleFilesWildcard1_Succeeds)
    {
        TestGetSourcesByPath("subfolder1/*file*", { "subfolder1/somefile.tif", "subfolder1/otherfile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_MultipleFilesWildcard2_Succeeds)
    {
        TestGetSourcesByPath("subfolder1/*", { "subfolder1/somefile.tif", "subfolder1/otherfile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_MultipleFilesWildcard_AbsolutePath_Succeeds)
    {
        QDir tempDir(m_tempDir.path());
        TestGetSourcesByPath(tempDir.absoluteFilePath("dev/subfolder1*").toUtf8().constData(), { "subfolder1/somefile.tif", "subfolder1/otherfile.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_MultipleFoldersWildcard_Succeeds)
    {
        QDir tempDir(m_tempDir.path());
        TestGetSourcesByPath("subfolder*/*", { "subfolder1/somefile.tif", "subfolder1/otherfile.tif", "subfolder2/file.tif" });
    }

    TEST_F(SourceFileRelocatorTest, GetSources_ScanFolder1_Fails)
    {
        TestGetSourcesByPath("dev", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_ScanFolder2_Fails)
    {
        TestGetSourcesByPath("dev/", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_MultipleScanFolders_Fails)
    {
        TestGetSourcesByPath("*", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_PartialPath_FailsWithNoResults)
    {
        TestGetSourcesByPath("older/*", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_AmbiguousPath1_Fails)
    {
        TestGetSourcesByPath("duplicate/file1.tif", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_AmbiguousPathWildcard_Fails)
    {
        TestGetSourcesByPath("duplicate/*.tif", { }, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_DuplicateFile_AbsolutePath_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");
        TestGetSourcesByPath(filePath.toUtf8().constData(), { "duplicate/file1.tif" }, true);
    }

    TEST_F(SourceFileRelocatorTest, GetMetaDataFile_AbsolutePath_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("dummy/foo.metadataextension");
        TestGetSourcesByPath(filePath.toUtf8().constData(), { "dummy/foo.metadataextension" }, true, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_HaveMetadata_AbsolutePath_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("folder/file.foo");
        TestGetSourcesByPath(filePath.toUtf8().constData(), { "folder/file.foo" , "folder/file.bar" }, true, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_HaveMetadata_Exclude_AbsolutePath_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("folder/file.foo");
        TestGetSourcesByPath(filePath.toUtf8().constData(), { "folder/file.foo" }, true, true);
    }

    TEST_F(SourceFileRelocatorTest, GetMetaDataFile_SingleFileWildcard_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("dummy/foo.metadataextension");
        TestGetSourcesByPath("dummy/*", { "dummy/foo.metadataextension" }, true, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_HaveMetadata_SingleFileWildcard_Succeeds)
    {
        TestGetSourcesByPath("folder/*", { "folder/file.foo" , "folder/file.bar" }, true, false);
    }

    TEST_F(SourceFileRelocatorTest, GetSources_HaveMetadata_Exclude_SingleFileWildcard_Succeeds)
    {
        TestGetSourcesByPath("folder/*", { "folder/file.foo" }, true, true);
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_SourceEndsWithWildcard_DestinationEndsWithWildcard_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/fi*", destinationPath + "rename*", "dev/duplicate/file1.tif", destinationPath + "renameile1.tif", destinationPath + "rename*", false);
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_SourceEndsWithWildcardFolder_DestinationEndsWithWildcard_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/folder*", destinationPath + "rename*", "dev/duplicate/folder/file1.tif", destinationPath + "rename/file1.tif", destinationPath + "rename*", false);
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_RepeatCharacters1_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("aaaaaaab", "a*b*", "a*bb*");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "aaaaaaabb");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_RepeatCharacters2_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("aaaaaaaaaa", "a*a*", "a*b*");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "aaaaaaaaab");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_RepeatCharacters3_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("aaabbbaaabbb", "a*a*", "a*c*");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "aaabbbaacbbb");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_RepeatCharacters4_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("aabccbedd", "a*b*dd", "1*2*3");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "1abcc2e3");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_ZeroLengthMatch_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("aabb", "aabb*", "1*");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "1");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_ZeroLengthMatch_MultipleWildcards_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("abcdef", "a*b*e*", "1*2*3*");

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_STREQ(result.GetValue().c_str(), "12cd3f");
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_Complex_Succeeds)
    {
        auto result = m_data->m_reporter->HandleWildcard("subfolder1somefile.tif", "*o*some*.tif", "*1*2*3");

        ASSERT_TRUE(result.IsSuccess());
    }

    TEST_F(SourceFileRelocatorTest, HandleWildcard_TooComplex_Fails)
    {
        auto result = m_data->m_reporter->HandleWildcard("subfolder1/somefile.tif", "*o*some*.tif", "*1*2*3");

        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(SourceFileRelocatorTest, GatherDependencies_Succeeds)
    {
        SourceFileRelocationContainer entryContainer;

        QDir tempPath(m_tempDir.path());
        const ScanFolderInfo* info;

        ASSERT_TRUE(m_data->m_reporter->GetSourcesByPath(tempPath.absoluteFilePath(AZStd::string("folder/o*").c_str()).toUtf8().constData(), entryContainer, info));
        ASSERT_EQ(entryContainer.size(), 1);
        ASSERT_STREQ(entryContainer[0].m_sourceEntry.m_sourceName.c_str(), "otherfile.tif");

        m_data->m_reporter->PopulateDependencies(entryContainer);

        AZStd::vector<AZ::Uuid> databaseSourceNames;
        AZStd::vector<AZ::s64> databaseProductDependencyNames;

        for(const auto& relocationInfo : entryContainer)
        {
            for (const auto& dependencyEntry : relocationInfo.m_sourceDependencyEntries)
            {
                databaseSourceNames.push_back(dependencyEntry.m_sourceGuid);
            }
        }

        for(const auto& relocationInfo : entryContainer)
        {
            for(const auto& productDependency : relocationInfo.m_productDependencyEntries)
            {
                databaseProductDependencyNames.push_back(productDependency.m_productPK);
            }
        }

        ASSERT_THAT(databaseSourceNames, testing::UnorderedElementsAreArray({ m_data->m_dependency2Uuid }));
        ASSERT_THAT(databaseProductDependencyNames, testing::UnorderedElementsAreArray({ 2 }));
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveFolder_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/o*.tif", "o*.tif", "newfolder/makeafolder/o*.tif", "newfolder/makeafolder/otherfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_RenameFile_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/otherfile.tif", "otherfile.tif", "anewfile.png", "anewfile.png");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveFolderDeeper_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder1, "dev/*o*/some*.tif", "*o*/some*.tif", "subfolder2/subfolder3/*o*/some*.tif", "subfolder2/subfolder3/subfolder1/somefile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveFileUpAFolder_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder1, "dev/subfolder1/somefile.tif", "subfolder1/somefile.tif", "somefile.tif", "somefile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveFileUpAFolderAndRename_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder1, "dev/subfolder1/somefile.tif", "subfolder1/somefile.tif", "somenewfile.tif", "somenewfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveFileUpPartial_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*", "a/b/c/*", "a/*", "a/d/otherfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_AbsolutePathBoth_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*",
            tempPath.absoluteFilePath("folder/a/b/*").toUtf8().constData(),
            tempPath.absoluteFilePath("folder/a/*").toUtf8().constData(),
            "a/c/d/otherfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_AbsolutePathSource_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*",
            tempPath.absoluteFilePath("folder/a/b/*").toUtf8().constData(),
            "a/*",
            "a/c/d/otherfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_AbsolutePathDestination_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*",
            "a/b/*",
            tempPath.absoluteFilePath("folder/a/*").toUtf8().constData(),
            "a/c/d/otherfile.tif");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_AbsolutePathRename_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*",
            "a/b/c/d/otherfile.tif",
            tempPath.absoluteFilePath("folder/a/c/d/newlyNamed.png").toUtf8().constData(),
            "a/c/d/newlyNamed.png");
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MoveOutsideScanfolder_Fails)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*", "a/b/c/*", m_tempDir.path().toUtf8().constData(), "", false);
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_PathNavigation_Fails)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/*", "a/b/c/*", "../a*", "", false);
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_WildcardAcrossDirectories_Fails)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "*/c/*", "*/d/*", "", false);
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_MismatchedWildcardCount_Fails)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/*/*", "*/d", "", false);
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_InvalidCharacters_Fails)
    {
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/c/*", "d/*?", "", false);
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/c/*", "d/*<", "", false);
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/c/*", "d/*>", "", false);
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/c/*", "d/*\"", "", false);
        TestComputeDestination(m_data->m_scanFolder2, "folder/a/b/c/*", "a/b/c/*", "d/*|", "", false);
    }

    TEST_F(SourceFileRelocatorTest, ComputeDestination_Directory_Succeeds)
    {
        TestComputeDestination(m_data->m_scanFolder1, "dev/subfolder1/somefile.tif", "subfolder1/somefile.tif", "subfolder2/", "subfolder2/somefile.tif");
        TestComputeDestination(m_data->m_scanFolder1, "dev/subfolder1/s*", "subfolder1/s*", "subfolder2/", "subfolder2/somefile.tif");
        TestComputeDestination(m_data->m_scanFolder1, "dev/test.txt", "test.txt", "subfolder2/", "subfolder2/test.txt");
    }

    TEST_F(SourceFileRelocatorTest, BuildReport_Succeeds)
    {
        SourceFileRelocationContainer entryContainer;

        QDir tempPath(m_tempDir.path());
        const ScanFolderInfo* info = nullptr;
        const ScanFolderInfo* destInfo = nullptr;
        FileUpdateTasks updateTasks;

        ASSERT_TRUE(m_data->m_reporter->GetSourcesByPath(tempPath.absoluteFilePath("folder/*").toUtf8().constData(), entryContainer, info));
        ASSERT_EQ(entryContainer.size(), 3);

        m_data->m_reporter->ComputeDestination(entryContainer, info, "*", "someOtherPlace/*", destInfo);
        m_data->m_reporter->PopulateDependencies(entryContainer);
        AZStd::string report = m_data->m_reporter->BuildReport(entryContainer, updateTasks, true, false);

        ASSERT_FALSE(report.empty());
    }

    TEST_F(SourceFileRelocatorTest, Move_Preview_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        auto result = m_data->m_reporter->Move(tempPath.absoluteFilePath((m_data->m_scanFolder1.m_scanFolder + "/subfolder*").c_str()).toUtf8().constData(), "someOtherPlace/*", true);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_EQ(result.GetValue().m_relocationContainer.size(), 3);
    }

    TEST_F(SourceFileRelocatorTest, TestInterface)
    {
        auto* sourceFileRelocator = AZ::Interface<ISourceFileRelocation>::Get();

        ASSERT_NE(sourceFileRelocator, nullptr);
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");
        auto newFilePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("someOtherPlace/file1.tif");

        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));

        auto result = m_data->m_reporter->Move(filePath.toUtf8().constData(), "someOtherPlace/file1.tif", false);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_FALSE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));
        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(newFilePath.toUtf8().constData()));

        RelocationSuccess successResult = result.TakeValue();

        ASSERT_EQ(successResult.m_moveSuccessCount, 1);
        ASSERT_EQ(successResult.m_moveFailureCount, 0);
        ASSERT_EQ(successResult.m_moveTotalCount, 1);
        ASSERT_EQ(successResult.m_updateTotalCount, 0);
    }

    TEST_F(SourceFileRelocatorTest, MoveMetadataEnabledType_Real_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");
        auto newFilePath =
            QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("someOtherPlace/renamed.tif");
        auto metadataPath = AzToolsFramework::MetadataManager::ToMetadataPath(filePath.toUtf8().constData());
        auto newMetadataPath = AzToolsFramework::MetadataManager::ToMetadataPath(newFilePath.toUtf8().constData());

        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));

        auto* uuidInterface = AZ::Interface<AssetProcessor::IUuidRequests>::Get();
        ASSERT_TRUE(uuidInterface);

        uuidInterface->EnableGenerationForTypes({ ".tif" });

        AZ::Utils::WriteFile("unit test file", metadataPath.c_str());

        auto result = m_data->m_reporter->Move(filePath.toUtf8().constData(), "someOtherPlace/renamed.tif", false);

        auto* io = AZ::IO::FileIOBase::GetInstance();
        ASSERT_TRUE(io);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_FALSE(io->Exists(filePath.toUtf8().constData()));
        ASSERT_TRUE(io->Exists(newFilePath.toUtf8().constData()));
        ASSERT_FALSE(io->Exists(metadataPath.c_str()));
        ASSERT_TRUE(io->Exists(newMetadataPath.c_str()));

        RelocationSuccess successResult = result.TakeValue();

        ASSERT_EQ(successResult.m_moveSuccessCount, 2);
        ASSERT_EQ(successResult.m_moveFailureCount, 0);
        ASSERT_EQ(successResult.m_moveTotalCount, 2);
        ASSERT_EQ(successResult.m_updateTotalCount, 0);
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_ReadOnlyFile_Fails)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");
        auto newFilePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("someOtherPlace/file1.tif");

        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));
        ASSERT_TRUE(AZ::IO::SystemFile::SetWritable(filePath.toUtf8().constData(), false));

        auto result = m_data->m_reporter->Move(filePath.toUtf8().constData(), "someOtherPlace/file1.tif", false);

        ASSERT_TRUE(result.IsSuccess());
        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));
        ASSERT_FALSE(AZ::IO::FileIOBase::GetInstance()->Exists(newFilePath.toUtf8().constData()));

        RelocationSuccess successResult = result.TakeValue();

        ASSERT_EQ(successResult.m_moveSuccessCount, 0);
        ASSERT_EQ(successResult.m_moveFailureCount, 1);
        ASSERT_EQ(successResult.m_moveTotalCount, 1);
        ASSERT_EQ(successResult.m_updateTotalCount, 0);
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_WithDependencies_Fails)
    {
        QDir tempPath(m_tempDir.path());
        auto result = m_data->m_reporter->Move("subfolder1/otherfile.tif", "someOtherPlace/otherfile.tif", false);

        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_WithDependenciesUpdateReferences_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto result = m_data->m_reporter->Move("subfolder1/otherfile.tif", "someOtherPlace/otherfile.tif", RelocationParameters_RemoveEmptyFoldersFlag | RelocationParameters_UpdateReferencesFlag);

        ASSERT_TRUE(result.IsSuccess());

        RelocationSuccess successResult = result.TakeValue();

        ASSERT_EQ(successResult.m_moveSuccessCount, 1);
        ASSERT_EQ(successResult.m_moveFailureCount, 0);
        ASSERT_EQ(successResult.m_moveTotalCount, 1);
        ASSERT_EQ(successResult.m_updateSuccessCount, 1);
        ASSERT_EQ(successResult.m_updateFailureCount, 1); // Since we have both product and source dependencies from the same file, the 2nd attempt to update fails
        ASSERT_EQ(successResult.m_updateTotalCount, 2);
    }

    TEST_F(SourceFileRelocatorTest, Delete_Real_Succeeds)
    {
        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");

        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));

        auto result = m_data->m_reporter->Delete(filePath.toUtf8().constData(), false);

        ASSERT_TRUE(result.IsSuccess());

        RelocationSuccess successResult = result.TakeValue();

        ASSERT_EQ(successResult.m_moveSuccessCount, 1);
        ASSERT_EQ(successResult.m_moveFailureCount, 0);
        ASSERT_EQ(successResult.m_moveTotalCount, 1);
        ASSERT_EQ(successResult.m_updateTotalCount, 0);

        ASSERT_FALSE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));
    }

    TEST_F(SourceFileRelocatorTest, Delete_Real_Readonly_Fails)
    {
        struct AutoResetDirectoryReadOnlyState
        {
            AutoResetDirectoryReadOnlyState(QString dirName)
                : m_dirName(AZStd::move(dirName))
            {
                AZ::IO::SystemFile::SetWritable(m_dirName.toUtf8().constData(), false);
            }
            ~AutoResetDirectoryReadOnlyState()
            {
                AZ::IO::SystemFile::SetWritable(m_dirName.toUtf8().constData(), true);
            }
            AZ_DISABLE_COPY_MOVE(AutoResetDirectoryReadOnlyState)
        private:
            QString m_dirName;
        };

        QDir tempPath(m_tempDir.path());

        auto filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");

        ASSERT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));

        AutoResetDirectoryReadOnlyState readOnlyResetter(QFileInfo(filePath).absoluteDir().absolutePath());

        AZ::IO::SystemFile::SetWritable(filePath.toUtf8().constData(), false);

        auto result = m_data->m_reporter->Delete(filePath.toUtf8().constData(), false);

        EXPECT_TRUE(result.IsSuccess());

        RelocationSuccess successResult = result.TakeValue();

        EXPECT_EQ(successResult.m_moveSuccessCount, 0);
        EXPECT_EQ(successResult.m_moveFailureCount, 1);
        EXPECT_EQ(successResult.m_moveTotalCount, 1);
        EXPECT_EQ(successResult.m_updateTotalCount, 0);

        EXPECT_TRUE(AZ::IO::FileIOBase::GetInstance()->Exists(filePath.toUtf8().constData()));
    }

    TEST_F(SourceFileRelocatorTest, Delete_Real_WithDependencies_Fails)
    {
        QDir tempPath(m_tempDir.path());
        auto result = m_data->m_reporter->Delete("subfolder1/otherfile.tif", false);

        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_DestinationIsPathOnly_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/file1.tif", destinationPath, "dev/duplicate/file1.tif",  destinationPath + "file1.tif", destinationPath + "file1.tif", false);
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_DestinationIsPathOnly_SourceWithWildcard_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/fil*1.tif", destinationPath, "dev/duplicate/file1.tif", destinationPath + "file1.tif", destinationPath + "fil*1.tif", false);
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_SourceContainsWildcard_DestinationEndsWithWildcard_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/fil*1.tif", destinationPath + "*", "dev/duplicate/file1.tif", destinationPath + "e", destinationPath + "*", false);
    }

    TEST_F(SourceFileRelocatorTest, Move_Real_SourceEndsWithWildcard_DestinationContainsWildcard_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/fi*", destinationPath + "*.rename", "dev/duplicate/file1.tif", destinationPath + "le1.tif.rename", destinationPath + "*.rename", false);
    }

    struct SourceFileRelocatorPerforceMockTest
        : SourceFileRelocatorTest
    {
        void SetUp() override
        {
            SourceFileRelocatorTest::SetUp();

            EnableSourceControl();
        }
    };

    TEST_F(SourceFileRelocatorPerforceMockTest, GetSources_NonExistentFile_Fails)
    {
        SourceFileRelocationContainer relocationContainer;

        QDir tempPath(m_tempDir.path());

        m_command.m_persistFstatResponse = true;
        m_command.m_fstatErrorResponse = (tempPath.absoluteFilePath("dev/subfolder1/doesNotExist*.txt") + " - no such file(s)\n"
            + tempPath.absoluteFilePath("folder/subfolder1/doesNotExist*.txt") + " - no such file(s)\n").toUtf8().constData();

        const ScanFolderInfo* info;
        auto result = m_data->m_reporter->GetSourcesByPath("subfolder1/doesNotExist*.txt", relocationContainer, info);
        ASSERT_EQ(result.IsSuccess(), false);

        auto&& error = result.TakeError();

        ASSERT_STREQ(error.c_str(), "Wildcard search did not match any files.\n");
    }

    TEST_F(SourceFileRelocatorPerforceMockTest, Move_Real_DestinationIsPathOnly_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/file1.tif", destinationPath, "dev/duplicate/file1.tif", destinationPath + "file1.tif", destinationPath + "file1.tif", true);
    }

    TEST_F(SourceFileRelocatorPerforceMockTest, Move_Real_DestinationIsPathOnly_SourceWithWildcard_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/fil*1.tif", destinationPath, "dev/duplicate/file1.tif", destinationPath + "file1.tif", destinationPath + "fil*1.tif", true);
    }

    TEST_F(SourceFileRelocatorPerforceMockTest, Move_Real_SourceContainsWildcard_DestinationEndsWithWildcard_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/fil*1.tif", destinationPath + "*", "dev/duplicate/file1.tif", destinationPath + "e", destinationPath + "*", true);
    }

    TEST_F(SourceFileRelocatorPerforceMockTest, Move_Real_SourceEndsWithWildcard_DestinationContainsWildcard_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/f*", destinationPath + "*.rename", "dev/duplicate/file1.tif", destinationPath + "ile1.tif.rename", destinationPath + "*.rename", true);
    }

    TEST_F(SourceFileRelocatorPerforceMockTest, Delete_Real_Succeeds)
    {
        QDir tempPath(m_tempDir.path());
        QString filePath = QDir(tempPath.absoluteFilePath(m_data->m_scanFolder1.m_scanFolder.c_str())).absoluteFilePath("duplicate/file1.tif");

        AZStd::string deleteParams;

        m_command.m_fstatResponse =
            R"(... depotFile //depot/dev/duplicate/file1.tif)" "\r\n"
            R"(... isMapped)" "\r\n"
            R"(... action edit)" "\r\n"
            R"(... headAction integrate)" "\r\n"
            R"(... headType text)" "\r\n"
            R"(... headTime 1454346715)" "\r\n"
            R"(... headRev 3)" "\r\n"
            R"(... headChange 147109)" "\r\n"
            R"(... headModTime 1452731919)" "\r\n"
            R"(... haveRev 3)" "\r\n";

        m_command.m_fstatResponse.append("... clientFile ")
            .append(filePath.toUtf8().constData())
            .append("\r\n\r\n");

        m_command.m_deleteCallback = [this, &deleteParams, filePath](AZStd::string params)
        {
            deleteParams = AZStd::move(params);
            m_command.m_rawOutput.outputResult = "delete called";

            m_command.m_fstatResponse =
                R"(... depotFile //depot/dev/duplicate/file1.tif)" "\r\n"
                R"(... isMapped)" "\r\n"
                R"(... action delete)" "\r\n"
                R"(... headAction integrate)" "\r\n"
                R"(... headType text)" "\r\n"
                R"(... headTime 1454346715)" "\r\n"
                R"(... headRev 3)" "\r\n"
                R"(... headChange 147109)" "\r\n"
                R"(... headModTime 1452731919)" "\r\n"
                R"(... haveRev 3)" "\r\n";

            m_command.m_fstatResponse.append("... clientFile ")
            .append(filePath.toUtf8().constData())
            .append("\r\n\r\n");
        };

        auto result = m_data->m_reporter->Delete(filePath.toUtf8().constData(), false);

        ASSERT_TRUE(result.IsSuccess());

        RelocationSuccess report = result.TakeValue();
        ASSERT_EQ(report.m_moveFailureCount, 0);
        ASSERT_GT(report.m_moveSuccessCount, 0);

        ASSERT_FALSE(deleteParams.empty());

        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(deleteParams, tokens, " ");

        ASSERT_EQ(tokens.size(), 4);
        AZ::StringFunc::Strip(tokens[3], "\\\"", true, true, true);
        ASSERT_STREQ(tokens[3].c_str(), filePath.toUtf8().constData());
    }

    TEST_F(SourceFileRelocatorPerforceMockTest, Move_Real_SourceEndsWithWildcard_DestinationEndsWithWildcard_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/fi*", destinationPath + "rename*", "dev/duplicate/file1.tif", destinationPath + "renamele1.tif", destinationPath + "rename...", true);
    }

    TEST_F(SourceFileRelocatorPerforceMockTest, Move_Real_SourceEndsWithWildcardFolder_DestinationEndsWithWildcard_Succeeds)
    {
        const QString destinationPath = "someOtherPlace/";

        TestMove("duplicate/folder*", destinationPath + "rename*", "dev/duplicate/folder/file1.tif", destinationPath + "rename/file1.tif", destinationPath + "rename...", true);
    }
}
