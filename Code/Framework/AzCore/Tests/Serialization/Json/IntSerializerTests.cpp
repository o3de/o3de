/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <limits>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Serialization/Json/IntSerializer.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <AzCore/std/string/conversions.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

#include <AzCore/Serialization/Json/CastingHelpers.h>

namespace JsonSerializationTests
{
    template<typename NumberType, typename Serializer>
    class IntegerSerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<NumberType>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<Serializer>();
        }

        AZStd::shared_ptr<NumberType> CreateDefaultInstance() override
        {
            return AZStd::make_shared<NumberType>(0);
        }

        AZStd::shared_ptr<NumberType> CreateFullySetInstance() override
        {
            return AZStd::make_shared<NumberType>(4);
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "4";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kStringType);
            features.EnableJsonType(rapidjson::kFalseType);
            features.EnableJsonType(rapidjson::kTrueType);
            features.EnableJsonType(rapidjson::kNumberType);
            features.m_supportsInjection = false;
            features.m_supportsPartialInitialization = false;
        }

        bool AreEqual(const NumberType& lhs, const NumberType& rhs) override
        {
            return lhs == rhs;
        }
    };

    using IntegerSerializerConformityTestTypes = ::testing::Types<
        IntegerSerializerTestDescription<char, AZ::JsonCharSerializer>,
        IntegerSerializerTestDescription<short, AZ::JsonShortSerializer>,
        IntegerSerializerTestDescription<int, AZ::JsonIntSerializer>,
        IntegerSerializerTestDescription<long, AZ::JsonLongSerializer>,
        IntegerSerializerTestDescription<long long, AZ::JsonLongLongSerializer>,
        IntegerSerializerTestDescription<unsigned char, AZ::JsonUnsignedCharSerializer>,
        IntegerSerializerTestDescription<unsigned short, AZ::JsonUnsignedShortSerializer>,
        IntegerSerializerTestDescription<unsigned int, AZ::JsonUnsignedIntSerializer>,
        IntegerSerializerTestDescription<unsigned long, AZ::JsonUnsignedLongSerializer>,
        IntegerSerializerTestDescription<unsigned long long, AZ::JsonUnsignedLongLongSerializer>
    >;
    INSTANTIATE_TYPED_TEST_CASE_P(JsonIntSerializer, JsonSerializerConformityTests, IntegerSerializerConformityTestTypes);


    template<typename> struct SerializerInfo {};

    template<> struct SerializerInfo<AZ::JsonCharSerializer>
    {
        using DataType = char;
        static DataType GetValue(rapidjson::Value& value) { return static_cast<DataType>(value.GetInt()); }
    };

    template<> struct SerializerInfo<AZ::JsonShortSerializer>
    {
        using DataType = short;
        static DataType GetValue(rapidjson::Value& value) { return static_cast<DataType>(value.GetInt()); }
    };

    template<> struct SerializerInfo<AZ::JsonIntSerializer>
    {
        using DataType = int;
        static DataType GetValue(rapidjson::Value& value) { return static_cast<DataType>(value.GetInt()); }
    };

    template<> struct SerializerInfo<AZ::JsonLongSerializer>
    {
        using DataType = long;
        static DataType GetValue(rapidjson::Value& value) { return static_cast<DataType>(value.GetInt64()); }
    };

    template<> struct SerializerInfo<AZ::JsonLongLongSerializer>
    {
        using DataType = long long;
        static DataType GetValue(rapidjson::Value& value) { return static_cast<DataType>(value.GetInt64()); }
    };

    template<> struct SerializerInfo<AZ::JsonUnsignedCharSerializer>
    {
        using DataType = unsigned char;
        static DataType GetValue(rapidjson::Value& value) { return static_cast<DataType>(value.GetUint64()); }
    };

    template<> struct SerializerInfo<AZ::JsonUnsignedShortSerializer>
    {
        using DataType = unsigned short;
        static DataType GetValue(rapidjson::Value& value) { return static_cast<DataType>(value.GetUint64()); }
    };

    template<> struct SerializerInfo<AZ::JsonUnsignedIntSerializer>
    {
        using DataType = unsigned int;
        static DataType GetValue(rapidjson::Value& value) { return static_cast<DataType>(value.GetUint64()); }
    };

    template<> struct SerializerInfo<AZ::JsonUnsignedLongSerializer>
    {
        using DataType = unsigned long;
        static DataType GetValue(rapidjson::Value& value) { return static_cast<DataType>(value.GetUint64()); }
    };

    template<> struct SerializerInfo<AZ::JsonUnsignedLongLongSerializer>
    {
        using DataType = unsigned long long;
        static DataType GetValue(rapidjson::Value& value) { return static_cast<DataType>(value.GetUint64()); }
    };

    template<typename SerializerType>
    class TypedJsonIntSerializerTests
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

        template<typename T, typename AZStd::enable_if_t<AZStd::is_floating_point<T>::value, int> = 0>
        void SetValue(rapidjson::Value& out, T in)
        {
            out.SetDouble(in);
        }

        template<typename T, typename
            AZStd::enable_if_t<AZStd::is_signed<T>::value && !AZStd::is_floating_point<T>::value, int> = 0>
            void SetValue(rapidjson::Value& out, T in)
        {
            out.SetInt64(in);
        }

        template<typename T, typename AZStd::enable_if_t<AZStd::is_unsigned<T>::value, int> = 0>
        void SetValue(rapidjson::Value& out, T in)
        {
            out.SetUint64(in);
        }

        template<typename TestType>
        void TestMaxValue()
        {
            using namespace AZ::JsonSerializationResult;

            using CommonArithmeticType = AZStd::common_type_t<typename SerializerInfo<SerializerType>::DataType, TestType>;

            constexpr TestType maxValue = std::numeric_limits<TestType>::max();
            rapidjson::Value testValue;
            SetValue(testValue, maxValue);
            typename SerializerInfo<SerializerType>::DataType convertedValue{};
            ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<SerializerType>::DataType>(),
                testValue, *m_jsonDeserializationContext);
            if (aznumeric_cast<CommonArithmeticType>(std::numeric_limits<typename SerializerInfo<SerializerType>::DataType>::max()) >= maxValue)
            {
                EXPECT_EQ(Outcomes::Success, result.GetOutcome());
                EXPECT_EQ(maxValue, convertedValue);
            }
            else
            {
                EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                EXPECT_EQ(typename SerializerInfo<SerializerType>::DataType(), convertedValue);
            }
        }

        template<typename TestType>
        void TestMaxStringValue()
        {
            using namespace AZ::JsonSerializationResult;

            using CommonArithmeticType = AZStd::common_type_t<typename SerializerInfo<SerializerType>::DataType, TestType>;

            constexpr TestType maxValue = std::numeric_limits<TestType>::max();
            AZStd::string maxValueAsString = AZStd::to_string(maxValue);

            rapidjson::Value testValue;
            testValue.SetString(rapidjson::StringRef(maxValueAsString.c_str()));

            typename SerializerInfo<SerializerType>::DataType convertedValue{};
            ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<SerializerType>::DataType>(),
                testValue, *m_jsonDeserializationContext);
            if (aznumeric_cast<CommonArithmeticType>(std::numeric_limits<typename SerializerInfo<SerializerType>::DataType>::max()) >= maxValue)
            {
                EXPECT_EQ(Outcomes::Success, result.GetOutcome());
                EXPECT_EQ(maxValue, convertedValue);
            }
            else
            {
                EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                EXPECT_EQ(typename SerializerInfo<SerializerType>::DataType(), convertedValue);
            }
        }

        template<typename TestType>
        void TestMinValue()
        {
            using namespace AZ::JsonSerializationResult;

            static_assert(AZStd::is_signed<TestType>::value || AZStd::is_floating_point<TestType>::value, "TestMinValue is only setup to test signed values.");

            constexpr TestType minValue = std::numeric_limits<TestType>::lowest();
            rapidjson::Value testValue;
            SetValue(testValue, minValue);
            typename SerializerInfo<SerializerType>::DataType convertedValue{};
            ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<SerializerType>::DataType>(),
                testValue, *m_jsonDeserializationContext);

            bool expectParseSuccess = AZStd::is_signed<typename SerializerInfo<SerializerType>::DataType>::value && std::numeric_limits<typename SerializerInfo<SerializerType>::DataType>::lowest() <= minValue;
            if (expectParseSuccess)
            {
                EXPECT_EQ(Outcomes::Success, result.GetOutcome());
                EXPECT_EQ(minValue, convertedValue);
            }
            else
            {
                EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                EXPECT_EQ(typename SerializerInfo<SerializerType>::DataType(), convertedValue);
            }
        }

        template <typename TestType>
        void TestMinStringValue()
        {
            using namespace AZ::JsonSerializationResult;

            static_assert(AZStd::is_signed<TestType>::value || AZStd::is_floating_point<TestType>::value, "TestMinValue is only setup to test signed values.");

            constexpr TestType minValue = std::numeric_limits<TestType>::lowest();
            AZStd::string minValueAsString = AZStd::to_string(minValue);

            rapidjson::Value testValue;
            testValue.SetString(rapidjson::StringRef(minValueAsString.c_str()));

            typename SerializerInfo<SerializerType>::DataType convertedValue{};
            ResultCode result = m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<SerializerType>::DataType>(),
                testValue, *m_jsonDeserializationContext);

            constexpr bool expectParseSuccess =
                AZStd::is_signed<typename SerializerInfo<SerializerType>::DataType>::value &&
                std::numeric_limits<typename SerializerInfo<SerializerType>::DataType>::lowest() <= minValue;
            if constexpr(expectParseSuccess)
            {
                EXPECT_EQ(Outcomes::Success, result.GetOutcome());
                EXPECT_EQ(minValue, convertedValue);
            }
            else
            {
                EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
                EXPECT_EQ(typename SerializerInfo<SerializerType>::DataType(), convertedValue);
            }
        }
    };

    using IntSerializationTypes = ::testing::Types<
        AZ::JsonCharSerializer,
        AZ::JsonShortSerializer,
        AZ::JsonIntSerializer,
        AZ::JsonLongSerializer,
        AZ::JsonLongLongSerializer,

        AZ::JsonUnsignedCharSerializer,
        AZ::JsonUnsignedShortSerializer,
        AZ::JsonUnsignedIntSerializer,
        AZ::JsonUnsignedLongSerializer,
        AZ::JsonUnsignedLongLongSerializer >;
    TYPED_TEST_CASE(TypedJsonIntSerializerTests, IntSerializationTypes);

    TYPED_TEST(TypedJsonIntSerializerTests, Load_FalseBoolean_ValueIsZero)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(false);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_TrueBoolean_ValueIsOne)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetBool(true);
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(1, convertedValue);
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_ValidIntegerString_StringIsParsed)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString("34");
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(34, convertedValue);
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_ValidNegativeIntegerString_StringIsParsed)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString("-34");
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        if (AZStd::is_signed<typename SerializerInfo<TypeParam>::DataType>::value)
        {
            EXPECT_EQ(Outcomes::Success, result.GetOutcome());
            EXPECT_EQ(-34, convertedValue);
        }
        else
        {
            EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
            EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
        }
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_ValidIntegerStringWithExtraText_StringIsParsed)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString("34 Hello");
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(34, convertedValue);
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_ValidNegativeIntegerStringWithExtraText_StringIsParsed)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString("-34 Hello");
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        if (AZStd::is_signed<typename SerializerInfo<TypeParam>::DataType>::value)
        {
            EXPECT_EQ(Outcomes::Success, result.GetOutcome());
            EXPECT_EQ(-34, convertedValue);
        }
        else
        {
            EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
            EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
        }
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_ValidDoubleString_StringIsParsed)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString("12.6");
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(12, convertedValue);
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_ValidNegativeDoubleString_StringIsParsed)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString("-12.6");
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        if (AZStd::is_signed<typename SerializerInfo<TypeParam>::DataType>::value)
        {
            EXPECT_EQ(Outcomes::Success, result.GetOutcome());
            EXPECT_EQ(-12, convertedValue);
        }
        else
        {
            EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
            EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
        }
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_Invalid_UnsupportedConversionReported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString("hello 2.4");
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_PartialNumberString_UnsupportedConversionReported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString(".5");
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_PartialNegativeNumberString_UnsupportedConversionReported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString("-.5");
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_EmptyString_UnsupportedConversionReported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testValue;
        testValue.SetString("");
        typename SerializerInfo<TypeParam>::DataType convertedValue{};
        ResultCode result = this->m_serializer->Load(&convertedValue, azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(),
            testValue, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(typename SerializerInfo<TypeParam>::DataType(), convertedValue);
    }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxInt8Value_ConvertIfFitsOrUnsupported)     { this->template TestMaxValue<int8_t>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxShortValue_ConvertIfFitsOrUnsupported)    { this->template TestMaxValue<short>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxIntValue_ConvertIfFitsOrUnsupported)      { this->template TestMaxValue<int>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxLongValue_ConvertIfFitsOrUnsupported)     { this->template TestMaxValue<long>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxLongLongValue_ConvertIfFitsOrUnsupported) { this->template TestMaxValue<long long>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxFloatValue_ConvertIfFitsOrUnsupported)    { this->template TestMaxValue<float>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxDoubleValue_ConvertIfFitsOrUnsupported)   { this->template TestMaxValue<double>(); }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinInt8Value_ConvertIfFitsOrUnsupported)     { this->template TestMinValue<int8_t>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinShortValue_ConvertIfFitsOrUnsupported)    { this->template TestMinValue<short>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinIntValue_ConvertIfFitsOrUnsupported)      { this->template TestMinValue<int>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinLongValue_ConvertIfFitsOrUnsupported)     { this->template TestMinValue<long>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinLongLongValue_ConvertIfFitsOrUnsupported) { this->template TestMinValue<long long>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinFloatValue_ConvertIfFitsOrUnsupported)    { this->template TestMinValue<float>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinDoubleValue_ConvertIfFitsOrUnsupported)   { this->template TestMinValue<double>(); }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxUnsignedInt8Value_ConvertIfFitsOrUnsupported)     { this->template TestMaxValue<uint8_t>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxUnsignedShortValue_ConvertIfFitsOrUnsupported)    { this->template TestMaxValue<unsigned short>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxUnsignedIntValue_ConvertIfFitsOrUnsupported)      { this->template TestMaxValue<unsigned int>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxUnsignedLongValue_ConvertIfFitsOrUnsupported)     { this->template TestMaxValue<unsigned long>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxUnsignedLongLongValue_ConvertIfFitsOrUnsupported) { this->template TestMaxValue<unsigned long long>(); }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxInt8StringValue_ConvertIfFitsOrUnsupported)     { this->template TestMaxStringValue<int8_t>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxShortStringValue_ConvertIfFitsOrUnsupported)    { this->template TestMaxStringValue<short>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxIntStringValue_ConvertIfFitsOrUnsupported)      { this->template TestMaxStringValue<int>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxLongStringValue_ConvertIfFitsOrUnsupported)     { this->template TestMaxStringValue<long>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxLongLongStringValue_ConvertIfFitsOrUnsupported) { this->template TestMaxStringValue<long long>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxFloatStringValue_ConvertIfFitsOrUnsupported)    { this->template TestMaxStringValue<float>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxDoubleStringValue_ConvertIfFitsOrUnsupported)   { this->template TestMaxStringValue<double>(); }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinInt8StringValue_ConvertIfFitsOrUnsupported)     { this->template TestMinStringValue<int8_t>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinShortStringValue_ConvertIfFitsOrUnsupported)    { this->template TestMinStringValue<short>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinIntStringValue_ConvertIfFitsOrUnsupported)      { this->template TestMinStringValue<int>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinLongStringValue_ConvertIfFitsOrUnsupported)     { this->template TestMinStringValue<long>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinLongLongStringValue_ConvertIfFitsOrUnsupported) { this->template TestMinStringValue<long long>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinFloatStringValue_ConvertIfFitsOrUnsupported)    { this->template TestMinStringValue<float>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MinDoubleStringValue_ConvertIfFitsOrUnsupported)   { this->template TestMinStringValue<double>(); }

    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxUnsignedInt8StringValue_ConvertIfFitsOrUnsupported)     { this->template TestMaxStringValue<uint8_t>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxUnsignedShortStringValue_ConvertIfFitsOrUnsupported)    { this->template TestMaxStringValue<unsigned short>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxUnsignedIntStringValue_ConvertIfFitsOrUnsupported)      { this->template TestMaxStringValue<unsigned int>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxUnsignedLongStringValue_ConvertIfFitsOrUnsupported)     { this->template TestMaxStringValue<unsigned long>(); }
    TYPED_TEST(TypedJsonIntSerializerTests, Load_MaxUnsignedLongLongStringValue_ConvertIfFitsOrUnsupported) { this->template TestMaxStringValue<unsigned long long>(); }

    TYPED_TEST(TypedJsonIntSerializerTests, Store_CheckStorageType_Uint64ForUnsignedAndInt64ForSigned)
    {
        using namespace AZ::JsonSerializationResult;

        typename SerializerInfo<TypeParam>::DataType value;
        if constexpr (AZStd::is_signed<typename SerializerInfo<TypeParam>::DataType>::value)
        {
            value = -1;
        }
        else
        {
            value = 1;
        }
        rapidjson::Value convertedValue(rapidjson::kObjectType);

        ResultCode result = this->m_serializer->Store(convertedValue, &value, nullptr,
            azrtti_typeid<typename SerializerInfo<TypeParam>::DataType>(), *this->m_jsonSerializationContext);

        if constexpr (AZStd::is_signed<typename SerializerInfo<TypeParam>::DataType>::value)
        {
            EXPECT_TRUE(convertedValue.IsInt64());
            EXPECT_FALSE(convertedValue.IsUint64());
        }
        else
        {
            EXPECT_TRUE(convertedValue.IsUint64());
            EXPECT_TRUE(convertedValue.IsInt64()); // For positive numbers RapidJSON will mark a numeric value as both signed and unsigned.
        }
    }
} // namespace JsonSerializationTests
