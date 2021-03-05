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

#include <AzCore/Math/UuidSerializer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    class UuidSerializerTestDescription
        : public JsonSerializerConformityTestDescriptor<AZ::Uuid>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonUuidSerializer>();
        }

        AZStd::shared_ptr<AZ::Uuid> CreateDefaultInstance() override
        {
            return AZStd::make_shared<AZ::Uuid>(AZ::Uuid::CreateNull());
        }

        AZStd::shared_ptr<AZ::Uuid> CreateFullySetInstance() override
        {
            return AZStd::make_shared<AZ::Uuid>("{5E970016-0039-4454-8E0D-5EE58A390324}");
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"("{5E970016-0039-4454-8E0D-5EE58A390324}")";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kStringType);
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
        }

        bool AreEqual(const AZ::Uuid& lhs, const AZ::Uuid& rhs) override
        {
            return lhs == rhs;
        }
    };

    using UuidSerializerConformityTestTypes = ::testing::Types<UuidSerializerTestDescription>;
    INSTANTIATE_TYPED_TEST_CASE_P(JsonUuidSerializer, JsonSerializerConformityTests, UuidSerializerConformityTestTypes);

    class JsonUuidSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<AZ::JsonUuidSerializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::JsonUuidSerializer> m_serializer;
        AZ::Uuid m_testValue = AZ::Uuid("{8651B6DA-9792-407C-A46D-A6E2E39CA900}");
        AZ::Uuid m_testValueAlternative = AZ::Uuid("{01A6F1D2-BFCE-431A-B33A-D1C29672EDBC}");
    };

    TEST_F(JsonUuidSerializerTests, Load_ParseStringValueFormatWithDashes_ValidUuidReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(m_testValue.ToString<AZStd::string>(true, true).c_str(), m_jsonDocument->GetAllocator());
        AZ::Uuid convertedValue = AZ::Uuid::CreateNull();
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseStringValueFormatWithoutDashes_ValidUuidReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(m_testValue.ToString<AZStd::string>(true, false).c_str(), m_jsonDocument->GetAllocator());
        AZ::Uuid convertedValue = AZ::Uuid::CreateNull();
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseStringValueWithExtraCharacters_ValidUuidReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        AZStd::string uuidString = m_testValue.ToString<AZStd::string>();
        uuidString += " hello world";
        testValue.SetString(uuidString.c_str(), m_jsonDocument->GetAllocator());
        AZ::Uuid convertedValue = AZ::Uuid::CreateNull();
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseStringValueWithEmbeddedUuid_ValidUuidReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        AZStd::string uuidString = "Hello";
        uuidString += m_testValue.ToString<AZStd::string>();
        uuidString += "world";
        testValue.SetString(uuidString.c_str(), m_jsonDocument->GetAllocator());
        AZ::Uuid convertedValue = AZ::Uuid::CreateNull();
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseInvalidStringValue_ErrorReturnedAndUuidNotChanged)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(rapidjson::StringRef("Hello world"));
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseInvalidUuid_ErrorReturnedAndUuidNotChanged)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(rapidjson::StringRef("{8651B6DA-9792-407C-A46D-AXXX39CA900}"));
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseWithMissingPart_ErrorReturnedAndUuidNotChanged)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(rapidjson::StringRef("{51CD2896-A963-4CA1-AE9039AEBDA72364}"));
        AZ::Uuid convertedValue = m_testValue;
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }

    TEST_F(JsonUuidSerializerTests, Load_ParseStringWithMultipleUuids_FirstValidUuidReturned)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        AZStd::string uuidString = m_testValue.ToString<AZStd::string>();
        uuidString += m_testValueAlternative.ToString<AZStd::string>();
        testValue.SetString(uuidString.c_str(), m_jsonDocument->GetAllocator());
        AZ::Uuid convertedValue = AZ::Uuid::CreateNull();
        ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Uuid>(), testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(m_testValue, convertedValue);
    }
} // namespace JsonSerializationTests
