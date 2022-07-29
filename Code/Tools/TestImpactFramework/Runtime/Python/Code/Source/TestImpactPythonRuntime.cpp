/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/Python/TestImpactPythonRuntime.h>

namespace TestImpact
{
    PythonRuntime::PythonRuntime(
        PythonRuntimeConfig&& config,
        const AZStd::optional<RepoPath>& dataFile,
        [[maybe_unused]] const AZStd::optional<RepoPath>& previousRunDataFile,
        const AZStd::vector<ExcludedTarget>& testsToExclude,
        SuiteType suiteFilter,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::FailedTestCoverage failedTestCoveragePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::IntegrityFailure integrationFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapturey)
        //: m_config(AZStd::move(config))
        //, m_suiteFilter(suiteFilter)
        //, m_executionFailurePolicy(executionFailurePolicy)
        //, m_failedTestCoveragePolicy(failedTestCoveragePolicy)
        //, m_testFailurePolicy(testFailurePolicy)
        //, m_integrationFailurePolicy(integrationFailurePolicy)
        //, m_targetOutputCapture(targetOutputCapture)
    {
        // Construct the build targets from the build target descriptors
        m_buildTargets = ConstructPythonBuildTargetList(
            suiteFilter, m_config.m_commonConfig.m_buildTargetDescriptor, m_config.m_commonConfig.m_testTargetMeta);

        // Construct the dynamic dependency map from the build targets
        m_dynamicDependencyMap = AZStd::make_unique<DynamicDependencyMap<NativeTestTarget, NativeProductionTarget>>(m_buildTargets.get());

        // Construct the test selector and prioritizer from the dependency graph data (NOTE: currently not implemented)
        m_testSelectorAndPrioritizer = AZStd::make_unique<TestSelectorAndPrioritizer<NativeTestTarget, NativeProductionTarget>>(
            m_dynamicDependencyMap.get(), DependencyGraphDataMap{});

        // Construct the target exclude list from the exclude file if provided, otherwise use target configuration data
        if (!testsToExclude.empty())
        {
            // Construct using data from excludeTestFile
            m_regularTestTargetExcludeList =
                ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList(), testsToExclude);
            m_instrumentedTestTargetExcludeList =
                ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList(), testsToExclude);
        }
        else
        {
            // Construct using data from config file.
            m_regularTestTargetExcludeList = ConstructTestTargetExcludeList(
                m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList(), m_config.m_target.m_excludedRegularTestTargets);
            m_instrumentedTestTargetExcludeList = ConstructTestTargetExcludeList(
                m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList(), m_config.m_target.m_excludedInstrumentedTestTargets);
        }

        // Construct the test engine with the workspace path and launcher binaries
        m_testEngine = AZStd::make_unique<NativeTestEngine>(
            m_config.m_commonConfig.m_repo.m_root,
            m_config.m_target.m_outputDirectory,
            m_config.m_commonConfig.m_workspace.m_temp.m_enumerationCacheDirectory,
            m_config.m_commonConfig.m_workspace.m_temp.m_artifactDirectory,
            m_config.m_testEngine.m_testRunner.m_binary,
            m_config.m_testEngine.m_instrumentation.m_binary,
            m_maxConcurrency);

        try
        {
            if (dataFile.has_value())
            {
                m_sparTiaFile = dataFile.value().String();
            }
            else
            {
                m_sparTiaFile = m_config.m_commonConfig.m_workspace.m_active.m_root / RepoPath(SuiteTypeAsString(m_suiteFilter)) /
                    m_config.m_commonConfig.m_workspace.m_active.m_sparTiaFile;
            }

            // Populate the dynamic dependency map with the existing source coverage data (if any)
            const auto tiaDataRaw = ReadFileContents<Exception>(m_sparTiaFile);
            const auto tiaData = DeserializeSourceCoveringTestsList(tiaDataRaw);
            if (tiaData.GetNumSources())
            {
                m_dynamicDependencyMap->ReplaceSourceCoverage(tiaData);
                m_hasImpactAnalysisData = true;

                // Enumerate new test targets
                // const auto testTargetsWithNoEnumeration = m_dynamicDependencyMap->GetNotCoveringTests();
                // if (!testTargetsWithNoEnumeration.empty())
                //{
                //    m_testEngine->UpdateEnumerationCache(
                //        testTargetsWithNoEnumeration,
                //        Policy::ExecutionFailure::Ignore,
                //        Policy::TestFailure::Continue,
                //        AZStd::nullopt,
                //        AZStd::nullopt,
                //        AZStd::nullopt);
                //}
            }
        } catch (const DependencyException& e)
        {
            if (integrationFailurePolicy == Policy::IntegrityFailure::Abort)
            {
                throw RuntimeException(e.what());
            }
        } catch ([[maybe_unused]] const Exception& e)
        {
            AZ_Printf(
                LogCallSite,
                AZStd::string::format(
                    "No test impact analysis data found for suite '%s' at %s\n",
                    SuiteTypeAsString(m_suiteFilter).c_str(),
                    m_sparTiaFile.c_str())
                    .c_str());
        }
    }

    NativeRuntime::~NativeRuntime() = default;
} // namespace TestImpact
