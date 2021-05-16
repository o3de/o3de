/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

namespace TestImpact
{
    class DynamicDependencyMap;

    //! Policy for handling of test targets that fail to execute (e.g. due to the binary not being found).
    //! @note Test targets that fail to execute will be tagged such that their execution can be attempted at a later date. This is
    //! important as otherwise it would be erroneously assumed that they cover no sources due to having no entries in the dynamic
    //! dependency map.
    enum class ExecutionFailurePolicy
    {
        Abort, //!< Abort the test sequence and report a failure.
        Continue, //!< Continue the test sequence but treat the execution failures as test failures after the run.
        Ignore //!< Continue the test sequence and ignore the execution failures.
    };

    //! Policy for handling test targets that report failing tests.
    enum class TestFailurePolicy
    {
        Abort, //!< Abort the test sequence and report the test failure.
        Continue //!< Continue the test sequence and report the test failures after the run.
    };

    //! Policy for handling integrity failures of the dynamic dependency map and the source to target mappings.
    enum class IntegrityFailurePolicy
    {
        Abort, //!< Abort the test sequence and report the test failure.
        Continue //!< Continue the test sequence and report the test failures after the run.
    };

    //! Policy for sharding test targets that have been marked for test sharding.
    enum class TestShardingPolicy
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

    enum class TestResult
    {
        NotExecuted,
        FailedToExecute,
        Timeout,
        TestFailures,
        Success
    };

    //! 
    enum class TestSequenceResult
    {
        Success, //! All tests ran with no failures.
        Failure/*,*/ //! One or more tests failed and/or timed out and/or failed to launch (if execution failure policy is not Ignore) and/or an integrity failure was encountered.
        /*Timeout*/ //! The global timeout for the sequence was exceeded.
        // TODO: deduce global timeout!!!
    };
}
