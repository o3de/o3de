/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/TestImpactTestRunnerException.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumerationSerializer.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/stringbuffer.h>

namespace TestImpact
{
    namespace TestEnumerationSerializer
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "suites",
            "name",
            "enabled",
            "tests"
        };

        enum Fields
        {
            SuitesKey,
            NameKey,
            EnabledKey,
            TestsKey,
            // Checksum
            _CHECKSUM_
        };
    } // namespace TestEnumerationSerializer

    AZStd::string SerializeTestEnumeration(const TestEnumeration& testEnum)
    {
        static_assert(TestEnumerationSerializer::Fields::_CHECKSUM_ == AZStd::size(TestEnumerationSerializer::Keys));
        rapidjson::StringBuffer stringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);

        writer.StartObject();
        writer.Key(TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::SuitesKey]);
        writer.StartArray();
        for (const auto& suite : testEnum.GetTestSuites())
        {
            writer.StartObject();
            writer.Key(TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::NameKey]);
            writer.String(suite.m_name.c_str());
            writer.Key(TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::EnabledKey]);
            writer.Bool(suite.m_enabled);
            writer.Key(TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::TestsKey]);
            writer.StartArray();
            for (const auto& test : suite.m_tests)
            {
                writer.StartObject();
                writer.Key(TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::NameKey]);
                writer.String(test.m_name.c_str());
                writer.Key(TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::EnabledKey]);
                writer.Bool(test.m_enabled);
                writer.EndObject();
            }
            writer.EndArray();
            writer.EndObject();
        }
        writer.EndArray();
        writer.EndObject();

        return stringBuffer.GetString();
    }

    TestEnumeration DeserializeTestEnumeration(const AZStd::string& testEnumString)
    {
        AZStd::vector<TestEnumerationSuite> testSuites;
        rapidjson::Document doc;

        if (doc.Parse<0>(testEnumString.c_str()).HasParseError())
        {
            throw TestRunnerException("Could not parse enumeration data");
        }

        for (const auto& suite : doc[TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::SuitesKey]].GetArray())
        {
            testSuites.emplace_back(TestEnumerationSuite{suite[TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::NameKey]].GetString(), suite[TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::EnabledKey]].GetBool(), {}});
            for (const auto& test : suite[TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::TestsKey]].GetArray())
            {
                testSuites.back().m_tests.emplace_back(
                    TestEnumerationCase{test[TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::NameKey]].GetString(), test[TestEnumerationSerializer::Keys[TestEnumerationSerializer::Fields::EnabledKey]].GetBool()});
            }
        }

        return TestEnumeration(std::move(testSuites));
    }
} // namespace TestImpact
