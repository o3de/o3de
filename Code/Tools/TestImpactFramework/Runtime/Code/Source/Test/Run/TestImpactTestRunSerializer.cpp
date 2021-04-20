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

#include <Test/Run/TestImpactTestRunException.h>
#include <Test/Run/TestImpactTestRunSerializer.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/stringbuffer.h>

namespace TestImpact
{
    namespace
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
        writer.Key(Keys[DurationKey]);
        writer.Uint(testRun.GetDuration().count());

        // Suites
        writer.Key(Keys[SuitesKey]);
        writer.StartArray();

        for (const auto& suite : testRun.GetTestSuites())
        {
            // Suite
            writer.StartObject();

            // Suite name
            writer.Key(Keys[NameKey]);
            writer.String(suite.m_name.c_str());

            // Suite duration
            writer.Key(Keys[DurationKey]);
            writer.Uint(suite.m_duration.count());

            // Suite enabled
            writer.Key(Keys[EnabledKey]);
            writer.Bool(suite.m_enabled);

            // Suite tests
            writer.Key(Keys[TestsKey]);
            writer.StartArray();
            for (const auto& test : suite.m_tests)
            {
                // Test
                writer.StartObject();

                // Test name
                writer.Key(Keys[NameKey]);
                writer.String(test.m_name.c_str());

                // Test enabled
                writer.Key(Keys[EnabledKey]);
                writer.Bool(test.m_enabled);

                // Test duration
                writer.Key(Keys[DurationKey]);
                writer.Uint(test.m_duration.count());

                // Test status
                writer.Key(Keys[StatusKey]);
                writer.Bool(static_cast<bool>(test.m_status));

                // Test result
                if (test.m_status == TestRunStatus::Run)
                {
                    writer.Key(Keys[ResultKey]);
                    writer.Bool(static_cast<size_t>(test.m_result.value()));
                }
                else
                {
                    writer.Key(Keys[ResultKey]);
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
            throw TestRunException("Could not parse enumeration data");
        }

        // Run duration
        const AZStd::chrono::milliseconds runDuration = AZStd::chrono::milliseconds{doc[Keys[DurationKey]].GetUint()};

        // Suites
        for (const auto& suite : doc[Keys[SuitesKey]].GetArray())
        {
            // Suite name
            const AZStd::string name = suite[Keys[NameKey]].GetString();

            // Suite duration
            const AZStd::chrono::milliseconds suiteDuration = AZStd::chrono::milliseconds{suite[Keys[DurationKey]].GetUint()};

            // Suite enabled
            const bool enabled = suite[Keys[EnabledKey]].GetBool();

            testSuites.emplace_back(TestRunSuite{
                suite[Keys[NameKey]].GetString(),
                suite[Keys[EnabledKey]].GetBool(),
                {},
                AZStd::chrono::milliseconds{suite[Keys[DurationKey]].GetUint()}});

            // Suite tests
            for (const auto& test : suite[Keys[TestsKey]].GetArray())
            {
                AZStd::optional<TestRunResult> result;
                TestRunStatus status = static_cast<TestRunStatus>(test[Keys[StatusKey]].GetBool());
                if (status == TestRunStatus::Run)
                {
                    result = static_cast<TestRunResult>(test[Keys[ResultKey]].GetBool());
                }
                const AZStd::chrono::milliseconds testDuration = AZStd::chrono::milliseconds{test[Keys[DurationKey]].GetUint()};
                testSuites.back().m_tests.emplace_back(
                    TestRunCase{test[Keys[NameKey]].GetString(), test[Keys[EnabledKey]].GetBool(), result, testDuration, status});
            }
        }

        return TestRun(std::move(testSuites), runDuration);
    }
} // namespace TestImpact
