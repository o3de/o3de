/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Factory/TestImpactPythonTestTargetMetaMapFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/JSON/document.h>

namespace TestImpact
{
    PythonTestTargetMetaMap PythonTestTargetMetaMapFactory(const AZStd::string& testListData, SuiteType suiteType)
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
            "command"
        };

        enum
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
            TestCommandKey
        };

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
            const auto testSuites = test[Keys[TestSuitesKey]].GetArray();
            for (const auto& suite : testSuites)
            {
                // Check to see if this test target has the suite we're looking for
                if (const auto suiteName = suite[Keys[SuiteKey]].GetString();
                    strcmp(SuiteTypeAsString(suiteType).c_str(), suiteName) == 0)
                {
                    testMeta.m_testTargetMeta.m_namespace = test[Keys[NamespaceKey]].GetString();
                    testMeta.m_testTargetMeta.m_suiteMeta.m_name = suiteName;
                    testMeta.m_testTargetMeta.m_suiteMeta.m_timeout = AZStd::chrono::seconds{ suite[Keys[TimeoutKey]].GetUint() };
                    testMeta.m_scriptMeta.m_scriptPath = suite[Keys[ScriptKey]].GetString();
                    testMeta.m_scriptMeta.m_testCommand = suite[Keys[TestCommandKey]].GetString();

                    AZStd::string name = test[Keys[NameKey]].GetString();
                    AZ_TestImpact_Eval(!name.empty(), ArtifactException, "Test name field cannot be empty");
                    //AZ_TestImpact_Eval(!testMeta.m_scriptPath.empty(), ArtifactException, "Test script field cannot be empty");
                    testMetas.emplace(AZStd::move(name), AZStd::move(testMeta));
                    break;
                }
            }
        }

        // If there's no tests in the repo then something is seriously wrong
        AZ_TestImpact_Eval(!testMetas.empty(), ArtifactException, "No tests were found in the repository");

        return testMetas;
    }
} // namespace TestImpact
