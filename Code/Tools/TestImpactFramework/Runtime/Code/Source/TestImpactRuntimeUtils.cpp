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

#include <TestImpactFramework/TestImpactFileUtils.h>
#include <TestImpactFramework/TestImpactRuntimeException.h>

#include <TestImpactRuntimeUtils.h>
#include <Artifact/Factory/TestImpactTestTargetMetaMapFactory.h>
#include <Artifact/Factory/TestImpactBuildTargetDescriptorFactory.h>
#include <Artifact/Static/TestImpactTargetDescriptorCompiler.h>

#include <filesystem>

namespace TestImpact
{
    TestTargetMetaMap ReadTestTargetMetaMapFile(const RepoPath& testTargetMetaConfigFile)
    {
        const auto masterTestListData = ReadFileContents<RuntimeException>(testTargetMetaConfigFile);
        return TestTargetMetaMapFactory(masterTestListData);
    }

    AZStd::vector<TestImpact::BuildTargetDescriptor> ReadBuildTargetDescriptorFiles(const BuildTargetDescriptorConfig& buildTargetDescriptorConfig)
    {
        AZStd::vector<TestImpact::BuildTargetDescriptor> buildTargetDescriptors;
        for (const auto& buildTargetDescriptorFile : std::filesystem::directory_iterator(buildTargetDescriptorConfig.m_mappingDirectory.c_str()))
        {
            const auto buildTargetDescriptorContents = ReadFileContents<RuntimeException>(buildTargetDescriptorFile.path().string().c_str());
            auto buildTargetDescriptor = TestImpact::BuildTargetDescriptorFactory(
                buildTargetDescriptorContents,
                buildTargetDescriptorConfig.m_staticInclusionFilters,
                buildTargetDescriptorConfig.m_inputInclusionFilters,
                buildTargetDescriptorConfig.m_inputOutputPairer);
            buildTargetDescriptors.emplace_back(AZStd::move(buildTargetDescriptor));
        }

        return buildTargetDescriptors;
    }

    AZStd::unique_ptr<TestImpact::DynamicDependencyMap> ConstructDynamicDependencyMap(
        const BuildTargetDescriptorConfig& buildTargetDescriptorConfig,
        const TestTargetMetaConfig& testTargetMetaConfig)
    {
        auto testTargetmetaMap = ReadTestTargetMetaMapFile(testTargetMetaConfig.m_metaFile);
        auto buildTargetDescriptors = ReadBuildTargetDescriptorFiles(buildTargetDescriptorConfig);
        auto buildTargets = CompileTargetDescriptors(AZStd::move(buildTargetDescriptors), AZStd::move(testTargetmetaMap));
        auto&& [productionTargets, testTargets] = buildTargets;
        return AZStd::make_unique<TestImpact::DynamicDependencyMap>(AZStd::move(productionTargets), AZStd::move(testTargets));
    }

    AZStd::unordered_set<const TestTarget*> ConstructTestTargetExcludeList(const TestTargetList& testTargets, const AZStd::vector<AZStd::string>& excludedTestTargets)
    {
        AZStd::unordered_set<const TestTarget*> testTargetExcludeList;
        for (const auto& testTargetName : excludedTestTargets)
        {
            if (const auto* testTarget = testTargets.GetTarget(testTargetName); testTarget != nullptr)
            {
                testTargetExcludeList.insert(testTarget);
            }
        }

        return testTargetExcludeList;
    }

    AZStd::vector<AZStd::string> ExtractTestTargetNames(const AZStd::vector<const TestTarget*> testTargets)
    {
        AZStd::vector<AZStd::string> testNames;
        AZStd::transform(testTargets.begin(), testTargets.end(), AZStd::back_inserter(testNames), [](const TestTarget* testTarget)
        {
            return testTarget->GetName();
        });

        return testNames;
    }

    SourceCoveringTestsList CreateSourceCoveringTestFromTestCoverages(const AZStd::vector<TestEngineInstrumentedRun>& jobs, const RepoPath& root)
    {
        AZStd::unordered_map<AZStd::string, AZStd::unordered_set<AZStd::string>> coverage;
        for (const auto& job : jobs)
        {
            if (const auto testResult = job.GetTestResult();
                testResult == Client::TestRunResult::AllTestsPass || testResult == Client::TestRunResult::TestFailures)
            {
                if (testResult == Client::TestRunResult::AllTestsPass)
                {
                    // Passing tests should have coverage data, otherwise something is very wrong
                    AZ_TestImpact_Eval(
                        job.GetTestCoverge().has_value(),
                        RuntimeException,
                        AZStd::string::format(
                            "Test target '%s' completed its test run successfully but produced no coverage data",
                            job.GetTestTarget()->GetName().c_str()));
                }
                else if (!job.GetTestCoverge().has_value())
                {
                    // When a test run completes with failing tests but produces no coverage artifact that's typically a sign of the
                    // test aborting due to an unhandled exception, in which case ignore it and let it be picked up in the failure report
                    continue;
                }

                for (const auto& source : job.GetTestCoverge().value().GetSourcesCovered())
                {
                    coverage[source.String()].insert(job.GetTestTarget()->GetName());
                }
            }
        }

        AZStd::vector<SourceCoveringTests> sourceCoveringTests;
        sourceCoveringTests.reserve(coverage.size());
        for (auto&& [source, testTargets] : coverage)
        {
            if (const auto sourcePath = RepoPath(source);
                sourcePath.IsRelativeTo(root))
            {
                sourceCoveringTests.push_back(SourceCoveringTests(RepoPath(sourcePath.LexicallyRelative(root)), AZStd::move(testTargets)));
            }
            else
            {
                AZ_Warning("TestImpact", false, "Ignoring source, source it outside of repo: %s", sourcePath.c_str());
            }
        }

        return SourceCoveringTestsList(AZStd::move(sourceCoveringTests));
    }
}
