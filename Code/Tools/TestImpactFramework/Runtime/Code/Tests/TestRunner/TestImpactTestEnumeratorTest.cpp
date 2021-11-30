///*
// * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
// * its licensors.
// *
// * For complete copyright and license terms please see the LICENSE at the root of this
// * distribution (the "License"). All use of this software is governed by the License,
// * or, if provided, by the license below or the license accompanying this file. Do not
// * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
// * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// *
// */
//
//#include <TestImpactFramework/TestImpactRuntime.h>
//
//#include <TestImpactTestJobRunnerCommon.h>
//#include <TestImpactTestUtils.h>
//
//#include <TestRunner/Enumeration/TestImpactTestEnumerationSerializer.h>
//#include <TestRunner/Enumeration/TestImpactTestEnumerator.h>
//
//#include <AzCore/UnitTest/TestTypes.h>
//#include <AzCore/std/containers/array.h>
//#include <AzCore/std/containers/vector.h>
//#include <AzCore/std/optional.h>
//#include <AzCore/std/smart_ptr/unique_ptr.h>
//#include <AzCore/std/string/string.h>
//#include <AzTest/AzTest.h>
//
//namespace UnitTest
//{
//    using JobExceptionPolicy = TestImpact::TestEnumerator::JobExceptionPolicy;
//    using CacheExceptionPolicy = TestImpact::TestEnumerator::CacheExceptionPolicy;
//
//    // Generates the command to run the given test target through AzTestRunner and get gtest to output the enumeration file
//    TestImpact::TestEnumerator::Command GetEnumerateCommandForTarget(AZStd::pair<TestImpact::RepoPath, TestImpact::RepoPath> testTarget)
//    {
//        return TestImpact::TestEnumerator::Command{ AZStd::string::format(
//            "%s %s AzRunUnitTests --gtest_list_tests --gtest_output=xml:%s",
//            LY_TEST_IMPACT_AZ_TESTRUNNER_BIN,
//            testTarget.first.c_str(),     // Path to test target bin
//            testTarget.second.c_str()) }; // Path to test target gtest enumeration file
//    }
//
//    class TestEnumeratorFixture
//        : public AllocatorsTestFixture
//    {
//    public:
//        void SetUp() override;
//
//    protected:
//        using JobInfo = TestImpact::TestEnumerator::JobInfo;
//        using JobData = TestImpact::TestEnumerator::JobData;
//        using Command = TestImpact::TestEnumerator::Command;
//
//        AZStd::vector<JobInfo> m_jobInfos;
//        AZStd::unique_ptr<TestImpact::TestEnumerator> m_testEnumerator;
//        AZStd::vector<Command> m_testTargetJobArgs;
//        AZStd::vector<AZStd::pair<TestImpact::RepoPath, TestImpact::RepoPath>> m_testTargetPaths;
//        AZStd::vector<TestImpact::TestEnumeration> m_expectedTestTargetEnumerations;
//        AZStd::vector<AZStd::string> m_cacheFiles;
//        size_t m_maxConcurrency = 0;
//    };
//
//    void TestEnumeratorFixture::SetUp()
//    {
//        UnitTest::AllocatorsTestFixture::SetUp();
//
//        DeleteFiles(LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR, "*.cache");
//        DeleteFiles(LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR, "*.xml");
//
//        // first: path to test target bin
//        // second: path to test target gtest enumeration file in XML format
//        const AZStd::string enumPath = AZStd::string(LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR) + "/%s.Enumeration.xml";
//        m_testTargetPaths.emplace_back(
//            LY_TEST_IMPACT_TEST_TARGET_A_BIN, AZStd::string::format(enumPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_A_BASE_NAME));
//        m_testTargetPaths.emplace_back(
//            LY_TEST_IMPACT_TEST_TARGET_B_BIN, AZStd::string::format(enumPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_B_BASE_NAME));
//        m_testTargetPaths.emplace_back(
//            LY_TEST_IMPACT_TEST_TARGET_C_BIN, AZStd::string::format(enumPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_C_BASE_NAME));
//        m_testTargetPaths.emplace_back(
//            LY_TEST_IMPACT_TEST_TARGET_D_BIN, AZStd::string::format(enumPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_D_BASE_NAME));
//
//        m_expectedTestTargetEnumerations.emplace_back(GetTestTargetATestEnumerationSuites());
//        m_expectedTestTargetEnumerations.emplace_back(GetTestTargetBTestEnumerationSuites());
//        m_expectedTestTargetEnumerations.emplace_back(GetTestTargetCTestEnumerationSuites());
//        m_expectedTestTargetEnumerations.emplace_back(GetTestTargetDTestEnumerationSuites());
//
//        // Path to enumeration file in TIAF internal JSON format
//        m_cacheFiles.emplace_back(
//            AZStd::string::format("%s/%s.cache", LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR, LY_TEST_IMPACT_TEST_TARGET_A_BASE_NAME));
//        m_cacheFiles.emplace_back(
//            AZStd::string::format("%s/%s.cache", LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR, LY_TEST_IMPACT_TEST_TARGET_B_BASE_NAME));
//        m_cacheFiles.emplace_back(
//            AZStd::string::format("%s/%s.cache", LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR, LY_TEST_IMPACT_TEST_TARGET_C_BASE_NAME));
//        m_cacheFiles.emplace_back(
//            AZStd::string::format("%s/%s.cache", LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR, LY_TEST_IMPACT_TEST_TARGET_D_BASE_NAME));
//
//        for (const auto& testTarget : m_testTargetPaths)
//        {
//            m_testTargetJobArgs.emplace_back(GetEnumerateCommandForTarget(testTarget));
//        }
//    }
//
//    // Fixture parameterized for different max number of concurrent jobs
//    class TestEnumeratorFixtureWithConcurrencyParams
//        : public TestEnumeratorFixture
//        , public ::testing::WithParamInterface<size_t>
//    {
//    public:
//        void SetUp() override
//        {
//            TestEnumeratorFixture::SetUp();
//            m_maxConcurrency = GetParam();
//        }
//    };    
//
//    using ConcurrencyAndJobExceptionPermutation = AZStd::tuple
//        <
//            size_t,             // Max number of concurrent processes
//            JobExceptionPolicy  // Test job exception policy
//        >;
//
//    // Fixture parameterized for different max number of concurrent jobs and different job exception policies
//    class TestEnumeratorFixtureWithConcurrencyAndJobExceptionParams
//        : public TestEnumeratorFixture
//        , public ::testing::WithParamInterface<ConcurrencyAndJobExceptionPermutation>
//    {
//    public:
//        void SetUp() override
//        {
//            TestEnumeratorFixture::SetUp();
//            const auto& [maxConcurrency, jobExceptionPolicy] = GetParam();
//            m_maxConcurrency = maxConcurrency;
//            m_jobExceptionPolicy = jobExceptionPolicy;
//        }
//
//    protected:
//        JobExceptionPolicy m_jobExceptionPolicy = JobExceptionPolicy::Never;
//    };
//
//    using ConcurrencyAndCacheExceptionPermutation = AZStd::tuple
//        <
//            size_t,                 // Max number of concurrent processes
//            CacheExceptionPolicy    // Test enumeration exception policy
//        >;
//
//    // Fixture parameterized for different max number of concurrent jobs and different enumeration exception policies
//    class TestEnumeratorFixtureWithConcurrencyAndCacheExceptionParams
//        : public TestEnumeratorFixture
//        , public ::testing::WithParamInterface<ConcurrencyAndCacheExceptionPermutation>
//    {
//    public:
//        void SetUp() override
//        {
//            TestEnumeratorFixture::SetUp();
//            const auto& [maxConcurrency, cacheExceptionPolicy] = GetParam();
//            m_maxConcurrency = maxConcurrency;
//            m_cacheExceptionPolicy = cacheExceptionPolicy;
//        }
//
//    protected:
//        CacheExceptionPolicy m_cacheExceptionPolicy = CacheExceptionPolicy::Never;
//    };
//
//    namespace
//    {
//        AZStd::array<size_t, 4> MaxConcurrentEnumerations = {{1, 2, 3, 4}};
//
//        AZStd::array<JobExceptionPolicy, 3> JobExceptionPolicies =
//        {
//            JobExceptionPolicy::Never,
//            JobExceptionPolicy::OnExecutedWithFailure,
//            JobExceptionPolicy::OnFailedToExecute
//        };
//
//        AZStd::array<CacheExceptionPolicy, 2> CacheExceptionPolicies =
//        {
//            CacheExceptionPolicy::Never,
//            CacheExceptionPolicy::OnCacheWriteFailure
//        };
//    }
//
//    // Validates that the specified job successfully read from its test enumeration cache
//    void ValidateJobSuccessfulCacheRead(const TestImpact::TestEnumerator::Job& job)
//    {
//        EXPECT_EQ(job.GetJobResult(), TestImpact::JobResult::NotExecuted);
//        EXPECT_EQ(job.GetStartTime(), AZStd::chrono::high_resolution_clock::time_point());
//        EXPECT_EQ(job.GetEndTime(), AZStd::chrono::high_resolution_clock::time_point());
//        EXPECT_EQ(job.GetDuration(), AZStd::chrono::milliseconds(0));
//        EXPECT_FALSE(job.GetReturnCode().has_value());
//        EXPECT_TRUE(job.GetPayload().has_value());
//    }
//
//    // Validates that the specified test enumeration matched the expected output
//    void ValidateTestTargetEnumeration(const TestImpact::TestEnumeration& actualResult, const TestImpact::TestEnumeration& expectedResult)
//    {
//        EXPECT_TRUE(actualResult == expectedResult);
//        EXPECT_EQ(actualResult.GetNumTestSuites(), CalculateNumTestSuites(expectedResult.GetTestSuites()));
//        EXPECT_EQ(actualResult.GetNumTests(), CalculateNumTests(expectedResult.GetTestSuites()));
//        EXPECT_EQ(actualResult.GetNumEnabledTests(), CalculateNumEnabledTests(expectedResult.GetTestSuites()));
//        EXPECT_EQ(actualResult.GetNumDisabledTests(), CalculateNumDisabledTests(expectedResult.GetTestSuites()));
//    }
//
//    // Validates that the specified test enumeration cache matches the expected output
//    void ValidateTestEnumerationCache(const TestImpact::RepoPath& cacheFile, const TestImpact::TestEnumeration& expectedEnumeration)
//    {
//        // Cache file must exist
//        const auto fileSize = AZ::IO::SystemFile::Length(cacheFile.c_str());
//        EXPECT_GT(fileSize, 0);
//
//        // Read raw byte data from cache
//        AZStd::vector<char> buffer(fileSize + 1);
//        buffer[fileSize] = 0;
//        EXPECT_TRUE(AZ::IO::SystemFile::Read(cacheFile.c_str(), buffer.data()));
//
//        // Transform raw byte data to raw string data and attempt to construct the test enumeration
//        AZStd::string rawEnum(buffer.begin(), buffer.end());
//        TestImpact::TestEnumeration actualEnumeration = TestImpact::DeserializeTestEnumeration(rawEnum);
//
//        // Check that the constructed test enumeration matches the expected enumeration
//        ValidateTestTargetEnumeration(actualEnumeration, expectedEnumeration);
//    }
//
//    // Validates that the specified cache file does not exist
//    void ValidateInvalidTestEnumerationCache(const TestImpact::RepoPath& cacheFile)
//    {
//        EXPECT_FALSE(AZ::IO::SystemFile::Exists(cacheFile.c_str()));
//    }
//
//    TEST_P(
//        TestEnumeratorFixtureWithConcurrencyAndJobExceptionParams, InvalidCommandArgument_ExpectJobResulFailedToExecuteeOrTestJobException)
//    {
//        // Given a test enumerator with no client callback or enumeration timeout or enumerator timeout
//        m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//            AZStd::nullopt,
//            m_maxConcurrency,
//            AZStd::nullopt,
//            AZStd::nullopt);
//
//        // Given a mixture of test enumeration jobs with valid and invalid command arguments
//        for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//        {
//            const Command args = (jobId % 2) ? Command{ InvalidProcessPath } : m_testTargetJobArgs[jobId];
//            JobData jobData(m_testTargetPaths[jobId].second, AZStd::nullopt);
//            m_jobInfos.emplace_back(JobInfo({jobId}, args, AZStd::move(jobData)));
//        }
//
//        try
//        {
//            // When the test enumeration jobs are executed with different exception policies
//            const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, m_jobExceptionPolicy);
//
//            // Expect this statement to be reachable only if no exception policy for launch failures
//            EXPECT_FALSE(::IsFlagSet(m_jobExceptionPolicy, JobExceptionPolicy::OnFailedToExecute));
//
//            for (const auto& job : enumerationJobs)
//            {
//                const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//                if (jobId % 2)
//                {
//                    // Expect invalid jobs have a job result of FailedToExecute
//                    ValidateJobFailedToExecute(job);
//                }
//                else
//                {
//                    // Expect the valid jobs job to successfully result in a test enumeration that matches the expected test enumeration data
//                    ValidateJobExecutedSuccessfully(job);
//                    ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//                }
//            }
//        }
//        catch ([[maybe_unused]] const FailedJob& job)
//        {
//            // Expect this statement to be reachable only if there is an exception policy for launch failures
//            EXPECT_TRUE(::IsFlagSet(m_jobExceptionPolicy, JobExceptionPolicy::OnFailedToExecute));
//        }
//        catch (...)
//        {
//            // Do not expect any other exceptions
//            FAIL();
//        }
//    }
//
//    //TEST_P(
//    //    TestEnumeratorFixtureWithConcurrencyAndJobExceptionParams, ErroneousReturnCode_ExpectJobResultExecutedWithFailureOrTestJobException)
//    //{
//    //    // Given a test enumerator with no client callback or enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        m_maxConcurrency,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given a mixture of test enumeration jobs that execute and return either successfully or with failure
//    //    for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//    //    {
//    //        JobData jobData(m_testTargetPaths[jobId].second, AZStd::nullopt);
//    //        const Command args = (jobId % 2)
//    //            ? Command{AZStd::string::format("%s %s", ValidProcessPath, ConstructTestProcessArgs(jobId, AZStd::chrono::milliseconds(0)).c_str())}
//    //            : m_testTargetJobArgs[jobId];
//    //        m_jobInfos.emplace_back(JobInfo({jobId}, args, AZStd::move(jobData)));
//    //    }
//
//    //    try
//    //    {
//    //        // When the test enumeration jobs are executed with different exception policies
//    //        const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, m_jobExceptionPolicy);
//
//    //        // Expect this statement to be reachable only if no exception policy for jobs that return with error
//    //        EXPECT_FALSE(::IsFlagSet(m_jobExceptionPolicy, JobExceptionPolicy::OnExecutedWithFailure));
//
//    //        for (const auto& job : enumerationJobs)
//    //        {
//    //            const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//    //            if (jobId % 2)
//    //            {
//    //                // Expect failed jobs to have job result ExecutedWithFailure and a non-zero return code
//    //                ValidateJobExecutedWithFailure(job);
//    //            }
//    //            else
//    //            {
//    //                // Expect the valid jobs job to successfully result in a test enumeration that matches the expected test enumeration data
//    //                ValidateJobExecutedSuccessfully(job);
//    //                ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //            }
//    //        }
//    //    }
//    //    catch ([[maybe_unused]] const FailedJob& job)
//    //    {
//    //        // Expect this statement to be reachable only if there is an exception policy for jobs that return with error
//    //        EXPECT_TRUE(::IsFlagSet(m_jobExceptionPolicy, JobExceptionPolicy::OnExecutedWithFailure));
//    //    }
//    //    catch (...)
//    //    {
//    //        // Do not expect any other exceptions
//    //        FAIL();
//    //    }
//    //}
//
//    //TEST_P(TestEnumeratorFixtureWithConcurrencyAndCacheExceptionParams, EmptyCacheRead_NoCacheDataButEnumerationsMatchTestSuitesInTarget)
//    //{
//    //    // Given a test enumerator with no client callback, enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        m_maxConcurrency,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job for each test target that reads from an enumeration caching
//    //    for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//    //    {
//    //        JobData jobData(m_testTargetPaths[jobId].second, JobData::Cache{JobData::CachePolicy::Read, m_cacheFiles[jobId]});
//    //        m_jobInfos.emplace_back(JobInfo({jobId}, m_testTargetJobArgs[jobId], AZStd::move(jobData)));
//    //    }
//
//    //    try
//    //    {
//    //        // When the test enumeration jobs are executed with different exception policies
//    //        const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, m_cacheExceptionPolicy, JobExceptionPolicy::Never);
//
//    //        // Expect this statement to be reachable only if no exception policy for read attempts of non-existent caches
//    //        EXPECT_FALSE(::IsFlagSet(m_cacheExceptionPolicy, CacheExceptionPolicy::OnCacheNotExist));
//
//    //        // Expect each job to successfully result in a test enumeration that matches the expected test enumeration data for that test target
//    //        // even though the cache files could not be read
//    //        for (const auto& job : enumerationJobs)
//    //        {
//    //            const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//    //            ValidateJobExecutedSuccessfully(job);
//    //            ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //            ValidateInvalidTestEnumerationCache(job.GetJobInfo().GetCache()->m_file);
//    //        }
//    //    }
//    //    catch ([[maybe_unused]] const TestImpact::TestEnumerationException& e)
//    //    {
//    //        // Expect this statement to be reachable only if there is an exception policy for read attempts of non-existent caches
//    //        EXPECT_TRUE(::IsFlagSet(m_cacheExceptionPolicy, CacheExceptionPolicy::OnCacheNotExist));
//    //    }
//    //    catch (...)
//    //    {
//    //        // Do not expect any other exceptions
//    //        FAIL();
//    //    }
//    //}
//
//    //// Note: this test only cues up one test for enumeration but still runs the permutations for max concurrency so there is duplicated work
//    //TEST_P(
//    //    TestEnumeratorFixtureWithConcurrencyAndCacheExceptionParams,
//    //    EmptyCacheDataRead_ExpectEnumerationsMatchTestSuitesInTargetOrTestEnumerationException)
//    //{
//    //    // Given an enumeration cache for Test Target A with invalid JSON data
//    //    WriteTextToFile("", m_cacheFiles[TestTargetA]);
//
//    //    // Given a test enumerator with no client callback, enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        m_maxConcurrency,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job that will attempt to read an invalid enumeration cache
//    //    JobData jobData(m_testTargetPaths[TestTargetA].second, JobData::Cache{JobData::CachePolicy::Read, m_cacheFiles[TestTargetA]});
//    //    m_jobInfos.emplace_back(JobInfo({TestTargetA}, m_testTargetJobArgs[TestTargetA], AZStd::move(jobData)));
//
//    //    try
//    //    {
//    //        // When the test enumeration jobs are executed with different exception policies
//    //        const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, m_cacheExceptionPolicy, JobExceptionPolicy::Never);
//
//    //        // Expect this statement to be reachable only if no exception policy for cache reads that fail
//    //        EXPECT_FALSE(::IsFlagSet(m_cacheExceptionPolicy, CacheExceptionPolicy::OnCacheReadFailure));
//
//    //        for (const auto& job : enumerationJobs)
//    //        {
//    //            const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//
//    //            // Expect the valid jobs job to successfully result in a test enumeration that matches the expected test enumeration data
//    //            ValidateJobExecutedSuccessfully(job);
//    //            ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //        }
//    //    }
//    //    catch ([[maybe_unused]] const TestImpact::TestEnumerationException& e)
//    //    {
//    //        // Expect this statement to be reachable only if there is an exception policy for cache reads that fail
//    //        EXPECT_TRUE(::IsFlagSet(m_cacheExceptionPolicy, CacheExceptionPolicy::OnCacheReadFailure));
//    //    }
//    //    catch (...)
//    //    {
//    //        // Do not expect any other exceptions
//    //        FAIL();
//    //    }
//    //}
//
//    //TEST_P(
//    //    TestEnumeratorFixtureWithConcurrencyAndCacheExceptionParams,
//    //    InvalidCacheWrite_ExpectEnumerationsMatchTestSuitesInTargetOrTestEnumerationException)
//    //{
//    //    // Given a test enumerator with no client callback,, enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        m_maxConcurrency,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job that will attempt to write to an invalid enumeration cache
//    //    JobData jobData(m_testTargetPaths[TestTargetA].second, JobData::Cache{JobData::CachePolicy::Write, InvalidProcessPath});
//    //    m_jobInfos.emplace_back(JobInfo({TestTargetA}, m_testTargetJobArgs[TestTargetA], AZStd::move(jobData)));
//
//    //    try
//    //    {
//    //        // When the test enumeration job is executed
//    //        const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, m_cacheExceptionPolicy, JobExceptionPolicy::Never);
//
//    //        // Expect this statement to be reachable only if no exception policy for cache writes that fail
//    //        EXPECT_FALSE(::IsFlagSet(m_cacheExceptionPolicy, CacheExceptionPolicy::OnCacheWriteFailure));
//
//    //        for (const auto& job : enumerationJobs)
//    //        {
//    //            const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//
//    //            // Expect the valid jobs job to successfully result in a test enumeration that matches the expected test enumeration data
//    //            ValidateJobExecutedSuccessfully(job);
//    //            ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //        }
//    //    }
//    //    catch ([[maybe_unused]] const TestImpact::TestEnumerationException& e)
//    //    {
//    //        // Expect this statement to be reachable only if there is an exception policy for cache writes that fail
//    //        EXPECT_TRUE(::IsFlagSet(m_cacheExceptionPolicy, CacheExceptionPolicy::OnCacheWriteFailure));
//    //    }
//    //    catch (...)
//    //    {
//    //        // Do not expect any other exceptions
//    //        FAIL();
//    //    }
//    //}
//
//    //TEST_P(TestEnumeratorFixtureWithConcurrencyParams, ValidAndInvalidCacheRead_CachedEnumerationsMatchTestSuitesInTarget)
//    //{
//    //    // Given the cache file written for only test target B
//    //    WriteTextToFile(TestImpact::SerializeTestEnumeration(m_expectedTestTargetEnumerations[TestTargetB]), m_cacheFiles[TestTargetB]);
//
//    //    // Given a test enumerator with no client callback, enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        m_maxConcurrency,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given test enumeration jobs test target A and D with no enumeration caching
//    //    m_jobInfos.emplace_back(
//    //        JobInfo({TestTargetA}, m_testTargetJobArgs[TestTargetA], JobData{m_testTargetPaths[TestTargetA].second, AZStd::nullopt}));
//    //    m_jobInfos.emplace_back(
//    //        JobInfo({TestTargetD}, m_testTargetJobArgs[TestTargetD], JobData{m_testTargetPaths[TestTargetD].second, AZStd::nullopt}));
//
//    //    // Given test target B with enumeration cache reading and a valid cache file
//    //    m_jobInfos.emplace_back(JobInfo(
//    //        {TestTargetB}, m_testTargetJobArgs[TestTargetB],
//    //        JobData{m_testTargetPaths[TestTargetB].second, JobInfo::Cache{JobData::CachePolicy::Read, m_cacheFiles[TestTargetB]}}));
//
//    //    // Given test target C with enumeration cache reading and an invalid cache file
//    //    m_jobInfos.emplace_back(JobInfo(
//    //        {TestTargetC}, m_testTargetJobArgs[TestTargetC],
//    //        JobData{m_testTargetPaths[TestTargetC].second, JobInfo::Cache{JobData::CachePolicy::Read, "nothing"}}));
//
//    //    // When the test enumeration jobs are executed
//    //    const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, JobExceptionPolicy::Never);
//
//    //    // Expect each job to successfully result in a test enumeration that matches the expected test enumeration data for that test target
//    //    for (const auto& job : enumerationJobs)
//    //    {
//    //        const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//
//    //        switch (jobId)
//    //        {
//    //        case TestTargetA: // No cache read
//    //        case TestTargetC: // Cache read, but invalid cache, so re-enumerate anyway
//    //        case TestTargetD: // No cache read
//    //        {
//    //            ValidateJobExecutedSuccessfully(job);
//    //            break;
//    //        }
//    //        case TestTargetB: // Cache read, successful cache read, so job not executed
//    //        {
//    //            ValidateJobSuccessfulCacheRead(job);
//    //            break;
//    //        }
//    //        default:
//    //        {
//    //            FAIL();
//    //        }
//    //        }
//
//    //        // Regardless of cache policy and cache failures all targets should still produce the expected test enumerations
//    //        ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //    }
//    //}
//
//    //TEST_F(TestEnumeratorFixture, InvalidCacheDataRead_TestEnumerationException)
//    //{
//    //    // Given an enumeration cache for Test Target A with invalid JSON data
//    //    WriteTextToFile("There is no valid cache data here", m_cacheFiles[TestTargetA]);
//
//    //    // Given a test enumerator with no client callback, concurrency, enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        OneConcurrentProcess,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job that will attempt to read an invalid enumeration cache
//    //    JobData jobData(m_testTargetPaths[TestTargetA].second, JobData::Cache{JobData::CachePolicy::Read, m_cacheFiles[TestTargetA]});
//    //    m_jobInfos.emplace_back(JobInfo({TestTargetA}, m_testTargetJobArgs[TestTargetA], AZStd::move(jobData)));
//
//    //    try
//    //    {
//    //        // When the test enumeration jobs are executed with different exception policies
//    //        const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, JobExceptionPolicy::Never);
//    //    }
//    //    catch ([[maybe_unused]] const TestImpact::TestEnumerationException& e)
//    //    {
//    //        // Expect this statement to be reachable only if there is an exception policy for jobs that return with error
//    //        SUCCEED();
//    //    }
//    //    catch (...)
//    //    {
//    //        // Do not expect any other exceptions
//    //        FAIL();
//    //    }
//    //}
//
//    //TEST_P(TestEnumeratorFixtureWithConcurrencyParams, ValidCacheWrite_CachedEnumerationsMatchTestSuitesInTarget)
//    //{
//    //    // Given a test enumerator with no client callback, enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        m_maxConcurrency,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job for each test target with write enumeration caching
//    //    for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//    //    {
//    //        JobData jobData(m_testTargetPaths[jobId].second, JobData::Cache{JobData::CachePolicy::Write, m_cacheFiles[jobId]});
//    //        m_jobInfos.emplace_back(JobInfo({jobId}, m_testTargetJobArgs[jobId], AZStd::move(jobData)));
//    //    }
//
//    //    // When the test enumeration jobs are executed
//    //    const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, JobExceptionPolicy::Never);
//
//    //    // Expect each job to successfully result in a test enumeration and cache that matches the expected test enumeration data for that
//    //    // test target
//    //    for (const auto& job : enumerationJobs)
//    //    {
//    //        const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//    //        ValidateJobExecutedSuccessfully(job);
//    //        ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //        ValidateTestEnumerationCache(job.GetJobInfo().GetCache()->m_file, m_expectedTestTargetEnumerations[jobId]);
//    //    }
//    //}
//
//    //TEST_F(TestEnumeratorFixture, EmptyArtifact_ExpectTestEnumerationException)
//    //{
//    //    // Given a test enumerator with no client callback, concurrency, enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        OneConcurrentProcess,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job that will return successfully but with an empty artifact string
//    //    m_jobInfos.emplace_back(JobInfo({0}, m_testTargetJobArgs[0], JobData("", AZStd::nullopt)));
//
//    //    try
//    //    {
//    //        // When the test enumeration job is executed
//    //        const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, JobExceptionPolicy::Never);
//
//    //        // Do not expect this statement to be reachable
//    //        FAIL();
//    //    }
//    //    catch ([[maybe_unused]] const TestImpact::TestEnumerationException& e)
//    //    {
//    //        // Expect an enumeration exception
//    //        SUCCEED();
//    //    }
//    //    catch (...)
//    //    {
//    //        // Do not expect any other exceptions
//    //        FAIL();
//    //    }
//    //}
//
//    //TEST_F(TestEnumeratorFixture, InvalidArtifact_ExpectTestEnumerationException)
//    //{
//    //    // Given a test enumerator with no client callback, concurrency, enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        OneConcurrentProcess,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job that will return successfully but not produce an artifact
//    //    m_jobInfos.emplace_back(JobInfo(
//    //        { 0 }, Command{ AZStd::string::format("%s %s", ValidProcessPath, ConstructTestProcessArgs(0, AZStd::chrono::milliseconds(0)).c_str()) },
//    //        JobData("", AZStd::nullopt)));
//
//    //    try
//    //    {
//    //        // When the test enumeration job is executed
//    //        const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, JobExceptionPolicy::Never);
//
//    //        // Do not expect this statement to be reachable
//    //        FAIL();
//    //    }
//    //    catch ([[maybe_unused]] const TestImpact::TestEnumerationException& e)
//    //    {
//    //        // Expect an enumeration exception
//    //        SUCCEED();
//    //    }
//    //    catch (...)
//    //    {
//    //        // Do not expect any other exceptions
//    //        FAIL();
//    //    }
//    //}
//
//    //TEST_P(TestEnumeratorFixtureWithConcurrencyParams, EnumerateTestTargets_EnumerationsMatchTestSuitesInTarget)
//    //{
//    //    // Given a test enumerator with no client callback, enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        m_maxConcurrency,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job for each test target with no enumeration caching
//    //    for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//    //    {
//    //        JobData jobData(m_testTargetPaths[jobId].second, AZStd::nullopt);
//    //        m_jobInfos.emplace_back(JobInfo({jobId}, m_testTargetJobArgs[jobId], AZStd::move(jobData)));
//    //    }
//
//    //    // When the test enumeration jobs are executed
//    //    const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, JobExceptionPolicy::Never);
//
//    //    // Expect each job to successfully result in a test enumeration that matches the expected test enumeration data for that test target
//    //    for (const auto& job : enumerationJobs)
//    //    {
//    //        const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//    //        ValidateJobExecutedSuccessfully(job);
//    //        ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //    }
//    //}
//
//    //TEST_P(TestEnumeratorFixtureWithConcurrencyParams, EnumerateTestTargetsWithArbitraryJobIds_EnumerationsMatchTestSuitesInTarget)
//    //{
//    //    // Given a set of arbitrary job ids to be used for the test target jobs
//    //    enum
//    //    {
//    //        ArbitraryA = 36,
//    //        ArbitraryB = 890,
//    //        ArbitraryC = 19,
//    //        ArbitraryD = 1
//    //    };
//
//    //    const AZStd::unordered_map<JobInfo::IdType, JobInfo::IdType> sequentialToArbitrary =
//    //    {
//    //        {TestTargetA, ArbitraryA},
//    //        {TestTargetB, ArbitraryB},
//    //        {TestTargetC, ArbitraryC},
//    //        {TestTargetD, ArbitraryD},
//    //    };
//
//    //    const AZStd::unordered_map<JobInfo::IdType, JobInfo::IdType> arbitraryToSequential =
//    //    {
//    //        {ArbitraryA, TestTargetA},
//    //        {ArbitraryB, TestTargetB},
//    //        {ArbitraryC, TestTargetC},
//    //        {ArbitraryD, TestTargetD},
//    //    };
//
//    //    // Given a test enumerator with no client callback, enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        m_maxConcurrency,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job for each test target with no enumeration caching
//    //    for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//    //    {
//    //        JobData jobData(m_testTargetPaths[jobId].second, AZStd::nullopt);
//    //        m_jobInfos.emplace_back(JobInfo({sequentialToArbitrary.at(jobId)}, m_testTargetJobArgs[jobId], AZStd::move(jobData)));
//    //    }
//
//    //    // When the test enumeration jobs are executed
//    //    const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, JobExceptionPolicy::Never);
//
//    //    // Expect each job to successfully result in a test enumeration that matches the expected test enumeration data for that test target
//    //    for (const auto& job : enumerationJobs)
//    //    {
//    //        const JobInfo::IdType jobId = arbitraryToSequential.at(job.GetJobInfo().GetId().m_value);
//    //        ValidateJobExecutedSuccessfully(job);
//    //        ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //    }
//    //}
//
//    //TEST_P(TestEnumeratorFixtureWithConcurrencyParams, EnumerateTestTargetsWithCallback_EnumerationsMatchTestSuitesInTarget)
//    //{
//    //    // Given a client callback function that tracks the number of successful enumerations
//    //    size_t numSuccesses = 0;
//    //    const auto jobCallback = [&numSuccesses]([[maybe_unused]] const TestImpact::TestEnumerator::JobInfo& jobInfo, const TestImpact::JobMeta& meta)
//    //    {
//    //        if (meta.m_result == TestImpact::JobResult::ExecutedWithSuccess)
//    //        {
//    //            numSuccesses++;
//    //        }
//    //    };
//
//    //    // Given a test enumerator with no enumeration timeout or enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        jobCallback,
//    //        m_maxConcurrency,
//    //        AZStd::nullopt,
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job for each test target with no enumeration caching
//    //    for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//    //    {
//    //        JobData jobData(m_testTargetPaths[jobId].second, AZStd::nullopt);
//    //        m_jobInfos.emplace_back(JobInfo({jobId}, m_testTargetJobArgs[jobId], AZStd::move(jobData)));
//    //    }
//
//    //    // When the test enumeration jobs are executed
//    //    const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, JobExceptionPolicy::Never);
//
//    //    // Expect the number of successful enumerations tracked in the callback to match the number of test targets enumerated
//    //    EXPECT_EQ(numSuccesses, enumerationJobs.size());
//
//    //    // Expect each job to successfully result in a test enumeration that matches the expected test enumeration data for that test target
//    //    for (const auto& job : enumerationJobs)
//    //    {
//    //        const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//    //        ValidateJobExecutedSuccessfully(job);
//    //        ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //    }
//    //}
//
//    //TEST_P(TestEnumeratorFixtureWithConcurrencyParams, JobRunnerTimeout_InFlightJobsTimeoutAndQueuedJobsUnlaunched)
//    //{
//    //    // Given a test enumerator with no client callback or enumerator timeout and 500ms enumeration timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        m_maxConcurrency,
//    //        AZStd::chrono::milliseconds(500),
//    //        AZStd::nullopt);
//
//    //    // Given an test enumeration job for each test target with no enumeration caching where half will sleep indefinitely
//    //    for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//    //    {
//    //        JobData jobData(m_testTargetPaths[jobId].second, AZStd::nullopt);
//    //        const Command args = (jobId % 2)
//    //            ? Command{ AZStd::string::format("%s %s", ValidProcessPath, ConstructTestProcessArgs(jobId, LongSleep).c_str()) }
//    //            : m_testTargetJobArgs[jobId];
//    //        m_jobInfos.emplace_back(JobInfo({jobId}, args, AZStd::move(jobData)));
//    //    }
//
//    //    // When the test enumeration jobs are executed
//    //    const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, JobExceptionPolicy::Never);
//
//    //    // Expect half the jobs to successfully result in a test enumeration that matches the expected test enumeration data for that test
//    //    // target with the other half having timed out
//    //    for (const auto& job : enumerationJobs)
//    //    {
//    //        const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//    //        if (jobId % 2)
//    //        {
//    //            ValidateJobTimeout(job);
//    //        }
//    //        else
//    //        {
//    //            ValidateJobExecutedSuccessfully(job);
//    //            ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //        }
//    //    }
//    //}
//
//    //TEST_F(TestEnumeratorFixture, JobTimeout_InFlightJobTimeoutAndQueuedJobsUnlaunched)
//    //{
//    //    // Given a test enumerator with no client callback or enumeration timeout and a 5 second enumerator timeout
//    //    m_testEnumerator = AZStd::make_unique<TestImpact::TestEnumerator>(
//    //        AZStd::nullopt,
//    //        FourConcurrentProcesses,
//    //        AZStd::nullopt,
//    //        AZStd::chrono::milliseconds(5000));
//
//    //    // Given an test enumeration job for each test target with no enumeration caching where half will sleep indefinitely
//    //    for (size_t jobId = 0; jobId < m_testTargetJobArgs.size(); jobId++)
//    //    {
//    //        JobData jobData(m_testTargetPaths[jobId].second, AZStd::nullopt);
//    //        const Command args = (jobId % 2)
//    //            ? Command{ AZStd::string::format("%s %s", ValidProcessPath, ConstructTestProcessArgs(jobId, LongSleep).c_str()) }
//    //            : m_testTargetJobArgs[jobId];
//    //        m_jobInfos.emplace_back(JobInfo({jobId}, args, AZStd::move(jobData)));
//    //    }
//
//    //    // When the test enumeration jobs are executed
//    //    const auto enumerationJobs = m_testEnumerator->Enumerate(m_jobInfos, CacheExceptionPolicy::Never, JobExceptionPolicy::Never);
//
//    //    // Expect half the jobs to successfully result in a test enumeration that matches the expected test enumeration data for that test
//    //    // target with the other half having timed out
//    //    for (const auto& job : enumerationJobs)
//    //    {
//    //        const JobInfo::IdType jobId = job.GetJobInfo().GetId().m_value;
//    //        if (jobId % 2)
//    //        {
//    //            ValidateJobTimeout(job);
//    //        }
//    //        else
//    //        {
//    //            ValidateJobExecutedSuccessfully(job);
//    //            ValidateTestTargetEnumeration(job.GetPayload().value(), m_expectedTestTargetEnumerations[jobId]);
//    //        }
//    //    }
//    //}
//
//    INSTANTIATE_TEST_CASE_P(
//        ,
//        TestEnumeratorFixtureWithConcurrencyAndJobExceptionParams,
//        ::testing::Combine(
//            ::testing::ValuesIn(MaxConcurrentEnumerations),
//            ::testing::ValuesIn(JobExceptionPolicies))
//        );
//
//    INSTANTIATE_TEST_CASE_P(
//        ,
//        TestEnumeratorFixtureWithConcurrencyAndCacheExceptionParams,
//        ::testing::Combine(
//            ::testing::ValuesIn(MaxConcurrentEnumerations),
//            ::testing::ValuesIn(CacheExceptionPolicies))
//        );
//
//    INSTANTIATE_TEST_CASE_P(
//        ,
//        TestEnumeratorFixtureWithConcurrencyParams,
//        ::testing::ValuesIn(MaxConcurrentEnumerations)
//        );
//} // namespace UnitTest
