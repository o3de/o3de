/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Factory/TestImpactTestRunSuiteFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/containers/unordered_map.h>

namespace TestImpact
{
    namespace GTest
    {
        AZStd::vector<TestRunSuite> TestRunSuitesFactory(const AZStd::string& testRunData)
        {
            // Keys for pertinent XML node and attribute names
            constexpr const char* Keys[] =
            {
                "testsuites",
                "testsuite",
                "name",
                "testcase",
                "status",
                "run",
                "notrun",
                "time"
            };

            enum Fields
            {
                TestSuitesKey,
                TestSuiteKey,
                NameKey,
                TestCaseKey,
                StatusKey,
                RunKey,
                NotRunKey,
                DurationKey,
                // Checksum
                _CHECKSUM_
            };

            static_assert(Fields::_CHECKSUM_ == AZStd::size(Keys));
            AZ_TestImpact_Eval(!testRunData.empty(), ArtifactException, "Cannot parse test run, string is empty");
            AZStd::vector<TestRunSuite> testSuites;
            AZStd::vector<char> rawData(testRunData.begin(), testRunData.end());

            try
            {
                AZ::rapidxml::xml_document<> doc;
                // Parse the XML doc with default flags
                doc.parse<0>(rawData.data());

                const auto testsuites_node = doc.first_node(Keys[TestSuitesKey]);
                AZ_TestImpact_Eval(testsuites_node, ArtifactException, "Could not parse enumeration, XML is invalid");
                for (auto testsuite_node = testsuites_node->first_node(Keys[TestSuiteKey]); testsuite_node;
                     testsuite_node = testsuite_node->next_sibling())
                {
                    const auto isEnabled = [](const AZStd::string& name)
                    {
                        return !name.starts_with("DISABLED_") && name.find("/DISABLED_") == AZStd::string::npos;
                    };

                    const auto getDuration = [&](const AZ::rapidxml::xml_node<>* node)
                    {
                        const AZStd::string duration = node->first_attribute(Keys[DurationKey])->value();
                        return AZStd::chrono::milliseconds(static_cast<AZStd::sys_time_t>(AZStd::stof(duration) * 1000.f));
                    };

                    TestRunSuite testSuite;
                    testSuite.m_name = testsuite_node->first_attribute(Keys[NameKey])->value();
                    testSuite.m_enabled = isEnabled(testSuite.m_name);
                    testSuite.m_duration = getDuration(testsuite_node);

                    for (auto testcase_node = testsuite_node->first_node(Keys[TestCaseKey]); testcase_node;
                         testcase_node = testcase_node->next_sibling())
                    {
                        const auto getStatus = [&](const AZ::rapidxml::xml_node<>* node)
                        {
                            const AZStd::string status = node->first_attribute(Keys[StatusKey])->value();
                            if (status == Keys[RunKey])
                            {
                                return TestRunStatus::Run;
                            }
                            else if (status == Keys[NotRunKey])
                            {
                                return TestRunStatus::NotRun;
                            }

                            throw ArtifactException(AZStd::string::format("Unexpected run status: %s", status.c_str()));
                        };

                        const auto getResult = [](const AZ::rapidxml::xml_node<>* node)
                        {
                            if (node->first_node("failure"))
                            {
                                return TestRunResult::Failed;
                            }

                            return TestRunResult::Passed;
                        };

                        TestRunCase testCase;
                        testCase.m_name = testcase_node->first_attribute(Keys[NameKey])->value();
                        testCase.m_enabled = isEnabled(testCase.m_name);
                        testCase.m_duration = getDuration(testcase_node);
                        testCase.m_status = getStatus(testcase_node);

                        if (testCase.m_status == TestRunStatus::Run)
                        {
                            testCase.m_result = getResult(testcase_node);
                        }

                        testSuite.m_tests.emplace_back(AZStd::move(testCase));
                    }

                    testSuites.emplace_back(AZStd::move(testSuite));
                }
            }
            catch (const std::exception& e)
            {
                AZ_Error("TestRunSuitesFactory", false, e.what());
                throw ArtifactException(e.what());
            }
            catch (...)
            {
                throw ArtifactException("An unknown error occurred parsing the XML data");
            }

            return testSuites;
        }
    } // namespace GTest

    namespace JUnit
    {
        AZStd::vector<TestRunSuite> TestRunSuitesFactory(const AZStd::string& testRunData)
        {
            // Keys for pertinent XML node and attribute names
            constexpr const char* Keys[] =
            {
                "testsuites",
                "testsuite",
                "errors",
                "failures",
                "skipped",
                "tests",
                "time",
                "testcase",
                "classname",
                "name",
                "error",
                "skipped",
                "failure"
            };

            enum Fields
            {
                TestSuitesKey,
                TestSuiteKey,
                NumErrorsKey,
                NumFailuresKey,
                NumSkippedKey,
                TestsKey,
                TimeKey,
                TestCaseKey,
                TestSuiteNameKey,
                TestCaseNameKey,
                TestCaseErrorKey,
                TestCaseSkippedKey,
                TestCaseFailedKey,
                // Checksum
                _CHECKSUM_
            };

            static_assert(Fields::_CHECKSUM_ == AZStd::size(Keys));
            AZ_TestImpact_Eval(!testRunData.empty(), ArtifactException, "Cannot parse test run, string is empty");
            AZStd::vector<TestRunSuite> testSuites;
            AZStd::vector<char> rawData(testRunData.begin(), testRunData.end());

            try
            {
                AZ::rapidxml::xml_document<> doc;
                // Parse the XML doc with default flags
                doc.parse<0>(rawData.data());

                const auto testsuites_node = doc.first_node(Keys[TestSuitesKey]);
                AZ_TestImpact_Eval(testsuites_node, ArtifactException, "Could not parse enumeration, XML is invalid");
                
                // PyTest Quirks
                // 1. PyTest's JUnit files have an unusual layout insofar that the number of test suites is always one, yet the test
                //    suites are instead encoded in the test cases themselves, of which there can be multiple suites
                // 2. Aborting on first failure *will not* populate the not-run test cases meaning that the XML *may not* contain all
                //    of the tests should it abort on failure
                auto masterTestsuiteNode = testsuites_node->first_node(Keys[TestSuiteKey]);
                
                const auto getDuration = [&](const AZ::rapidxml::xml_node<>* node)
                {
                    if (const auto attribute = node->first_attribute(Keys[TimeKey]))
                    {
                        const AZStd::string duration = attribute->value();
                        return AZStd::chrono::milliseconds(static_cast<AZStd::sys_time_t>(AZStd::stof(duration) * 1000.f));
                    }

                    return AZStd::chrono::milliseconds{ 0 };
                };
                
                const auto getNumerical = [](const AZ::rapidxml::xml_node<>* node, const char* key)
                {
                    return AZStd::stoul(AZStd::string(node->first_attribute(key)->value()));
                };
                
                // Metrics from the suite header we will use as extra validation for parsed test cases
                const size_t expectedTestErrorCount = getNumerical(masterTestsuiteNode, Keys[NumErrorsKey]);
                const size_t expectedTestFailureCount = getNumerical(masterTestsuiteNode, Keys[NumFailuresKey]);
                const size_t expectedTestSkippedCount = getNumerical(masterTestsuiteNode, Keys[NumSkippedKey]);
                const size_t expectedTestCount = getNumerical(masterTestsuiteNode, Keys[TestsKey]);
                const size_t expectedTestPassCount =
                    expectedTestCount - (expectedTestErrorCount + expectedTestFailureCount + expectedTestSkippedCount);

                size_t testErrorCount = 0;
                size_t testFailureCount = 0;
                size_t testSkippedCount = 0;
                size_t testPassCount = 0;
                size_t testCount = 0;
                
                AZStd::optional<TestRunSuite> testSuite;
                for (auto testCaseNode = masterTestsuiteNode->first_node(Keys[TestCaseKey]); testCaseNode;
                     testCaseNode = testCaseNode->next_sibling(), testCount++)
                {
                    const auto getResult = [&]
                    (const AZ::rapidxml::xml_node<>* node) -> AZStd::optional<TestRunResult>
                    {
                        if (node->first_node(Keys[TestCaseFailedKey]))
                        {
                            testFailureCount++;
                            return TestRunResult::Failed;
                        }
                        else if (node->first_node(Keys[TestCaseErrorKey]))
                        {
                            testErrorCount++;
                            return AZStd::nullopt;
                        }
                        else if (node->first_node(Keys[TestCaseSkippedKey]))
                        {
                            testSkippedCount++;
                            return AZStd::nullopt;
                        }

                        testPassCount++;
                        return TestRunResult::Passed;
                    };

                    TestRunCase testCase;
                    testCase.m_name = testCaseNode->first_attribute(Keys[TestCaseNameKey])->value();
                    testCase.m_enabled = testCaseNode->first_node(Keys[TestCaseSkippedKey]) == nullptr;
                    testCase.m_duration = getDuration(testCaseNode);
                    testCase.m_status = testCase.m_enabled ? TestRunStatus::Run : TestRunStatus::NotRun;
                    testCase.m_result = getResult(testCaseNode);

                    //
                    const AZStd::string testSuiteName = testCaseNode->first_attribute(Keys[TestSuiteNameKey])->value();
                    if (!testSuite.has_value())
                    {
                        // First test suite
                        testSuite = TestRunSuite{};
                        testSuite->m_name = testSuiteName;
                        testSuite->m_enabled = true;
                        testSuite->m_duration += testCase.m_duration;
                        testSuite->m_tests.emplace_back(AZStd::move(testCase));
                    }
                    else if (testSuite->m_name != testSuiteName)
                    {
                        testSuites.emplace_back(AZStd::move(testSuite.value()));

                        // Next test suite
                        testSuite = TestRunSuite{};
                        testSuite->m_name = testSuiteName;
                        testSuite->m_enabled = true;
                        testSuite->m_duration += testCase.m_duration;
                        testSuite->m_tests.emplace_back(AZStd::move(testCase));
                    }
                    else
                    {
                        // Continuation of existing test suite
                        testSuite->m_duration += testCase.m_duration;
                        testSuite->m_tests.emplace_back(AZStd::move(testCase));
                    }
                }

                if (testSuite.has_value())
                {
                    testSuites.emplace_back(AZStd::move(testSuite.value()));
                }

                AZ_TestImpact_Eval(
                    expectedTestCount == testCount, ArtifactException,
                    AZStd::string::format("Expected %llu test cases, got %llu", expectedTestCount, testCount));

                AZ_TestImpact_Eval(
                    expectedTestErrorCount == testErrorCount, ArtifactException,
                    AZStd::string::format("Expected %llu test case errors, got %llu", expectedTestErrorCount, testErrorCount));

                AZ_TestImpact_Eval(
                    expectedTestFailureCount == testFailureCount, ArtifactException,
                    AZStd::string::format("Expected %llu test case failures, got %llu", expectedTestFailureCount, testFailureCount));

                AZ_TestImpact_Eval(
                    expectedTestPassCount == testPassCount, ArtifactException,
                    AZStd::string::format("Expected %llu test case passes, got %llu", expectedTestPassCount, testPassCount));

                AZ_TestImpact_Eval(
                    expectedTestSkippedCount == testSkippedCount, ArtifactException,
                    AZStd::string::format("Expected %llu test cases skipped, got %llu", expectedTestSkippedCount, testSkippedCount));
            }
            catch (const std::exception& e)
            {
                AZ_Error("TestRunSuitesFactory", false, e.what());
                throw ArtifactException(e.what());
            }
            catch (...)
            {
                throw ArtifactException("An unknown error occurred parsing the XML data");
            }

            return testSuites;
        }
    }
} // namespace TestImpact
