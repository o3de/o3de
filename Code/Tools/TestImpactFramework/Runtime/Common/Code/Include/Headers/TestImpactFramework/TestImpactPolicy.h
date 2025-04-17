/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace TestImpact
{
    namespace Policy
    {
        //! Policy for handling of test targets that fail to execute (e.g. due to the binary not being found).
        //! @note Test targets that fail to execute will be tagged such that their execution can be attempted at a later date. This is
        //! important as otherwise it would be erroneously assumed that they cover no sources due to having no entries in the dynamic
        //! dependency map.
        enum class ExecutionFailure : AZ::u8
        {
            Abort, //!< Abort the test sequence and report a failure.
            Continue, //!< Continue the test sequence but treat the execution failures as test failures after the run.
            Ignore //!< Continue the test sequence and ignore the execution failures.
        };

        //! Policy for which test runner should be used when running Python Tests.
        enum class TestRunner : AZ::u8
        {
            UseNullTestRunner, //!< Use the Null Test Runner that consumes JUnit XML artifacts and carries out selection then returns those results without actually running the tests.
            UseLiveTestRunner //!< Use the normal Test Runner that executes the Python tests.
        };

        //! Policy for handling the coverage data of failed tests targets (both tests that failed to execute and tests that ran but failed).
        enum class FailedTestCoverage : AZ::u8
        {
            Discard, //!< Discard the coverage data produced by the failing tests, causing them to be drafted into future test runs.
            Keep //!< Keep any existing coverage data and update the coverage data for failed test targets that produce coverage.
        };

        //! Policy for prioritizing selected tests.
        enum class TestPrioritization : AZ::u8
        {
            None, //!< Do not attempt any test prioritization.
            DependencyLocality //!< Prioritize test targets according to the locality of the production targets they cover in the build
                               //!< dependency graph.
        };

        //! Policy for handling test targets that report failing tests.
        enum class TestFailure : AZ::u8
        {
            Abort, //!< Abort the test sequence and report the test failure.
            Continue //!< Continue the test sequence and report the test failures after the run.
        };

        //! Policy for handling integrity failures of the dynamic dependency map and the source to target mappings.
        enum class IntegrityFailure : AZ::u8
        {
            Abort, //!< Abort the test sequence and report the test failure.
            Continue //!< Continue the test sequence and report the test failures after the run.
        };


        //! Policy for drafting in test targets outside of the selection to be run in conjunction with the selected targets.
        enum class Drafting
        {
            NoCoverageOnly,
            FailingTestsOnly,
            All
        };

        //! Policy for updating the dynamic dependency map with the coverage data of produced by test sequences.
        enum class DynamicDependencyMap : AZ::u8
        {
            Discard, //!< Discard the coverage data produced by test sequences.
            Update //!< Update the dynamic dependency map with the coverage data produced by test sequences.
        };

        //! Standard output capture of test target runs.
        enum class TargetOutputCapture : AZ::u8
        {
            None, //!< Do not capture any output.
            StdOut, //!< Send captured output to standard output
            File, //!< Write captured output to file.
            StdOutAndFile //!< Send captured output to standard output and write to file.
        };
    } // namespace Policy
} // namespace TestImpact
