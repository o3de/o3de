/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRuntimeException.h>
#include <TestImpactFramework/TestImpactPolicy.h>

#include <AzCore/std/containers/array.h>

namespace TestImpact
{
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
        Sandbox,
        AWSI
    };

    //! Result of a test sequence that was run.
    enum class TestSequenceResult
    {
        Success, //!< All tests ran with no failures.
        Failure, //!< One or more tests failed and/or timed out and/or failed to launch and/or an integrity failure was encountered.
        Timeout //!< The global timeout for the sequence was exceeded.
    };

    //! Base representation of runtime policies.
    struct PolicyStateBase
    {
        Policy::ExecutionFailure m_executionFailurePolicy = Policy::ExecutionFailure::Continue;
        Policy::FailedTestCoverage m_failedTestCoveragePolicy = Policy::FailedTestCoverage::Keep;
        Policy::TestFailure m_testFailurePolicy = Policy::TestFailure::Abort;
        Policy::IntegrityFailure m_integrityFailurePolicy = Policy::IntegrityFailure::Abort;
        Policy::TestSharding m_testShardingPolicy = Policy::TestSharding::Never;
        Policy::TargetOutputCapture m_targetOutputCapture = Policy::TargetOutputCapture::None;
    };

    //! Representation of regular and seed sequence policies.
    struct SequencePolicyState
    {
        PolicyStateBase m_basePolicies;
    };

    //! Representation of impact analysis sequence policies.
    struct ImpactAnalysisSequencePolicyState
    {
        PolicyStateBase m_basePolicies;
        Policy::TestPrioritization m_testPrioritizationPolicy = Policy::TestPrioritization::None;
        Policy::DynamicDependencyMap m_dynamicDependencyMap = Policy::DynamicDependencyMap::Update;
    };

    //! Representation of safe impact analysis sequence policies.
    struct SafeImpactAnalysisSequencePolicyState
    {
        PolicyStateBase m_basePolicies;
        Policy::TestPrioritization m_testPrioritizationPolicy = Policy::TestPrioritization::None;
    };
} // namespace TestImpact
