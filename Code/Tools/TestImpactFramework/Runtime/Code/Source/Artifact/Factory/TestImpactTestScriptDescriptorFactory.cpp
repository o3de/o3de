/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Factory/TestImpactTestScriptDescriptorFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/JSON/document.h>

namespace TestImpact
{
    AZStd::vector<TestScriptDescriptor> TestScriptDescriptorFactory(const AZStd::string& masterTestListData, SuiteType suiteType)
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "python",
            "test",
            "tests",
            "suites",
            "suite",
            "name",
            "command",
            "timeout",
            "script"
        };

        enum
        {
            PythonKey,
            TestKey,
            TestsKey,
            TestSuitesKey,
            SuiteKey,
            NameKey,
            CommandKey,
            TimeoutKey,
            ScriptKey
        };

        AZ_TestImpact_Eval(!masterTestListData.empty(), ArtifactException, "Test meta-data cannot be empty");

        AZStd::vector<TestScriptDescriptor> testScriptDescriptors;
        rapidjson::Document masterTestList;

        const auto tests = masterTestList[Keys[PythonKey]][Keys[TestKey]][Keys[TestsKey]].GetArray();
        for (const auto& test : tests)
        {
            TestScriptDescriptor testScriptDescriptor;
            const auto testSuites = test[Keys[TestSuitesKey]].GetArray();
            for (const auto& suite : testSuites)
            {
                // Check to see if this test target has the suite we're looking for
                if (const auto suiteName = suite[Keys[SuiteKey]].GetString();
                    strcmp(SuiteTypeAsString(suiteType).c_str(), suiteName) == 0)
                {
                    testScriptDescriptor.m_suiteMeta.m_name = suiteName;
                    testScriptDescriptor.m_suiteMeta.m_timeout = AZStd::chrono::seconds{ suite[Keys[TimeoutKey]].GetUint() };
                    testScriptDescriptor.m_name = test[Keys[NameKey]].GetString();
                    testScriptDescriptor.m_scriptPath = test[Keys[ScriptKey]].GetString();

                    AZ_TestImpact_Eval(!testScriptDescriptor.m_name.empty(), ArtifactException, "Test name field cannot be empty");
                    AZ_TestImpact_Eval(!testScriptDescriptor.m_scriptPath.empty(), ArtifactException, "Test script field cannot be empty");
                    testScriptDescriptors.push_back(AZStd::move(testScriptDescriptor));
                }
            }
        }

        return testScriptDescriptors;
    }
} // namespace TestImpact
