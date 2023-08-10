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

#include <AzCore/std/containers/set.h>

namespace TestImpact
{
    //! Set of test suites that tests can be drawn from.
    //! @note An ordered set is used so that the serialized string of the set order is always the same regardless of the order that the
    //! suites are specified.
    using SuiteSet = AZStd::set<AZStd::string>;

    //! Set of test suite labels that will be used to exclude any test targets that have test suite labels matching any labels in this set.
    //! @note An ordered set is used so that the serialized string of the set order is always the same regardless of the order that the
    //! labels are specified.
    using SuiteLabelExcludeSet = AZStd::set<AZStd::string>;

    //! The CTest label that test target suites need to have in order to be run as part of TIAF.
    inline constexpr const char* RequiresTiafLabel = "REQUIRES_tiaf";

    //! The CTest label that test target suites need to have in order to be sharded test-by-test.
    inline constexpr const char* TiafShardingTestInterleavedLabel = "TIAF_shard_test";

    //! The CTest label that test target suites need to have in order to be sharded fixture-by-fixture.
    inline constexpr const char* TiafShardingFixtureInterleavedLabel = "TIAF_shard_fixture";

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
