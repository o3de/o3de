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

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Factory/TestImpactTestTargetMetaMapFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/JSON/document.h>

#include <cstring>

namespace TestImpact
{
    TestTargetMetaMap TestTargetMetaMapFactory(const AZStd::string& masterTestListData, SuiteType suiteType)
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "google",
            "test",
            "tests",
            "suites",
            "suite",
            "launch_method",
            "test_runner",
            "stand_alone",
            "name",
            "command",
            "timeout"
        };

        enum
        {
            GoogleKey,
            TestKey,
            TestsKey,
            TestSuitesKey,
            SuiteKey,
            LaunchMethodKey,
            TestRunnerKey,
            StandAloneKey,
            NameKey,
            CommandKey,
            TimeoutKey
        };

        AZ_TestImpact_Eval(!masterTestListData.empty(), ArtifactException, "test meta-data cannot be empty");

        TestTargetMetaMap testMetas;
        rapidjson::Document masterTestList;

        if (masterTestList.Parse(masterTestListData.c_str()).HasParseError())
        {
            throw TestImpact::ArtifactException("Could not parse test meta-data");
        }

        const auto tests = masterTestList[Keys[GoogleKey]][Keys[TestKey]][Keys[TestsKey]].GetArray();
        for (const auto& test : tests)
        {
            TestTargetMeta testMeta;
            const auto testSuites = test[Keys[TestSuitesKey]].GetArray();
            for (const auto& suite : testSuites)
            {
                // Check to see if this test target has the suite we're looking for
                if (const auto suiteName = suite[Keys[SuiteKey]].GetString();
                    strcmp(SuiteTypeAsString(suiteType).c_str(), suiteName) == 0)
                {
                    testMeta.m_suite = suiteName;
                    testMeta.m_customArgs = suite[Keys[CommandKey]].GetString();
                    testMeta.m_timeout = AZStd::chrono::seconds{ suite[Keys[TimeoutKey]].GetUint() };
                    if (const auto buildTypeString = test[Keys[LaunchMethodKey]].GetString(); strcmp(buildTypeString, Keys[TestRunnerKey]) == 0)
                    {
                        testMeta.m_launchMethod = LaunchMethod::TestRunner;
                    }
                    else if (strcmp(buildTypeString, Keys[StandAloneKey]) == 0)
                    {
                        testMeta.m_launchMethod = LaunchMethod::StandAlone;
                    }
                    else
                    {
                        throw(ArtifactException("Unexpected test build type"));
                    }

                    AZStd::string name = test[Keys[NameKey]].GetString();
                    AZ_TestImpact_Eval(!name.empty(), ArtifactException, "Test name field cannot be empty");
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
