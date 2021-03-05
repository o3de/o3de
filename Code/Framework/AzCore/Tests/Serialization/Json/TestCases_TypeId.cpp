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

#include <AzCore/Math/Vector3.h>
#include <Tests/Serialization/Json/JsonSerializationTests.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>
#include <Tests/Serialization/Json/TestCases_Pointers.h>

namespace JsonSerializationTests
{
    // Through Load
    TEST_F(JsonSerializationTests, Load_TypeWithWhitespace_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        ComplexNullInheritedPointer::Reflect(m_serializeContext, true);

        m_jsonDocument->Parse(
            R"({
                    "pointer": 
                    {
                        "$type": "    SimpleInheritence    "
                    }
                })");

        ComplexNullInheritedPointer instance;
        ResultCode loadResult = AZ::JsonSerialization::Load(instance, *m_jsonDocument, *m_deserializationSettings);
        ASSERT_EQ(Outcomes::DefaultsUsed, loadResult.GetOutcome());

        ASSERT_NE(nullptr, instance.m_pointer);
        EXPECT_EQ(azrtti_typeid(instance.m_pointer), azrtti_typeid <SimpleInheritence>());
    }

    // LoadTypeId

    TEST_F(JsonSerializationTests, LoadTypeId_FromUuidString_Success)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        AZStd::string_view idString = "{0CC36DD4-2337-40B4-A86C-69C452C48555}";
        Uuid id(idString.data(), idString.length());
        Uuid output;

        ResultCode result = JsonSerialization::LoadTypeId(output,
            m_jsonDocument->SetString(rapidjson::StringRef(idString.data())), nullptr,
            AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(id, output);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_FromUuidStringWithNoise_Success)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        AZStd::string_view idString = "Some comments {0CC36DD4-2337-40B4-A86C-69C452C48555} Vector3";
        Uuid id("{0CC36DD4-2337-40B4-A86C-69C452C48555}");
        Uuid output;

        ResultCode result = JsonSerialization::LoadTypeId(output,
            m_jsonDocument->SetString(rapidjson::StringRef(idString.data())), nullptr,
            AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(id, output);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_LoadFromNameString_Success)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Vector3"),
            nullptr, AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(azrtti_typeid<Vector3>(), output);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_FromNameStringWithExtraWhitespace_Success)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("    Vector3      "),
            nullptr, AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(azrtti_typeid<Vector3>(), output);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_FromNameString_UuidSerializerDoesNotReportUnsupported)
    {
        // The plain string is ran through the Uuid Serializer, which doesn't recognize it as a valid uuid format,
        // but as a type id it's still valid. A bug caused this to be reported by the Uuid Serializer as an error, but
        // didn't prevent the type id from working. This test is added to make sure that the bug doesn't regress and
        // that the error from Uuid Serializer isn't reported when deserializing a type id.

        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        m_deserializationSettings->m_reporting = [](AZStd::string_view,
            JsonSerializationResult::ResultCode result, AZStd::string_view)->ResultCode
        {
            EXPECT_NE(Outcomes::Unsupported, result.GetOutcome());
            return result;
        };

        Uuid output;
        JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Vector3"),
            nullptr, AZStd::string_view{}, *m_deserializationSettings);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_UnknownName_FailToLoad)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Unknown"),
            nullptr, AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE(output.IsNull());
    }

    TEST_F(JsonSerializationTests, LoadTypeId_AmbigiousName_FailToLoad)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        m_serializeContext->Class<A::Inherited, BaseClass>();
        m_serializeContext->Class<B::Inherited, BaseClass>();

        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Inherited"),
            nullptr, AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE(output.IsNull());
    }

    TEST_F(JsonSerializationTests, LoadTypeId_InvalidBaseType_FailToLoad)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "$type": "{0CC36DD4-2337-40B4-A86C-69C452C48555}"
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        Uuid invalidBaseType("{A2189202-D645-4D5A-822F-A77218A010CF}");
        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, *m_jsonDocument, &invalidBaseType,
            AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE(output.IsNull());
    }

    TEST_F(JsonSerializationTests, LoadTypeId_InvalidJsonTypeForTypeId_FailToLoad)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "$type": 42
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        Uuid output;
        Uuid placeholderTypeId = azrtti_typeid<int>();
        ResultCode result = JsonSerialization::LoadTypeId(output, *m_jsonDocument, &placeholderTypeId,
            AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE(output.IsNull());
    }

    TEST_F(JsonSerializationTests, LoadTypeId_ResolvedAmbigiousName_SuccessAndBaseClassPicked)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        BaseClass::Reflect(m_serializeContext);
        m_serializeContext->Class<A::Inherited, BaseClass>();
        m_serializeContext->Class<C::Inherited>();

        Uuid baseClass = azrtti_typeid<BaseClass>();
        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Inherited"),
            &baseClass, AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(azrtti_typeid<A::Inherited>(), output);
    }

    TEST_F(JsonSerializationTests, LoadTypeId_UnresolvedAmbigiousName_FailToLoad)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        BaseClass::Reflect(m_serializeContext);
        m_serializeContext->Class<A::Inherited, BaseClass>();
        m_serializeContext->Class<B::Inherited, BaseClass>();

        Uuid baseClass = azrtti_typeid<BaseClass>();
        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetString("Inherited"),
            &baseClass, AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE(output.IsNull());
    }

    TEST_F(JsonSerializationTests, LoadTypeId_InputIsNotAString_FailToLoad)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid output;
        ResultCode result = JsonSerialization::LoadTypeId(output, m_jsonDocument->SetBool(true),
            nullptr, AZStd::string_view{}, *m_deserializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
        EXPECT_TRUE(output.IsNull());
    }


    // StoreTypeId

    TEST_F(JsonSerializationTests, StoreTypeId_SingleInstanceType_StoresName)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid input = azrtti_typeid<Vector3>();
        ResultCode result = JsonSerialization::StoreTypeId(*m_jsonDocument, m_jsonDocument->GetAllocator(), input,
            AZStd::string_view{}, *m_serializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq(R"("Vector3")");
    }

    TEST_F(JsonSerializationTests, StoreTypeId_DuplicateInstanceType_StoresUuidWithName)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        m_serializeContext->Class<A::Inherited, BaseClass>();
        m_serializeContext->Class<B::Inherited, BaseClass>();

        Uuid input = azrtti_typeid<A::Inherited>();
        ResultCode result = JsonSerialization::StoreTypeId(*m_jsonDocument, m_jsonDocument->GetAllocator(), input,
            AZStd::string_view{}, *m_serializationSettings);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq(R"("{E7829F37-C577-4F2B-A85B-6F331548354C} Inherited")", false);
    }

    TEST_F(JsonSerializationTests, StoreTypeId_UnknowmType_StoresUuidWithName)
    {
        using namespace AZ;
        using namespace AZ::JsonSerializationResult;

        Uuid input = azrtti_typeid<BaseClass>();
        ResultCode result = JsonSerialization::StoreTypeId(*m_jsonDocument, m_jsonDocument->GetAllocator(), input,
            AZStd::string_view{}, *m_serializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
    }
} // namespace JsonSerializationTests
