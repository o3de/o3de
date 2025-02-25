/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Serialization/Json/StringSerializer.h>
#include <AzCore/Serialization/Locale.h>

#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    template<typename String, typename Serializer>
    class StringTestDescription :
        public JsonSerializerConformityTestDescriptor<String>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<Serializer>();
        }

        AZStd::shared_ptr<String> CreateDefaultInstance() override
        {
            return AZStd::make_shared<String>();
        }

        AZStd::shared_ptr<String> CreateFullySetInstance() override
        {
            return AZStd::make_shared<String>("Hello");
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"("Hello")";
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

        bool AreEqual(const String& lhs, const String& rhs) override
        {
            return lhs.compare(rhs) == 0;
        }
    };

    using StringConformityTestTypes = ::testing::Types<
        StringTestDescription<AZStd::string, AZ::JsonStringSerializer>,
        StringTestDescription<AZ::OSString, AZ::JsonOSStringSerializer>
    >;
    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_CASE_P(String, JsonSerializerConformityTests, StringConformityTestTypes));

    template<typename> struct SerializerInfo {};

    template<> struct SerializerInfo<AZ::JsonStringSerializer>
    {
        using DataType = AZStd::string;
    };

    template<> struct SerializerInfo<AZ::JsonOSStringSerializer>
    {
        using DataType = AZ::OSString;
    };

    template<typename SerializerType>
    class TypedJsonStringSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        AZStd::unique_ptr<SerializerType> m_serializer;

        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<SerializerType>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        const char* m_testString = "Hello world";
        const char* m_testStringAlternative = "Goodbye cruel world";
    };

    using StringSerializationTypes = ::testing::Types<
        AZ::JsonStringSerializer,
        AZ::JsonOSStringSerializer >;
    TYPED_TEST_CASE(TypedJsonStringSerializerTests, StringSerializationTypes);

    TYPED_TEST(TypedJsonStringSerializerTests, Load_FalseBoolean_FalseAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(false);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STRCASEEQ("False", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_TrueBoolean_TrueAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(true);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STRCASEEQ("True", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseUnsignedIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetUint(42);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("42", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseUnsigned64BitIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetUint64(42);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("42", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseSignedIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetInt(-42);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("-42", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseSigned64BitIntegerValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetInt64(-42);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("-42", convertedValue.c_str());
    }

    TYPED_TEST(TypedJsonStringSerializerTests, Load_ParseFloatingPointValue_NumberReturnedAsString)
    {
        using namespace AZ::JsonSerializationResult;

        AZ::Locale::ScopedSerializationLocale scopedLocale; // For the purposes of test stability, assume the culture-invariant locale.

        rapidjson::Value testValue;
        testValue.SetDouble(3.1415);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_STREQ("3.141500", convertedValue.c_str());
    }
} // namespace JsonSerializationTests
