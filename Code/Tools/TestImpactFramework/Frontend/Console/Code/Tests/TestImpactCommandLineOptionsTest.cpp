/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactCommandLineOptionsException.h>
#include <TestImpactCommandLineOptions.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class CommandLineOptionsTestFixture
        : public AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();
            m_args.push_back("program.exe");
        }
    protected:
        void InitOptions();

        AZStd::unique_ptr<TestImpact::CommandLineOptions> m_options;
        AZStd::vector<const char*> m_args;
    };

    void CommandLineOptionsTestFixture::InitOptions()
    {
        m_options = AZStd::make_unique<TestImpact::CommandLineOptions>(m_args.size(), const_cast<char**>(m_args.data()));
    }

    TEST_F(CommandLineOptionsTestFixture, CheckEmptyArgs_ExpectDefaultValues)
    {
        InitOptions();
        EXPECT_EQ(m_options->GetConfigurationFile(), LY_TEST_IMPACT_DEFAULT_CONFIG_FILE);
        EXPECT_EQ(m_options->GetFailedTestCoveragePolicy(), TestImpact::Policy::FailedTestCoverage::Keep);
        EXPECT_EQ(m_options->GetExecutionFailurePolicy(), TestImpact::Policy::ExecutionFailure::Continue);
        EXPECT_FALSE(m_options->GetGlobalTimeout().has_value());
        EXPECT_FALSE(m_options->GetTestTargetTimeout().has_value());
        EXPECT_FALSE(m_options->GetMaxConcurrency().has_value());
        EXPECT_FALSE(m_options->HasOutputChangeList());
        EXPECT_EQ(m_options->GetTargetOutputCapture(), TestImpact::Policy::TargetOutputCapture::None);
        EXPECT_EQ(m_options->GetTestFailurePolicy(), TestImpact::Policy::TestFailure::Abort);
        EXPECT_EQ(m_options->GetIntegrityFailurePolicy(), TestImpact::Policy::IntegrityFailure::Abort);
        EXPECT_EQ(m_options->GetTestPrioritizationPolicy(), TestImpact::Policy::TestPrioritization::None);
        EXPECT_EQ(m_options->GetTestSequenceType(), TestImpact::TestSequenceType::None);
        EXPECT_EQ(m_options->GetTestShardingPolicy(), TestImpact::Policy::TestSharding::Never);
        EXPECT_FALSE(m_options->HasDataFile());
        EXPECT_FALSE(m_options->GetDataFile().has_value());
        EXPECT_FALSE(m_options->HasChangeListFile());
        EXPECT_FALSE(m_options->GetChangeListFile().has_value());
        EXPECT_FALSE(m_options->HasSafeMode());
        EXPECT_EQ(m_options->GetSuiteFilter(), TestImpact::SuiteType::Main);
    }

    TEST_F(CommandLineOptionsTestFixture, ConfigurationFileHasEmptyPath_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-config");
        
        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, ConfigurationFileHasSpecifiedPath_ExpectPath)
    {
        m_args.push_back("-config");
        m_args.push_back("Foo\\Bar");
        InitOptions();
        EXPECT_STREQ(m_options->GetConfigurationFile().c_str(), "Foo\\Bar");
    }

    TEST_F(CommandLineOptionsTestFixture, ConfigurationFileHasMultiplePaths_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-config");
        m_args.push_back("value1,value2");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, DataFileHasEmptyPath_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-datafile");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, DataFileHasSpecifiedPath_ExpectPath)
    {
        m_args.push_back("-datafile");
        m_args.push_back("Foo\\Bar");
        InitOptions();
        EXPECT_TRUE(m_options->HasDataFile());
        EXPECT_STREQ(m_options->GetDataFile()->c_str(), "Foo\\Bar");
    }

    TEST_F(CommandLineOptionsTestFixture, DataFileHasMultiplePaths_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-datafile");
        m_args.push_back("value1,value2");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, ChangeListFileHasEmptyPath_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-changelist");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, ChangeListFileHasSpecifiedPath_ExpectPath)
    {
        m_args.push_back("-changelist");
        m_args.push_back("Foo\\Bar");
        InitOptions();
        EXPECT_TRUE(m_options->HasChangeListFile());
        EXPECT_STREQ(m_options->GetChangeListFile()->c_str(), "Foo\\Bar");
    }

    TEST_F(CommandLineOptionsTestFixture, ChangeListFileHasMultiplePaths_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-changelist");
        m_args.push_back("value1,value2");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, SequenceReportFileHasEmptyPath_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-report");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, SequenceReportFileHasSpecifiedPath_ExpectPath)
    {
        m_args.push_back("-report");
        m_args.push_back("Foo\\Bar");
        InitOptions();
        EXPECT_TRUE(m_options->HasSequenceReportFile());
        EXPECT_STREQ(m_options->HasSequenceReportFile()->c_str(), "Foo\\Bar");
    }

    TEST_F(CommandLineOptionsTestFixture, SequenceReportFileHasMultiplePaths_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-report");
        m_args.push_back("value1,value2");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, TestSequenceTypeHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-sequence");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TestSequenceTypeHasNoneOption_ExpectNoneTestSequenceType)
    {
        m_args.push_back("-sequence");
        m_args.push_back("none");
        InitOptions();
        EXPECT_EQ(m_options->GetTestSequenceType(), TestImpact::TestSequenceType::None);
    }

    TEST_F(CommandLineOptionsTestFixture, TestSequenceTypeHasSeedOption_ExpectSeedTestSequenceType)
    {
        m_args.push_back("-sequence");
        m_args.push_back("seed");
        InitOptions();
        EXPECT_EQ(m_options->GetTestSequenceType(), TestImpact::TestSequenceType::Seed);
    }

    TEST_F(CommandLineOptionsTestFixture, TestSequenceTypeHasRegularOption_ExpectRegularTestSequenceType)
    {
        m_args.push_back("-sequence");
        m_args.push_back("regular");
        InitOptions();
        EXPECT_EQ(m_options->GetTestSequenceType(), TestImpact::TestSequenceType::Regular);
    }

    TEST_F(CommandLineOptionsTestFixture, TestSequenceTypeHasImpactAnalysisOption_ExpectImpactAnalysisTestSequenceType)
    {
        m_args.push_back("-sequence");
        m_args.push_back("tia");
        InitOptions();
        EXPECT_EQ(m_options->GetTestSequenceType(), TestImpact::TestSequenceType::ImpactAnalysis);
    }

    TEST_F(CommandLineOptionsTestFixture, TestSequenceTypeHasImpactAnalysisNoWriteOption_ExpectImpactAnalysisNoWriteTestSequenceType)
    {
        m_args.push_back("-sequence");
        m_args.push_back("tianowrite");
        InitOptions();
        EXPECT_EQ(m_options->GetTestSequenceType(), TestImpact::TestSequenceType::ImpactAnalysisNoWrite);
    }

    TEST_F(CommandLineOptionsTestFixture, TestSequenceTypeHasSafeImpactAnalysisOption_ExpectSafeImpactAnalysisTestSequenceType)
    {
        m_args.push_back("-sequence");
        m_args.push_back("tiaorseed");
        InitOptions();
        EXPECT_EQ(m_options->GetTestSequenceType(), TestImpact::TestSequenceType::ImpactAnalysisOrSeed);
    }

    TEST_F(CommandLineOptionsTestFixture, TestSequenceTypeHasInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-sequence");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TestSequenceTypeHasMultipleValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-sequence");
        m_args.push_back("seed,tia");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, TestPrioritizationPolicyHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-ppolicy");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TestPrioritizationPolicyHasNoneOption_ExpectNoneTestPrioritizationPolicy)
    {
        m_args.push_back("-ppolicy");
        m_args.push_back("none");
        InitOptions();
        EXPECT_EQ(m_options->GetTestPrioritizationPolicy(), TestImpact::Policy::TestPrioritization::None);
    }

    TEST_F(CommandLineOptionsTestFixture, TestPrioritizationPolicyHasDependencyLocalityOption_ExpectDependencyLocalityTestPrioritizationPolicy)
    {
        m_args.push_back("-ppolicy");
        m_args.push_back("locality");
        InitOptions();
        EXPECT_EQ(m_options->GetTestPrioritizationPolicy(), TestImpact::Policy::TestPrioritization::DependencyLocality);
    }

    TEST_F(CommandLineOptionsTestFixture, TestPrioritizationPolicyInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-ppolicy");
        m_args.push_back("none,locality");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, ExecutionFailurePolicyHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-ppolicy");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, ExecutionFailurePolicyHasAbortOption_ExpectAbortExecutionFailurePolicy)
    {
        m_args.push_back("-epolicy");
        m_args.push_back("abort");
        InitOptions();
        EXPECT_EQ(m_options->GetExecutionFailurePolicy(), TestImpact::Policy::ExecutionFailure::Abort);
    }

    TEST_F(CommandLineOptionsTestFixture, ExecutionFailurePolicyHasContinueOption_ExpectContinueExecutionFailurePolicy)
    {
        m_args.push_back("-epolicy");
        m_args.push_back("continue");
        InitOptions();
        EXPECT_EQ(m_options->GetExecutionFailurePolicy(), TestImpact::Policy::ExecutionFailure::Continue);
    }

    TEST_F(CommandLineOptionsTestFixture, ExecutionFailurePolicyHasIgnoreOption_ExpectIgnoreExecutionFailurePolicy)
    {
        m_args.push_back("-epolicy");
        m_args.push_back("ignore");
        InitOptions();
        EXPECT_EQ(m_options->GetExecutionFailurePolicy(), TestImpact::Policy::ExecutionFailure::Ignore);
    }

    TEST_F(CommandLineOptionsTestFixture, ExecutionFailurePolicyHasInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-epolicy");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, ExecutionFailurePolicyHasMultipleValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-epolicy");
        m_args.push_back("abort,ingore");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, ExecutionFailureDraftingPolicyHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-rexecfailures");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, FailedTestCoveragePolicyHasKeepOption_ExpectKeepFailedTestCoveragePolicy)
    {
        m_args.push_back("-cpolicy");
        m_args.push_back("keep");
        InitOptions();
        EXPECT_EQ(m_options->GetFailedTestCoveragePolicy(), TestImpact::Policy::FailedTestCoverage::Keep);
    }

    TEST_F(CommandLineOptionsTestFixture, FailedTestCoveragePolicyHasDiscardOption_ExpectDiscardFailedTestCoveragePolicy)
    {
        m_args.push_back("-cpolicy");
        m_args.push_back("discard");
        InitOptions();
        EXPECT_EQ(m_options->GetFailedTestCoveragePolicy(), TestImpact::Policy::FailedTestCoverage::Discard);
    }

    TEST_F(CommandLineOptionsTestFixture, FailedTestCoveragePolicyInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-cpolicy");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, FailedTestCoveragePolicyPolicyHasMultipleValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("--cpolicy");
        m_args.push_back("keep,discard");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, TestFailurePolicyHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-fpolicy");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TestFailurePolicyHasAbortOption_ExpectAbortTestFailurePolicy)
    {
        m_args.push_back("-fpolicy");
        m_args.push_back("abort");
        InitOptions();
        EXPECT_EQ(m_options->GetTestFailurePolicy(), TestImpact::Policy::TestFailure::Abort);
    }

    TEST_F(CommandLineOptionsTestFixture, TestFailurePolicyHasContinueOption_ExpectContinueTestFailurePolicy)
    {
        m_args.push_back("-fpolicy");
        m_args.push_back("continue");
        InitOptions();
        EXPECT_EQ(m_options->GetTestFailurePolicy(), TestImpact::Policy::TestFailure::Continue);
    }

    TEST_F(CommandLineOptionsTestFixture, TestFailurePolicyInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-fpolicy");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TestFailurePolicyHasMultipeValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-fpolicy");
        m_args.push_back("abort,continue");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, IntegrityFailurePolicyHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-ipolicy");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, IntegrityFailurePolicyHasAbortOption_ExpectAbortIntegrityFailurePolicy)
    {
        m_args.push_back("-ipolicy");
        m_args.push_back("abort");
        InitOptions();
        EXPECT_EQ(m_options->GetIntegrityFailurePolicy(), TestImpact::Policy::IntegrityFailure::Abort);
    }

    TEST_F(CommandLineOptionsTestFixture, IntegrityFailurePolicyHasContinueOption_ExpectContinueIntegrityFailurePolicy)
    {
        m_args.push_back("-ipolicy");
        m_args.push_back("continue");
        InitOptions();
        EXPECT_EQ(m_options->GetIntegrityFailurePolicy(), TestImpact::Policy::IntegrityFailure::Continue);
    }

    TEST_F(CommandLineOptionsTestFixture, IntegrityFailurePolicyInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-ipolicy");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, IntegrityFailurePolicyHasMultipeValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-ipolicy");
        m_args.push_back("abort,continue");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, TestShardingHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-shard");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TestShardingHasOnOption_ExpectOnTestSharding)
    {
        m_args.push_back("-shard");
        m_args.push_back("on");
        InitOptions();
        EXPECT_EQ(m_options->GetTestShardingPolicy(), TestImpact::Policy::TestSharding::Always);
    }

    TEST_F(CommandLineOptionsTestFixture, TestShardingHasOffOption_ExpectOffTestSharding)
    {
        m_args.push_back("-shard");
        m_args.push_back("off");
        InitOptions();
        EXPECT_EQ(m_options->GetTestShardingPolicy(), TestImpact::Policy::TestSharding::Never);
    }

    TEST_F(CommandLineOptionsTestFixture, TestShardingInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-shard");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TestShardingHasMultipeValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-shard");
        m_args.push_back("on,off");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, TargetOutputCaptureHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-targetout");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TargetOutputCaptureHasStdOutOption_ExpectStdOutTargetOutputCapture)
    {
        m_args.push_back("-targetout");
        m_args.push_back("stdout");
        InitOptions();
        EXPECT_EQ(m_options->GetTargetOutputCapture(), TestImpact::Policy::TargetOutputCapture::StdOut);
    }

    TEST_F(CommandLineOptionsTestFixture, TargetOutputCaptureHasFileOption_ExpectFileTargetOutputCapture)
    {
        m_args.push_back("-targetout");
        m_args.push_back("file");
        InitOptions();
        EXPECT_EQ(m_options->GetTargetOutputCapture(), TestImpact::Policy::TargetOutputCapture::File);
    }

    TEST_F(CommandLineOptionsTestFixture, TargetOutputCaptureHasStdOutAndFileOption_ExpectStdOutAndFileTargetOutputCapture)
    {
        m_args.push_back("-targetout");
        m_args.push_back("stdout,file");
        InitOptions();
        EXPECT_EQ(m_options->GetTargetOutputCapture(), TestImpact::Policy::TargetOutputCapture::StdOutAndFile);
    }

    TEST_F(CommandLineOptionsTestFixture, TargetOutputCaptureInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-targetout");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TargetOutputCaptureHasExcessValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-targetout");
        m_args.push_back("stdout,file,stdout");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, MaxConcurrencyHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-maxconcurrency");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, MaxConcurrencyHasInRangeOptions_ExpectInRangeMaxConcurrency)
    {
        m_args.push_back("-maxconcurrency");
        m_args.push_back("10");
        InitOptions();
        EXPECT_EQ(m_options->GetMaxConcurrency(), 10);
    }

    TEST_F(CommandLineOptionsTestFixture, MaxConcurrencyHasOutOfRangeOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-maxconcurrency");
        m_args.push_back("-1");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, MaxConcurrencyInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-maxconcurrency");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, MaxConcurrencyHasMultipeValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-maxconcurrency");
        m_args.push_back("10,20");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, TestTargetTimeoutHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-ttimeout");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TestTargetTimeoutHasInRangeOptions_ExpectInRangeTestTargetTimeout)
    {
        m_args.push_back("-ttimeout");
        m_args.push_back("10");
        InitOptions();
        EXPECT_EQ(m_options->GetTestTargetTimeout(), AZStd::chrono::seconds(10));
    }

    TEST_F(CommandLineOptionsTestFixture, TestTargetTimeoutHasOutOfRangeOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-ttimeout");
        m_args.push_back("-1");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TestTargetTimeoutInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-ttimeout");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, TestTargetTimeoutHasMultipeValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-ttimeout");
        m_args.push_back("10,20");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, GlobalTimeoutHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-gtimeout");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, GlobalTimeoutHasInRangeOptions_ExpectInRangeGlobalTimeout)
    {
        m_args.push_back("-gtimeout");
        m_args.push_back("10");
        InitOptions();
        EXPECT_EQ(m_options->GetGlobalTimeout(), AZStd::chrono::seconds(10));
    }

    TEST_F(CommandLineOptionsTestFixture, GlobalTimeoutHasOutOfRangeOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-gtimeout");
        m_args.push_back("-1");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, GlobalTimeoutInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-gtimeout");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, GlobalTimeoutHasMultipeValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-gtimeout");
        m_args.push_back("10,20");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    //

    TEST_F(CommandLineOptionsTestFixture, SafeModeHasEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-safemode");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, SafeModeHasOnOption_ExpectOnSafeMode)
    {
        m_args.push_back("-safemode");
        m_args.push_back("on");
        InitOptions();
        EXPECT_TRUE(m_options->HasSafeMode());
    }

    TEST_F(CommandLineOptionsTestFixture, SafeModeHasOffOption_ExpectOffSafeMode)
    {
        m_args.push_back("-safemode");
        m_args.push_back("off");
        InitOptions();
        EXPECT_FALSE(m_options->HasSafeMode());
    }

    TEST_F(CommandLineOptionsTestFixture, SafeModeInvalidOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-safemode");
        m_args.push_back("foo");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, SafeModeHasMultipeValues_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-safemode");
        m_args.push_back("on,off");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, SuiteFilterEmptyOption_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-suite");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        }
        catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        }
        catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, SuiteFilterMultipleOptions_ExpectCommandLineOptionsException)
    {
        m_args.push_back("-suite");
        m_args.push_back("periodic,smoke");

        try
        {
            InitOptions();

            // Do not expect the command line options construction to succeed
            FAIL();
        } catch ([[maybe_unused]] const TestImpact::CommandLineOptionsException& e)
        {
            // Expect a command line options to be thrown
            SUCCEED();
        } catch (...)
        {
            // Do not expect any other exceptions
            FAIL();
        }
    }

    TEST_F(CommandLineOptionsTestFixture, SuitesFilterMainOption_ExpectMainSuiteFilter)
    {
        m_args.push_back("-suites");
        m_args.push_back("main");
        InitOptions();
        EXPECT_EQ(m_options->GetSuiteFilter(), TestImpact::SuiteType::Main);
    }

    TEST_F(CommandLineOptionsTestFixture, SuitesFilterPeriodicOption_ExpectPeriodicSuiteFilter)
    {
        m_args.push_back("-suites");
        m_args.push_back("periodic");
        InitOptions();
        EXPECT_EQ(m_options->GetSuiteFilter(), TestImpact::SuiteType::Periodic);
    }

    TEST_F(CommandLineOptionsTestFixture, SuitesFilterSandboxOption_ExpectSandboxSuiteFilter)
    {
        m_args.push_back("-suites");
        m_args.push_back("sandbox");
        InitOptions();
        EXPECT_EQ(m_options->GetSuiteFilter(), TestImpact::SuiteType::Sandbox);
    }
}
