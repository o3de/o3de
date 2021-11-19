/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactTestUtils.h>

#include <TestImpactFramework/TestImpactException.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/regex.h>

namespace UnitTest
{
    AZStd::string ConstructTestProcessArgs(TestImpact::ProcessId pid, AZStd::chrono::milliseconds sleepTime)
    {
        return AZStd::string::format("--id %i --sleep %u", pid, sleepTime.count());
    }

    AZStd::string ConstructTestProcessArgsLargeText(TestImpact::ProcessId pid, AZStd::chrono::milliseconds sleepTime)
    {
        return ConstructTestProcessArgs(pid, sleepTime) + " large";
    }

    AZStd::string KnownTestProcessOutputString(TestImpact::ProcessId pid)
    {
        return AZStd::string("TestProcessMainStdOut" + AZStd::to_string(pid));
    }

    AZStd::string KnownTestProcessErrorString(TestImpact::ProcessId pid)
    {
        return AZStd::string("TestProcessMainStdErr" + AZStd::to_string(pid));
    }

    AZStd::string GenerateSuiteOrFixtureName(const AZStd::string& name)
    {
        return name;
    }

    AZStd::string GenerateTypedFixtureName(const AZStd::string& name, size_t typeNum)
    {
        return AZStd::string::format("%s/%u", name.c_str(), typeNum);
    }

    AZStd::string GenerateParameterizedFixtureName(const AZStd::string& name, const AZStd::optional<const AZStd::string>& prefix)
    {
        if (prefix.has_value())
        {
            return AZStd::string::format("%s/%s", prefix->c_str(), name.c_str());
        }

        return AZStd::string::format("%s", name.c_str());
    }

    AZStd::string GenerateParameterizedTestName(const AZStd::string& name, size_t testNum)
    {
        return AZStd::string::format("%s/%u", name.c_str(), testNum);
    }

    AZStd::string JSONSafeString(const AZStd::string output)
    {
        return AZStd::regex_replace(output, AZStd::regex("\\\\"), "/");
    }

    AZStd::string StringVectorToJSONElements(const AZStd::vector<TestImpact::RepoPath> strings)
    {
        AZStd::string output;

        for (size_t i = 0, last = strings.size() - 1; i < strings.size(); i++)
        {
            output += AZStd::string::format("\"%s\"", strings[i].c_str());

            if (i != last)
            {
                output += ",";
            }

            output += "\n";
        }

        return JSONSafeString(output);
    }

    AZStd::string GenerateBuildTargetDescriptorString(
        const AZStd::string& name,
        const AZStd::string& outputName,
        const TestImpact::RepoPath& path,
        const AZStd::vector<TestImpact::RepoPath>& staticSources,
        const AZStd::vector<TestImpact::RepoPath>& autogenInputs,
        const AZStd::vector<TestImpact::RepoPath>& autogenOutputs)
    {
        constexpr const char* const targetTemplate =
            "{\n"
            "    \"sources\": {\n"
            "        \"input\": [\n"
            "%s\n"
            "        ],\n"
            "        \"output\": [\n"
            "%s\n"
            "        ],\n"
            "        \"static\": [\n"
            "%s\n"
            "        ]\n"
            "    },\n"
            "    \"target\": {\n"
            "        \"name\": \"%s\",\n"
            "        \"output_name\": \"%s\",\n"
            "        \"path\": \"%s\"\n"
            "    }\n"
            "}\n"
            "\n";

        AZStd::string output = AZStd::string::format(targetTemplate,
            StringVectorToJSONElements(autogenInputs).c_str(),
            StringVectorToJSONElements(autogenOutputs).c_str(),
            StringVectorToJSONElements(staticSources).c_str(),
            name.c_str(),
            outputName.c_str(),
            JSONSafeString(path.String()).c_str());

        return output;
    }

    TestImpact::BuildTargetDescriptor GenerateBuildTargetDescriptor(
        const AZStd::string& name,
        const AZStd::string& outputName,
        const TestImpact::RepoPath& path,
        const AZStd::vector<TestImpact::RepoPath>& staticSources,
        const TestImpact::AutogenSources& autogenSources)
    {
        TestImpact::BuildTargetDescriptor targetDescriptor;
        targetDescriptor.m_buildMetaData = {name, outputName, path};
        targetDescriptor.m_sources = {staticSources, autogenSources};
        return targetDescriptor;
    }

    TestImpact::TestEnumerationSuite GenerateParamterizedSuite(
        const AZStd::pair<AZStd::string, bool>& fixture,
        const AZStd::optional<AZStd::string>& permutation,
        const AZStd::vector<AZStd::pair<AZStd::string, bool>> tests,
        size_t permutationCount)
    {
        TestImpact::TestEnumerationSuite suite =
            TestImpact::TestEnumerationSuite{GenerateParameterizedFixtureName(fixture.first, permutation), fixture.second, {}};

        for (const auto& test : tests)
        {
            for (auto i = 0; i < permutationCount; i++)
            {
                suite.m_tests.push_back(TestImpact::TestEnumerationCase{GenerateParameterizedTestName(test.first, i), test.second});
            }
        }

        return suite;
    }

    void GenerateTypedSuite(
        const AZStd::pair<AZStd::string, bool>& fixture,
        const AZStd::vector<AZStd::pair<AZStd::string, bool>> tests,
        size_t permutationCount,
        AZStd::vector<TestImpact::TestEnumerationSuite>& parentSuiteList)
    {
        for (auto i = 0; i < permutationCount; i++)
        {
            parentSuiteList.push_back(TestImpact::TestEnumerationSuite{GenerateTypedFixtureName(fixture.first, i), fixture.second, {}});

            for (const auto& test : tests)
            {
                parentSuiteList.back().m_tests.push_back(TestImpact::TestEnumerationCase{test.first, test.second});
            }
        }
    }

    size_t CalculateNumPassedTests(const AZStd::vector<TestImpact::TestRunSuite>& suites)
    {
        size_t numPassedTests = 0;
        for (const auto& suite : suites)
        {
            numPassedTests += AZStd::count_if(suite.m_tests.begin(), suite.m_tests.end(), [](const auto& test) {
                return test.m_result.has_value() && test.m_result.value() == TestImpact::TestRunResult::Passed;
            });
        }

        return numPassedTests;
    }

    size_t CalculateNumFailedTests(const AZStd::vector<TestImpact::TestRunSuite>& suites)
    {
        size_t numFailedTests = 0;
        for (const auto& suite : suites)
        {
            for (const auto& test : suite.m_tests)
            {
                if (test.m_result.has_value() && test.m_result.value() == TestImpact::TestRunResult::Failed)
                {
                    numFailedTests++;
                }
            }
        }

        return numFailedTests;
    }

    size_t CalculateNumRunTests(const AZStd::vector<TestImpact::TestRunSuite>& suites)
    {
        size_t numRunTests = 0;
        for (const auto& suite : suites)
        {
            for (const auto& test : suite.m_tests)
            {
                if (test.m_status == TestImpact::TestRunStatus::Run)
                {
                    numRunTests++;
                }
            }
        }

        return numRunTests;
    }

    size_t CalculateNumNotRunTests(const AZStd::vector<TestImpact::TestRunSuite>& suites)
    {
        size_t numNotRunTests = 0;
        for (const auto& suite : suites)
        {
            for (const auto& test : suite.m_tests)
            {
                if (test.m_status == TestImpact::TestRunStatus::NotRun)
                {
                    numNotRunTests++;
                }
            }
        }

        return numNotRunTests;
    }

    AZStd::vector<TestImpact::TestEnumerationSuite> GetTestTargetATestEnumerationSuites()
    {
        AZStd::vector<TestImpact::TestEnumerationSuite> suites;

        suites.push_back(TestImpact::TestEnumerationSuite{GenerateSuiteOrFixtureName("TestCase"), true, {}});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test1_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test2_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test3_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test4_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test5_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test6_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test7_WillFail", true});

        suites.push_back(TestImpact::TestEnumerationSuite{GenerateSuiteOrFixtureName("TestFixture"), true, {}});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test1_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test2_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test3_WillPass", true});

        return suites;
    }

    AZStd::vector<TestImpact::TestEnumerationSuite> GetTestTargetBTestEnumerationSuites()
    {
        AZStd::vector<TestImpact::TestEnumerationSuite> suites;

        suites.push_back(TestImpact::TestEnumerationSuite{GenerateSuiteOrFixtureName("TestCase"), true, {}});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test1_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test2_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test3_WillPass", true});

        suites.push_back(TestImpact::TestEnumerationSuite{GenerateSuiteOrFixtureName("TestFixture"), true, {}});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test1_WillPass", true});

        const size_t numParams = 27;
        suites.push_back(GenerateParamterizedSuite(
            {"TestFixtureWithParams", true}, "PermutationA", {{"Test1_WillPass", true}, {"Test2_WillPass", true}}, numParams));
        suites.push_back(GenerateParamterizedSuite(
            {"TestFixtureWithParams", true}, AZStd::nullopt, {{"Test1_WillPass", true}, {"Test2_WillPass", true}}, numParams));

        return suites;
    }

    AZStd::vector<TestImpact::TestEnumerationSuite> GetTestTargetCTestEnumerationSuites()
    {
        AZStd::vector<TestImpact::TestEnumerationSuite> suites;

        suites.push_back(TestImpact::TestEnumerationSuite{GenerateSuiteOrFixtureName("TestFixture"), true, {}});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test1_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test2_WillPass", true});

        const size_t numTypes = 4;
        for (auto i = 0; i < numTypes; i++)
        {
            suites.push_back(TestImpact::TestEnumerationSuite{GenerateTypedFixtureName("TestFixtureWithTypes", i), true, {}});
            for (auto j = 1; j < numTypes + 1; j++)
            {
                suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{AZStd::string::format("Test%u_WillPass", j), true});
            }
        }

        return suites;
    }

    AZStd::vector<TestImpact::TestEnumerationSuite> GetTestTargetDTestEnumerationSuites()
    {
        AZStd::vector<TestImpact::TestEnumerationSuite> suites;

        suites.push_back(TestImpact::TestEnumerationSuite{GenerateSuiteOrFixtureName("TestCase"), true, {}});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test1_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"DISABLED_Test2_WillPass", false});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test3_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test4_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test5_WillPass", true});

        suites.push_back(TestImpact::TestEnumerationSuite{GenerateSuiteOrFixtureName("TestFixture1"), true, {}});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test1_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test2_WillPass", true});

        suites.push_back(TestImpact::TestEnumerationSuite{GenerateSuiteOrFixtureName("DISABLED_TestFixture2"), false, {}});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test1_WillPass", true});
        suites.back().m_tests.push_back(TestImpact::TestEnumerationCase{"Test2_WillPass", true});

        const size_t numTypes = 4;
        GenerateTypedSuite(
            {"TestFixtureWithTypes1", true}, {{"Test1_WillPass", true}, {"DISABLED_Test2_WillPass", false}, {"Test3_WillPass", true}},
            numTypes, suites);

        GenerateTypedSuite(
            {"DISABLED_TestFixtureWithTypes2", false},
            {{"Test1_WillPass", true}, {"DISABLED_Test2_WillPass", false}, {"Test3_WillPass", true}}, numTypes, suites);

        const size_t numParams = 27;
        suites.push_back(GenerateParamterizedSuite(
            {"TestFixtureWithParams1", true}, "PermutationA", {{"Test1_WillPass", true}, {"DISABLED_Test2_WillPass", false}}, numParams));
        suites.push_back(GenerateParamterizedSuite(
            {"TestFixtureWithParams1", true}, AZStd::nullopt, {{"Test1_WillPass", true}, {"DISABLED_Test2_WillPass", false}}, numParams));
        suites.push_back(GenerateParamterizedSuite(
            {"DISABLED_TestFixtureWithParams2", false}, "PermutationA", {{"Test1_WillPass", true}, {"DISABLED_Test2_WillPass", false}},
            numParams));
        suites.push_back(GenerateParamterizedSuite(
            {"DISABLED_TestFixtureWithParams2", false}, AZStd::nullopt, {{"Test1_WillPass", true}, {"DISABLED_Test2_WillPass", false}},
            numParams));

        return suites;
    }

    TestImpact::TestRunCase TestRunCaseFromTestEnumerationCase(const TestImpact::TestEnumerationCase& enumCase)
    {
        TestImpact::TestRunCase runCase;
        runCase.m_name = enumCase.m_name;
        runCase.m_enabled = enumCase.m_enabled;
        return runCase;
    }

    TestImpact::TestRunSuite TestRunSuiteFromTestEnumerationSuite(const TestImpact::TestEnumerationSuite& enumSuite)
    {
        TestImpact::TestRunSuite runSuite;
        runSuite.m_name = enumSuite.m_name;
        runSuite.m_enabled = enumSuite.m_enabled;

        for (const auto& enumCase : enumSuite.m_tests)
        {
            runSuite.m_tests.push_back(TestRunCaseFromTestEnumerationCase(enumCase));
        }

        return runSuite;
    }

    AZStd::vector<TestImpact::TestRunSuite> TestRunSuitesFromTestEnumerationSuites(
        const AZStd::vector<TestImpact::TestEnumerationSuite>& enumSuites)
    {
        AZStd::vector<TestImpact::TestRunSuite> runSuites;
        runSuites.reserve(enumSuites.size());

        for (const auto& enumSuite : enumSuites)
        {
            runSuites.push_back(TestRunSuiteFromTestEnumerationSuite(enumSuite));
        }

        return runSuites;
    }

    void SetTestRunSuiteData(TestImpact::TestRunSuite& testSuite, AZStd::chrono::milliseconds duration)
    {
        testSuite.m_duration = duration;
    }

    void SetTestRunCaseData(
        TestImpact::TestRunCase& testCase, AZStd::chrono::milliseconds duration, TestImpact::TestRunStatus status,
        AZStd::optional<TestImpact::TestRunResult> result = AZStd::nullopt)
    {
        testCase.m_duration = duration;
        testCase.m_status = status;
        testCase.m_result = result;
    }

    AZStd::vector<TestImpact::TestRunSuite> GetTestTargetATestRunSuites()
    {
        AZStd::vector<TestImpact::TestRunSuite> suites(TestRunSuitesFromTestEnumerationSuites(GetTestTargetATestEnumerationSuites()));

        enum
        {
            TestCaseIndex = 0,
            TestFixtureIndex
        };

        {
            auto& suite = suites[TestCaseIndex];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{3});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[3], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[4], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[5], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[6], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Failed);
        }
        {
            auto& suite = suites[TestFixtureIndex];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{38});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{4}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }

        return suites;
    }

    AZStd::vector<TestImpact::TestRunSuite> GetTestTargetBTestRunSuites()
    {
        AZStd::vector<TestImpact::TestRunSuite> suites(TestRunSuitesFromTestEnumerationSuites(GetTestTargetBTestEnumerationSuites()));

        enum
        {
            TestCaseIndex = 0,
            TestFixtureIndex,
            PermutationATestFixtureWithParamsIndex,
            TestFixtureWithParamsIndex
        };

        {
            auto& suite = suites[TestCaseIndex];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{202});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{3}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[TestFixtureIndex];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{62});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{5}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[PermutationATestFixtureWithParamsIndex];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{3203});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[3], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[4], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[5], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[6], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[7], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[8], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[9], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[10], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[11], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[12], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[13], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[14], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[15], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[16], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[17], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[18], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[19], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[20], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[21], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[22], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[23], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[24], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[25], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[26], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[27], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[28], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[29], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[30], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[31], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[32], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[33], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[34], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[35], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[36], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[37], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[38], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[39], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[40], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[41], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[42], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[43], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[44], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[45], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[46], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[47], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[48], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[49], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[50], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[51], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[52], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[53], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[TestFixtureWithParamsIndex];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{3360});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[3], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[4], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[5], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[6], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[7], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[8], AZStd::chrono::milliseconds{2}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[9], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[10], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[11], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[12], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[13], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[14], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[15], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[16], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[17], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[18], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[19], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[20], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[21], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[22], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[23], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[24], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[25], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[26], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[27], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[28], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[29], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[30], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[31], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[32], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[33], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[34], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[35], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[36], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[37], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[38], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[39], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[40], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[41], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[42], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[43], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[44], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[45], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[46], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[47], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[48], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[49], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[50], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[51], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[52], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[53], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }

        return suites;
    }

    AZStd::vector<TestImpact::TestRunSuite> GetTestTargetCTestRunSuites()
    {
        AZStd::vector<TestImpact::TestRunSuite> suites(TestRunSuitesFromTestEnumerationSuites(GetTestTargetCTestEnumerationSuites()));

        enum
        {
            TestCaseIndex = 0,
            TestFixtureWithTypes0Index,
            TestFixtureWithTypes1Index,
            TestFixtureWithTypes2Index,
            TestFixtureWithTypes3Index
        };

        {
            auto& suite = suites[TestCaseIndex];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{125});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{4}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[TestFixtureWithTypes0Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{210});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[3], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[TestFixtureWithTypes1Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{208});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[3], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[TestFixtureWithTypes2Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{199});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[3], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[TestFixtureWithTypes3Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{49});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[3], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }

        return suites;
    }

    AZStd::vector<TestImpact::TestRunSuite> GetTestTargetDTestRunSuites()
    {
        AZStd::vector<TestImpact::TestRunSuite> suites(TestRunSuitesFromTestEnumerationSuites(GetTestTargetDTestEnumerationSuites()));

        enum
        {
            TestCaseIndex = 0,
            TestFixture1Index,
            DISABLEDTestFixture2Index,
            TestFixtureWithTypes10Index,
            TestFixtureWithTypes11Index,
            TestFixtureWithTypes12Index,
            TestFixtureWithTypes13Index,
            DISABLEDTestFixtureWithTypes20Index,
            DISABLEDTestFixtureWithTypes21Index,
            DISABLEDTestFixtureWithTypes22Index,
            DISABLEDTestFixtureWithTypes23Index,
            PermutationATestFixtureWithParams1Index,
            TestFixtureWithParams1Index,
            PermutationADISABLED_TestFixtureWithParams2Index,
            DISABLED_TestFixtureWithParams2Index,
        };

        {
            auto& suite = suites[TestCaseIndex];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{3});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[3], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[4], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[TestFixture1Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{4});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{2}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[DISABLEDTestFixture2Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{0});
            SetTestRunCaseData(suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
        }
        {
            auto& suite = suites[TestFixtureWithTypes10Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{1});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[TestFixtureWithTypes11Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{3});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[TestFixtureWithTypes12Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{0});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[TestFixtureWithTypes13Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{1});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
        }
        {
            auto& suite = suites[DISABLEDTestFixtureWithTypes20Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{0});
            SetTestRunCaseData(suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
        }
        {
            auto& suite = suites[DISABLEDTestFixtureWithTypes21Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{0});
            SetTestRunCaseData(suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
        }
        {
            auto& suite = suites[DISABLEDTestFixtureWithTypes22Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{0});
            SetTestRunCaseData(suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
        }
        {
            auto& suite = suites[DISABLEDTestFixtureWithTypes23Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{0});
            SetTestRunCaseData(suite.m_tests[0], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
        }
        {
            auto& suite = suites[PermutationATestFixtureWithParams1Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{173});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[3], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[4], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[5], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[6], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[7], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[8], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[9], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[10], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[11], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[12], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[13], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[14], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[15], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[16], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[17], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[18], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[19], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[20], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[21], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[22], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[23], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[24], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[25], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[26], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(suite.m_tests[27], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[28], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[29], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[30], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[31], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[32], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[33], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[34], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[35], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[36], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[37], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[38], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[39], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[40], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[41], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[42], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[43], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[44], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[45], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[46], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[47], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[48], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[49], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[50], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[51], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[52], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[53], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
        }
        {
            auto& suite = suites[TestFixtureWithParams1Index];
            SetTestRunSuiteData(suite, AZStd::chrono::milliseconds{102});
            SetTestRunCaseData(
                suite.m_tests[0], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[1], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[2], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[3], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[4], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[5], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[6], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[7], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[8], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[9], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[10], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[11], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[12], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[13], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[14], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[15], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[16], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[17], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[18], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[19], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[20], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[21], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[22], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[23], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[24], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[25], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(
                suite.m_tests[26], AZStd::chrono::milliseconds{1}, TestImpact::TestRunStatus::Run, TestImpact::TestRunResult::Passed);
            SetTestRunCaseData(suite.m_tests[27], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[28], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[29], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[30], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[31], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[32], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[33], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[34], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[35], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[36], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[37], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[38], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[39], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[40], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[41], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[42], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[43], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[44], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[45], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[46], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[47], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[48], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[49], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[50], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[51], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[52], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
            SetTestRunCaseData(suite.m_tests[53], AZStd::chrono::milliseconds{0}, TestImpact::TestRunStatus::NotRun);
        }
        // The other two fixtures are all disabled (not run)

        return suites;
    }

    AZStd::vector<TestImpact::ModuleCoverage> GetTestTargetASourceModuleCoverages()
    {
        AZStd::vector<TestImpact::ModuleCoverage> moduleCoverages;

        TestImpact::ModuleCoverage moduleCoverage;
        moduleCoverage.m_path = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_A_BIN).c_str();

        TestImpact::SourceCoverage sourceCoverage;
        sourceCoverage.m_path =
            TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
                / "Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp").c_str();

        moduleCoverage.m_sources.push_back(AZStd::move(sourceCoverage));
        moduleCoverages.push_back(AZStd::move(moduleCoverage));
        return moduleCoverages;
    }

    AZStd::vector<TestImpact::ModuleCoverage> GetTestTargetBSourceModuleCoverages()
    {
        AZStd::vector<TestImpact::ModuleCoverage> moduleCoverages;

        TestImpact::ModuleCoverage moduleCoverage;
        moduleCoverage.m_path = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_B_BIN).c_str();

        TestImpact::SourceCoverage sourceCoverage;
        sourceCoverage.m_path =
            TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
                / "Tests\\TestTargetB\\Code\\Tests\\TestImpactTestTargetB.cpp").c_str();

        moduleCoverage.m_sources.push_back(AZStd::move(sourceCoverage));
        moduleCoverages.push_back(AZStd::move(moduleCoverage));
        return moduleCoverages;
    }

    AZStd::vector<TestImpact::ModuleCoverage> GetTestTargetCSourceModuleCoverages()
    {
        AZStd::vector<TestImpact::ModuleCoverage> moduleCoverages;

        TestImpact::ModuleCoverage moduleCoverage;
        moduleCoverage.m_path = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_C_BIN).String();

        TestImpact::SourceCoverage sourceCoverage;
        sourceCoverage.m_path =
            TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
                / "Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp").String();

        moduleCoverage.m_sources.push_back(AZStd::move(sourceCoverage));
        moduleCoverages.push_back(AZStd::move(moduleCoverage));
        return moduleCoverages;
    }

    AZStd::vector<TestImpact::ModuleCoverage> GetTestTargetDSourceModuleCoverages()
    {
        AZStd::vector<TestImpact::ModuleCoverage> moduleCoverages;

        TestImpact::ModuleCoverage moduleCoverage;
        moduleCoverage.m_path = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_D_BIN).c_str();

        TestImpact::SourceCoverage sourceCoverage;
        sourceCoverage.m_path =
            TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
                / "Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp").c_str();

        moduleCoverage.m_sources.push_back(AZStd::move(sourceCoverage));
        moduleCoverages.push_back(AZStd::move(moduleCoverage));
        return moduleCoverages;
    }

    AZStd::vector<TestImpact::ModuleCoverage> GetTestTargetALineModuleCoverages()
    {
        AZStd::vector<TestImpact::ModuleCoverage> moduleCoverages;

        TestImpact::ModuleCoverage moduleCoverage;
        moduleCoverage.m_path = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_A_BIN).c_str();

        TestImpact::SourceCoverage sourceCoverage;
        sourceCoverage.m_path =
            TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
                / "Tests\\TestTargetA\\Code\\Tests\\TestImpactTestTargetA.cpp").c_str();
        sourceCoverage.m_coverage = AZStd::vector<TestImpact::LineCoverage>();

        auto& lines = sourceCoverage.m_coverage;
        lines.push_back(TestImpact::LineCoverage{22, 1});
        lines.push_back(TestImpact::LineCoverage{23, 1});
        lines.push_back(TestImpact::LineCoverage{24, 1});
        lines.push_back(TestImpact::LineCoverage{25, 1});
        lines.push_back(TestImpact::LineCoverage{27, 1});
        lines.push_back(TestImpact::LineCoverage{28, 1});
        lines.push_back(TestImpact::LineCoverage{29, 1});
        lines.push_back(TestImpact::LineCoverage{30, 1});
        lines.push_back(TestImpact::LineCoverage{32, 1});
        lines.push_back(TestImpact::LineCoverage{33, 1});
        lines.push_back(TestImpact::LineCoverage{34, 1});
        lines.push_back(TestImpact::LineCoverage{35, 1});
        lines.push_back(TestImpact::LineCoverage{37, 1});
        lines.push_back(TestImpact::LineCoverage{38, 1});
        lines.push_back(TestImpact::LineCoverage{39, 1});
        lines.push_back(TestImpact::LineCoverage{40, 1});
        lines.push_back(TestImpact::LineCoverage{42, 1});
        lines.push_back(TestImpact::LineCoverage{43, 1});
        lines.push_back(TestImpact::LineCoverage{44, 1});
        lines.push_back(TestImpact::LineCoverage{45, 1});
        lines.push_back(TestImpact::LineCoverage{47, 1});
        lines.push_back(TestImpact::LineCoverage{48, 1});
        lines.push_back(TestImpact::LineCoverage{49, 1});
        lines.push_back(TestImpact::LineCoverage{50, 1});
        lines.push_back(TestImpact::LineCoverage{52, 1});
        lines.push_back(TestImpact::LineCoverage{53, 1});
        lines.push_back(TestImpact::LineCoverage{54, 1});
        lines.push_back(TestImpact::LineCoverage{55, 1});
        lines.push_back(TestImpact::LineCoverage{57, 1});
        lines.push_back(TestImpact::LineCoverage{58, 1});
        lines.push_back(TestImpact::LineCoverage{59, 1});
        lines.push_back(TestImpact::LineCoverage{60, 1});
        lines.push_back(TestImpact::LineCoverage{62, 1});
        lines.push_back(TestImpact::LineCoverage{63, 1});
        lines.push_back(TestImpact::LineCoverage{64, 1});
        lines.push_back(TestImpact::LineCoverage{65, 1});
        lines.push_back(TestImpact::LineCoverage{67, 1});
        lines.push_back(TestImpact::LineCoverage{68, 1});
        lines.push_back(TestImpact::LineCoverage{69, 1});
        lines.push_back(TestImpact::LineCoverage{70, 1});
        lines.push_back(TestImpact::LineCoverage{73, 1});

        moduleCoverage.m_sources.push_back(AZStd::move(sourceCoverage));
        moduleCoverages.push_back(AZStd::move(moduleCoverage));
        return moduleCoverages;
    }

    AZStd::vector<TestImpact::ModuleCoverage> GetTestTargetBLineModuleCoverages()
    {
        AZStd::vector<TestImpact::ModuleCoverage> moduleCoverages;

        TestImpact::ModuleCoverage moduleCoverage;
        moduleCoverage.m_path = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_B_BIN).String();

        TestImpact::SourceCoverage sourceCoverage;
        sourceCoverage.m_path =
            TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
                / "Tests\\TestTargetB\\Code\\Tests\\TestImpactTestTargetB.cpp").String();
        sourceCoverage.m_coverage = AZStd::vector<TestImpact::LineCoverage>();

        auto& lines = sourceCoverage.m_coverage;
        lines.push_back(TestImpact::LineCoverage{29, 1});
        lines.push_back(TestImpact::LineCoverage{30, 1});
        lines.push_back(TestImpact::LineCoverage{31, 1});
        lines.push_back(TestImpact::LineCoverage{32, 1});
        lines.push_back(TestImpact::LineCoverage{34, 1});
        lines.push_back(TestImpact::LineCoverage{35, 1});
        lines.push_back(TestImpact::LineCoverage{36, 1});
        lines.push_back(TestImpact::LineCoverage{37, 1});
        lines.push_back(TestImpact::LineCoverage{39, 1});
        lines.push_back(TestImpact::LineCoverage{40, 1});
        lines.push_back(TestImpact::LineCoverage{41, 1});
        lines.push_back(TestImpact::LineCoverage{42, 1});
        lines.push_back(TestImpact::LineCoverage{44, 1});
        lines.push_back(TestImpact::LineCoverage{45, 1});
        lines.push_back(TestImpact::LineCoverage{46, 1});
        lines.push_back(TestImpact::LineCoverage{47, 1});
        lines.push_back(TestImpact::LineCoverage{49, 1});
        lines.push_back(TestImpact::LineCoverage{50, 1});
        lines.push_back(TestImpact::LineCoverage{51, 1});
        lines.push_back(TestImpact::LineCoverage{52, 1});
        lines.push_back(TestImpact::LineCoverage{54, 1});
        lines.push_back(TestImpact::LineCoverage{55, 1});
        lines.push_back(TestImpact::LineCoverage{56, 1});
        lines.push_back(TestImpact::LineCoverage{57, 1});
        lines.push_back(TestImpact::LineCoverage{59, 1});
        lines.push_back(TestImpact::LineCoverage{66, 1});
        lines.push_back(TestImpact::LineCoverage{68, 1});
        lines.push_back(TestImpact::LineCoverage{75, 1});
        lines.push_back(TestImpact::LineCoverage{78, 1});

        moduleCoverage.m_sources.push_back(AZStd::move(sourceCoverage));
        moduleCoverages.push_back(AZStd::move(moduleCoverage));
        return moduleCoverages;
    }

    AZStd::vector<TestImpact::ModuleCoverage> GetTestTargetCLineModuleCoverages()
    {
        AZStd::vector<TestImpact::ModuleCoverage> moduleCoverages;

        TestImpact::ModuleCoverage moduleCoverage;
        moduleCoverage.m_path = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_C_BIN).String();

        TestImpact::SourceCoverage sourceCoverage;
        sourceCoverage.m_path =
            TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
                / "Tests\\TestTargetC\\Code\\Tests\\TestImpactTestTargetC.cpp").String();
        sourceCoverage.m_coverage = AZStd::vector<TestImpact::LineCoverage>();

        auto& lines = sourceCoverage.m_coverage;
        lines.push_back(TestImpact::LineCoverage{32, 1});
        lines.push_back(TestImpact::LineCoverage{33, 1});
        lines.push_back(TestImpact::LineCoverage{34, 1});
        lines.push_back(TestImpact::LineCoverage{35, 1});
        lines.push_back(TestImpact::LineCoverage{37, 1});
        lines.push_back(TestImpact::LineCoverage{38, 1});
        lines.push_back(TestImpact::LineCoverage{39, 1});
        lines.push_back(TestImpact::LineCoverage{40, 1});
        lines.push_back(TestImpact::LineCoverage{42, 1});
        lines.push_back(TestImpact::LineCoverage{43, 1});
        lines.push_back(TestImpact::LineCoverage{44, 1});
        lines.push_back(TestImpact::LineCoverage{45, 1});
        lines.push_back(TestImpact::LineCoverage{47, 1});
        lines.push_back(TestImpact::LineCoverage{48, 1});
        lines.push_back(TestImpact::LineCoverage{49, 1});
        lines.push_back(TestImpact::LineCoverage{50, 1});
        lines.push_back(TestImpact::LineCoverage{52, 1});
        lines.push_back(TestImpact::LineCoverage{53, 1});
        lines.push_back(TestImpact::LineCoverage{54, 1});
        lines.push_back(TestImpact::LineCoverage{55, 1});
        lines.push_back(TestImpact::LineCoverage{57, 1});
        lines.push_back(TestImpact::LineCoverage{58, 1});
        lines.push_back(TestImpact::LineCoverage{59, 1});
        lines.push_back(TestImpact::LineCoverage{60, 1});
        lines.push_back(TestImpact::LineCoverage{63, 1});

        moduleCoverage.m_sources.push_back(AZStd::move(sourceCoverage));
        moduleCoverages.push_back(AZStd::move(moduleCoverage));
        return moduleCoverages;
    }

    AZStd::vector<TestImpact::ModuleCoverage> GetTestTargetDLineModuleCoverages()
    {
        AZStd::vector<TestImpact::ModuleCoverage> moduleCoverages;

        TestImpact::ModuleCoverage moduleCoverage;
        moduleCoverage.m_path = TestImpact::RepoPath(LY_TEST_IMPACT_TEST_TARGET_D_BIN).String();

        TestImpact::SourceCoverage sourceCoverage;
        sourceCoverage.m_path =
            TestImpact::RepoPath(TestImpact::RepoPath(LY_TEST_IMPACT_COVERAGE_SOURCES_DIR)
                / "Tests\\TestTargetD\\Code\\Tests\\TestImpactTestTargetD.cpp").String();
        sourceCoverage.m_coverage = AZStd::vector<TestImpact::LineCoverage>();

        auto& lines = sourceCoverage.m_coverage;
        lines.push_back(TestImpact::LineCoverage{56, 1});
        lines.push_back(TestImpact::LineCoverage{57, 1});
        lines.push_back(TestImpact::LineCoverage{58, 1});
        lines.push_back(TestImpact::LineCoverage{59, 1});
        lines.push_back(TestImpact::LineCoverage{61, 1});
        lines.push_back(TestImpact::LineCoverage{62, 0});
        lines.push_back(TestImpact::LineCoverage{63, 0});
        lines.push_back(TestImpact::LineCoverage{64, 0});
        lines.push_back(TestImpact::LineCoverage{66, 1});
        lines.push_back(TestImpact::LineCoverage{67, 1});
        lines.push_back(TestImpact::LineCoverage{68, 1});
        lines.push_back(TestImpact::LineCoverage{69, 1});
        lines.push_back(TestImpact::LineCoverage{71, 1});
        lines.push_back(TestImpact::LineCoverage{72, 1});
        lines.push_back(TestImpact::LineCoverage{73, 1});
        lines.push_back(TestImpact::LineCoverage{74, 1});
        lines.push_back(TestImpact::LineCoverage{76, 1});
        lines.push_back(TestImpact::LineCoverage{77, 1});
        lines.push_back(TestImpact::LineCoverage{78, 1});
        lines.push_back(TestImpact::LineCoverage{79, 1});
        lines.push_back(TestImpact::LineCoverage{81, 1});
        lines.push_back(TestImpact::LineCoverage{82, 1});
        lines.push_back(TestImpact::LineCoverage{83, 1});
        lines.push_back(TestImpact::LineCoverage{84, 1});
        lines.push_back(TestImpact::LineCoverage{86, 1});
        lines.push_back(TestImpact::LineCoverage{87, 1});
        lines.push_back(TestImpact::LineCoverage{88, 1});
        lines.push_back(TestImpact::LineCoverage{89, 1});
        lines.push_back(TestImpact::LineCoverage{91, 1});
        lines.push_back(TestImpact::LineCoverage{92, 0});
        lines.push_back(TestImpact::LineCoverage{93, 0});
        lines.push_back(TestImpact::LineCoverage{94, 0});
        lines.push_back(TestImpact::LineCoverage{96, 1});
        lines.push_back(TestImpact::LineCoverage{97, 0});
        lines.push_back(TestImpact::LineCoverage{98, 0});
        lines.push_back(TestImpact::LineCoverage{99, 0});
        lines.push_back(TestImpact::LineCoverage{101, 1});
        lines.push_back(TestImpact::LineCoverage{102, 1});
        lines.push_back(TestImpact::LineCoverage{103, 1});
        lines.push_back(TestImpact::LineCoverage{104, 1});
        lines.push_back(TestImpact::LineCoverage{106, 1});
        lines.push_back(TestImpact::LineCoverage{107, 0});
        lines.push_back(TestImpact::LineCoverage{108, 0});
        lines.push_back(TestImpact::LineCoverage{109, 0});
        lines.push_back(TestImpact::LineCoverage{111, 1});
        lines.push_back(TestImpact::LineCoverage{112, 0});
        lines.push_back(TestImpact::LineCoverage{113, 0});
        lines.push_back(TestImpact::LineCoverage{114, 0});
        lines.push_back(TestImpact::LineCoverage{116, 1});
        lines.push_back(TestImpact::LineCoverage{117, 0});
        lines.push_back(TestImpact::LineCoverage{118, 0});
        lines.push_back(TestImpact::LineCoverage{119, 0});
        lines.push_back(TestImpact::LineCoverage{121, 1});
        lines.push_back(TestImpact::LineCoverage{128, 1});
        lines.push_back(TestImpact::LineCoverage{130, 1});
        lines.push_back(TestImpact::LineCoverage{137, 1});
        lines.push_back(TestImpact::LineCoverage{139, 1});
        lines.push_back(TestImpact::LineCoverage{146, 1});
        lines.push_back(TestImpact::LineCoverage{148, 1});
        lines.push_back(TestImpact::LineCoverage{155, 1});
        lines.push_back(TestImpact::LineCoverage{157, 1});
        lines.push_back(TestImpact::LineCoverage{158, 1});
        lines.push_back(TestImpact::LineCoverage{159, 1});
        lines.push_back(TestImpact::LineCoverage{160, 1});
        lines.push_back(TestImpact::LineCoverage{162, 1});
        lines.push_back(TestImpact::LineCoverage{163, 0});
        lines.push_back(TestImpact::LineCoverage{164, 0});
        lines.push_back(TestImpact::LineCoverage{165, 0});
        lines.push_back(TestImpact::LineCoverage{167, 1});
        lines.push_back(TestImpact::LineCoverage{168, 1});
        lines.push_back(TestImpact::LineCoverage{169, 1});
        lines.push_back(TestImpact::LineCoverage{170, 1});
        lines.push_back(TestImpact::LineCoverage{172, 1});
        lines.push_back(TestImpact::LineCoverage{173, 0});
        lines.push_back(TestImpact::LineCoverage{174, 0});
        lines.push_back(TestImpact::LineCoverage{175, 0});
        lines.push_back(TestImpact::LineCoverage{177, 1});
        lines.push_back(TestImpact::LineCoverage{178, 0});
        lines.push_back(TestImpact::LineCoverage{179, 0});
        lines.push_back(TestImpact::LineCoverage{180, 0});
        lines.push_back(TestImpact::LineCoverage{182, 1});
        lines.push_back(TestImpact::LineCoverage{183, 0});
        lines.push_back(TestImpact::LineCoverage{184, 0});
        lines.push_back(TestImpact::LineCoverage{185, 0});
        lines.push_back(TestImpact::LineCoverage{188, 1});

        moduleCoverage.m_sources.push_back(AZStd::move(sourceCoverage));
        moduleCoverages.push_back(AZStd::move(moduleCoverage));
        return moduleCoverages;
    }

    template<typename TestCase>
    bool CheckTestCasesAreEqual(const TestCase& lhs, const TestCase& rhs)
    {
        if (lhs.m_enabled != rhs.m_enabled)
        {
            AZ_Error("CheckTestCasesAreEqual", false, "lhs.m_enabled: %u, rhs.m_enabled: %u", lhs.m_enabled, rhs.m_enabled);
            return false;
        }

        if (lhs.m_name != rhs.m_name)
        {
            AZ_Error("CheckTestCasesAreEqual", false, "lhs.m_name: %s, rhs.m_name: %s", lhs.m_name.c_str(), rhs.m_name.c_str());
            return false;
        }

        return true;
    }

    template<typename TestSuite>
    bool CheckTestSuitesAreEqual(const TestSuite& lhs, const TestSuite& rhs)
    {
        if (lhs.m_enabled != rhs.m_enabled)
        {
            AZ_Error("CheckTestSuitesAreEqual", false, "lhs.m_enabled: %u, rhs.m_enabled: %u", lhs.m_enabled, rhs.m_enabled);
            return false;
        }

        if (lhs.m_name != rhs.m_name)
        {
            AZ_Error("CheckTestSuitesAreEqual", false, "lhs.m_name: %s, rhs.m_name: %s", lhs.m_name.c_str(), rhs.m_name.c_str());
            return false;
        }

        return AZStd::equal(lhs.m_tests.begin(), lhs.m_tests.end(), rhs.m_tests.begin(), [](const auto& left, const auto& right) {
            return left == right;
        });
    }

    template<typename TestSuite>
    bool CheckTestSuiteVectorsAreEqual(const AZStd::vector<TestSuite>& lhs, const AZStd::vector<TestSuite>& rhs)
    {
        return AZStd::equal(lhs.begin(), lhs.end(), rhs.begin(), [](const TestSuite& left, const TestSuite& right) {
            return left == right;
        });
    }

    template<typename TestContainer>
    bool CheckTestContainersAreEqual(const TestContainer& lhs, const TestContainer& rhs)
    {
        if (lhs.GetTestSuites().size() != rhs.GetTestSuites().size())
        {
            return false;
        }

        return AZStd::equal(
            lhs.GetTestSuites().begin(), lhs.GetTestSuites().end(), rhs.GetTestSuites().begin(), [](const auto& left, const auto& right) {
                return left == right;
            });
    }

    bool operator==(const TestImpact::TestEnumerationCase& lhs, const TestImpact::TestEnumerationCase& rhs)
    {
        return CheckTestCasesAreEqual(lhs, rhs);
    }

    bool operator==(const TestImpact::TestRunCase& lhs, const TestImpact::TestRunCase& rhs)
    {
        if (!CheckTestCasesAreEqual(lhs, rhs))
        {
            return false;
        }

        if (lhs.m_status != rhs.m_status)
        {
            AZ_Error(
                "TestRunCase ==", false, "lhs.m_status: %u, rhs.m_status: %u", static_cast<size_t>(lhs.m_status),
                static_cast<size_t>(rhs.m_status));
            return false;
        }

        if (lhs.m_duration != rhs.m_duration)
        {
            AZ_Error("TestRunCase ==", false, "lhs.m_duration: %u, rhs.m_duration: %u", lhs.m_duration.count(), rhs.m_duration.count());
            return false;
        }

        if (lhs.m_result != rhs.m_result)
        {
            if (!lhs.m_result.has_value() && rhs.m_result.has_value())
            {
                AZ_Error("TestRunCase ==", false, "lhs.m_result: null, rhs.m_result: %u", static_cast<size_t>(rhs.m_result.value()));
            }
            else if (lhs.m_result.has_value() && !rhs.m_result.has_value())
            {
                AZ_Error("TestRunCase ==", false, "lhs.m_result: %u, rhs.m_result: null", static_cast<size_t>(lhs.m_result.value()));
            }
            else
            {
                AZ_Error(
                    "TestRunCase ==", false, "lhs.m_result: %u, rhs.m_result: %u", static_cast<size_t>(lhs.m_result.value()),
                    static_cast<size_t>(rhs.m_result.value()));
            }

            return false;
        }

        return true;
    }

    bool operator==(const TestImpact::TestEnumerationSuite& lhs, const TestImpact::TestEnumerationSuite& rhs)
    {
        return CheckTestSuitesAreEqual(lhs, rhs);
    }

    bool operator==(const TestImpact::TestRunSuite& lhs, const TestImpact::TestRunSuite& rhs)
    {
        if (!CheckTestSuitesAreEqual(lhs, rhs))
        {
            return false;
        }

        if (lhs.m_duration != rhs.m_duration)
        {
            AZ_Error(
                "TestEnumerationSuite ==", false, "lhs.m_duration: %u, rhs.m_duration: %u", lhs.m_duration.count(), rhs.m_duration.count());
            return false;
        }

        return true;
    }

    bool operator==(const AZStd::vector<TestImpact::TestEnumerationSuite>& lhs, const AZStd::vector<TestImpact::TestEnumerationSuite>& rhs)
    {
        return CheckTestSuiteVectorsAreEqual(lhs, rhs);
    }

    bool operator==(const AZStd::vector<TestImpact::TestRunSuite>& lhs, const AZStd::vector<TestImpact::TestRunSuite>& rhs)
    {
        return CheckTestSuiteVectorsAreEqual(lhs, rhs);
    }

    bool operator==(const TestImpact::TestEnumeration& lhs, const TestImpact::TestEnumeration& rhs)
    {
        return CheckTestContainersAreEqual(lhs, rhs);
    }

    bool operator==(const TestImpact::TestRun& lhs, const TestImpact::TestRun& rhs)
    {
        if (lhs.GetDuration() != rhs.GetDuration())
        {
            AZ_Error("TestRun ==", false, "lhs.GetDuration(): %u, rhs.GetDuration(): %u", lhs.GetDuration(), rhs.GetDuration());
            return false;
        }

        if (lhs.GetNumDisabledTests() != rhs.GetNumDisabledTests())
        {
            AZ_Error(
                "TestRun ==", false, "lhs.GetNumDisabledTests(): %u, rhs.GetNumDisabledTests(): %u", lhs.GetNumDisabledTests(),
                rhs.GetNumDisabledTests());
            return false;
        }

        if (lhs.GetNumEnabledTests() != rhs.GetNumEnabledTests())
        {
            AZ_Error(
                "TestRun ==", false, "lhs.GetNumEnabledTests(): %u, rhs.GetNumEnabledTests(): %u", lhs.GetNumEnabledTests(),
                rhs.GetNumEnabledTests());
            return false;
        }

        if (lhs.GetNumFailures() != rhs.GetNumFailures())
        {
            AZ_Error("TestRun ==", false, "lhs.GetNumFailures(): %u, rhs.GetNumFailures(): %u", lhs.GetNumFailures(), rhs.GetNumFailures());
            return false;
        }

        if (lhs.GetNumNotRuns() != rhs.GetNumNotRuns())
        {
            AZ_Error("TestRun ==", false, "lhs.GetNumNotRuns(): %u, rhs.GetNumNotRuns(): %u", lhs.GetNumNotRuns(), rhs.GetNumNotRuns());
            return false;
        }

        if (lhs.GetNumPasses() != rhs.GetNumPasses())
        {
            AZ_Error("TestRun ==", false, "lhs.GetNumPasses(): %u, rhs.GetNumPasses(): %u", lhs.GetNumPasses(), rhs.GetNumPasses());
            return false;
        }

        if (lhs.GetNumRuns() != rhs.GetNumRuns())
        {
            AZ_Error("TestRun ==", false, "lhs.GetNumRuns(): %u, rhs.GetNumRuns(): %u", lhs.GetNumRuns(), rhs.GetNumRuns());
            return false;
        }

        return CheckTestContainersAreEqual(lhs, rhs);
    }

    bool CheckTestRunCaseVectorsAreEqual(
        const AZStd::vector<TestImpact::TestRunCase>& lhs, const AZStd::vector<TestImpact::TestRunCase>& rhs)
    {
        return AZStd::equal(
            lhs.begin(), lhs.end(), rhs.begin(), [](const TestImpact::TestRunCase& leftCase, const TestImpact::TestRunCase& rightCase) {
                if (!CheckTestCasesAreEqual(leftCase, rightCase))
                {
                    return false;
                }

                if (leftCase.m_status != rightCase.m_status)
                {
                    AZ_Error(
                        "CheckTestRunsAreEqualIgnoreDurations", false, "leftCase.m_status: %u, rightCase.m_status: %u",
                        static_cast<size_t>(leftCase.m_status), static_cast<size_t>(rightCase.m_status));
                    return false;
                }

                if (leftCase.m_result != rightCase.m_result)
                {
                    if (!leftCase.m_result.has_value() && rightCase.m_result.has_value())
                    {
                        AZ_Error(
                            "CheckTestRunsAreEqualIgnoreDurations", false, "leftCase.m_result: null, rightCase.m_result: %u",
                            static_cast<size_t>(rightCase.m_result.value()));
                    }
                    else if (leftCase.m_result.has_value() && !rightCase.m_result.has_value())
                    {
                        AZ_Error(
                            "CheckTestRunsAreEqualIgnoreDurations", false, "leftCase.m_result: %u, rightCase.m_result: null",
                            static_cast<size_t>(leftCase.m_result.value()));
                    }
                    else
                    {
                        AZ_Error(
                            "CheckTestRunsAreEqualIgnoreDurations", false, "leftCase.m_result: %u, rightCase.m_result: %u",
                            static_cast<size_t>(leftCase.m_result.value()), static_cast<size_t>(rightCase.m_result.value()));
                    }

                    return false;
                }

                return true;
            });
    }

    bool CheckTestRunsAreEqualIgnoreDurations(const TestImpact::TestRun& lhs, const TestImpact::TestRun& rhs)
    {
        return AZStd::equal(
            lhs.GetTestSuites().begin(), lhs.GetTestSuites().end(), rhs.GetTestSuites().begin(),
            [](const TestImpact::TestRunSuite& leftSuite, const TestImpact::TestRunSuite& rightSuite) {
                if (leftSuite.m_enabled != rightSuite.m_enabled)
                {
                    AZ_Error(
                        "CheckTestRunsAreEqualIgnoreDurations", false, "leftSuite.m_enabled: %u, rightSuite.m_enabled: %u",
                        leftSuite.m_enabled, rightSuite.m_enabled);
                    return false;
                }

                if (leftSuite.m_name != rightSuite.m_name)
                {
                    AZ_Error(
                        "CheckTestRunsAreEqualIgnoreDurations", false, "leftSuite.m_name: %s, rightSuite.m_name: %s",
                        leftSuite.m_name.c_str(), rightSuite.m_name.c_str());
                    return false;
                }

                return CheckTestRunCaseVectorsAreEqual(leftSuite.m_tests, rightSuite.m_tests);
            });
    }

    bool operator==(const TestImpact::BuildMetaData& lhs, const TestImpact::BuildMetaData& rhs)
    {
        if (lhs.m_name != rhs.m_name)
        {
            return false;
        }
        else if (lhs.m_outputName != rhs.m_outputName)
        {
            return false;
        }
        else if (lhs.m_path != rhs.m_path)
        {
            return false;
        }

        return true;
    }

    bool operator==(const TestImpact::TargetSources& lhs, const TestImpact::TargetSources& rhs)
    {
        if (lhs.m_staticSources != rhs.m_staticSources)
        {
            return false;
        }
        else if (lhs.m_autogenSources.size() != rhs.m_autogenSources.size())
        {
            return false;
        }
        else
        {
            for (size_t i = 0; i < lhs.m_autogenSources.size(); i++)
            {
                if (lhs.m_autogenSources[i].m_input != rhs.m_autogenSources[i].m_input)
                {
                    return false;
                }
                else if (lhs.m_autogenSources[i].m_outputs.size() != rhs.m_autogenSources[i].m_outputs.size())
                {
                    return false;
                }

                for (size_t j = 0; j < lhs.m_autogenSources[i].m_outputs.size(); j++)
                {
                    if (lhs.m_autogenSources[i].m_outputs[j] != rhs.m_autogenSources[i].m_outputs[j])
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool operator==(const TestImpact::BuildTargetDescriptor& lhs, const TestImpact::BuildTargetDescriptor& rhs)
    {
        return lhs.m_buildMetaData == rhs.m_buildMetaData && lhs.m_sources == rhs.m_sources;
    }

    bool operator==(const TestImpact::TestSuiteMeta& lhs, const TestImpact::TestSuiteMeta& rhs)
    {
        if (lhs.m_name != rhs.m_name)
        {
            return false;
        }
        else if (lhs.m_timeout != rhs.m_timeout)
        {
            return false;
        }

        return true;
    }

    bool operator==(const TestImpact::TestTargetMeta& lhs, const TestImpact::TestTargetMeta& rhs)
    {
        if (!(lhs.m_suiteMeta == rhs.m_suiteMeta))
        {
            return false;
        }
        else if (lhs.m_launchMethod != rhs.m_launchMethod)
        {
            return false;
        }

        return true;
    }

    bool operator==(const TestImpact::ProductionTargetDescriptor& lhs, const TestImpact::ProductionTargetDescriptor& rhs)
    {
        return lhs.m_buildMetaData == rhs.m_buildMetaData;
    }

    bool operator==(const TestImpact::TestTargetDescriptor& lhs, const TestImpact::TestTargetDescriptor& rhs)
    {
        return lhs.m_buildMetaData == rhs.m_buildMetaData && lhs.m_sources == rhs.m_sources && lhs.m_testMetaData == rhs.m_testMetaData;
    }

    bool operator==(const TestImpact::LineCoverage& lhs, const TestImpact::LineCoverage& rhs)
    {
        if (lhs.m_hitCount != rhs.m_hitCount)
        {
            AZ_Error("LineCoverage ==", false, "lhs.m_hitCount: %u, rhs.m_hitCount: %u", lhs.m_hitCount, rhs.m_hitCount);
            return false;
        }

        if (lhs.m_lineNumber != rhs.m_lineNumber)
        {
            AZ_Error("LineCoverage ==", false, "lhs.m_lineNumber: %u, rhs.m_lineNumber: %u", lhs.m_lineNumber, rhs.m_lineNumber);
            return false;
        }

        return true;
    }

    bool operator==(const TestImpact::SourceCoverage& lhs, const TestImpact::SourceCoverage& rhs)
    {
        if (lhs.m_path != rhs.m_path)
        {
            AZ_Error("LineCoverage ==", false, "lhs.m_path: %s, rhs.m_path: %s", lhs.m_path.c_str(), rhs.m_path.c_str());
            return false;
        }

        if (lhs.m_coverage.empty() != rhs.m_coverage.empty())
        {
            AZ_Error(
                "LineCoverage ==", false, "lhs.m_coverage.empty(): %u, rhs.m_coverage.empty(): %u", lhs.m_coverage.empty(),
                rhs.m_coverage.empty());
            return false;
        }

        if (!lhs.m_coverage.empty())
        {
            return AZStd::equal(
                lhs.m_coverage.begin(), lhs.m_coverage.end(), rhs.m_coverage.begin(),
                [](const TestImpact::LineCoverage& left, const TestImpact::LineCoverage& right) {
                    return left == right;
                });
        }

        return true;
    }

    bool operator==(const TestImpact::ModuleCoverage& lhs, const TestImpact::ModuleCoverage& rhs)
    {
        if (lhs.m_path != rhs.m_path)
        {
            AZ_Error("ModuleCoverage ==", false, "lhs.m_path: %s, rhs.m_path: %s", lhs.m_path.c_str(), rhs.m_path.c_str());
            return false;
        }

        return AZStd::equal(
            lhs.m_sources.begin(), lhs.m_sources.end(), rhs.m_sources.begin(),
            [](const TestImpact::SourceCoverage& left, const TestImpact::SourceCoverage& right) {
                return left == right;
            });
    }

    bool operator==(const AZStd::vector<TestImpact::ModuleCoverage>& lhs, const AZStd::vector<TestImpact::ModuleCoverage>& rhs)
    {
        if (lhs.size() != rhs.size())
        {
            AZ_Error("ModuleCoverage ==", false, "lhs.size(): %u, rhs.size(): %u", lhs.size(), rhs.size());
            return false;
        }

        return AZStd::equal(
            lhs.begin(), lhs.end(), rhs.begin(), [](const TestImpact::ModuleCoverage& left, const TestImpact::ModuleCoverage& right) {
                return left == right;
            });
    }

    bool operator!=(const AZStd::vector<TestImpact::ModuleCoverage>& lhs, const AZStd::vector<TestImpact::ModuleCoverage>& rhs)
    {
        return !(lhs == rhs);
    }

    bool operator==(const TestImpact::TestCoverage& lhs, const TestImpact::TestCoverage& rhs)
    {
        if (lhs.GetNumModulesCovered() != rhs.GetNumModulesCovered())
        {
            return false;
        }

        if (lhs.GetNumSourcesCovered() != rhs.GetNumSourcesCovered())
        {
            return false;
        }

        if (lhs.GetModuleCoverages() != rhs.GetModuleCoverages())
        {
            return false;
        }

        if (lhs.GetSourcesCovered().size() != rhs.GetSourcesCovered().size())
        {
            return false;
        }

        return true;
    }

    bool operator==(const TestImpact::SourceCoveringTests& lhs, const TestImpact::SourceCoveringTests& rhs)
    {
        if (lhs.GetPath() != rhs.GetPath())
        {
            return false;
        }

        if (lhs.GetNumCoveringTestTargets() != rhs.GetNumCoveringTestTargets())
        {
            return false;
        }

        for (const auto& coveringTestTarget : lhs.GetCoveringTestTargets())
        {
            if (AZStd::find(rhs.GetCoveringTestTargets().begin(), rhs.GetCoveringTestTargets().end(), coveringTestTarget)
                == rhs.GetCoveringTestTargets().end())
            {
                return false;
            }
        }

        return true;
    }

    bool operator==(const AZStd::vector<TestImpact::SourceCoveringTests>& lhs, const AZStd::vector<TestImpact::SourceCoveringTests>& rhs)
    {
        if (lhs.size() != rhs.size())
        {
            AZ_Error("SourceCoveringTestsList ==", false, "lhs.size(): %u, rhs.size(): %u", lhs.size(), rhs.size());
            return false;
        }

        return AZStd::equal(
            lhs.begin(), lhs.end(), rhs.begin(), [](const TestImpact::SourceCoveringTests& left, const TestImpact::SourceCoveringTests& right) {
            return left == right;
        });
    }
} // namespace UnitTest
