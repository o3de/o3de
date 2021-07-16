/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Artifact/Factory/TestImpactTestEnumerationSuiteFactory.h>
#include <Artifact/TestImpactArtifactException.h>

#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    namespace GTest
    {
        AZStd::vector<TestEnumerationSuite> TestEnumerationSuitesFactory(const AZStd::string& testEnumerationData)
        {
            // Keys for pertinent XML node and attribute names
            constexpr const char* Keys[] =
            {
                "testsuites",
                "testsuite",
                "name",
                "testcase"
            };

            enum
            {
                TestSuitesKey,
                TestSuiteKey,
                NameKey,
                TestCaseKey
            };

            AZ_TestImpact_Eval(!testEnumerationData.empty(), ArtifactException, "Cannot parse enumeration, string is empty");
            AZStd::vector<TestEnumerationSuite> testSuites;
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

                    TestEnumerationSuite testSuite;
                    testSuite.m_name = testsuite_node->first_attribute(Keys[NameKey])->value();
                    testSuite.m_enabled = isEnabled(testSuite.m_name);

                    for (auto testcase_node = testsuite_node->first_node(Keys[TestCaseKey]); testcase_node;
                         testcase_node = testcase_node->next_sibling())
                    {
                        TestEnumerationCase testCase;
                        testCase.m_name = testcase_node->first_attribute(Keys[NameKey])->value();
                        testCase.m_enabled = isEnabled(testCase.m_name);
                        testSuite.m_tests.emplace_back(AZStd::move(testCase));
                    }

                    testSuites.emplace_back(AZStd::move(testSuite));
                }
            }
            catch (const std::exception& e)
            {
                AZ_Error("TestEnumerationSuitesFactory", false, e.what());
                throw ArtifactException(e.what());
            }
            catch (...)
            {
                throw ArtifactException("An unknown error occured parsing the XML data");
            }

            return testSuites;
        }
    } // namespace GTest
} // namespace TestImpact
