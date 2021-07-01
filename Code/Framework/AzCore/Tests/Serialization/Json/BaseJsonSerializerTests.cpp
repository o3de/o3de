/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/BaseJsonSerializer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

namespace JsonSerializationTests
{
    class JsonScopedContextReporterTests
        : public BaseJsonSerializerFixture
    {};

    TEST_F(JsonScopedContextReporterTests, PushPop_PushCustomReportThenPop_MessageIsSendToCustomReporter)
    {
        using namespace AZ::JsonSerializationResult;

        bool reported = false;
        auto reporter = [&reported](AZStd::string_view, ResultCode result, AZStd::string_view) -> ResultCode
        {
            reported = true;
            return result;
        };

        {
            AZ::ScopedContextReporter reporterScope(*m_jsonDeserializationContext, reporter);
            m_jsonDeserializationContext->Report(Tasks::RetrieveInfo, Outcomes::Invalid, "message");
            EXPECT_TRUE(reported);
        }
        reported = false;
        m_jsonDeserializationContext->Report(Tasks::RetrieveInfo, Outcomes::Invalid, "message");
        EXPECT_FALSE(reported);
    }

    class JsonScopedContextPathTests
        : public BaseJsonSerializerFixture
    {};

    TEST_F(JsonScopedContextPathTests, PushPop_PushStringValueThenPop_ValueIsPushedAndPoppedCorrectly)
    {
        {
            AZ::ScopedContextPath path(*m_jsonDeserializationContext, "ScopedContextPath test");
            EXPECT_EQ(0, m_jsonDeserializationContext->GetPath().Get().compare("/ScopedContextPath test"));
        }
        EXPECT_TRUE(m_jsonDeserializationContext->GetPath().Get().empty());
    }

    TEST_F(JsonScopedContextPathTests, PushPop_PushInValueThenPop_ValueIsPushedAndPoppedCorrectly)
    {
        {
            AZ::ScopedContextPath path(*m_jsonDeserializationContext, 42);
            EXPECT_EQ(0, m_jsonDeserializationContext->GetPath().Get().compare("/42"));
        }
        EXPECT_TRUE(m_jsonDeserializationContext->GetPath().Get().empty());
    }

    class BaseJsonSerializerTests
        : public BaseJsonSerializerFixture
        , public AZ::BaseJsonSerializer
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
        }

        void TearDown() override
        {
            BaseJsonSerializerFixture::TearDown();
        }

        AZ::JsonSerializationResult::Result Load(void*, const AZ::Uuid&, const rapidjson::Value&,
            AZ::JsonDeserializerContext& context) override
        {
            using namespace AZ::JsonSerializationResult;
            return context.Report(Tasks::RetrieveInfo, Outcomes::Catastrophic, "Test");
        }

        AZ::JsonSerializationResult::Result Store(rapidjson::Value&, const void*, const void*,
            const AZ::Uuid&, AZ::JsonSerializerContext& context) override
        {
            using namespace AZ::JsonSerializationResult;
            return context.Report(Tasks::RetrieveInfo, Outcomes::Catastrophic, "Test");
        }
    };

    //
    // ContinueLoading
    //

    TEST_F(BaseJsonSerializerTests, ContinueLoading_DirectInstance_ValueLoadedCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value json;
        json.Set(42);
        int value = 0;

        ResultCode result = ContinueLoading(&value, azrtti_typeid<int>(), json, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(42, value);
    }

    TEST_F(BaseJsonSerializerTests, ContinueLoading_ToPointerInstance_ValueLoadedCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value json;
        json.Set(42);
        int value = 0;
        int* ptrValue = &value;

        ResultCode result =
            ContinueLoading(&ptrValue, azrtti_typeid<int>(), json, *m_jsonDeserializationContext, ContinuationFlags::ResolvePointer);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        ASSERT_NE(nullptr, ptrValue);
        EXPECT_EQ(42, value);
    }

    TEST_F(BaseJsonSerializerTests, ContinueLoading_ToNullPointer_ValueLoadedCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value json;
        json.Set(42);
        int* ptrValue = nullptr;

        ResultCode result =
            ContinueLoading(&ptrValue, azrtti_typeid<int>(), json, *m_jsonDeserializationContext, ContinuationFlags::ResolvePointer);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        ASSERT_NE(nullptr, ptrValue);
        EXPECT_EQ(42, *ptrValue);

        azfree(ptrValue, AZ::SystemAllocator, sizeof(int), alignof(int));
    }

    TEST_F(BaseJsonSerializerTests, ContinueLoading_DefaultToNullPointer_ValueLoadedCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value json(rapidjson::kObjectType);
        int* ptrValue = nullptr;

        ResultCode result =
            ContinueLoading(&ptrValue, azrtti_typeid<int>(), json, *m_jsonDeserializationContext, ContinuationFlags::ResolvePointer);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        ASSERT_NE(nullptr, ptrValue);
        
        azfree(ptrValue, AZ::SystemAllocator, sizeof(int), alignof(int));
    }

    TEST_F(BaseJsonSerializerTests, ContinueLoading_NullDeletesObject_ValueLoadedCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value json(rapidjson::kNullType);
        int* ptrValue = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int), AZ::SystemAllocator));

        ResultCode result =
            ContinueLoading(&ptrValue, azrtti_typeid<int>(), json, *m_jsonDeserializationContext, ContinuationFlags::ResolvePointer);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        ASSERT_EQ(nullptr, ptrValue);
    }

    //
    // ContinueStoring
    //

    TEST_F(BaseJsonSerializerTests, ContinueStoring_StoreDirectInstance_ValueStoredCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        int value = 42;
        
        ResultCode result = ContinueStoring(*m_jsonDocument, &value, nullptr, azrtti_typeid<int>(), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq("42");
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoring_StorePointerInstance_ValueStoredCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        int value = 42;
        int* ptrValue = &value;

        ResultCode result = ContinueStoring(
            *m_jsonDocument, &ptrValue, nullptr, azrtti_typeid<int>(), *m_jsonSerializationContext, ContinuationFlags::ResolvePointer);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq("42");
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoring_StorePointerToFullDefaultedInstance_ValueStoredCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        int value = 42;
        int* ptrValue = &value;
        int value2 = 42;
        int* defaultPtrValue = &value2;

        ResultCode result = ContinueStoring(
            *m_jsonDocument, &ptrValue, &defaultPtrValue, azrtti_typeid<int>(), *m_jsonSerializationContext,
            ContinuationFlags::ResolvePointer);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq("{}");
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoring_StorePointerToNullptr_ValueStoredCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        int* ptrValue = nullptr;
        
        ResultCode result = ContinueStoring(
            *m_jsonDocument, &ptrValue, nullptr, azrtti_typeid<int>(), *m_jsonSerializationContext, ContinuationFlags::ResolvePointer);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq("null");
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoring_StorePointerToNullptrWithValueDefault_ValueStoredCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        int* ptrValue = nullptr;
        int value2 = 42;
        int* defaultPtrValue = &value2;

        ResultCode result = ContinueStoring(
            *m_jsonDocument, &ptrValue, &defaultPtrValue, azrtti_typeid<int>(), *m_jsonSerializationContext,
            ContinuationFlags::ResolvePointer);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq("null");
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoring_StorePointerToNullptrWithNullPtrDefault_NullPtrIsStored)
    {
        using namespace AZ::JsonSerializationResult;

        int* ptrValue = nullptr;
        int* defaultPtrValue = nullptr;

        ResultCode result = ContinueStoring(
            *m_jsonDocument, &ptrValue, &defaultPtrValue, azrtti_typeid<int>(), *m_jsonSerializationContext,
            ContinuationFlags::ResolvePointer);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq("null");
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoring_ReplaceDefault_ValueStoredCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        int value = 42;
        
        ResultCode result = ContinueStoring(
            *m_jsonDocument, &value, nullptr, azrtti_typeid<int>(), *m_jsonSerializationContext, ContinuationFlags::ReplaceDefault);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq("42");
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoring_ReplaceDefaultForPointer_ValueStoredCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        int value = 42;
        int* ptrValue = &value;

        ResultCode result = ContinueStoring(*m_jsonDocument, &ptrValue, nullptr, azrtti_typeid<int>(), *m_jsonSerializationContext,
            ContinuationFlags::ResolvePointer | ContinuationFlags::ReplaceDefault);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq("42");
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoring_ReplaceDefaultWithUnknownType_ProcessingWasHalted)
    {
        using namespace AZ::JsonSerializationResult;

        int value = 42;
        AZ::Uuid unknownType("{09AE3CEC-EBFC-41EC-A7F6-949721521716}");

        ResultCode result =
            ContinueStoring(*m_jsonDocument, &value, nullptr, unknownType, *m_jsonSerializationContext, ContinuationFlags::ReplaceDefault);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
    }

    //
    // LoadTypeId
    //

    TEST_F(BaseJsonSerializerTests, LoadTypeId_CallIsForwarded_ReturnsTypeIdInJson)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value value;
        value.SetString("{A14FB65D-8812-4B0D-A596-7191960B76BE}");

        AZ::Uuid typeId = AZ::Uuid::CreateNull();
        ResultCode result = LoadTypeId(typeId, value, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(AZ::Uuid("{A14FB65D-8812-4B0D-A596-7191960B76BE}"), typeId);
    }

    //
    // StoreTypeId
    // 

    TEST_F(BaseJsonSerializerTests, StoreTypeId_CallIsForwarded_ReturnsTypeIdInJson)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::Uuid typeId = azrtti_typeid<int>();
        ResultCode result = StoreTypeId(*m_jsonDocument, typeId, *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq(R"("int")");
    }

    //
    // ContinueLoadingFromJsonObjectField
    //

    TEST_F(BaseJsonSerializerTests, ContinueLoadingFromJsonObjectField_LoadIntMember_ValueLoadedCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse(R"(
            {
                "Value": 42
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        int value = 0;

        ResultCode result = ContinueLoadingFromJsonObjectField(&value, azrtti_typeid<int>(), *m_jsonDocument,
            "Value", *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(42, value);
    }

    TEST_F(BaseJsonSerializerTests, ContinueLoadingFromJsonObjectField_MemberNotFound_DefaultsUsed)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse("{}");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        int value = 0;

        ResultCode result = ContinueLoadingFromJsonObjectField(&value, azrtti_typeid<int>(), *m_jsonDocument,
            "Value", *m_jsonDeserializationContext);

        EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        EXPECT_EQ(0, value);
    }

    TEST_F(BaseJsonSerializerTests, ContinueLoadingFromJsonObjectField_InvalidJsonType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value json(rapidjson::kFalseType);
        int value = 0;

        ResultCode result = ContinueLoadingFromJsonObjectField(&value, azrtti_typeid<int>(), json,
            "Value", *m_jsonDeserializationContext);

        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(0, value);
    }

    //
    // ContinueStoringToJsonObjectField
    //

    TEST_F(BaseJsonSerializerTests, ContinueStoringToJsonObjectField_StoreIntMember_ValueStoredCorrectly)
    {
        using namespace AZ::JsonSerializationResult;

        int value = 42;

        m_jsonDocument->SetObject();
        ResultCode result = ContinueStoringToJsonObjectField(*m_jsonDocument, rapidjson::StringRef("Value"), &value, nullptr,
            azrtti_typeid<int>(), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq(R"(
            {
                "Value": 42
            })");
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoringToJsonObjectField_StoreIntMemberToNull_ValueStoredCorrectlyAsObject)
    {
        using namespace AZ::JsonSerializationResult;

        int value = 42;

        m_jsonDocument->SetNull();
        ResultCode result = ContinueStoringToJsonObjectField(*m_jsonDocument, rapidjson::StringRef("Value"), &value, nullptr,
            azrtti_typeid<int>(), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq(R"(
            {
                "Value": 42
            })");
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoringToJsonObjectField_StoreToInvalidTarget_ReportsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        int value = 42;

        m_jsonDocument->SetBool(false);
        ResultCode result = ContinueStoringToJsonObjectField(*m_jsonDocument, rapidjson::StringRef("Value"), &value, nullptr,
            azrtti_typeid<int>(), *m_jsonSerializationContext);

        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(BaseJsonSerializerTests, ContinueStoringToJsonObjectField_StoreIntMemberWithMatchingDefault_NothingStored)
    {
        using namespace AZ::JsonSerializationResult;

        int value = 42;

        m_jsonDocument->SetNull();
        ResultCode result = ContinueStoringToJsonObjectField(*m_jsonDocument, rapidjson::StringRef("Value"), &value, &value,
            azrtti_typeid<int>(), *m_jsonSerializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        Expect_DocStrEq("{}");
    }

    //
    // IsExplicitDefault
    // GetExplicitDefault
    //

    TEST_F(BaseJsonSerializerTests, ExplicitDefault_GetAndCheckDefault_ReturnedExplicitDefaultPassesExplicitDefaultTest)
    {
        rapidjson::Value value = GetExplicitDefault();
        EXPECT_TRUE(IsExplicitDefault(value));
    }
} // namespace JsonSerializationTests
