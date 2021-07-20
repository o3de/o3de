/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRuntimeException.h>

#include <AzCore/std/containers/array.h>

namespace TestImpact
{
    namespace Policy
    {
        //! Policy for handling of test targets that fail to execute (e.g. due to the binary not being found).
        //! @note Test targets that fail to execute will be tagged such that their execution can be attempted at a later date. This is
        //! important as otherwise it would be erroneously assumed that they cover no sources due to having no entries in the dynamic
        //! dependency map.
        enum class ExecutionFailure
        {
            Abort, //!< Abort the test sequence and report a failure.
            Continue, //!< Continue the test sequence but treat the execution failures as test failures after the run.
            Ignore //!< Continue the test sequence and ignore the execution failures.
        };

        //! Policy for handling the coverage data of failed tests targets (both test that failed to execute and tests that ran but failed).
        enum class FailedTestCoverage
        {
            Discard, //!< Discard the coverage data produced by the failing tests, causing them to be drafted into future test runs.
            Keep //!< Keep any existing coverage data and update the coverage data for failed test targetss that produce coverage.
        };

        //! Policy for prioritizing selected tests.
        enum class TestPrioritization
        {
            None, //!< Do not attempt any test prioritization.
            DependencyLocality //!< Prioritize test targets according to the locality of the production targets they cover in the build dependency graph.
        };

        //! Policy for handling test targets that report failing tests.
        enum class TestFailure
        {
            Abort, //!< Abort the test sequence and report the test failure.
            Continue //!< Continue the test sequence and report the test failures after the run.
        };

        //! Policy for handling integrity failures of the dynamic dependency map and the source to target mappings.
        enum class IntegrityFailure
        {
            Abort, //!< Abort the test sequence and report the test failure.
            Continue //!< Continue the test sequence and report the test failures after the run.
        };

        //! Policy for updating the dynamic dependency map with the coverage data of produced by test sequences.
        enum class DynamicDependencyMap
        {
            Discard, //!< Discard the coverage data produced by test sequences.
            Update //!< Update the dynamic dependency map with the coverage data produced by test sequences.
        };

        //! Policy for sharding test targets that have been marked for test sharding.
        enum class TestSharding
        {
            Never, //!< Do not shard any test targets.
            Always //!< Shard all test targets that have been marked for test sharding.
        };

        //! Standard output capture of test target runs. 
        enum class TargetOutputCapture
        {
            None, //!< Do not capture any output.
            StdOut, //!< Send captured output to standard output
            File, //!< Write captured output to file.
            StdOutAndFile //!< Send captured output to standard output and write to file.
        };
    }   

    //! Configuration for test targets that opt in to test sharding.
    enum class ShardConfiguration
    {
        Never, //!< Never shard this test target.
        FixtureContiguous, //!< Each shard contains contiguous fixtures of tests (safest but least optimal).
        TestContiguous, //!< Each shard contains contiguous tests agnostic of fixtures.
        FixtureInterleaved, //!< Fixtures of tests are interleaved across shards.
        TestInterleaved //!< Tests are interlaced across shards agnostic of fixtures (fastest but prone to inter-test dependency problems).
    };

    //! Test suite types to select from.
    enum class SuiteType : AZ::u8
    {
        Main = 0,
        Periodic,
        Sandbox
    };

    //! User-friendly names for the test suite types.
    inline AZStd::string GetSuiteTypeName(SuiteType suiteType)
    {
        switch (suiteType)
        {
        case SuiteType::Main:
            return "main";
        case SuiteType::Periodic:
            return "periodic";
        case SuiteType::Sandbox:
            return "sandbox";
        default:
            throw(RuntimeException("Unexpected suite type"));
        }
    }

    //! Result of a test sequence that was run.
    enum class TestSequenceResult
    {
        Success, //!< All tests ran with no failures.
        Failure, //!< One or more tests failed and/or timed out and/or failed to launch and/or an integrity failure was encountered.
        Timeout //!< The global timeout for the sequence was exceeded.
    };
} // namespace TestImpact
