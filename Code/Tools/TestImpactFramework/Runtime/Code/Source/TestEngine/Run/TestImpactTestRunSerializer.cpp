/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/TestImpactTestEngineException.h>
#include <TestEngine/Run/TestImpactTestRunSerializer.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/stringbuffer.h>

namespace TestImpact
{
    namespace TestRunFields
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

        enum
        {
            SuitesKey,
            NameKey,
            EnabledKey,
            TestsKey,
            DurationKey,
            StatusKey,
            ResultKey
        };
    } // namespace

    AZStd::string SerializeTestRun(const TestRun& testRun)
    {
        rapidjson::StringBuffer stringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);

        // Run
        writer.StartObject();

        // Run duration
        writer.Key(TestRunFields::Keys[TestRunFields::DurationKey]);
        writer.Uint(static_cast<unsigned int>(testRun.GetDuration().count()));

        // Suites
        writer.Key(TestRunFields::Keys[TestRunFields::SuitesKey]);
        writer.StartArray();

        for (const auto& suite : testRun.GetTestSuites())
        {
            // Suite
            writer.StartObject();

            // Suite name
            writer.Key(TestRunFields::Keys[TestRunFields::NameKey]);
            writer.String(suite.m_name.c_str());

            // Suite duration
            writer.Key(TestRunFields::Keys[TestRunFields::DurationKey]);
            writer.Uint(static_cast<unsigned int>(suite.m_duration.count()));

            // Suite enabled
            writer.Key(TestRunFields::Keys[TestRunFields::EnabledKey]);
            writer.Bool(suite.m_enabled);

            // Suite tests
            writer.Key(TestRunFields::Keys[TestRunFields::TestsKey]);
            writer.StartArray();
            for (const auto& test : suite.m_tests)
            {
                // Test
                writer.StartObject();

                // Test name
                writer.Key(TestRunFields::Keys[TestRunFields::NameKey]);
                writer.String(test.m_name.c_str());

                // Test enabled
                writer.Key(TestRunFields::Keys[TestRunFields::EnabledKey]);
                writer.Bool(test.m_enabled);

                // Test duration
                writer.Key(TestRunFields::Keys[TestRunFields::DurationKey]);
                writer.Uint(static_cast<unsigned int>(test.m_duration.count()));

                // Test status
                writer.Key(TestRunFields::Keys[TestRunFields::StatusKey]);
                writer.Bool(static_cast<bool>(test.m_status));

                // Test result
                if (test.m_status == TestRunStatus::Run)
                {
                    writer.Key(TestRunFields::Keys[TestRunFields::ResultKey]);
                    writer.Bool(static_cast<size_t>(test.m_result.value()));
                }
                else
                {
                    writer.Key(TestRunFields::Keys[TestRunFields::ResultKey]);
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
            throw TestEngineException("Could not parse enumeration data");
        }

        // Run duration
        const AZStd::chrono::milliseconds runDuration = AZStd::chrono::milliseconds{doc[TestRunFields::Keys[TestRunFields::DurationKey]].GetUint()};

        // Suites
        for (const auto& suite : doc[TestRunFields::Keys[TestRunFields::SuitesKey]].GetArray())
        {
            // Suite name
            const AZStd::string name = suite[TestRunFields::Keys[TestRunFields::NameKey]].GetString();

            // Suite duration
            const AZStd::chrono::milliseconds suiteDuration = AZStd::chrono::milliseconds{suite[TestRunFields::Keys[TestRunFields::DurationKey]].GetUint()};

            // Suite enabled
            testSuites.emplace_back(TestRunSuite{
                suite[TestRunFields::Keys[TestRunFields::NameKey]].GetString(),
                suite[TestRunFields::Keys[TestRunFields::EnabledKey]].GetBool(),
                {},
                AZStd::chrono::milliseconds{suite[TestRunFields::Keys[TestRunFields::DurationKey]].GetUint()}});

            // Suite tests
            for (const auto& test : suite[TestRunFields::Keys[TestRunFields::TestsKey]].GetArray())
            {
                AZStd::optional<TestRunResult> result;
                TestRunStatus status = static_cast<TestRunStatus>(test[TestRunFields::Keys[TestRunFields::StatusKey]].GetBool());
                if (status == TestRunStatus::Run)
                {
                    result = static_cast<TestRunResult>(test[TestRunFields::Keys[TestRunFields::ResultKey]].GetBool());
                }
                const AZStd::chrono::milliseconds testDuration = AZStd::chrono::milliseconds{test[TestRunFields::Keys[TestRunFields::DurationKey]].GetUint()};
                testSuites.back().m_tests.emplace_back(
                    TestRunCase{test[TestRunFields::Keys[TestRunFields::NameKey]].GetString(), test[TestRunFields::Keys[TestRunFields::EnabledKey]].GetBool(), result, testDuration, status});
            }
        }

        return TestRun(std::move(testSuites), runDuration);
    }
} // namespace TestImpact
