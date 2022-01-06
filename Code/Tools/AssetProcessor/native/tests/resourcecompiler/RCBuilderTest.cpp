/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RCBuilderTest.h"



TEST_F(RCBuilderTest, CreateBuilderDesc_CreateBuilder_Valid)
{
    AssetBuilderSDK::AssetBuilderPattern    pattern;
    pattern.m_pattern = "*.foo";
    AZStd::vector<AssetBuilderSDK::AssetBuilderPattern> builderPatterns;
    builderPatterns.push_back(pattern);

    TestInternalRecognizerBasedBuilder  test(new MockRCCompiler());

    AssetBuilderSDK::AssetBuilderDesc result = test.CreateBuilderDesc(this->GetBuilderID(), builderPatterns);

    ASSERT_EQ(this->GetBuilderName(), result.m_name);
    ASSERT_EQ(this->GetBuilderUUID(), result.m_busId);
    ASSERT_EQ(false, result.IsExternalBuilder());
    ASSERT_TRUE(result.m_patterns.size() == 1);
    ASSERT_EQ(result.m_patterns[0].m_pattern, pattern.m_pattern);
}

TEST_F(RCBuilderTest, Shutdown_NormalShutdown_Requested)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    test.ShutDown();

    ASSERT_EQ(mockRC->m_request_quit, 1);
}


TEST_F(RCBuilderTest, Initialize_StandardInitializationWithDuplicateAndInvalidRecognizers_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);
    MockRecognizerConfiguration         configuration;

    // 3 Asset recognizers, 1 duplicate & 1 without platform should result in only 1 InternalAssetRecognizer

    // Good spec
    AssetRecognizer     good;
    good.m_name = "Good";
    good.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   good_spec;
    good_spec.m_extraRCParams = "/i";
    good.m_platformSpecs["pc"] = good_spec;

    // No Platform spec
    AssetRecognizer     no_platform;
    no_platform.m_name = "No Platform";
    no_platform.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.ccc", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);

    // Duplicate
    AssetRecognizer     duplicate(good.m_name, good.m_testLockSource, good.m_priority, good.m_isCritical, good.m_supportsCreateJobs, good.m_patternMatcher, good.m_version, good.m_productAssetType, good.m_outputProductDependencies);
    duplicate.m_platformSpecs["pc"] = good_spec;

    configuration.m_recognizerContainer["good"] = good;
    configuration.m_recognizerContainer["no_platform"] = no_platform;
    configuration.m_recognizerContainer["duplicate"] = duplicate;

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    ASSERT_EQ(mockRC->m_initialize, 1);

    AZStd::vector<AssetBuilderSDK::PlatformInfo> platformInfos;
    AZStd::unordered_set<AZStd::string> tags;
    tags.insert("tools");
    tags.insert("desktop");
    platformInfos.emplace_back(AssetBuilderSDK::PlatformInfo("pc", tags));

    InternalRecognizerPointerContainer  good_recognizers;
    bool good_recognizers_found = test.GetMatchingRecognizers(platformInfos, "test.foo", good_recognizers);
    ASSERT_TRUE(good_recognizers_found); // Should find at least 1
    ASSERT_EQ(good_recognizers.size(), 1);  // 1, not 2 since the duplicates should be removed
    ASSERT_EQ(good_recognizers.at(0)->m_name, good.m_name); // Match the same recognizer


    InternalRecognizerPointerContainer  bad_recognizers;
    bool no_recognizers_found = !test.GetMatchingRecognizers(platformInfos, "test.ccc", good_recognizers);
    ASSERT_TRUE(no_recognizers_found);
    ASSERT_EQ(bad_recognizers.size(), 0);  // 1, not 2 since the duplicates should be removed

    ASSERT_EQ(m_errorAbsorber->m_numWarningsAbsorbed, 1); // this should be the "duplicate builder" warning.
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 0);
    ASSERT_EQ(m_errorAbsorber->m_numAssertsAbsorbed, 0);
}


TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobStandard_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration         configuration;

    AssetRecognizer     good;
    good.m_name = "Good";
    good.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   good_spec;
    good_spec.m_extraRCParams = "/i";
    good.m_platformSpecs["pc"] = good_spec;
    configuration.m_recognizerContainer["good"] = good;


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_watchFolder = "c:\temp";
    request.m_sourceFile = "test.foo";
    request.m_enabledPlatforms = { AssetBuilderSDK::PlatformInfo("pc", { "desktop", "renderer" }) };
    AssetProcessor::BUILDER_ID_RC.GetUuid(request.m_builderid);

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::Success);
    ASSERT_EQ(response.m_createJobOutputs.size(), 1);

    AssetBuilderSDK::JobDescriptor descriptor = response.m_createJobOutputs.at(0);
    ASSERT_EQ(descriptor.GetPlatformIdentifier(), "pc");
    ASSERT_FALSE(descriptor.m_critical);
}

TEST_F(RCBuilderTest, CreateJobs_CreateMultiplesJobStandard_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration         configuration;

    AssetRecognizer         standard_AR_RC;
    const AZStd::string     job_key_rc = "RCjob";
    {
        standard_AR_RC.m_name = "RCjob";
        standard_AR_RC.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        AssetPlatformSpec   good_rc_spec;
        good_rc_spec.m_extraRCParams = "/i";
        standard_AR_RC.m_platformSpecs["pc"] = good_rc_spec;
    }
    configuration.m_recognizerContainer["rc_foo"] = standard_AR_RC;

    AssetRecognizer         standard_AR_Copy;
    const AZStd::string     job_key_copy = "Copyjob";
    {
        standard_AR_Copy.m_name = QString(job_key_copy.c_str());
        standard_AR_Copy.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        AssetPlatformSpec   good_copy_spec;
        good_copy_spec.m_extraRCParams = "copy";
        standard_AR_Copy.m_platformSpecs["pc"] = good_copy_spec;
    }
    configuration.m_recognizerContainer["copy_foo"] = standard_AR_Copy;


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    // Request is for the copy builder
    {
        AssetBuilderSDK::CreateJobsRequest  request_copy;
        AssetBuilderSDK::CreateJobsResponse response_copy;

        request_copy.m_watchFolder = "c:\temp";
        request_copy.m_sourceFile = "test.foo";

        request_copy.m_enabledPlatforms = { AssetBuilderSDK::PlatformInfo("pc",{ "desktop", "renderer" }) };
        AssetProcessor::BUILDER_ID_COPY.GetUuid(request_copy.m_builderid);
        test.CreateJobs(request_copy, response_copy);

        ASSERT_EQ(response_copy.m_result, AssetBuilderSDK::CreateJobsResultCode::Success);
        ASSERT_EQ(response_copy.m_createJobOutputs.size(), 1);

        AssetBuilderSDK::JobDescriptor descriptor = response_copy.m_createJobOutputs.at(0);
        ASSERT_EQ(descriptor.GetPlatformIdentifier(), "pc");
        ASSERT_EQ(descriptor.m_jobKey.compare(job_key_copy), 0);
        ASSERT_TRUE(descriptor.m_critical);
    }

    // Request is for the rc builder
    {
        AssetBuilderSDK::CreateJobsRequest  request_rc;
        AssetBuilderSDK::CreateJobsResponse response_rc;

        request_rc.m_watchFolder = "c:\temp";
        request_rc.m_sourceFile = "test.foo";
        request_rc.m_enabledPlatforms = { AssetBuilderSDK::PlatformInfo("pc",{ "desktop", "renderer" }) };
        AssetProcessor::BUILDER_ID_RC.GetUuid(request_rc.m_builderid);
        test.CreateJobs(request_rc, response_rc);

        ASSERT_EQ(response_rc.m_result, AssetBuilderSDK::CreateJobsResultCode::Success);
        ASSERT_EQ(response_rc.m_createJobOutputs.size(), 1);

        AssetBuilderSDK::JobDescriptor descriptor = response_rc.m_createJobOutputs.at(0);
        ASSERT_EQ(descriptor.GetPlatformIdentifier(), "pc");
        ASSERT_EQ(descriptor.m_jobKey.compare(job_key_rc), 0);
        ASSERT_FALSE(descriptor.m_critical);
    }

}

TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobCopy_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    AssetRecognizer     copy;
    copy.m_name = "Copy";
    copy.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.copy", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   copy_spec;
    copy_spec.m_extraRCParams = "copy";
    copy.m_platformSpecs["pc"] = copy_spec;
    configuration.m_recognizerContainer["copy"] = copy;



    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_watchFolder = "c:\temp";
    request.m_sourceFile = "test.copy";
    request.m_enabledPlatforms = { AssetBuilderSDK::PlatformInfo("pc",{ "desktop", "renderer" }) };
    AssetProcessor::BUILDER_ID_COPY.GetUuid(request.m_builderid);

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::Success);
    ASSERT_EQ(response.m_createJobOutputs.size(), 1);

    AssetBuilderSDK::JobDescriptor descriptor = response.m_createJobOutputs.at(0);
    ASSERT_EQ(descriptor.GetPlatformIdentifier(), "pc");
    ASSERT_TRUE(descriptor.m_critical);
}

TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobStandardSkip_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    {
        AssetRecognizer     skip;
        skip.m_name = "Skip";
        skip.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.skip", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        AssetPlatformSpec   skip_spec;
        skip_spec.m_extraRCParams = "skip";
        skip.m_platformSpecs["pc"] = skip_spec;
        configuration.m_recognizerContainer["skip"] = skip;
    }


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_watchFolder = "c:\temp";
    request.m_sourceFile = "test.skip";
    request.m_enabledPlatforms = { AssetBuilderSDK::PlatformInfo("pc",{ "desktop", "renderer" }) };
    request.m_builderid = this->GetBuilderUUID();

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::Success);
    ASSERT_EQ(response.m_createJobOutputs.size(), 0);
}


TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobStandard_Failed)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    AssetRecognizer     good;
    good.m_name = "Good";
    good.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   good_spec;
    good_spec.m_extraRCParams = "/i";
    good.m_platformSpecs["pc"] = good_spec;
    configuration.m_recognizerContainer["good"] = good;


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_watchFolder = "c:\temp";
    request.m_sourceFile = "test.ccc";
    request.m_enabledPlatforms = { AssetBuilderSDK::PlatformInfo("pc",{ "desktop", "renderer" }) };
    request.m_builderid = this->GetBuilderUUID();

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::Failed);
    ASSERT_EQ(response.m_createJobOutputs.size(), 0);
}

TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobStandard_ShuttingDown)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    AssetRecognizer     good;
    good.m_name = "Good";
    good.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   good_spec;
    good_spec.m_extraRCParams = "/i";
    good.m_platformSpecs["pc"] = good_spec;
    configuration.m_recognizerContainer["good"] = good;


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    test.ShutDown();

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_watchFolder = "c:\temp";
    request.m_sourceFile = "test.ccc";
    request.m_enabledPlatforms = { AssetBuilderSDK::PlatformInfo("pc",{ "desktop", "renderer" }) };
    request.m_builderid = this->GetBuilderUUID();

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::ShuttingDown);
    ASSERT_EQ(response.m_createJobOutputs.size(), 0);
}

TEST_F(RCBuilderTest, CreateJobs_CreateSingleJobBadJobRequest1_Failed)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration     configuration;

    AssetRecognizer     good;
    good.m_name = "Good";
    good.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
    AssetPlatformSpec   good_spec;
    good_spec.m_extraRCParams = "/i";
    good.m_platformSpecs["pc"] = good_spec;
    configuration.m_recognizerContainer["good"] = good;


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    AssetBuilderSDK::CreateJobsRequest  request;
    AssetBuilderSDK::CreateJobsResponse response;

    request.m_sourceFile = "test.ccc";
    request.m_enabledPlatforms = { AssetBuilderSDK::PlatformInfo("pc",{ "desktop", "renderer" }) };
    request.m_builderid = this->GetBuilderUUID();

    test.CreateJobs(request, response);

    ASSERT_EQ(response.m_result, AssetBuilderSDK::CreateJobsResultCode::Failed);
    ASSERT_EQ(response.m_createJobOutputs.size(), 0);
}

TEST_F(RCBuilderTest, ProcessLegacyRCJob_ProcessStandardSingleJob_Failed)
{
    AZ::Uuid                            assetTypeUUid = AZ::Uuid::CreateRandom();
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration         configuration;

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("file.c", false, "pc", 1);

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    // Case 1: execution failed
    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(1, true, ""));
    mockRC->SetResultExecute(false);
    AssetBuilderSDK::ProcessJobResponse responseCrashed;
    AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
    test.TestProcessLegacyRCJob(request, "/i", assetTypeUUid, jobCancelListener, responseCrashed);
    ASSERT_EQ(responseCrashed.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Crashed);

    // case 2: result code from execution non-zero
    mockRC->SetResultExecute(true);
    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(1, false, ""));
    AssetBuilderSDK::ProcessJobResponse responseFailed;
    test.TestProcessLegacyRCJob(request, "/i", assetTypeUUid, jobCancelListener, responseFailed);
    ASSERT_EQ(responseFailed.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Failed);
}


TEST_F(RCBuilderTest, ProcessLegacyRCJob_ProcessStandardSingleJob_Valid)
{
    AZ::Uuid                            assetTypeUUid = AZ::Uuid::CreateRandom();
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration         configuration;

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("file.c", false, "pc");


    test.AddTestFileInfo("c:\\temp\\file.a").AddTestFileInfo("c:\\temp\\file.b");


    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(0, false, "c:\\temp"));
    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
    test.TestProcessLegacyRCJob(request, "/i", assetTypeUUid, jobCancelListener, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);

    ASSERT_EQ(response.m_outputProducts.size(), 2); // file.c->(file.a, file.b)
}


TEST_F(RCBuilderTest, ProcessLegacyRCJob_ProcessCopySingleJob_Valid)
{
    AZStd::string                       name = "test";
    AZ::Uuid                            assetTypeUUid = AZ::Uuid::CreateRandom();
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);

    MockRecognizerConfiguration         configuration;

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("file.c", false, "pc");

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(0, false, "c:\\temp"));
    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);
    test.TestProcessCopyJob(request, assetTypeUUid, jobCancelListener, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);

    ASSERT_EQ(response.m_outputProducts.size(), 1); // file.c->(file.a, file.b)
    AssetBuilderSDK::JobProduct resultJobProd = response.m_outputProducts.at(0);
    ASSERT_EQ(resultJobProd.m_productAssetType, assetTypeUUid);
    ASSERT_EQ(resultJobProd.m_productFileName, request.m_fullPath);
}


TEST_F(RCBuilderTest, MatchTempFileToSkip_SkipRCFiles_true)
{
    const char* rc_skip_fileNames[] = {
        "rc_createdfiles.txt",
        "rc_log.log",
        "rc_log_warnings.log",
        "rc_log_errors.log"
    };

    for (const char* filename : rc_skip_fileNames)
    {
        ASSERT_TRUE(AssetProcessor::InternalRecognizerBasedBuilder::MatchTempFileToSkip(filename));
    }
}

TEST_F(RCBuilderTest, MatchTempFileToSkip_SkipRCFiles_false)
{
    const char* rc_not_skip_fileNames[] = {
        "foo.log",
        "bar.txt"
    };

    for (const char* filename : rc_not_skip_fileNames)
    {
        ASSERT_FALSE(AssetProcessor::InternalRecognizerBasedBuilder::MatchTempFileToSkip(filename));
    }
}



TEST_F(RCBuilderTest, ProcessJob_ProcessStandardRCSingleJob_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);
    MockRecognizerConfiguration         configuration;


    // Create a dummy test recognizer
    AZ::u32 recID = test.AddTestRecognizer(this->GetBuilderID(),QString("/i"), "pc");

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("test.tif", false, "pc");
    request.m_jobDescription.m_jobParameters[recID] = "/i";


    test.AddTestFileInfo("c:\\temp\\file.a").AddTestFileInfo("c:\\temp\\file.b");

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(0, false, "c:\\temp"));
    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    test.TestProcessJob(request, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);
    ASSERT_EQ(response.m_outputProducts.size(), 2); // file.c->(file.a, file.b)
}


TEST_F(RCBuilderTest, ProcessJob_ProcessStandardRCSingleJob_Failed)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);
    MockRecognizerConfiguration         configuration;


    // Create a dummy test recognizer
    AZ::u32 recID = test.AddTestRecognizer(this->GetBuilderID(), QString("/i"), "pc");

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("test.tif", false, "pc");
    request.m_jobDescription.m_jobParameters[recID] = "/i";

    test.AddTestFileInfo("c:\\temp\\file.a").AddTestFileInfo("c:\\temp\\file.b");

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultResultExecute(NativeLegacyRCCompiler::Result(1, false, "c:\\temp"));
    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    test.TestProcessJob(request, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Failed);
}


TEST_F(RCBuilderTest, ProcessJob_ProcessStandardCopySingleJob_Valid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);
    MockRecognizerConfiguration         configuration;


    // Create a dummy test recognizer
    AZ::u32 recID = test.AddTestRecognizer(this->GetBuilderID(), QString("copy"), "pc");

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("test.tif", true, "pc");
    request.m_jobDescription.m_jobParameters[recID] = "copy";

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    test.TestProcessJob(request, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);
    ASSERT_EQ(response.m_outputProducts.size(), 1); // test.
    ASSERT_TRUE(response.m_outputProducts[0].m_productFileName.find("test.tif") != AZStd::string::npos);
}

TEST_F(RCBuilderTest, ProcessJob_ProcessStandardSkippedSingleJob_Invalid)
{
    MockRCCompiler*                     mockRC = new MockRCCompiler();
    TestInternalRecognizerBasedBuilder  test(mockRC);
    MockRecognizerConfiguration         configuration;


    // Create a dummy test recognizer
    AZ::u32 recID = test.AddTestRecognizer(this->GetBuilderID(), QString("skip"), "pc");

    // Create the test job
    AssetBuilderSDK::ProcessJobRequest request = CreateTestJobRequest("test.tif", true, "pc");
    request.m_jobDescription.m_jobParameters[recID] = "copy";

    bool initialization_result = test.Initialize(configuration);
    ASSERT_TRUE(initialization_result);

    mockRC->SetResultExecute(true);
    AssetBuilderSDK::ProcessJobResponse response;
    test.TestProcessJob(request, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Failed);
}


TEST_F(RCBuilderTest, TestProcessRCResultFolder_LegacySystem)
{
    TestInternalRecognizerBasedBuilder  test(new MockRCCompiler());
    test.AddTestFileInfo("file.dds")
        .AddTestFileInfo("file.caf")
        .AddTestFileInfo("file.png")
        .AddTestFileInfo("rc_createdfiles.txt")
        .AddTestFileInfo("rc_log.log")
        .AddTestFileInfo("rc_log_warnings.log")
        .AddTestFileInfo("rc_log_errors.log")
        .AddTestFileInfo("ProcessJobRequest.xml")
        .AddTestFileInfo("ProcessJobResponse.xml");

    AZ::Uuid productGUID = AZ::Uuid("{60554E3C-D8D5-4429-AC77-740F0ED46193}");

    AssetBuilderSDK::ProcessJobResponse response;
    test.TestProcessRCResultFolder("c:\\temp", productGUID, false, response);

    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);

    // we expect it to have ignored most of the file cruft.
    ASSERT_EQ(response.m_outputProducts.size(), 3);

    AZStd::string fileJoined;

    AzFramework::StringFunc::Path::Join("c:\\temp", "file.dds", fileJoined);
    ASSERT_EQ(response.m_outputProducts[0].m_productFileName, fileJoined);
    ASSERT_EQ(response.m_outputProducts[0].m_productAssetType, productGUID);
    ASSERT_EQ(response.m_outputProducts[0].m_productSubID, 0x00000000);
    ASSERT_EQ(response.m_outputProducts[0].m_legacySubIDs.size(), 0);

    AzFramework::StringFunc::Path::Join("c:\\temp", "file.caf", fileJoined);
    ASSERT_EQ(response.m_outputProducts[1].m_productFileName, fileJoined);
    ASSERT_EQ(response.m_outputProducts[1].m_productAssetType, productGUID);
    ASSERT_EQ(response.m_outputProducts[1].m_productSubID, (AZ_CRC("file.caf", 0x91277b80) & 0x0000FFFF)); // legacy subids are just the lower 16 bits of the crc of filename.
    ASSERT_EQ(response.m_outputProducts[1].m_legacySubIDs.size(), 0);
    
    AzFramework::StringFunc::Path::Join("c:\\temp", "file.png", fileJoined);
    ASSERT_EQ(response.m_outputProducts[2].m_productFileName, fileJoined);
    ASSERT_EQ(response.m_outputProducts[2].m_productAssetType, productGUID);
    ASSERT_EQ(response.m_outputProducts[2].m_productSubID, (AZ_CRC("file.png", 0x7fd84af0) & 0x0000FFFF));
    ASSERT_EQ(response.m_outputProducts[2].m_legacySubIDs.size(), 0);
}

TEST_F(RCBuilderTest, TestProcessRCResultFolder_Fail_Fail)
{
    TestInternalRecognizerBasedBuilder  test(new MockRCCompiler());
    AssetBuilderSDK::ProcessJobResponse response;
    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
    
    AZ::Uuid productGUID = AZ::Uuid("{60554E3C-D8D5-4429-AC77-740F0ED46193}");
    test.TestProcessRCResultFolder("c:\\temp", productGUID, true, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResult_Failed);
}

TEST_F(RCBuilderTest, TestProcessRCResultFolder_Succeed_NothingBuilt)
{
    TestInternalRecognizerBasedBuilder  test(new MockRCCompiler());
    AssetBuilderSDK::ProcessJobResponse response;
    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    
    AZ::Uuid productGUID = AZ::Uuid("{60554E3C-D8D5-4429-AC77-740F0ED46193}");
    test.TestProcessRCResultFolder("c:\\temp", productGUID, true, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResult_Success);
}

TEST_F(RCBuilderTest, TestProcessRCResultFolder_Fail_BadName)
{
    TestInternalRecognizerBasedBuilder  test(new MockRCCompiler());
    AssetBuilderSDK::ProcessJobResponse response;
    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

    AZ::Uuid productGUID = AZ::Uuid("{60554E3C-D8D5-4429-AC77-740F0ED46193}");
    // note: empty name on next line
    response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("", productGUID, 1234));
    test.TestProcessRCResultFolder("c:\\temp", productGUID, true, response);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResult_Failed);
}

TEST_F(RCBuilderTest, TestProcessRCResultFolder_Fail_DuplicateFile)
{
    m_errorAbsorber->m_debugMessages = true;
    TestInternalRecognizerBasedBuilder  test(new MockRCCompiler());
    AssetBuilderSDK::ProcessJobResponse response;
    test.AddTestFileInfo("file.dds");
    AZ::Uuid productGUID = AZ::Uuid("{60554E3C-D8D5-4429-AC77-740F0ED46193}");
    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("file.dds", productGUID, 1234));
    response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("file.dds", productGUID, 5679));
    test.TestProcessRCResultFolder("c:\\temp", productGUID, true, response);
    m_errorAbsorber->AssertErrors(1);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResult_Failed);
}

TEST_F(RCBuilderTest, TestProcessRCResultFolder_Fail_DuplicateSubID)
{
    TestInternalRecognizerBasedBuilder  test(new MockRCCompiler());
    AssetBuilderSDK::ProcessJobResponse response;
    test.AddTestFileInfo("file.dds")
        .AddTestFileInfo("file.caf");
    AZ::Uuid productGUID = AZ::Uuid("{60554E3C-D8D5-4429-AC77-740F0ED46193}");

    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("file.dds", productGUID, 1234));
    response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("file.caf", productGUID, 1234));
    test.TestProcessRCResultFolder("c:\\temp", productGUID, true, response);
    ASSERT_EQ(m_errorAbsorber->m_numErrorsAbsorbed, 1);
    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResult_Failed);
}


TEST_F(RCBuilderTest, TestProcessRCResultFolder_WithResponseFromRC)
{
    TestInternalRecognizerBasedBuilder  test(new MockRCCompiler());
    test.AddTestFileInfo("file.dds")
        .AddTestFileInfo("file.caf")
        .AddTestFileInfo("file.png")
        .AddTestFileInfo("rc_createdfiles.txt")
        .AddTestFileInfo("rc_log.log")
        .AddTestFileInfo("rc_log_warnings.log")
        .AddTestFileInfo("rc_log_errors.log")
        .AddTestFileInfo("ProcessJobRequest.xml")
        .AddTestFileInfo("ProcessJobResponse.xml");

    AZ::Uuid productGUID = AZ::Uuid::CreateNull(); // this is to make sure that it doesn't matter what we pass in
    AZ::Uuid actualGUID = AZ::Uuid("{60554E3C-D8D5-4429-AC77-740F0ED46193}");

    AssetBuilderSDK::ProcessJobResponse response;

    response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("file.dds", actualGUID, 1234));
    response.m_outputProducts.back().m_legacySubIDs.push_back(3333);
    response.m_outputProducts.push_back(AssetBuilderSDK::JobProduct("file.caf", actualGUID, 3456));
    response.m_outputProducts.back().m_legacySubIDs.push_back(2222);
    response.m_outputProducts.back().m_legacySubIDs.push_back((AZ_CRC("file.caf", 0x91277b80) & 0x0000FFFF)); // push back the existing one to make sure no dupes.
    response.m_resultCode = AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success;

    // in this test we pretend the response was actually populated by the builder and make sure it populates the legacy IDs correctly
    // 1. there should actually BE legacy IDs
    // 2. there should be no duplicate IDs (legacy IDs should not duplicate ACTUAL ids)
    // 3. there should be no duplicate Legacy IDs (legacy IDs should not duplicate each other)
    // 4. if we provide legacy Ids, they should be used in addition to the automatic ones.

    test.TestProcessRCResultFolder("c:\\temp", productGUID, true, response);

    ASSERT_EQ(response.m_resultCode, AssetBuilderSDK::ProcessJobResultCode::ProcessJobResult_Success);

    // we expect it to only have accepted the products we specified.
    ASSERT_EQ(response.m_outputProducts.size(), 2);

    AZStd::string fileJoined;

    AzFramework::StringFunc::Path::Join("c:\\temp", "file.dds", fileJoined);
    ASSERT_EQ(response.m_outputProducts[0].m_productFileName, fileJoined);
    ASSERT_EQ(response.m_outputProducts[0].m_productAssetType, actualGUID);
    ASSERT_EQ(response.m_outputProducts[0].m_productSubID, 1234);
    ASSERT_EQ(response.m_outputProducts[0].m_legacySubIDs.size(), 2); // it must include our new one AND the zero that it would have generated before.
    ASSERT_EQ(response.m_outputProducts[0].m_legacySubIDs[0], 3333);
    ASSERT_EQ(response.m_outputProducts[0].m_legacySubIDs[1], 0);

    AzFramework::StringFunc::Path::Join("c:\\temp", "file.caf", fileJoined);
    ASSERT_EQ(response.m_outputProducts[1].m_productFileName, fileJoined);
    ASSERT_EQ(response.m_outputProducts[1].m_productAssetType, actualGUID);
    ASSERT_EQ(response.m_outputProducts[1].m_productSubID, 3456); // legacy subids are just the lower 16 bits of the crc of filename.
    ASSERT_EQ(response.m_outputProducts[1].m_legacySubIDs.size(), 2); // we only expect the one legacy, no dupes!
    ASSERT_EQ(response.m_outputProducts[1].m_legacySubIDs[0], 2222);
    ASSERT_EQ(response.m_outputProducts[1].m_legacySubIDs[1], (AZ_CRC("file.caf", 0x91277b80) & 0x0000FFFF));
}

class MockBuilderListener : public AssetBuilderSDK::AssetBuilderBus::Handler
{
public:
    void RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& builderDesc) override
    {
        m_wasCalled = true;
        m_result = builderDesc;
    }

    bool m_wasCalled = false;
    AssetBuilderSDK::AssetBuilderDesc m_result;
};


class RCBuilderFingerprintTest
    : public RCBuilderTest
{
    
public:

    // A utility function which feeds in the version and asset type to the builder, fingerprints it, and returns the fingerprint
    AZStd::string BuildFingerprint(int versionNumber, AZ::Uuid builderProductType)
    {
        MockRCCompiler*                     mockRC = new MockRCCompiler();
        TestInternalRecognizerBasedBuilder  test(mockRC);

        MockRecognizerConfiguration         configuration;

        AssetPlatformSpec   good_spec;
        good_spec.m_extraRCParams = "/i";

        AssetRecognizer     good;
        good.m_name = "Good";
        good.m_version = static_cast<char>(versionNumber);
        good.m_patternMatcher = AssetBuilderSDK::FilePatternMatcher("*.foo", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        good.m_platformSpecs["pc"] = good_spec;
        good.m_productAssetType = builderProductType;
        
        configuration.m_recognizerContainer["good"] = good;

        MockBuilderListener listener;
        listener.BusConnect();

        bool initialization_result = test.Initialize(configuration);
        listener.BusDisconnect();

        EXPECT_TRUE(listener.m_wasCalled);
        EXPECT_TRUE(initialization_result);

        EXPECT_STRNE(listener.m_result.m_analysisFingerprint.c_str(), "");
        return listener.m_result.m_analysisFingerprint;
    }
};

TEST_F(RCBuilderFingerprintTest, DifferentVersion_Has_DifferentAnalysisFingerprint)
{
    AZ::Uuid uuid1 = AZ::Uuid::CreateRandom();
    AZStd::string analysisFingerprint1 = BuildFingerprint(1, uuid1);
    AZStd::string analysisFingerprint2 = BuildFingerprint(2, uuid1);
    EXPECT_STRNE(analysisFingerprint1.c_str(), analysisFingerprint2.c_str());
}

TEST_F(RCBuilderFingerprintTest, DifferentAssetType_Has_DifferentAnalysisFingerprint)
{
    AZ::Uuid uuid1 = AZ::Uuid::CreateRandom();
    AZ::Uuid uuid2 = AZ::Uuid::CreateRandom();
    AZStd::string analysisFingerprint1 = BuildFingerprint(1, uuid1);
    AZStd::string analysisFingerprint2 = BuildFingerprint(1, uuid2);
    EXPECT_STRNE(analysisFingerprint1.c_str(), analysisFingerprint2.c_str());
}

TEST_F(RCBuilderFingerprintTest, DifferentAssetTypeAndVersion_Has_DifferentAnalysisFingerprint)
{
    AZ::Uuid uuid1 = AZ::Uuid::CreateRandom();
    AZ::Uuid uuid2 = AZ::Uuid::CreateRandom();
    AZStd::string analysisFingerprint1 = BuildFingerprint(1, uuid1);
    AZStd::string analysisFingerprint2 = BuildFingerprint(2, uuid2);
    EXPECT_STRNE(analysisFingerprint1.c_str(), analysisFingerprint2.c_str());
}

TEST_F(RCBuilderFingerprintTest, SameVersionAndSameType_Has_SameAnalysisFingerprint)
{
    AZ::Uuid uuid1 = AZ::Uuid::CreateRandom();
    AZStd::string analysisFingerprint1 = BuildFingerprint(1, uuid1);
    AZStd::string analysisFingerprint2 = BuildFingerprint(1, uuid1);
    EXPECT_STREQ(analysisFingerprint1.c_str(), analysisFingerprint2.c_str());
}

