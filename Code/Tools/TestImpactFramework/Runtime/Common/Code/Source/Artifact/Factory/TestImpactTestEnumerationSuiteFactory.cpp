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
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/regex.h>

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
    namespace Python
    {

        AZStd::vector<AZStd::pair<AZStd::string, AZStd::vector<TestEnumerationSuite>>> TestEnumerationSuitesFactory(
            const AZStd::string& testEnumerationData)
        {
            const char newLineDelim = '\n';
            size_t startLine;
            size_t endLine = 0;

            AZStd::vector<AZStd::pair<AZStd::string, AZStd::vector<TestEnumerationSuite>>> pairs =
                AZStd::vector<AZStd::pair<AZStd::string, AZStd::vector<TestEnumerationSuite>>>();
            AZStd::map<AZStd::string, AZStd::map<AZStd::string, AZStd::set<AZStd::string>>> testMapping =
                AZStd::map<AZStd::string, AZStd::map<AZStd::string, AZStd::set<AZStd::string>>>();

            // Steps through our string line by line. Assumes lines are split by newline characters.
            while ((startLine = testEnumerationData.find_first_not_of(newLineDelim, endLine)) != AZStd::string::npos)
            {
                endLine = testEnumerationData.find(newLineDelim, startLine);
                AZStd::string curLine = testEnumerationData.substr(startLine, endLine - startLine);

                curLine = curLine.substr(0, curLine.find_first_of('['));
                // This regex matches pytest test names, in the form ScriptPath::TestFixture::TestName. It will accept both parameterized
                // and unparamaterized tests (with or without [] at the end).
                AZStd::basic_regex testNamePattern = AZStd::basic_regex("([a-zA-Z_/\\-0-9.]*\\.py)::([A-Za-z_/\\-0-9]*)::(.*)");
                AZStd::smatch matchResults;
                if (AZStd::regex_search(curLine, matchResults, testNamePattern))
                {
                    AZStd::string absoluteScriptPath = matchResults[1];
                    AZStd::string testFixture = matchResults[2];
                    AZStd::string testName = matchResults[3];

                    // Fetch or create the map for this script
                    AZStd::map<AZStd::string, AZStd::set<AZStd::string>>& moduleMap = testMapping[absoluteScriptPath];

                    // Fetch or create the set for this test fixture, then add our test name to the set.
                    AZStd::set<AZStd::string>& testNameSet = moduleMap[testFixture];
                    testNameSet.insert(testName);
                }
            }

            // Create a list of test suites for each script
            // Lookup the map for that script path, and for each test class in that map, create a test suite object and add all the tests to
            // that test suite. Create a pair consisting of the script path and the test suites attached to that script, and add to our
            // vector.
            for (const auto& [scriptPath, moduleMap] : testMapping)
            {
                AZStd::vector<TestEnumerationSuite> testSuites = AZStd::vector<TestEnumerationSuite>();
                for (const auto& [moduleName, testSet] : moduleMap)
                {
                    TestEnumerationSuite testSuite;
                    testSuite.m_name = moduleName;
                    testSuite.m_enabled = true;
                    for (const AZStd::string testName : testSet)
                    {
                        TestEnumerationCase testCase;
                        testCase.m_name = testName;
                        testCase.m_enabled = true;
                        testSuite.m_tests.emplace_back(AZStd::move(testCase));
                    }
                    testSuites.emplace_back(AZStd::move(testSuite));
                }
                pairs.emplace_back(AZStd::pair<AZStd::string, AZStd::vector<TestEnumerationSuite>>(scriptPath, testSuites));
            }
            return pairs;
        }

    } // namespace Python
} // namespace TestImpact
