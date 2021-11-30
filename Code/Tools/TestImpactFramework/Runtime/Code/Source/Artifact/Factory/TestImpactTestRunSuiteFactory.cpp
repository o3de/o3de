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

namespace TestImpact
{
    namespace GTest
    {
        AZStd::vector<TestRunSuite> TestRunSuitesFactory(const AZStd::string& testEnumerationData)
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

            enum
            {
                TestSuitesKey,
                TestSuiteKey,
                NameKey,
                TestCaseKey,
                StatusKey,
                RunKey,
                NotRunKey,
                DurationKey
            };

            AZ_TestImpact_Eval(!testEnumerationData.empty(), ArtifactException, "Cannot parse test run, string is empty");
            AZStd::vector<TestRunSuite> testSuites;
            AZStd::vector<char> rawData(testEnumerationData.begin(), testEnumerationData.end());

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

                    AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr
                                                                              // in the capture. Newer versions issue unused warning
                    const auto getDuration = [Keys](const AZ::rapidxml::xml_node<>* node)
                    AZ_POP_DISABLE_WARNING
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
                        AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the capture.
                                                                                  // Newer versions issue unused warning
                        const auto getStatus = [Keys](const AZ::rapidxml::xml_node<>* node)
                        AZ_POP_DISABLE_WARNING
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
                            if (auto child_node = node->first_node("failure"))
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
} // namespace TestImpact
