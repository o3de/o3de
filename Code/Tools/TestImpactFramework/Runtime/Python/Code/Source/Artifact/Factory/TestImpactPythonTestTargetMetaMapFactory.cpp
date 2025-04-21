/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Factory/TestImpactPythonTestTargetMetaMapFactory.h>
#include <Artifact/Factory/TestImpactTestTargetMetaMapFactoryUtils.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/JSON/document.h>

namespace TestImpact
{
    PythonTestTargetMetaMap PythonTestTargetMetaMapFactory(
        const AZStd::string& testListData, const SuiteSet& suiteSet, const SuiteLabelExcludeSet& suiteLabelExcludeSet)
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "python",
            "test",
            "tests",
            "suites",
            "suite",
            "namespace",
            "name",
            "timeout",
            "script",
            "command",
            "labels"
        };

        enum Fields
        {
            PythonKey,
            TestKey,
            TestsKey,
            TestSuitesKey,
            SuiteKey,
            NamespaceKey,
            NameKey,
            TimeoutKey,
            ScriptKey,
            TestCommandKey,
            SuiteLabelsKey,
            // Checksum
            _CHECKSUM_
        };

        static_assert(Fields::_CHECKSUM_ == AZStd::size(Keys));
        AZ_TestImpact_Eval(!testListData.empty(), ArtifactException, "Test meta-data cannot be empty");

        PythonTestTargetMetaMap testMetas;
        rapidjson::Document masterTestList;

        if (masterTestList.Parse(testListData.c_str()).HasParseError())
        {
            throw TestImpact::ArtifactException("Could not parse test meta-data");
        }

        const auto tests = masterTestList[Keys[PythonKey]][Keys[TestKey]][Keys[TestsKey]].GetArray();
        for (const auto& test : tests)
        {
            PythonTestTargetMeta testMeta;
            AZStd::string name = test[Keys[NameKey]].GetString();
            AZ_TestImpact_Eval(!name.empty(), ArtifactException, "Test name field cannot be empty");
            testMeta.m_testTargetMeta.m_namespace = test[Keys[NamespaceKey]].GetString();

            const auto testSuites = test[Keys[TestSuitesKey]].GetArray();
            for (const auto& suite : testSuites)
            {
                // Check to see if this test target has a suite we're looking for (first suite to
                // match will be "the" suite for this test)
                if (const auto suiteName = suite[Keys[SuiteKey]].GetString();
                    suiteSet.contains(suiteName))
                {
                    if (auto labelSet = ExtractTestSuiteLabelSet(suite[Keys[SuiteLabelsKey]].GetArray(), suiteLabelExcludeSet);
                        labelSet.has_value())
                    {
                        testMeta.m_testTargetMeta.m_suiteMeta.m_labelSet = AZStd::move(labelSet.value());
                        testMeta.m_testTargetMeta.m_suiteMeta.m_name = suiteName;
                        testMeta.m_testTargetMeta.m_suiteMeta.m_timeout = AZStd::chrono::seconds{ suite[Keys[TimeoutKey]].GetUint() };
                        testMeta.m_scriptMeta.m_scriptPath = suite[Keys[ScriptKey]].GetString();
                        testMeta.m_scriptMeta.m_testCommand = suite[Keys[TestCommandKey]].GetString();
                        testMetas.emplace(AZStd::move(name), AZStd::move(testMeta));
                    }

                    // We either have one matching suite or the suite contians a label in the exclude set so we will break
                    // out of the suite loop
                    break;
                }
            }
        }

        // If there's no tests in the repo then something is seriously wrong
        AZ_TestImpact_Eval(!testMetas.empty(), ArtifactException, "No tests were found in the repository");

        return testMetas;
    }
} // namespace TestImpact
