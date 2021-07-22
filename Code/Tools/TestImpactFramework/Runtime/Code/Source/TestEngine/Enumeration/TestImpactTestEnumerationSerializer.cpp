/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/TestImpactTestEngineException.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerationSerializer.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/stringbuffer.h>

namespace TestImpact
{
    namespace TestEnumFields
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "suites",
            "name",
            "enabled",
            "tests"
        };

        enum
        {
            SuitesKey,
            NameKey,
            EnabledKey,
            TestsKey
        };
    } // namespace

    AZStd::string SerializeTestEnumeration(const TestEnumeration& testEnum)
    {
        rapidjson::StringBuffer stringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);

        writer.StartObject();
        writer.Key(TestEnumFields::Keys[TestEnumFields::SuitesKey]);
        writer.StartArray();
        for (const auto& suite : testEnum.GetTestSuites())
        {
            writer.StartObject();
            writer.Key(TestEnumFields::Keys[TestEnumFields::NameKey]);
            writer.String(suite.m_name.c_str());
            writer.Key(TestEnumFields::Keys[TestEnumFields::EnabledKey]);
            writer.Bool(suite.m_enabled);
            writer.Key(TestEnumFields::Keys[TestEnumFields::TestsKey]);
            writer.StartArray();
            for (const auto& test : suite.m_tests)
            {
                writer.StartObject();
                writer.Key(TestEnumFields::Keys[TestEnumFields::NameKey]);
                writer.String(test.m_name.c_str());
                writer.Key(TestEnumFields::Keys[TestEnumFields::EnabledKey]);
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
            throw TestEngineException("Could not parse enumeration data");
        }

        for (const auto& suite : doc[TestEnumFields::Keys[TestEnumFields::SuitesKey]].GetArray())
        {
            testSuites.emplace_back(TestEnumerationSuite{suite[TestEnumFields::Keys[TestEnumFields::NameKey]].GetString(), suite[TestEnumFields::Keys[TestEnumFields::EnabledKey]].GetBool(), {}});
            for (const auto& test : suite[TestEnumFields::Keys[TestEnumFields::TestsKey]].GetArray())
            {
                testSuites.back().m_tests.emplace_back(
                    TestEnumerationCase{test[TestEnumFields::Keys[TestEnumFields::NameKey]].GetString(), test[TestEnumFields::Keys[TestEnumFields::EnabledKey]].GetBool()});
            }
        }

        return TestEnumeration(std::move(testSuites));
    }
} // namespace TestImpact
