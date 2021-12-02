/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Name/NameJsonSerializer.h>
#include <AzCore/Name/NameDictionary.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    class NameJsonSerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<AZ::Name>
    {
    public:
        void SetUp() override
        {
            AZ::NameDictionary::Create();
        }

        void TearDown() override
        {
            AZ::NameDictionary::Destroy();
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            AZ::Name::Reflect(context.get());
        }

        void Reflect(AZStd::unique_ptr<AZ::JsonRegistrationContext>& context) override
        {
            AZ::Name::Reflect(context.get());
        }

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::NameJsonSerializer>();
        }

        AZStd::shared_ptr<AZ::Name> CreateDefaultInstance() override
        {
            return AZStd::make_shared<AZ::Name>();
        }

        AZStd::shared_ptr<AZ::Name> CreateFullySetInstance() override
        {
            return AZStd::make_shared<AZ::Name>("hello");
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"("hello")";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kStringType);
            features.EnableJsonType(rapidjson::kFalseType);
            features.EnableJsonType(rapidjson::kTrueType);
            features.EnableJsonType(rapidjson::kNumberType);
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
        }

        bool AreEqual(const AZ::Name& lhs, const AZ::Name& rhs) override
        {
            return lhs == rhs;
        }
    };

    using NameJsonSerializerTestTypes = ::testing::Types<NameJsonSerializerTestDescription>;
    INSTANTIATE_TYPED_TEST_CASE_P(NameJsonSerializer, JsonSerializerConformityTests, NameJsonSerializerTestTypes);

    class NameJsonSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        AZStd::unique_ptr<AZ::NameJsonSerializer> m_serializer;

        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            AZ::NameDictionary::Create();
            m_serializer = AZStd::make_unique<AZ::NameJsonSerializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            AZ::NameDictionary::Destroy();
            BaseJsonSerializerFixture::TearDown();
        }

        const char* m_testString = "Hello world";
        const char* m_testStringAlternative = "Goodbye cruel world";
    };

    TEST_F(NameJsonSerializerTests, Load_FalseBoolean_FalseAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(false);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STRCASEEQ("False", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_TrueBoolean_TrueAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(true);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STRCASEEQ("True", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseUnsignedIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetUint(42);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("42", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseUnsigned64BitIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetUint64(42);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("42", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseSignedIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetInt(-42);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("-42", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseSigned64BitIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetInt64(-42);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("-42", convertedValue.GetStringView().data());
    }

    TEST_F(NameJsonSerializerTests, Load_ParseFloatingPointValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetDouble(3.1415);
        AZ::Name convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<AZ::Name>(),
            testValue, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("3.141500", convertedValue.GetStringView().data());
    }
} // namespace JsonSerializationTests
