/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/assetmanager/IntermediateAssetTests.h>
#include <QCoreApplication>
#include <native/unittests/UnitTestRunner.h>

namespace UnitTests
{
    AssetBuilderSDK::CreateJobFunction CreateJobStage(
        const AZStd::string& name,
        bool commonPlatform,
        const AZStd::string& sourceDependencyPath = "")
    {
        using namespace AssetBuilderSDK;

        // Note: capture by copy because we need these to stay around for a long time
        return [name, commonPlatform, sourceDependencyPath]([[maybe_unused]] const CreateJobsRequest& request, CreateJobsResponse& response)
        {
            if (commonPlatform)
            {
                response.m_createJobOutputs.push_back(JobDescriptor{ "fingerprint", name, CommonPlatformName });
            }
            else
            {
                for (const auto& platform : request.m_enabledPlatforms)
                {
                    response.m_createJobOutputs.push_back(JobDescriptor{ "fingerprint", name, platform.m_identifier.c_str() });
                }
            }

            if (!sourceDependencyPath.empty())
            {
                response.m_sourceFileDependencyList.push_back(SourceFileDependency{ sourceDependencyPath, AZ::Uuid::CreateNull() });
            }

            response.m_result = CreateJobsResultCode::Success;
        };
    }

    AssetBuilderSDK::ProcessJobFunction ProcessJobStage(const AZStd::string& outputExtension, AssetBuilderSDK::ProductOutputFlags flags, bool outputExtraFile)
    {
        using namespace AssetBuilderSDK;

        // Capture by copy because we need these to stay around a long time
        return [outputExtension, flags, outputExtraFile](const ProcessJobRequest& request, ProcessJobResponse& response)
        {
            AZ::IO::Path outputFile = request.m_sourceFile;
            outputFile.ReplaceExtension(outputExtension.c_str());

            AZ::IO::LocalFileIO::GetInstance()->Copy(
                request.m_fullPath.c_str(), (AZ::IO::Path(request.m_tempDirPath) / outputFile).c_str());

            auto product = JobProduct{ outputFile.c_str(), AZ::Data::AssetType::CreateName(outputExtension.c_str()), 1 };

            product.m_outputFlags = flags;
            product.m_dependenciesHandled = true;
            response.m_outputProducts.push_back(product);

            if (outputExtraFile)
            {
                auto extraFilePath = AZ::IO::Path(request.m_tempDirPath) / "z_extra.txt"; // Z prefix to place at end of list when sorting for processing

                UnitTestUtils::CreateDummyFile(extraFilePath.c_str(), "unit test file");

                auto extraProduct = JobProduct{ extraFilePath.c_str(), AZ::Data::AssetType::CreateName("extra"), 2 };

                extraProduct.m_outputFlags = flags;
                extraProduct.m_dependenciesHandled = true;
                response.m_outputProducts.push_back(extraProduct);
            }

            response.m_resultCode = ProcessJobResult_Success;
        };
    }

    void IntermediateAssetTests::CreateBuilder(const char* name, const char* inputFilter, const char* outputExtension, bool createJobCommonPlatform, AssetBuilderSDK::ProductOutputFlags outputFlags, bool outputExtraFile)
    {
        using namespace AssetBuilderSDK;

        m_builderInfoHandler.CreateBuilderDesc(
            name, AZ::Uuid::CreateRandom().ToFixedString().c_str(),
            { AssetBuilderPattern{ inputFilter, AssetBuilderPattern::Wildcard } }, CreateJobStage(name, createJobCommonPlatform),
            ProcessJobStage(outputExtension, outputFlags, outputExtraFile), "fingerprint");
    }

    void IntermediateAssetTests::SetUp()
    {
        AssetManagerTestingBase::SetUp();

        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test.stage1";
        m_testFilePath = (scanFolderDir / testFilename).AsPosix().c_str();

        UnitTestUtils::CreateDummyFile(m_testFilePath.c_str(), "unit test file");

        m_rc = AZStd::make_unique<AssetProcessor::RCController>(1, 1);

        m_rc->SetDispatchPaused(false);

        QObject::connect(
            m_rc.get(), &AssetProcessor::RCController::FileFailed,
            [this](auto entryIn)
            {
                m_fileFailed = true;
            });

        QObject::connect(
            m_rc.get(), &AssetProcessor::RCController::FileCompiled,
            [this](auto jobEntry, auto response)
            {
                m_fileCompiled = true;
                m_processedJobEntry = jobEntry;
                m_processJobResponse = response;
            });

        m_localFileIo->SetAlias("@log@", (AZ::IO::Path(m_tempDir.GetDirectory()) / "logs").c_str());
    }

    void IntermediateAssetTests::TearDown()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

        AssetManagerTestingBase::TearDown();
    }

    // Since AP will redirect any failures to a job log file, we won't see them output by default
    // This will cause any error/assert to be printed out and mark the test as failed
    bool IntermediateAssetTests::OnPreAssert(const char* fileName, int line, const char* /*func*/, const char* message)
    {
        if (m_expectedErrors > 0)
        {
            --m_expectedErrors;
            return false;
        }

        UnitTest::ColoredPrintf(UnitTest::COLOR_RED, "Assert: %s\n", message);

        ADD_FAILURE_AT(fileName, line);

        return false;
    }

    bool IntermediateAssetTests::OnPreError(const char* /*window*/, const char* fileName, int line, const char* /*func*/, const char* message)
    {
        if (m_expectedErrors > 0)
        {
            --m_expectedErrors;
            return false;
        }

        UnitTest::ColoredPrintf(UnitTest::COLOR_RED, "Error: %s\n", message);

        ADD_FAILURE_AT(fileName, line);

        return false;
    }

    void IntermediateAssetTests::IncorrectBuilderConfigurationTest(bool commonPlatform, AssetBuilderSDK::ProductOutputFlags flags)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", commonPlatform, flags);

        m_expectedErrors = 1;

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, m_testFilePath.c_str()));
        QCoreApplication::processEvents();

        RunFile(1);
        ProcessJob(*m_rc, m_jobDetailsList[0]);

        ASSERT_TRUE(m_fileFailed);
    }

    AZStd::string IntermediateAssetTests::MakePath(const char* filename, bool intermediate)
    {
        auto cacheDir = AZ::IO::Path(m_tempDir.GetDirectory()) / "Cache";

        if (intermediate)
        {
            cacheDir = AssetUtilities::GetIntermediateAssetsFolder(cacheDir);

            return (cacheDir / filename).StringAsPosix();
        }

        return (cacheDir / "pc" / filename).StringAsPosix();
    }

    void IntermediateAssetTests::CheckProduct(const char* relativePath, bool exists)
    {
        auto expectedProductPath = MakePath(relativePath, false);
        EXPECT_EQ(AZ::IO::SystemFile::Exists(expectedProductPath.c_str()), exists) << expectedProductPath.c_str();
    }

    void IntermediateAssetTests::CheckIntermediate(const char* relativePath, bool exists)
    {
        auto expectedIntermediatePath = MakePath(relativePath, true);
        EXPECT_EQ(AZ::IO::SystemFile::Exists(expectedIntermediatePath.c_str()), exists) << expectedIntermediatePath.c_str();
    }

    void IntermediateAssetTests::ProcessSingleStep(int expectedJobCount, int expectedFileCount, int jobToRun, bool expectSuccess)
    {
        // Reset state
        m_jobDetailsList.clear();
        m_fileCompiled = false;
        m_fileFailed = false;

        RunFile(expectedJobCount, expectedFileCount);

        std::stable_sort(
            m_jobDetailsList.begin(), m_jobDetailsList.end(),
            [](const AssetProcessor::JobDetails& a, const AssetProcessor::JobDetails& b) -> bool
            {
                return a.m_jobEntry.m_databaseSourceName.compare(b.m_jobEntry.m_databaseSourceName) < 0;
            });

        ProcessJob(*m_rc, m_jobDetailsList[jobToRun]);

        if (expectSuccess)
        {
            ASSERT_TRUE(m_fileCompiled);
            m_assetProcessorManager->AssetProcessed(m_processedJobEntry, m_processJobResponse);
        }
        else
        {
            ASSERT_TRUE(m_fileFailed);
        }
    }

    void IntermediateAssetTests::ProcessFileMultiStage(
        int endStage, bool doProductOutputCheck, const char* file, int startStage, bool expectAutofail, bool hasExtraFile)
    {
        auto cacheDir = AZ::IO::Path(m_tempDir.GetDirectory()) / "Cache";
        auto intermediatesDir = AssetUtilities::GetIntermediateAssetsFolder(cacheDir);

        if (file == nullptr)
        {
            file = m_testFilePath.c_str();
        }

        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, file));
        QCoreApplication::processEvents();

        for (int i = startStage; i <= endStage; ++i)
        {
            int expectedJobCount = 1;
            int expectedFileCount = 1;
            int jobToRun = 0;

            // If there's an extra file output, it'll only show up after the 1st iteration
            if (i > startStage && hasExtraFile)
            {
                expectedJobCount = 2;
                expectedFileCount = 2;
            }
            else if (expectAutofail)
            {
                expectedJobCount = 2;
                jobToRun = 1;
            }

            ProcessSingleStep(expectedJobCount, expectedFileCount, jobToRun, true);

            if (i < endStage)
            {
                auto expectedIntermediatePath = intermediatesDir / AZStd::string::format("test.stage%d", i + 1);
                EXPECT_TRUE(AZ::IO::SystemFile::Exists(expectedIntermediatePath.c_str())) << expectedIntermediatePath.c_str();
            }

            // Only first job should have an autofail due to a conflict
            expectAutofail = false;
        }

        m_assetProcessorManager->CheckFilesToExamine(0);
        m_assetProcessorManager->CheckActiveFiles(0);
        m_assetProcessorManager->CheckJobEntries(0);

        if (doProductOutputCheck)
        {
            CheckProduct(AZStd::string::format("test.stage%d", endStage + 1).c_str());
        }
    }

    TEST_F(IntermediateAssetTests, FileProcessedAsIntermediateIntoProduct)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2","stage3", false, ProductOutputFlags::ProductAsset);

        ProcessFileMultiStage(2, true);
    }

    TEST_F(IntermediateAssetTests, IntermediateOutputWithWrongPlatform_CausesFailure)
    {
        IncorrectBuilderConfigurationTest(false, AssetBuilderSDK::ProductOutputFlags::IntermediateAsset);
    }

    TEST_F(IntermediateAssetTests, ProductOutputWithWrongPlatform_CausesFailure)
    {
        IncorrectBuilderConfigurationTest(true, AssetBuilderSDK::ProductOutputFlags::ProductAsset);
    }

    TEST_F(IntermediateAssetTests, IntermediateAndProductOutputFlags_NormalPlatform_CausesFailure)
    {
        IncorrectBuilderConfigurationTest(false, AssetBuilderSDK::ProductOutputFlags::IntermediateAsset | AssetBuilderSDK::ProductOutputFlags::ProductAsset);
    }

    TEST_F(IntermediateAssetTests, IntermediateAndProductOutputFlags_CommonPlatform_CausesFailure)
    {
        IncorrectBuilderConfigurationTest(true, AssetBuilderSDK::ProductOutputFlags::IntermediateAsset | AssetBuilderSDK::ProductOutputFlags::ProductAsset);
    }

    TEST_F(IntermediateAssetTests, NoFlags_CausesFailure)
    {
        IncorrectBuilderConfigurationTest(false, (AssetBuilderSDK::ProductOutputFlags)(0));
    }

    TEST_F(IntermediateAssetTests, ABALoop_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage2",  true, ProductOutputFlags::IntermediateAsset); // Loop back to an intermediate

        ProcessFileMultiStage(3, false);

        EXPECT_EQ(m_jobDetailsList.size(), 3);
        EXPECT_TRUE(m_jobDetailsList[1].m_autoFail);
        EXPECT_TRUE(m_jobDetailsList[2].m_autoFail);

        EXPECT_EQ(m_jobDetailsList[1].m_jobEntry.m_databaseSourceName, "test.stage3");
        EXPECT_EQ(m_jobDetailsList[2].m_jobEntry.m_databaseSourceName, "test.stage1");
    }

    TEST_F(IntermediateAssetTests, AALoop_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage1", true, ProductOutputFlags::IntermediateAsset); // Loop back to the source

        ProcessFileMultiStage(2, false);

        EXPECT_EQ(m_jobDetailsList.size(), 3);
        EXPECT_TRUE(m_jobDetailsList[1].m_autoFail);
        EXPECT_TRUE(m_jobDetailsList[2].m_autoFail);

        EXPECT_EQ(m_jobDetailsList[1].m_jobEntry.m_databaseSourceName, "test.stage2");
        EXPECT_EQ(m_jobDetailsList[2].m_jobEntry.m_databaseSourceName, "test.stage1");
    }

    TEST_F(IntermediateAssetTests, SelfLoop_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage1", true, ProductOutputFlags::IntermediateAsset); // Loop back to the source with a single job

        ProcessFileMultiStage(1, false);

        EXPECT_EQ(m_jobDetailsList.size(), 2);
        EXPECT_TRUE(m_jobDetailsList[1].m_autoFail);

        EXPECT_EQ(m_jobDetailsList[1].m_jobEntry.m_databaseSourceName, "test.stage1");
    }

    TEST_F(IntermediateAssetTests, CopyJob_Works)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage1", false, ProductOutputFlags::ProductAsset); // Copy jobs are ok

        ProcessFileMultiStage(1, false);

        auto expectedProduct = AZ::IO::Path(m_tempDir.GetDirectory()) / "Cache" / "pc" / "test.stage1";

        EXPECT_EQ(m_jobDetailsList.size(), 1);
        EXPECT_TRUE(AZ::IO::SystemFile::Exists(expectedProduct.c_str())) << expectedProduct.c_str();
    }

    TEST_F(IntermediateAssetTests, DeleteSourceIntermediate_DeletesAllProducts)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        ProcessFileMultiStage(3, true);

        AZ::IO::SystemFile::Delete(m_testFilePath.c_str());
        m_assetProcessorManager->AssessDeletedFile(m_testFilePath.c_str());
        RunFile(0);

        CheckIntermediate("test.stage2", false);
        CheckIntermediate("test.stage3", false);
        CheckProduct("test.stage4", false);
    }

    void IntermediateAssetTests::DeleteIntermediateTest(const char* deleteFilePath)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        ProcessFileMultiStage(3, true);

        AZ::IO::SystemFile::Delete(deleteFilePath);
        m_assetProcessorManager->AssessDeletedFile(deleteFilePath);
        RunFile(0); // Process the delete

        // Reprocess the file
        m_jobDetailsList.clear();

        // AssessModifiedFile is going to set up a OneShotTimer with a 1ms delay on it.  We have to wait a short time for that timer to
        // elapse before we can process that event. If we use the alternative processEvents that loops for X milliseconds we could
        // accidentally process too many events.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2));

        // Unfortunately we need to just process the events a few times without doing any checks here
        // due to the previous step queuing work which is sometimes executed immediately.
        // Without a way to consistently be sure whether the work has been done or not, we need to just run enough until the job is emitted

        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();

        ASSERT_EQ(m_jobDetailsList.size(), 1);

        ProcessJob(*m_rc, m_jobDetailsList[0]);

        ASSERT_TRUE(m_fileCompiled);

        m_assetProcessorManager->AssetProcessed(m_processedJobEntry, m_processJobResponse);

        CheckIntermediate("test.stage2");
        CheckIntermediate("test.stage3");
        CheckProduct("test.stage4");
    }

    TEST_F(IntermediateAssetTests, DeleteIntermediateProduct_Reprocesses)
    {
        DeleteIntermediateTest(MakePath("test.stage2", true).c_str());
    }

    TEST_F(IntermediateAssetTests, DeleteFinalProduct_Reprocesses)
    {
        DeleteIntermediateTest(MakePath("test.stage4", false).c_str());
    }

    TEST_F(IntermediateAssetTests, Override_NormalFileProcessedFirst_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        // Make and process a source file which matches an intermediate output name we will create later
        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test.stage2";
        AZStd::string testFilePath = (scanFolderDir / testFilename).AsPosix();

        UnitTestUtils::CreateDummyFile(testFilePath.c_str(), "unit test file");

        ProcessFileMultiStage(3, true, testFilePath.c_str(), 2);

        // Now process another file which produces intermediates that conflict with the existing source file above
        // Only go to stage 1 since we're expecting a failure at that point
        ProcessFileMultiStage(1, false);

        EXPECT_EQ(m_jobDetailsList.size(), 2);
        EXPECT_TRUE(m_jobDetailsList[1].m_autoFail);

        EXPECT_EQ(m_jobDetailsList[1].m_jobEntry.m_databaseSourceName, "test.stage1");
    }

    TEST_F(IntermediateAssetTests, DeleteFileInIntermediateFolder_CorrectlyDeletesOneFile)
    {
        using namespace AzToolsFramework::AssetDatabase;

        // Set up the test files and database entries
        SourceDatabaseEntry source1{ m_scanfolder.m_scanFolderID, "folder/parent.txt", AZ::Uuid::CreateRandom(), "fingerprint" };
        SourceDatabaseEntry source2{ m_platformConfig->GetIntermediateAssetsScanFolderId().value(), "folder/child.txt", AZ::Uuid::CreateRandom(), "fingerprint" };

        auto sourceFile = AZ::IO::Path(m_scanfolder.m_scanFolder) / "folder/parent.txt";
        auto intermediateFile = MakePath("folder/child.txt", true);
        auto cacheFile = MakePath("pc/folder/product.txt", false);
        auto cacheFile2 = MakePath("pc/folder/product777.txt", false);
        UnitTestUtils::CreateDummyFile(sourceFile.Native().c_str(), QString("tempdata"));
        UnitTestUtils::CreateDummyFile(intermediateFile.c_str(), QString("tempdata"));
        UnitTestUtils::CreateDummyFile(cacheFile.c_str(), QString("tempdata"));
        UnitTestUtils::CreateDummyFile(cacheFile2.c_str(), QString("tempdata"));

        ASSERT_TRUE(m_stateData->SetSource(source1));
        ASSERT_TRUE(m_stateData->SetSource(source2));

        JobDatabaseEntry job1{ source1.m_sourceID,
                               "Mock Job",
                               1234,
                               "pc",
                               m_builderInfoHandler.m_builderDescMap.begin()->second.m_busId,
                               AzToolsFramework::AssetSystem::JobStatus::Completed,
                               999 };

        JobDatabaseEntry job2{ source2.m_sourceID,
                               "Mock Job",
                               1234,
                               "pc",
                               m_builderInfoHandler.m_builderDescMap.begin()->second.m_busId,
                               AzToolsFramework::AssetSystem::JobStatus::Completed,
                               888 };

        ASSERT_TRUE(m_stateData->SetJob(job1));
        ASSERT_TRUE(m_stateData->SetJob(job2));

        ProductDatabaseEntry product1{ job1.m_jobID,
                                       0,
                                       "pc/folder/product.txt",
                                       AZ::Uuid::CreateName("one"),
                                       AZ::Uuid::CreateName("product.txt"),
                                       0,
                                       static_cast<int>(AssetBuilderSDK::ProductOutputFlags::ProductAsset) };
        ProductDatabaseEntry product2{ job2.m_jobID,
                                       777,
                                       "pc/folder/product777.txt",
                                       AZ::Uuid::CreateName("two"),
                                       AZ::Uuid::CreateName("product777.txt"),
                                       0,
                                       static_cast<int>(AssetBuilderSDK::ProductOutputFlags::ProductAsset) };

        ASSERT_TRUE(m_stateData->SetProduct(product1));
        ASSERT_TRUE(m_stateData->SetProduct(product2));

        // Record the folder so its marked as a known folder
        auto folderPath = MakePath("folder", true);
        m_assetProcessorManager->RecordFoldersFromScanner(
            QSet{ AssetProcessor::AssetFileInfo{ folderPath.c_str(), QDateTime::currentDateTime(), 0,
                                                 m_platformConfig->GetScanFolderForFile(folderPath.c_str()), true } });

        // Delete the file and folder in the intermediate folder
        AZ::IO::LocalFileIO::GetInstance()->DestroyPath(folderPath.c_str());

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessDeletedFile", Qt::QueuedConnection,
            Q_ARG(QString, MakePath("folder", true).c_str()));
        QCoreApplication::processEvents();

        RunFile(0);

        // Only 1 file (the one in the intermediate folder) should be marked for delete
        m_assetProcessorManager->CheckActiveFiles(1);
        m_assetProcessorManager->CheckFilesToExamine(0);
        m_assetProcessorManager->CheckJobEntries(0);
    }

    TEST_F(IntermediateAssetTests, Override_IntermediateFileProcessedFirst_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        // Process a file from stage1 -> stage4, this will create several intermediates
        ProcessFileMultiStage(3, true);

        // Now make a source file which is the same name as an existing intermediate and process it
        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test.stage2";
        AZStd::string testFilePath = (scanFolderDir / testFilename).AsPosix();

        UnitTestUtils::CreateDummyFile(testFilePath.c_str(), "unit test file");

        ProcessFileMultiStage(3, true, testFilePath.c_str(), 2, true);

        EXPECT_EQ(m_jobDetailsList.size(), 1);
        EXPECT_FALSE(m_jobDetailsList[0].m_autoFail);
    }

    TEST_F(IntermediateAssetTests, DuplicateOutputs_CausesFailure)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset, true);
        CreateBuilder("stage2", "*.stage2", "stage3", false, ProductOutputFlags::ProductAsset);

        ProcessFileMultiStage(2, true, nullptr, 1, false, true);

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "test2.stage1";

        UnitTestUtils::CreateDummyFile((scanFolderDir / testFilename).c_str(), "unit test file");

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, (scanFolderDir / testFilename).c_str()));
        QCoreApplication::processEvents();

        RunFile(1);
        ProcessJob(*m_rc, m_jobDetailsList[0]);

        ASSERT_TRUE(m_fileCompiled);

        m_jobDetailsList.clear();

        m_assetProcessorManager->AssetProcessed(m_processedJobEntry, m_processJobResponse);

        EXPECT_EQ(m_jobDetailsList.size(), 1);
        EXPECT_TRUE(m_jobDetailsList[0].m_autoFail);
    }

    TEST_F(IntermediateAssetTests, SourceAsset_SourceDependencyOnIntermediate_Reprocesses)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", false, ProductOutputFlags::ProductAsset);

        // Builder for the normal file, with a source dependency on the .stage2 intermediate
        m_builderInfoHandler.CreateBuilderDesc(
            "normal file builder", AZ::Uuid::CreateName("normal file builder").ToFixedString().c_str(),
            { AssetBuilderPattern{ "*.test", AssetBuilderPattern::Wildcard } },
            UnitTests::MockMultiBuilderInfoHandler::AssetBuilderExtraInfo{ "", "test.stage2", "", "", {} });

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "one.test";

        UnitTestUtils::CreateDummyFile((scanFolderDir / testFilename).c_str(), "unit test file");

        // Process the intermediate-style file first
        ProcessFileMultiStage(2, true);
        // Process the regular source second
        ProcessFileMultiStage(1, false, (scanFolderDir / testFilename).c_str());

        // Modify the intermediate-style file so it will be processed again
        QFile writer(m_testFilePath.c_str());
        ASSERT_TRUE(writer.open(QFile::WriteOnly));

        {
            QTextStream ts(&writer);
            ts.setCodec("UTF-8");
            ts << "modified test file";
        }

        // Start processing the test.stage1 file again
        QMetaObject::invokeMethod(m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, m_testFilePath.c_str()));
        QCoreApplication::processEvents();

        // Process test.stage1, which should queue up test.stage2
        ProcessSingleStep();
        // Start processing test.stage2, this should cause one.test to also be placed in the processing queue
        RunFile(1, 1, 1);
    }

    TEST_F(IntermediateAssetTests, IntermediateAsset_SourceDependencyOnSourceAsset_Reprocesses)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);

        m_builderInfoHandler.CreateBuilderDesc(
            "stage2", AZ::Uuid::CreateRandom().ToFixedString().c_str(), { AssetBuilderPattern{ "*.stage2", AssetBuilderPattern::Wildcard } },
            CreateJobStage("stage2", false, "one.test"),
            ProcessJobStage("stage3", ProductOutputFlags::ProductAsset, false), "fingerprint");

        CreateBuilder("normal file builder", "*.test", "test", false, ProductOutputFlags::ProductAsset);

        AZ::IO::Path scanFolderDir(m_scanfolder.m_scanFolder);
        AZStd::string testFilename = "one.test";

        UnitTestUtils::CreateDummyFile((scanFolderDir / testFilename).c_str(), "unit test file");

        // Process the normal source first
        ProcessFileMultiStage(1, false, (scanFolderDir / testFilename).c_str());
        // Process the intermediate-style source second
        ProcessFileMultiStage(2, true);

        // Modify the normal source so it will be processed again
        QFile writer((scanFolderDir / testFilename).c_str());
        ASSERT_TRUE(writer.open(QFile::WriteOnly));

        {
            QTextStream ts(&writer);
            ts.setCodec("UTF-8");
            ts << "modified test file";
        }

        // Start processing the one.test file again
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, (scanFolderDir / testFilename).c_str()));
        QCoreApplication::processEvents();

        // Start processing one.test, this should cause test.stage2 to also be placed in the processing queue
        RunFile(1, 1, 1);
    }

    TEST_F(IntermediateAssetTests, RequestReprocess_ReprocessesAllIntermediates)
    {
        using namespace AssetBuilderSDK;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        ProcessFileMultiStage(3, true);

        EXPECT_EQ(m_assetProcessorManager->RequestReprocess(m_testFilePath.c_str()), 3);
        EXPECT_EQ(m_assetProcessorManager->RequestReprocess(MakePath("test.stage2", true).c_str()), 3);
    }

    TEST_F(IntermediateAssetTests, PriorProductsAreCleanedUp)
    {
        using namespace AssetBuilderSDK;
        int counter = 0;

        CreateBuilder("stage1", "*.stage1", "stage2", true, ProductOutputFlags::IntermediateAsset);
        // Custom builder with random output
        m_builderInfoHandler.CreateBuilderDesc(
            "stage2",
            AZ::Uuid::CreateRandom().ToFixedString().c_str(),
            { AssetBuilderPattern{ "*.stage2", AssetBuilderPattern::Wildcard } },
            CreateJobStage("stage2", true),
            [&counter](const ProcessJobRequest& request, ProcessJobResponse& response)
            {
                ++counter;

                AZ::IO::Path outputFile = request.m_sourceFile;
                outputFile.ReplaceExtension(AZStd::string::format("stage3_%d", counter).c_str());

                AZ::IO::LocalFileIO::GetInstance()->Copy(
                    request.m_fullPath.c_str(), (AZ::IO::Path(request.m_tempDirPath) / outputFile).c_str());

                auto product = JobProduct{ outputFile.c_str(), AZ::Data::AssetType::CreateName("stage2"), 1 };

                product.m_outputFlags = ProductOutputFlags::IntermediateAsset;
                product.m_dependenciesHandled = true;
                response.m_outputProducts.push_back(product);

                response.m_resultCode = ProcessJobResult_Success;
            },
            "fingerprint");
        CreateBuilder("stage3", "*.stage3_*", "stage4", false, ProductOutputFlags::ProductAsset);

        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, m_testFilePath.c_str()));
        QCoreApplication::processEvents();

        // Process test.stage1, which should queue up test.stage2
        // We're going to do this manually instead of using the helper because this test uses a different file naming convention
        ProcessSingleStep();
        CheckIntermediate("test.stage2");
        ProcessSingleStep();
        CheckIntermediate("test.stage3_1");
        ProcessSingleStep();
        CheckProduct("test.stage4");

        // Modify the source file
        UnitTestUtils::CreateDummyFile(m_testFilePath.c_str(), "modified unit test file");

        // Run again, this time expecting the stage3_2 to be output instead of stage3_1
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, m_testFilePath.c_str()));
        QCoreApplication::processEvents();

        ProcessSingleStep();
        CheckIntermediate("test.stage2");
        ProcessSingleStep();
        CheckIntermediate("test.stage3_1", false); // Prior intermediate is deleted
        CheckIntermediate("test.stage3_2", true); // New intermediate created
        ProcessSingleStep();
        CheckProduct("test.stage4"); // Same product result at the end
    }

    TEST_F(IntermediateAssetTests, UpdateSource_OutputDoesntChange_IntermediateDoesNotReprocess)
    {
        using namespace AssetBuilderSDK;

        // Custom builder with fixed product output
        m_builderInfoHandler.CreateBuilderDesc(
            "stage1",
            AZ::Uuid::CreateRandom().ToFixedString().c_str(),
            { AssetBuilderPattern{ "*.stage1", AssetBuilderPattern::Wildcard } },
            CreateJobStage("stage1", true),
            [](const ProcessJobRequest& request, ProcessJobResponse& response)
            {
                AZ::IO::Path outputFile = request.m_sourceFile;
                outputFile.ReplaceExtension("stage2");

                auto* fileIo = AZ::IO::LocalFileIO::GetInstance();
                AZ::IO::HandleType fileHandle;
                AZStd::string data = "hello world"; // We'll output the same product every time no matter what the input source is

                fileIo->Open((AZ::IO::Path(request.m_tempDirPath) / outputFile).c_str(), AZ::IO::OpenMode::ModeWrite, fileHandle);
                fileIo->Write(fileHandle, data.data(), data.size());
                fileIo->Close(fileHandle);

                auto product = JobProduct{ outputFile.c_str(), AZ::Data::AssetType::CreateName("stage2"), 1 };

                product.m_outputFlags = ProductOutputFlags::IntermediateAsset;
                product.m_dependenciesHandled = true;
                response.m_outputProducts.push_back(product);

                response.m_resultCode = ProcessJobResult_Success;
            }, "fingerprint");

        CreateBuilder("stage2", "*.stage2", "stage3", true, ProductOutputFlags::IntermediateAsset);
        CreateBuilder("stage3", "*.stage3", "stage4", false, ProductOutputFlags::ProductAsset);

        // Process once
        ProcessFileMultiStage(3, true);

        // Modify the source file
        UnitTestUtils::CreateDummyFile(m_testFilePath.c_str(), "modified unit test file");

        // Start processing the test.stage1 file again
        QMetaObject::invokeMethod(
            m_assetProcessorManager.get(), "AssessAddedFile", Qt::QueuedConnection, Q_ARG(QString, m_testFilePath.c_str()));
        QCoreApplication::processEvents();

        // Process test.stage1, which should queue up test.stage2
        ProcessSingleStep();
        // Start processing test.stage2, this shouldn't create a job since the input is the same
        RunFile(0, 1, 0);
    }
} // namespace UnitTests
