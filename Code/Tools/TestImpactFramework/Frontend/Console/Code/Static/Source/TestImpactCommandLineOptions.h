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

#include <TestImpactFramework/TestImpactRuntime.h> // MOVE THESE ENUMS ETC. OUT OF THIS FILE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_set.h>

namespace TestImpact
{
    enum class TestSequenceType
    {
        Seed,
        Regular,
        ImpactAnalysis,
        ImpactAnalysisOrSeed
    };

    class CommandLineOptions
    {
    public:
        struct OutputChangeList
        {
            bool m_stdOut;
            AZStd::optional<AZ::IO::Path> m_file;
        };

        CommandLineOptions(int argc, char** argv);
        static AZStd::string GetCommandLineUsageString();

        bool HasUnifiedDiffFile() const;
        bool HasOutputChangeList() const;
        bool HasTestSequence() const;
        bool HasSafeMode() const;

        const AZ::IO::Path& GetConfigurationFile() const;
        const AZStd::optional<AZ::IO::Path>& GetUnifiedDiffFile() const;
        const AZStd::optional<OutputChangeList>& GetOutputChangeList() const;
        const AZStd::optional<TestSequenceType>& GetTestSequenceType() const;
        TestPrioritizationPolicy GetTestPrioritizationPolicy() const;
        ExecutionFailurePolicy GetExecutionFailurePolicy() const;
        ExecutionFailureDraftingPolicy GetExecutionFailureDraftingPolicy() const;
        TestFailurePolicy GetTestFailurePolicy() const;
        IntegrityFailurePolicy GetIntegrityFailurePolicy() const;
        TestShardingPolicy GetTestShardingPolicy() const;
        TargetOutputCapture GetTargetOutputCapture() const;
        const AZStd::optional<size_t>& GetMaxConcurrency() const;
        const AZStd::optional<AZStd::chrono::milliseconds>& GetTestTargetTimeout() const;
        const AZStd::optional<AZStd::chrono::milliseconds>& GetGlobalTimeout() const;
        const AZStd::unordered_set<AZStd::string>& GetSuitesFilter() const;

    private:
        AZ::IO::Path m_configurationFile;
        AZStd::optional<AZ::IO::Path> m_unifiedDiffFile;
        AZStd::optional<OutputChangeList> m_outputChangeList;
        AZStd::optional<TestSequenceType> m_testSequenceType;
        TestPrioritizationPolicy m_testPrioritizationPolicy = TestPrioritizationPolicy::None;
        ExecutionFailurePolicy m_executionFailurePolicy = ExecutionFailurePolicy::Continue;
        ExecutionFailureDraftingPolicy m_executionFailureDraftingPolicy = ExecutionFailureDraftingPolicy::Always;
        TestFailurePolicy m_testFailurePolicy = TestFailurePolicy::Abort;
        IntegrityFailurePolicy m_integrityFailurePolicy = IntegrityFailurePolicy::Abort;
        TestShardingPolicy m_testShardingPolicy = TestShardingPolicy::Never;
        TargetOutputCapture m_targetOutputCapture = TargetOutputCapture::None;
        AZStd::optional<size_t> m_maxConcurrency;
        AZStd::optional<AZStd::chrono::milliseconds> m_testTargetTimeout;
        AZStd::optional<AZStd::chrono::milliseconds> m_globalTimeout;
        AZStd::unordered_set<AZStd::string> m_suitesFilter;
        bool m_safeMode = false;
    };
}
