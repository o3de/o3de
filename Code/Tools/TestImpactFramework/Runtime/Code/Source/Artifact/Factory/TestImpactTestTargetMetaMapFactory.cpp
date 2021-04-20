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

#include <Artifact/Factory/TestImpactTestTargetMetaMapFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/JSON/document.h>

#include <cstring>

namespace TestImpact
{
    namespace
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "google",
            "test",
            "tests",
            "suite",
            "launch_method",
            "test_runner",
            "stand_alone",
            "name"
        };

        enum
        {
            GoogleKey,
            TestKey,
            TestsKey,
            SuiteKey,
            LaunchMethodKey,
            TestRunnerKey,
            StandAloneKey,
            NameKey
        };
    } // namespace

    TestTargetMetaMap TestTargetMetaMapFactory(const AZStd::string& masterTestListData)
    {
        TestTargetMetaMap testMetas;
        rapidjson::Document masterTestList;

        if (masterTestList.Parse(masterTestListData.c_str()).HasParseError())
        {
            throw TestImpact::ArtifactException("Could not parse test meta-data file");
        }

        const auto tests = masterTestList[Keys[GoogleKey]][Keys[TestKey]][Keys[TestsKey]].GetArray();
        for (const auto& test : tests)
        {
            TestTargetMeta testMeta;
            testMeta.m_suite = test[Keys[SuiteKey]].GetString();

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
        }

        // If there's no tests in the repo then something is seriously wrong
        AZ_TestImpact_Eval(!testMetas.empty(), ArtifactException, "No tests were found in the repository");

        return testMetas;
    }
} // namespace TestImpact
