/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Factory/TestImpactPythonTestTargetDescriptorFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/JSON/document.h>

namespace TestImpact
{
    AZStd::vector<PythonTestTargetDescriptor> PythonTestTargetDescriptorFactory(const AZStd::string& masterTestListData, SuiteType suiteType)
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

        AZStd::vector<PythonTestTargetDescriptor> PythonTestTargetDescriptors;
        rapidjson::Document masterTestList;

        const auto tests = masterTestList[Keys[PythonKey]][Keys[TestKey]][Keys[TestsKey]].GetArray();
        for (const auto& test : tests)
        {
            PythonTestTargetDescriptor PythonTestTargetDescriptor;
            const auto testSuites = test[Keys[TestSuitesKey]].GetArray();
            for (const auto& suite : testSuites)
            {
                // Check to see if this test target has the suite we're looking for
                if (const auto suiteName = suite[Keys[SuiteKey]].GetString();
                    strcmp(SuiteTypeAsString(suiteType).c_str(), suiteName) == 0)
                {
                    PythonTestTargetDescriptor.m_testSuiteMeta.m_name = suiteName;
                    PythonTestTargetDescriptor.m_testSuiteMeta.m_timeout = AZStd::chrono::seconds{ suite[Keys[TimeoutKey]].GetUint() };
                    PythonTestTargetDescriptor.m_name = test[Keys[NameKey]].GetString();
                    PythonTestTargetDescriptor.m_scriptPath = test[Keys[ScriptKey]].GetString();

                    AZ_TestImpact_Eval(!PythonTestTargetDescriptor.m_name.empty(), ArtifactException, "Test name field cannot be empty");
                    AZ_TestImpact_Eval(!PythonTestTargetDescriptor.m_scriptPath.empty(), ArtifactException, "Test script field cannot be empty");
                    PythonTestTargetDescriptors.push_back(AZStd::move(PythonTestTargetDescriptor));
                }
            }
        }

        return PythonTestTargetDescriptors;
    }
} // namespace TestImpact
