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

#include <TestImpactFramework/TestImpactConfiguration.h>
#include <TestImpactFramework/TestImpactTestSelection.h>
#include <TestImpactFramework/TestImpactFailureReport.h>

#include <Artifact/Static/TestImpactTestTargetMeta.h>
#include <Artifact/Static/TestImpactBuildTargetDescriptor.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>
#include <Dependency/TestImpactSourceCoveringTestsList.h>
#include <Target/TestImpactTestTarget.h>
#include <TestEngine/Enumeration/TestImpactTestEnumeration.h>
#include <TestEngine/TestImpactTestEngineInstrumentedRun.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    TestTargetMetaMap ReadTestTargetMetaMapFile(const TestTargetMetaConfig& testTargetMetaConfig);

    AZStd::vector<TestImpact::BuildTargetDescriptor> ReadBuildTargetDescriptorFiles(const BuildTargetDescriptorConfig& buildTargetDescriptorConfig);

    AZStd::unique_ptr<TestImpact::DynamicDependencyMap> ConstructDynamicDependencyMap(
        const TestTargetMetaConfig& testTargetMetaConfig,
        const BuildTargetDescriptorConfig& buildTargetDescriptorConfig);

    AZStd::unordered_set<const TestTarget*> ConstructTestTargetExcludeList(const TestTargetList& testTargets, const TargetConfig& targetConfig);

    TestSelection CreateTestSelection(
        const AZStd::vector<const TestTarget*>& includedTestTargets,
        const AZStd::vector<const TestTarget*>& excludedTestTargets);

    SourceCoveringTestsList CreateSourceCoveringTestFromTestCoverages(const AZStd::vector<TestEngineInstrumentedRun>& jobs, const AZ::IO::Path& root);

    template<typename TestJob>
    FailureReport CreateFailureReport([[maybe_unused]] const AZStd::vector<TestJob>& testJobs)
    {
        AZStd::vector<ExecutionFailure> executionFailures;
        AZStd::vector<LauncherFailure> launcherFailures;
        AZStd::vector<TestRunFailure> testRunFailures;
        AZStd::vector<TargetFailure> unexecutedTestRsuns;

        //for (const auto& testJob : testJobs)
        //{
        //    //switch (testJob.GetResult())
        //    //{
        //    //case JobResult::FailedToExecute:
        //    //{
        //    //    executionFailures.push_back()
        //    //}
        //    //}
        //}

        return FailureReport(AZStd::move(executionFailures), AZStd::move(launcherFailures), AZStd::move(testRunFailures), AZStd::move(unexecutedTestRsuns));
    }
}
