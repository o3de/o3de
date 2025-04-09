/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/AssetProcessorTest.h>
#include <native/tests/UnitTestUtilities.h>
#include <native/resourcecompiler/rcjob.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <tests/assetmanager/AssetManagerTestingBase.h>
#include <native/utilities/AssetServerHandler.h>

namespace UnitTests
{
    using namespace testing;
    using ::testing::NiceMock;

    using namespace AssetProcessor;
    using namespace AssetBuilderSDK;

    class IgnoreNotifyTracker : public ProcessingJobInfoBus::Handler
    {
    public:
        // Will notify other systems which old product is just about to get removed from the cache
        // before we copy the new product instead along.
        void BeginCacheFileUpdate(const char* productPath) override
        {
            m_capturedStartPaths.push_back(productPath);
        }

        // Will notify other systems which product we are trying to copy in the cache
        // along with status of whether that copy succeeded or failed.
        void EndCacheFileUpdate(const char* productPath, bool /*queueAgainForProcessing*/) override
        {
            m_capturedStopPaths.push_back(productPath);
        }

        AZStd::vector<AZStd::string> m_capturedStartPaths;
        AZStd::vector<AZStd::string> m_capturedStopPaths;
    };

    class RCJobTest : public AssetProcessorTest
    {
    public:
        void SetUp() override
        {
            AssetProcessorTest::SetUp();
            m_data.reset(new StaticData());
            m_data->tempDirPath = QDir(m_data->m_tempDir.path());
            m_data->m_absolutePathToTempInputFolder = m_data->tempDirPath.absoluteFilePath("InputFolder").toUtf8().constData();
            // note that the case of OutputFolder is intentionally upper/lower case because
            // while files inside the output folder should be lowercased, the path to there should not be lowercased by RCJob.
            m_data->m_absolutePathToTempOutputFolder = m_data->tempDirPath.absoluteFilePath("OutputFolder").toUtf8().constData();
            m_data->tempDirPath.mkpath(QString::fromUtf8(m_data->m_absolutePathToTempInputFolder.c_str()));
            m_data->m_notifyTracker.BusConnect();

            // this can be overridden in each test but if you don't override it, then this fixture will do it.
            ON_CALL(m_data->m_diskSpaceResponder, CheckSufficientDiskSpace(_, _))
                .WillByDefault(Return(true));
        }

        void TearDown() override
        {
            m_data->m_notifyTracker.BusDisconnect();
            m_data.reset();
            AssetProcessorTest::TearDown();
        }
    protected:

        struct StaticData
        {
            QTemporaryDir m_tempDir;
            QDir tempDirPath;
            AZStd::string m_absolutePathToTempInputFolder;
            AZStd::string m_absolutePathToTempOutputFolder;
            AzToolsFramework::UuidUtilComponent m_uuidUtil;
            NiceMock<MockDiskSpaceResponder> m_diskSpaceResponder;
            IgnoreNotifyTracker m_notifyTracker;
        };

        AZStd::unique_ptr<StaticData> m_data;
    };


    TEST_F(RCJobTest, CopyCompiledAssets_NoWorkToDo_Succeeds)
    {
        BuilderParams builderParams;
        ProcessJobResponse response;
        EXPECT_TRUE(RCJob::CopyCompiledAssets(builderParams, response));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numWarningsAbsorbed, 0);
    }

    TEST_F(RCJobTest, CopyCompiledAssets_InvalidOutputPath_FailsAndAsserts)
    {
        BuilderParams builderParams;
        ProcessJobResponse response;
        response.m_resultCode = ProcessJobResult_Success;
        response.m_outputProducts.push_back({ "file1.txt" }); // make sure that there is at least one product so that it doesn't early out.

        // set only the input path, not the output path:
        builderParams.m_processJobRequest.m_tempDirPath = m_data->m_absolutePathToTempInputFolder.c_str(); // input working scratch space folder

        EXPECT_FALSE(RCJob::CopyCompiledAssets(builderParams, response));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 1);
    }

    TEST_F(RCJobTest, CopyCompiledAssets_InvalidInputPath_FailsAndAsserts)
    {
        BuilderParams builderParams;
        ProcessJobResponse response;
        response.m_resultCode = ProcessJobResult_Success;
        response.m_outputProducts.push_back({ "file1.txt" }); // make sure that there is at least one product so that it doesn't early out.

        // set the input dir to be a broken invalid dir:
        builderParams.m_processJobRequest.m_tempDirPath = AZ::Uuid::CreateRandom().ToString<AZStd::string>();
        builderParams.m_cacheOutputDir = m_data->m_absolutePathToTempOutputFolder;  // output folder in the 'cache'
        builderParams.m_intermediateOutputDir = AssetUtilities::GetIntermediateAssetsFolder(m_data->m_absolutePathToTempOutputFolder.c_str());

        EXPECT_FALSE(RCJob::CopyCompiledAssets(builderParams, response));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 1);
    }

    TEST_F(RCJobTest, CopyCompiledAssets_TooLongPath_FailsButDoesNotAssert)
    {
        BuilderParams builderParams;
        ProcessJobResponse response;
        response.m_resultCode = ProcessJobResult_Success;
        // set only the output path, but not the input path:
        builderParams.m_processJobRequest.m_tempDirPath = m_data->m_absolutePathToTempInputFolder.c_str(); // input working scratch space folder
        builderParams.m_cacheOutputDir = m_data->m_absolutePathToTempOutputFolder;  // output folder in the 'cache'
        builderParams.m_intermediateOutputDir = AssetUtilities::GetIntermediateAssetsFolder(m_data->m_absolutePathToTempOutputFolder.c_str());

        // give it an overly long file name:
        AZStd::string reallyLongFileName;
        reallyLongFileName.resize(4096, 'x');
        response.m_outputProducts.push_back({ reallyLongFileName.c_str() });

        EXPECT_FALSE(RCJob::CopyCompiledAssets(builderParams, response));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 1);
    }

    TEST_F(RCJobTest, CopyCompiledAssets_OutOfDiskSpace_FailsButDoesNotAssert)
    {
        BuilderParams builderParams;
        ProcessJobResponse response;
        response.m_resultCode = ProcessJobResult_Success;
        // set only the output path, but not the input path:
        builderParams.m_processJobRequest.m_tempDirPath = m_data->m_absolutePathToTempInputFolder.c_str(); // input working scratch space folder
        builderParams.m_cacheOutputDir = m_data->m_absolutePathToTempOutputFolder;  // output folder in the 'cache'
        builderParams.m_intermediateOutputDir =
            AssetUtilities::GetIntermediateAssetsFolder(m_data->m_absolutePathToTempOutputFolder.c_str());
        response.m_resultCode = ProcessJobResult_Success;
        response.m_outputProducts.push_back({ "file1.txt" }); // make sure that there is at least one product so that it doesn't early out.
        UnitTestUtils::CreateDummyFile(QDir(m_data->m_absolutePathToTempInputFolder.c_str()).absoluteFilePath("file1.txt"), "output of file 1");
        response.m_outputProducts.push_back({ "file2.txt" }); // make sure that there is at least one product so that it doesn't early out.
        UnitTestUtils::CreateDummyFile(QDir(m_data->m_absolutePathToTempInputFolder.c_str()).absoluteFilePath("file2.txt"), "output of file 2");

        // we expect exactly one call to check for disk space, (not once for each file), and in this case, we'll return false.
        EXPECT_CALL(m_data->m_diskSpaceResponder, CheckSufficientDiskSpace(_,_))
            .Times(1)
            .WillRepeatedly(Return(false));

        EXPECT_FALSE(RCJob::CopyCompiledAssets(builderParams, response));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 1);

        // no notifies should be hit since the operation should not have been attempted at all (disk space should be checked up front)
        ASSERT_EQ(m_data->m_notifyTracker.m_capturedStartPaths.size(), 0);
        ASSERT_EQ(m_data->m_notifyTracker.m_capturedStopPaths.size(), 0);

        // no cached files should have been copied at all.
        QString expectedFinalOutputPath = QDir(QString::fromUtf8(m_data->m_absolutePathToTempOutputFolder.c_str())).absoluteFilePath("file1.txt");
        EXPECT_FALSE(QFile::exists(expectedFinalOutputPath));
        expectedFinalOutputPath = QDir(QString::fromUtf8(m_data->m_absolutePathToTempOutputFolder.c_str())).absoluteFilePath("file2.txt");
        EXPECT_FALSE(QFile::exists(expectedFinalOutputPath));
    }

    // The RC Copy Compiled Assets routine is supposed to check up front for problem situations such as out of disk space
    // or missing source files, before it tries to perform any operation.  This test gives it one file which does work
    // but one missing file also, and expects it to fail (without asserting) but without even trying to copy the files at all.
    TEST_F(RCJobTest, CopyCompiledAssets_MissingInputFile_Fails_DoesNotAssert_DoesNotAlterCache)
    {
        BuilderParams builderParams;
        ProcessJobResponse response;
        response.m_resultCode = ProcessJobResult_Success;
        // set only the output path, but not the input path:
        builderParams.m_processJobRequest.m_tempDirPath = m_data->m_absolutePathToTempInputFolder.c_str(); // input working scratch space folder
        builderParams.m_cacheOutputDir = m_data->m_absolutePathToTempOutputFolder;  // output folder in the 'cache'
        builderParams.m_intermediateOutputDir = AssetUtilities::GetIntermediateAssetsFolder(m_data->m_absolutePathToTempOutputFolder.c_str());
        response.m_resultCode = ProcessJobResult_Success;
        response.m_outputProducts.push_back({ "FiLe1.TxT" }); // make sure that there is at least one product so that it doesn't early out.
        UnitTestUtils::CreateDummyFile(QDir(m_data->m_absolutePathToTempInputFolder.c_str()).absoluteFilePath("FiLe1.TxT"), "output of file 1");
        response.m_outputProducts.push_back({ "FiLe2.txt" });
        // note well that we create the first file but we don't acutally create the second one, so it is missing.

        EXPECT_FALSE(RCJob::CopyCompiledAssets(builderParams, response));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 1);

        // no notifies should be hit since the operation should not have been attempted at all.
        ASSERT_EQ(m_data->m_notifyTracker.m_capturedStartPaths.size(), 0);
        ASSERT_EQ(m_data->m_notifyTracker.m_capturedStopPaths.size(), 0);
        QString expectedFinalOutputPath = QDir(QString::fromUtf8(m_data->m_absolutePathToTempOutputFolder.c_str())).absoluteFilePath("file1.txt");
        EXPECT_FALSE(QFile::exists(expectedFinalOutputPath));
    }

    TEST_F(RCJobTest, CopyCompiledAssets_AbsolutePath_SucceedsAndNotifiesAboutCacheDelete)
    {
        BuilderParams builderParams;
        ProcessJobResponse response;
        response.m_resultCode = ProcessJobResult_Success;
        // set only the output path, but not the input path:
        builderParams.m_processJobRequest.m_tempDirPath = m_data->m_absolutePathToTempInputFolder.c_str(); // input working scratch space folder
        builderParams.m_cacheOutputDir = m_data->m_absolutePathToTempOutputFolder;  // output folder in the 'cache'
        builderParams.m_intermediateOutputDir = AssetUtilities::GetIntermediateAssetsFolder(m_data->m_absolutePathToTempOutputFolder.c_str());
        builderParams.m_relativePath = "";
        response.m_resultCode = ProcessJobResult_Success;

        // make up a completely different random path to put an absolute file in:
        QTemporaryDir extraDir;
        QDir randomDir(extraDir.path());
        randomDir.mkpath(extraDir.path());
        QString absolutePathToCreate = randomDir.absoluteFilePath("someabsolutefile.txt");
        UnitTestUtils::CreateDummyFile(absolutePathToCreate, "output of the file");
        response.m_outputProducts.push_back({ absolutePathToCreate.toUtf8().constData() }); // absolute path to file not actually in the product scratch space folder.

        // this should copy that file into the target path.
        EXPECT_TRUE(RCJob::CopyCompiledAssets(builderParams, response));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);
        ASSERT_EQ(m_data->m_notifyTracker.m_capturedStartPaths.size(), 1);
        ASSERT_EQ(m_data->m_notifyTracker.m_capturedStopPaths.size(), 1);

        // note that output files are automatically lowercased within the cache but the path to the cache folder itself is not lowered, just the output file.
        // this is to make sure that game code never has to worry about the casing of output file paths, CRYPAK can just always lower the relpath and always know
        // that even on case-sensitive platforms it won't cause trouble or a difference of behavior from non-case-sensitive ones.
        QString expectedFinalOutputPath = QDir(QString::fromUtf8(m_data->m_absolutePathToTempOutputFolder.c_str())).absoluteFilePath("someabsolutefile.txt");
        ASSERT_STREQ(m_data->m_notifyTracker.m_capturedStartPaths[0].c_str(), m_data->m_notifyTracker.m_capturedStopPaths[0].c_str());
        ASSERT_STREQ(m_data->m_notifyTracker.m_capturedStartPaths[0].c_str(), expectedFinalOutputPath.toUtf8().constData());
        EXPECT_TRUE(QFile::exists(expectedFinalOutputPath));
    }

    TEST_F(RCJobTest, CopyCompiledAssets_RelativePath_SucceedsAndNotifiesAboutCacheDelete)
    {
        BuilderParams builderParams;
        ProcessJobResponse response;
        response.m_resultCode = ProcessJobResult_Success;
        // set only the output path, but not the input path:
        builderParams.m_processJobRequest.m_tempDirPath = m_data->m_absolutePathToTempInputFolder.c_str(); // input working scratch space folder
        builderParams.m_cacheOutputDir = m_data->m_absolutePathToTempOutputFolder;  // output folder in the 'cache'
        builderParams.m_intermediateOutputDir = AssetUtilities::GetIntermediateAssetsFolder(m_data->m_absolutePathToTempOutputFolder.c_str());
        response.m_resultCode = ProcessJobResult_Success;
        response.m_outputProducts.push_back({ "FiLe1.TxT" }); // make sure that there is at least one product so that it doesn't early out.
        UnitTestUtils::CreateDummyFile(QDir(m_data->m_absolutePathToTempInputFolder.c_str()).absoluteFilePath("FiLe1.TxT"), "output of file 1");

        EXPECT_TRUE(RCJob::CopyCompiledAssets(builderParams, response));
        EXPECT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
        EXPECT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);
        ASSERT_EQ(m_data->m_notifyTracker.m_capturedStartPaths.size(), 1);
        ASSERT_EQ(m_data->m_notifyTracker.m_capturedStopPaths.size(), 1);

        // note that output files are automatically lowercased within the cache but the path to the cache folder itself is not lowered, just the output file.
        // this is to make sure that game code never has to worry about the casing of output file paths, CRYPAK can just always lower the relpath and always know
        // that even on case-sensitive platforms it won't cause trouble or a difference of behavior from non-case-sensitive ones.
        QString expectedFinalOutputPath = QDir(QString::fromUtf8(m_data->m_absolutePathToTempOutputFolder.c_str())).absoluteFilePath("file1.txt");
        ASSERT_STREQ(m_data->m_notifyTracker.m_capturedStartPaths[0].c_str(), m_data->m_notifyTracker.m_capturedStopPaths[0].c_str());
        ASSERT_STREQ(m_data->m_notifyTracker.m_capturedStartPaths[0].c_str(), expectedFinalOutputPath.toUtf8().constData());
        EXPECT_TRUE(QFile::exists(expectedFinalOutputPath));

        // Start and end paths should, however, be normalized even if the input is not.
        QString normalizedStartPath = QString::fromUtf8(m_data->m_notifyTracker.m_capturedStartPaths[0].c_str());
        normalizedStartPath = AssetUtilities::NormalizeFilePath(normalizedStartPath);
        EXPECT_STREQ(normalizedStartPath.toUtf8().constData(), m_data->m_notifyTracker.m_capturedStartPaths[0].c_str());

        QString normalizedStopPath = QString::fromUtf8(m_data->m_notifyTracker.m_capturedStopPaths[0].c_str());
        normalizedStopPath = AssetUtilities::NormalizeFilePath(normalizedStopPath);
        EXPECT_STREQ(normalizedStopPath.toUtf8().constData(), m_data->m_notifyTracker.m_capturedStopPaths[0].c_str());
    }

    TEST_F(RCJobTest, BeforeStoringJobResult_NonNormalPath_Succeeds)
    {
        auto serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        AssetBuilderSDK::ProcessJobResponse::Reflect(&*serializeContext);
        AssetBuilderSDK::JobProduct::Reflect(&*serializeContext);
        AzToolsFramework::AssetSystem::AssetJobLogResponse::Reflect(&*serializeContext);
        AzFramework::AssetSystem::BaseAssetProcessorMessage::Reflect(&*serializeContext);

        auto mockComponentApplication = NiceMock<::UnitTests::MockComponentApplication>();
        ON_CALL(mockComponentApplication, GetSerializeContext()).WillByDefault(Return(serializeContext.get()));

        AssetProcessor::RCJob rcJob;

        BuilderParams builderParams(&rcJob);
        builderParams.m_processJobRequest.m_tempDirPath = m_data->m_absolutePathToTempInputFolder.c_str(); // input working scratch space folder

        AZStd::string backSlashedPath = builderParams.m_processJobRequest.m_tempDirPath;
        AZStd::replace(backSlashedPath.begin(), backSlashedPath.end(), '/', '\\');

        AZStd::string frontSlashedPath = builderParams.m_processJobRequest.m_tempDirPath;
        AZStd::replace(frontSlashedPath.begin(), frontSlashedPath.end(), '\\', '/');

        AZStd::string mixedSlashedPath = builderParams.m_processJobRequest.m_tempDirPath;
        auto slashPos = mixedSlashedPath.find_first_of('\\');
        if (slashPos == AZStd::string::npos)
        {
            slashPos = mixedSlashedPath.find_first_of('/');
            ASSERT_TRUE(slashPos != AZStd::string::npos);
            mixedSlashedPath.replace(slashPos, 1, 1, '\\');
        }
        else
        {
            mixedSlashedPath.replace(slashPos, 1, 1, '/');
        }

        AssetBuilderSDK::ProcessJobResponse jobResponse;
        jobResponse.m_outputProducts.push_back({ (AZ::IO::Path(backSlashedPath) / "file1.txt").c_str() });
        jobResponse.m_outputProducts.push_back({ (AZ::IO::Path(frontSlashedPath) / "file2.txt").c_str() });
        jobResponse.m_outputProducts.push_back({ (AZ::IO::Path(mixedSlashedPath) / "file3.txt").c_str() });

        auto outcome = RCJob::BeforeStoringJobResult(builderParams, jobResponse);

        EXPECT_TRUE(outcome.IsSuccess());

        AZStd::string responseFilePath;
        AzFramework::StringFunc::Path::ConstructFull(builderParams.m_processJobRequest.m_tempDirPath.c_str(), AssetBuilderSDK::s_processJobResponseFileName, responseFilePath, true);
        const auto* responseOnDisk = AZ::Utils::LoadObjectFromFile<AssetBuilderSDK::ProcessJobResponse>(responseFilePath);
        ASSERT_TRUE(responseOnDisk);

        EXPECT_STREQ(responseOnDisk->m_outputProducts[0].m_productFileName.c_str(), (AZ::IO::Path("%TEMP%") / "file1.txt").c_str());
        EXPECT_STREQ(responseOnDisk->m_outputProducts[1].m_productFileName.c_str(), (AZ::IO::Path("%TEMP%") / "file2.txt").c_str());
        EXPECT_STREQ(responseOnDisk->m_outputProducts[2].m_productFileName.c_str(), (AZ::IO::Path("%TEMP%") / "file3.txt").c_str());

        delete responseOnDisk;
    }

    TEST_F(RCJobTest, RCJob_DoWork_SetsProductOutputFlagsCachedAssetAsServer)
    {
        struct TestRCJob
            : public RCJob
        {
            void DoWork(AssetBuilderSDK::ProcessJobResponse& result, BuilderParams& builderParams, AssetUtilities::QuitListener& listener) override
            {
                RCJob::DoWork(result, builderParams, listener);
            }
        };

        struct ProxyAssetServerBus
            : public AssetServerHandler
        {
            AssetServerMode GetRemoteCachingMode() const override
            {
                return AssetServerMode::Server;
            }

            bool StoreJobResult(const AssetProcessor::BuilderParams&, AZStd::vector<AZStd::string>&) override
            {
                return true;
            }
        };

        auto serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        AssetBuilderSDK::ProcessJobResponse::Reflect(&*serializeContext);
        AssetBuilderSDK::JobProduct::Reflect(&*serializeContext);
        AzToolsFramework::AssetSystem::AssetJobLogResponse::Reflect(&*serializeContext);
        AzFramework::AssetSystem::BaseAssetProcessorMessage::Reflect(&*serializeContext);

        auto mockComponentApplication = NiceMock<::UnitTests::MockComponentApplication>();
        ON_CALL(mockComponentApplication, GetSerializeContext()).WillByDefault(Return(serializeContext.get()));

        ProxyAssetServerBus proxyAssetServerBus;

        JobDetails jobDetails;
        jobDetails.m_jobEntry.m_jobRunKey = 1;
        jobDetails.m_checkServer = true;

        AssetProcessor::RCJob rcJob;
        rcJob.Init(jobDetails);

        BuilderParams builderParams;
        builderParams.m_rcJob = &rcJob;
        builderParams.m_assetBuilderDesc.m_processJobFunction = [](const ProcessJobRequest&, ProcessJobResponse& response)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            response.m_outputProducts.emplace_back(AssetBuilderSDK::JobProduct("outputfile_1.txt"));
            response.m_outputProducts.emplace_back(AssetBuilderSDK::JobProduct("outputfile_2.txt"));
        };

        AssetBuilderSDK::ProcessJobResponse result;
        AssetUtilities::QuitListener listener;

        TestRCJob testRcJob;
        testRcJob.Init(jobDetails);
        testRcJob.DoWork(result, builderParams, listener);

        EXPECT_EQ(result.m_outputProducts.size(), 2);
        EXPECT_EQ(result.m_outputProducts.at(0).m_outputFlags & ProductOutputFlags::CachedAsset, ProductOutputFlags::CachedAsset);
        EXPECT_EQ(result.m_outputProducts.at(1).m_outputFlags & ProductOutputFlags::CachedAsset, ProductOutputFlags::CachedAsset);
    }

} // end namespace UnitTests
