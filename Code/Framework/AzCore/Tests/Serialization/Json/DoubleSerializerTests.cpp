/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <limits>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Serialization/Json/DoubleSerializer.h>
#include <AzCore/Serialization/Json/CastingHelpers.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    template<typename FloatingPointType, typename Serializer>
    class DoubleSerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<FloatingPointType>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<Serializer>();
        }

        AZStd::shared_ptr<FloatingPointType> CreateDefaultInstance() override
        {
            return AZStd::make_shared<FloatingPointType>(0.0f);
        }

        AZStd::shared_ptr<FloatingPointType> CreateFullySetInstance() override
        {
            return AZStd::make_shared<FloatingPointType>(4.0f);
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "4.0";
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

        bool AreEqual(const FloatingPointType& lhs, const FloatingPointType& rhs) override
        {
            return lhs == rhs;
        }
    };

    using DoubleSerializerConformityTestTypes = ::testing::Types<
        DoubleSerializerTestDescription<double, AZ::JsonDoubleSerializer>,
        DoubleSerializerTestDescription<float, AZ::JsonFloatSerializer>
    >;
    INSTANTIATE_TYPED_TEST_CASE_P(JsonDoubleSerializer, JsonSerializerConformityTests, DoubleSerializerConformityTestTypes);

    class JsonDoubleSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        struct DoublePointerWrapper
        {
            AZ_TYPE_INFO(DoublePointerWrapper, "{C2FD9E0B-2641-4D24-A3D9-A29FD1A21A81}");

            double* m_double{ nullptr };
            float* m_float{ nullptr };

            ~DoublePointerWrapper()
            {
                azfree(m_float);
                azfree(m_double);
            }
        };

        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_floatSerializer = AZStd::make_unique<AZ::JsonFloatSerializer>();
            m_doubleSerializer = AZStd::make_unique<AZ::JsonDoubleSerializer>();
        }

        void TearDown() override
        {
            m_floatSerializer.reset();
            m_doubleSerializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        using BaseJsonSerializerFixture::RegisterAdditional;
        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& serializeContext) override
        {
            serializeContext->Class<DoublePointerWrapper>()
                ->Field("Double", &DoublePointerWrapper::m_double)
                ->Field("Float", &DoublePointerWrapper::m_float);
        }

        void TestSerializers(rapidjson::Value& testVal, double expectedValue, AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            using namespace AZ::JsonSerializationResult;

            AZ::StackedString path(AZ::StackedString::Format::JsonPointer);
            float value;
            ASSERT_EQ(Outcomes::Success, AZ::JsonNumericCast(value, expectedValue, path, m_deserializationSettings->m_reporting).GetOutcome());
            TestSerializers(testVal, expectedValue, value, expectedOutcome);
        }

        void TestSerializers(rapidjson::Value& testVal, double expectedDouble, float expectedFloat, 
            AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            using namespace AZ::JsonSerializationResult;

            // double
            double testDouble = DEFAULT_DOUBLE;
            ResultCode doubleResult = m_doubleSerializer->Load(&testDouble, azrtti_typeid<double>(), testVal, *m_jsonDeserializationContext);
            EXPECT_EQ(expectedOutcome, doubleResult.GetOutcome());
            EXPECT_DOUBLE_EQ(testDouble, expectedDouble);

            // float
            float testFloat = DEFAULT_FLOAT;
            ResultCode floatResult = m_floatSerializer->Load(&testFloat, azrtti_typeid <float>(), testVal, *m_jsonDeserializationContext);
            EXPECT_EQ(expectedOutcome, floatResult.GetOutcome());
            EXPECT_FLOAT_EQ(testFloat, expectedFloat);
        }

        void TestString(const char* testString, double expectedDouble, float expectedFloat, 
            AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            TestSerializers(rapidjson::Value().SetString(rapidjson::StringRef(testString)), expectedDouble, expectedFloat, expectedOutcome);
        }

        void TestInt(int64_t testInt, [[maybe_unused]] double expectedDouble, float expectedFloat, AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            TestSerializers(rapidjson::Value().SetInt64(testInt), expectedFloat, expectedOutcome);
        }

        void TestUint(uint64_t testInt, [[maybe_unused]] double expectedDouble, float expectedFloat, AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            TestSerializers(rapidjson::Value().SetUint64(testInt), expectedFloat, expectedOutcome);
        }

        void TestDouble(double testDouble, double expectedValue)
        {
            using namespace AZ::JsonSerializationResult;

            rapidjson::Value testVal;
            testVal.SetDouble(testDouble);
            EXPECT_EQ(testVal.GetDouble(), testDouble);

            TestSerializers(testVal, expectedValue, Outcomes::Success);
        }

        void TestBool(bool testBool, double expectedDouble, float expectedFloat, AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            TestSerializers(rapidjson::Value().SetBool(testBool), expectedDouble, expectedFloat, expectedOutcome);
        }

    protected:
        AZStd::unique_ptr<AZ::JsonFloatSerializer> m_floatSerializer;
        AZStd::unique_ptr<AZ::JsonDoubleSerializer> m_doubleSerializer;

        constexpr static float DEFAULT_FLOAT = 1.23456789f;
        constexpr static double DEFAULT_DOUBLE = -9.87654321;
    };

    // Strings

    TEST_F(JsonDoubleSerializerTests, Load_StringWithDouble_SuccessAndExpectedValueLoaded)
    {
        TestString("12.5", 12.5, 12.5f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_StringWithFloat_SuccessAndExpectedValueLoaded)
    {
        // The f isn't processed as the interpretation is a double.
        TestString("12.5f", 12.5, 12.5f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_StringWithInteger_SuccessAndExpectedValueLoaded)
    {
        TestString("42", 42.0, 42.0f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_StringWithNegativeNumber_SuccessAndExpectedValueLoaded)
    {
        TestString("-12.5", -12.5, -12.5f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_StringWithZero_SuccessAndExpectedValueLoaded)
    {
        TestString("0", 0.0, 0.0f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_StringWithAdditionalText_SuccessAndExpectedValueLoaded)
    {
        TestString("2.4 hello", 2.4, 2.4f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_StringWithMultipleNumbers_SuccessAndFirstNumberLoaded)
    {
        TestString("2.4 42.0", 2.4, 2.4f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_EmptyString_UnsupportedAndValueNotTouched)
    {
        TestString("", DEFAULT_DOUBLE, DEFAULT_FLOAT, AZ::JsonSerializationResult::Outcomes::Unsupported);
    }

    TEST_F(JsonDoubleSerializerTests, Load_StringWithNoNumericValues_UnsupportedAndValueNotTouched)
    {
        TestString("hello", DEFAULT_DOUBLE, DEFAULT_FLOAT, AZ::JsonSerializationResult::Outcomes::Unsupported);
    }

    TEST_F(JsonDoubleSerializerTests, Load_StringDoesNotStartWithNumericValues_UnsupportedAndValueNotTouched)
    {
        TestString("hello 2.4", DEFAULT_DOUBLE, DEFAULT_FLOAT, AZ::JsonSerializationResult::Outcomes::Unsupported);
    }

    TEST_F(JsonDoubleSerializerTests, Load_StringWithNumberThatIsTooBig_UnsupportedAndValueNotTouched)
    {
        constexpr char bigNumberText[] =
            "99999999999999999999999999999999999999999999999999"
            "99999999999999999999999999999999999999999999999999"
            "99999999999999999999999999999999999999999999999999"
            "99999999999999999999999999999999999999999999999999"
            "99999999999999999999999999999999999999999999999999"
            "99999999999999999999999999999999999999999999999999"
            "99999999999999999999999999999999999999999999999999"
            "99999999999999999999999999999999999999999999999999"
            "99999999999999999999999999999999999999999999999999"
            "99999999999999999999999999999999999999999999999999.0";
        TestString(bigNumberText, DEFAULT_DOUBLE, DEFAULT_FLOAT, AZ::JsonSerializationResult::Outcomes::Unsupported);
    }

    // Booleans

    TEST_F(JsonDoubleSerializerTests, Load_BooleanWithTrue_SuccessAndValueIsOne)
    {
        TestBool(true, 1.0, 1.0f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_BooleanWithFalse_SuccessAndValueIsZero)
    {
        TestBool(false, 0.0, 0.0f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    // Integers

    TEST_F(JsonDoubleSerializerTests, Load_IntegerWithPositiveValue_SuccessAndExpectedValue)
    {
        TestInt(34, 34.0, 34.0f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_IntegerWithZero_SuccessAndExpectedValue)
    {
        TestInt(0, 0.0, 0.0f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_IntegerNegativeValue_SuccessAndExpectedValue)
    {
        TestInt(-1, -1.0, -1.0f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_UnsignedIntegerWithPositiveValue_SuccessAndExpectedValue)
    {
        TestUint(34, 34.0, 34.0f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonDoubleSerializerTests, Load_UnsignedIntegerWithZero_SuccessAndExpectedValue)
    {
        TestUint(0, 0.0, 0.0f, AZ::JsonSerializationResult::Outcomes::Success);
    }

    // Floating point

    TEST_F(JsonDoubleSerializerTests, Load_DoubleValueTooBigToFitInFloat_UnsupportedAndValueNotTouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testVal;
        testVal.SetDouble(std::numeric_limits<double>::max());
        
        float value = 42.0f;
        ResultCode result = m_floatSerializer->Load(&value, azrtti_typeid<float>(), testVal, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(42.0f, value);
    }
} // namespace JsonSerializationTests
