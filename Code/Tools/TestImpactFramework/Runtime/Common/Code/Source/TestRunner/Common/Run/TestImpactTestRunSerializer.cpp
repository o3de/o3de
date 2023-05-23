/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/TestImpactTestRunnerException.h>
#include <TestRunner/Common/Run/TestImpactTestRunSerializer.h>

#include <AzCore/Date/DateFormat.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>

namespace TestImpact
{
    namespace TestRunSerializer
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "suites",
            "name",
            "enabled",
            "tests",
            "duration",
            "status",
            "result"
        };

        enum Fields
        {
            SuitesKey,
            NameKey,
            EnabledKey,
            TestsKey,
            DurationKey,
            StatusKey,
            ResultKey,
            // Checksum
            _CHECKSUM_
        };
    } // namespace TestRunSerializer

    namespace GTest
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "testsuites",
            "testsuite",
            "testcase",
            "failures",
            "failure",
            "disabled",
            "errors",
            "timestamp",
            "time",
            "classname",
            "message",
            "type"
        };
        
        enum Fields
        {
            TestSuitesKey,
            TestSuiteKey,
            TestCaseKey,
            FailuresKey,
            FailureKey,
            DisabledKey,
            ErrorsKey,
            TimestampKey,
            TimeKey,
            ClassNameKey,
            MessageKey,
            FailureTypeKey,
             // Checksum
            _CHECKSUM_
        };
        
        AZStd::string SerializeTestRun(const TestRun& testRun)
        {
            static_assert(GTest::Fields::_CHECKSUM_ == AZStd::size(GTest::Keys));
            AZ::rapidxml::xml_document<> doc;
        
            AZ::rapidxml::xml_node<>* decl = doc.allocate_node(AZ::rapidxml::node_declaration);
            decl->append_attribute(doc.allocate_attribute("version", "1.0"));
            decl->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
            doc.append_node(decl);

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            AZ::Date::GetIso8601ExtendedFormatNow(iso8601Timestamp);
        
            AZ::rapidxml::xml_node<>* rootNode = doc.allocate_node(AZ::rapidxml::node_element, GTest::Keys[GTest::Fields::TestSuitesKey]);

            // Number of tests in all test suites
            rootNode->append_attribute(doc.allocate_attribute(
                TestRunSerializer::Keys[TestRunSerializer::Fields::TestsKey], doc.allocate_string(AZStd::to_string(testRun.GetNumTests()).c_str())));

            // Number of failures in all test suites
            rootNode->append_attribute(doc.allocate_attribute(
                GTest::Keys[GTest::Fields::FailuresKey], doc.allocate_string(AZStd::to_string(testRun.GetNumFailures()).c_str())));

            // Number of disabled tests in all test suites
            rootNode->append_attribute(doc.allocate_attribute(
                GTest::Keys[GTest::Fields::DisabledKey], doc.allocate_string(AZStd::to_string(testRun.GetNumDisabledTests()).c_str())));

            // Number of errors in all test suites (not supported in TestRun)
            rootNode->append_attribute(doc.allocate_attribute(GTest::Keys[GTest::Fields::ErrorsKey], "0"));

            // Timestamp of test completion (aka time of serialization)
            rootNode->append_attribute(doc.allocate_attribute(GTest::Keys[GTest::Fields::TimestampKey], iso8601Timestamp.c_str()));

            // Total duration of all test suites
            rootNode->append_attribute(doc.allocate_attribute(
                GTest::Keys[GTest::Fields::TimeKey], doc.allocate_string(AZStd::to_string(testRun.GetDuration().count() / 1000.f).c_str())));

            // Name (unclear, all seem to be AllTests)
            rootNode->append_attribute(doc.allocate_attribute(TestRunSerializer::Keys[TestRunSerializer::Fields::NameKey], "AllTests"));
            doc.append_node(rootNode);

            // Individual test suites
            for (const auto& testSuite : testRun.GetTestSuites())
            {
                AZ::rapidxml::xml_node<>* testSuiteNode = doc.allocate_node(AZ::rapidxml::node_element, GTest::Keys[GTest::Fields::TestSuiteKey]);

                // Name of test suite
                testSuiteNode->append_attribute(
                    doc.allocate_attribute(TestRunSerializer::Keys[TestRunSerializer::Fields::NameKey], testSuite.m_name.c_str()));

                // Number of tests in test suite
                testSuiteNode->append_attribute(doc.allocate_attribute(
                    TestRunSerializer::Keys[TestRunSerializer::Fields::TestsKey], doc.allocate_string(AZStd::to_string(testSuite.m_tests.size()).c_str())));

                // Number of failures in test suite
                const auto numFailingTests = AZStd::count_if(
                    testSuite.m_tests.begin(),
                    testSuite.m_tests.end(),
                    [](const TestRunCase& test)
                    {
                        return test.m_result.has_value() && test.m_result.value() == TestRunResult::Failed;
                    });
                testSuiteNode->append_attribute(doc.allocate_attribute(
                    GTest::Keys[GTest::Fields::FailuresKey], doc.allocate_string(AZStd::to_string(numFailingTests).c_str())));

                // Number of disabled tests in test suite
                const auto numDisabledTests = AZStd::count_if(
                    testSuite.m_tests.begin(),
                    testSuite.m_tests.end(),
                    [](const TestRunCase& test)
                    {
                        return !test.m_enabled;
                    });
                testSuiteNode->append_attribute(doc.allocate_attribute(
                    GTest::Keys[GTest::Fields::DisabledKey], doc.allocate_string(AZStd::to_string(numDisabledTests).c_str())));

                // Total duration of all tests in the suite
                testSuiteNode->append_attribute(doc.allocate_attribute(
                    GTest::Keys[GTest::Fields::TimeKey], doc.allocate_string(AZStd::to_string(testSuite.m_duration.count() / 1000.f).c_str())));

                // Number of errors in test suite (not supported in TestRun)
                testSuiteNode->append_attribute(doc.allocate_attribute(GTest::Keys[GTest::Fields::ErrorsKey], "0"));

                // Individual tests in test suite
                for (const auto& testCase : testSuite.m_tests)
                {
                    AZ::rapidxml::xml_node<>* testCaseNode =
                        doc.allocate_node(AZ::rapidxml::node_element, GTest::Keys[GTest::Fields::TestCaseKey]);

                    // Name of test case
                    testCaseNode->append_attribute(
                        doc.allocate_attribute(TestRunSerializer::Keys[TestRunSerializer::Fields::NameKey], testCase.m_name.c_str()));

                    // Status of test case run
                    testCaseNode->append_attribute(doc.allocate_attribute(
                        TestRunSerializer::Keys[TestRunSerializer::Fields::StatusKey],
                        doc.allocate_string(testCase.m_status == TestRunStatus::Run ? "run" : "notrun")));

                    // Total duration of test case run
                    testCaseNode->append_attribute(doc.allocate_attribute(
                        GTest::Keys[GTest::Fields::TimeKey], doc.allocate_string(AZStd::to_string(testCase.m_duration.count() / 1000.f).c_str())));

                    // Name of parent test suite
                    testCaseNode->append_attribute(
                        doc.allocate_attribute(GTest::Keys[GTest::Fields::ClassNameKey], testSuite.m_name.c_str()));

                    // Test failure message (TestRun doesn't store this message string)
                    if (testCase.m_result.has_value() && testCase.m_result.value() == TestRunResult::Failed)
                    {
                        AZ::rapidxml::xml_node<>* testCaseFailureNode =
                            doc.allocate_node(AZ::rapidxml::node_element, GTest::Keys[GTest::Fields::FailureKey]);

                        // Failure message
                        testCaseFailureNode->append_attribute(doc.allocate_attribute(GTest::Keys[GTest::Fields::MessageKey], "Test failed (check log output for more details)"));

                        // Failure type (not supported by GTest)
                        testCaseFailureNode->append_attribute(doc.allocate_attribute(GTest::Keys[GTest::Fields::FailureTypeKey], ""));

                        testCaseNode->append_node(testCaseFailureNode);
                    }

                    testSuiteNode->append_node(testCaseNode);
                }

                rootNode->append_node(testSuiteNode);
            }

            AZStd::string xmlString;
            AZ::rapidxml::print(std::back_inserter(xmlString), doc);
            doc.clear();

            return xmlString;
        }
    } // namespace GTest

    AZStd::string SerializeTestRun(const TestRun& testRun)
    {
        static_assert(TestRunSerializer::Fields::_CHECKSUM_ == AZStd::size(TestRunSerializer::Keys));
        rapidjson::StringBuffer stringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);

        // Run
        writer.StartObject();

        // Run duration
        writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::DurationKey]);
        writer.Uint(static_cast<unsigned int>(testRun.GetDuration().count()));

        // Suites
        writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::SuitesKey]);
        writer.StartArray();

        for (const auto& suite : testRun.GetTestSuites())
        {
            // Suite
            writer.StartObject();

            // Suite name
            writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::NameKey]);
            writer.String(suite.m_name.c_str());

            // Suite duration
            writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::DurationKey]);
            writer.Uint(static_cast<unsigned int>(suite.m_duration.count()));

            // Suite enabled
            writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::EnabledKey]);
            writer.Bool(suite.m_enabled);

            // Suite tests
            writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::TestsKey]);
            writer.StartArray();
            for (const auto& test : suite.m_tests)
            {
                // Test
                writer.StartObject();

                // Test name
                writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::NameKey]);
                writer.String(test.m_name.c_str());

                // Test enabled
                writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::EnabledKey]);
                writer.Bool(test.m_enabled);

                // Test duration
                writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::DurationKey]);
                writer.Uint(static_cast<unsigned int>(test.m_duration.count()));

                // Test status
                writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::StatusKey]);
                writer.Bool(static_cast<bool>(test.m_status));

                // Test result
                if (test.m_status == TestRunStatus::Run)
                {
                    writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::ResultKey]);
                    writer.Bool(static_cast<size_t>(test.m_result.value()));
                }
                else
                {
                    writer.Key(TestRunSerializer::Keys[TestRunSerializer::Fields::ResultKey]);
                    writer.Null();
                }

                // End test
                writer.EndObject();
            }

            // End tests
            writer.EndArray();

            // End suite
            writer.EndObject();
        }

        // End suites
        writer.EndArray();

        // End run
        writer.EndObject();

        return stringBuffer.GetString();
    }

    TestRun DeserializeTestRun(const AZStd::string& testEnumString)
    {
        AZStd::vector<TestRunSuite> testSuites;
        rapidjson::Document doc;

        if (doc.Parse<0>(testEnumString.c_str()).HasParseError())
        {
            throw TestRunnerException("Could not parse enumeration data");
        }

        // Run duration
        const AZStd::chrono::milliseconds runDuration = AZStd::chrono::milliseconds{doc[TestRunSerializer::Keys[TestRunSerializer::Fields::DurationKey]].GetUint()};

        // Suites
        for (const auto& suite : doc[TestRunSerializer::Keys[TestRunSerializer::Fields::SuitesKey]].GetArray())
        {
            // Suite enabled
            testSuites.emplace_back(TestRunSuite{
                TestSuite<TestRunCase>{
                    suite[TestRunSerializer::Keys[TestRunSerializer::Fields::NameKey]].GetString(),
                    suite[TestRunSerializer::Keys[TestRunSerializer::Fields::EnabledKey]].GetBool()
                },
                AZStd::chrono::milliseconds{ suite[TestRunSerializer::Keys[TestRunSerializer::Fields::DurationKey]].GetUint() }
            });

            // Suite tests
            for (const auto& test : suite[TestRunSerializer::Keys[TestRunSerializer::Fields::TestsKey]].GetArray())
            {
                AZStd::optional<TestRunResult> result;
                TestRunStatus status = static_cast<TestRunStatus>(test[TestRunSerializer::Keys[TestRunSerializer::Fields::StatusKey]].GetBool());
                if (status == TestRunStatus::Run)
                {
                    result = static_cast<TestRunResult>(test[TestRunSerializer::Keys[TestRunSerializer::Fields::ResultKey]].GetBool());
                }
                const AZStd::chrono::milliseconds testDuration = AZStd::chrono::milliseconds{test[TestRunSerializer::Keys[TestRunSerializer::Fields::DurationKey]].GetUint()};
                testSuites.back().m_tests.emplace_back(
                    TestRunCase{
                        TestCase{
                            test[TestRunSerializer::Keys[TestRunSerializer::Fields::NameKey]].GetString(),
                            test[TestRunSerializer::Keys[TestRunSerializer::Fields::EnabledKey]].GetBool()
                        },
                         result, testDuration, status
                    });
            }
        }

        return TestRun(std::move(testSuites), runDuration);
    }
} // namespace TestImpact
