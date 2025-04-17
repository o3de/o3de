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

            enum Fields
            {
                TestSuitesKey,
                TestSuiteKey,
                NameKey,
                TestCaseKey,
                // Checksum
                _CHECKSUM_
            };

            static_assert(Fields::_CHECKSUM_ == AZStd::size(Keys));
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
        AZStd::vector<PythonTestEnumerationSuite> TestEnumerationSuitesFactory(const AZStd::string& testEnumerationData)
        {
            size_t startLine = 0;
            size_t endLine = 0;

            AZStd::map<AZStd::string, AZStd::vector<TestEnumerationSuite>> testSuiteMap;
            AZStd::string previousTestFixture;

            while ((startLine = testEnumerationData.find_first_not_of('\n', endLine)) != AZStd::string::npos)
            {
                enum MatchEntries
                {
                    FullMatch,
                    ScriptPath,
                    TestFixture,
                    TestName,
                    NumMatches
                };

                endLine = testEnumerationData.find('\n', startLine);
                const AZStd::string curLine = testEnumerationData.substr(startLine, endLine - startLine);

                // This regex matches pytest test names, in the form ScriptPath::TestFixture::TestName. It will accept both parameterized
                // and unparamaterized tests (with or without [] at the end)
                const AZStd::basic_regex testNamePattern = AZStd::basic_regex("(.*py)::([A-Za-z_/\\-0-9]*)::(.*)");

                // Steps through our string line by line. Assumes lines are split by newline characters.
                if (AZStd::smatch matchResults; AZStd::regex_search(curLine, matchResults, testNamePattern))
                {
                    AZ_TestImpact_Eval(
                        matchResults.size() == NumMatches,
                        ArtifactException,
                        "TestSuite match results did not equal expected number of capture groups");
                    const AZStd::string absoluteScriptPath = matchResults[ScriptPath];
                    const AZStd::string testFixture = matchResults[TestFixture];
                    const AZStd::string testName = matchResults[TestName];

                    // Fetch or create the vector for this script
                    AZStd::vector<TestEnumerationSuite>& suitesForThisScript = testSuiteMap[absoluteScriptPath];

                    if (previousTestFixture != testFixture || suitesForThisScript.empty())
                    {
                        // If we're not working with the same fixture as our prevous iteration, or our suites vector is empty, create a test
                        // suite, add the current test case to it, and add the suite to our vector
                        TestEnumerationSuite currentTestSuite =
                            TestEnumerationSuite{ testFixture, true, AZStd::vector<TestEnumerationCase>() };
                        currentTestSuite.m_tests.emplace_back(TestEnumerationCase{ testName, true });
                        suitesForThisScript.push_back(currentTestSuite);
                    }
                    else
                    {
                        // Else, find the test suite in our vector, get the reference to it and add our current test case to it
                        TestEnumerationSuite* currentTestSuitePointer = AZStd::find_if(
                            suitesForThisScript.begin(),
                            suitesForThisScript.end(),
                            [&](const TestEnumerationSuite& suite)
                            {
                                return suite.m_name == testFixture;
                            });
                        TestEnumerationSuite& currentTestSuite = *currentTestSuitePointer;
                        currentTestSuite.m_tests.emplace_back(TestEnumerationCase{ testName, true });
                    }   

                    // Update previous test fixture so that we can keep track of when test fixtures change
                    previousTestFixture = testFixture;
                }
            }

            // Extract the key/value pairs from our testSuiteMap and put them in our pairs output variable
            return AZStd::vector(testSuiteMap.begin(), testSuiteMap.end());
        }
    } // namespace Python
} // namespace TestImpact
