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

#include <AzCore/Serialization/Json/BoolSerializer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    class BoolSerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<bool>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonBoolSerializer>();
        }

        AZStd::shared_ptr<bool> CreateDefaultInstance() override
        {
            return AZStd::make_shared<bool>(false);
        }

        AZStd::shared_ptr<bool> CreateFullySetInstance() override
        {
            return AZStd::make_shared<bool>(true);
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "true";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kFalseType);
            features.EnableJsonType(rapidjson::kTrueType);
            features.EnableJsonType(rapidjson::kStringType);
            features.EnableJsonType(rapidjson::kNumberType);
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
        }

        bool AreEqual(const bool& lhs, const bool& rhs) override
        {
            return lhs == rhs;
        }
    };

    using BoolSerializerConformityTestTypes = ::testing::Types<BoolSerializerTestDescription>;
    INSTANTIATE_TYPED_TEST_CASE_P(JsonBoolSerializer, JsonSerializerConformityTests, BoolSerializerConformityTestTypes);

    class JsonBoolSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_boolSerializer = AZStd::make_unique<AZ::JsonBoolSerializer>();
        }

        void TearDown() override
        {
            m_boolSerializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        void Load(rapidjson::Value& testVal, bool expectedBool, AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            using namespace AZ::JsonSerializationResult;

            bool testBool = !expectedBool;
            ResultCode result = m_boolSerializer->Load(&testBool, azrtti_typeid<bool>(), testVal, *m_jsonDeserializationContext);
            EXPECT_EQ(expectedOutcome, result.GetOutcome());
            if (expectedOutcome == AZ::JsonSerializationResult::Outcomes::Success)
            {
                EXPECT_EQ(testBool, expectedBool);
            }
            else
            {
                EXPECT_NE(testBool, expectedBool);
            }
        }

        void LoadInt(int64_t loadInt, bool expectedBool, 
            AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            rapidjson::Value testVal;
            testVal.SetInt64(loadInt);
            Load(testVal, expectedBool, expectedOutcome);
        }

        void LoadDouble(double loadDouble, bool expectedBool, AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            rapidjson::Value testVal;
            testVal.SetDouble(loadDouble);
            Load(testVal, expectedBool, expectedOutcome);
        }

    protected:
        rapidjson::Value m_jsonValue;
        AZStd::unique_ptr<AZ::JsonBoolSerializer> m_boolSerializer;
    };

    TEST_F(JsonBoolSerializerTests, Load_JsonBooleanValue_ReturnsSuccess)
    {
        // True value already tested in conformity tests.
        Load(m_jsonValue.SetBool(false), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithFalse_ReturnsSuccessAndValueIsFalse)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("false")), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithFalseWithCapitalLetters_ReturnsSuccessAndValueIsFalse)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("fALse")), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithZeroInteger_ReturnsSuccessAndValueIsFalse)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("0")), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithZeroIntegerAndExtraLetters_ReturnsSuccessAndValueIsFalse)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("0eee")), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithZeroDouble_ReturnsSuccessAndValueIsFalse)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("0.0")), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithZeroFloat_ReturnsSuccessAndValueIsFalse)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("0.0f")), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithTrue_ReturnsSuccessAndValueIsTrue)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("true")), true, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithTrueWithCapitalLetters_ReturnsSuccessAndValueIsTrue)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("tRuE")), true, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithIntegerOne_ReturnsSuccessAndValueIsTrue)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("1")), true, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithDoubleOne_ReturnsSuccessAndValueIsTrue)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("1.0")), true, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithFloatOne_ReturnsSuccessAndValueIsTrue)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("1.0f")), true, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_StringWithNegativeValue_ReturnsSuccessAndValueIsTrue)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("-1.0f")), true, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_EmptyString_ReturnsUnsupportedAndValueIsUnchanged)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("")), false, AZ::JsonSerializationResult::Outcomes::Unsupported);
    }

    TEST_F(JsonBoolSerializerTests, Load_NonNumberStrings_ReturnsUnsupportedAndValueIsUnchanged)
    {
        Load(m_jsonValue.SetString(rapidjson::StringRef("Invalid")), false, AZ::JsonSerializationResult::Outcomes::Unsupported);
        Load(m_jsonValue.SetString(rapidjson::StringRef("   ")), false, AZ::JsonSerializationResult::Outcomes::Unsupported);
        Load(m_jsonValue.SetString(rapidjson::StringRef("ee0")), false, AZ::JsonSerializationResult::Outcomes::Unsupported);
    }

    TEST_F(JsonBoolSerializerTests, Load_SignedIntegerZero_ReturnsSuccessAndValueFalse)
    {
        Load(m_jsonValue.SetInt64(0), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_UnsignedIntegerZero_ReturnsSuccessAndValueFalse)
    {
        Load(m_jsonValue.SetUint64(0), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_SignedIntegerOne_ReturnsSuccessAndValueTrue)
    {
        Load(m_jsonValue.SetInt64(1), true, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_UnsignedIntegerOne_ReturnsSuccessAndValueTrue)
    {
        Load(m_jsonValue.SetUint64(1), true, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_AnyInteger_ReturnsSuccessAndValueTrue)
    {
        Load(m_jsonValue.SetInt64(-1), true, AZ::JsonSerializationResult::Outcomes::Success);
        Load(m_jsonValue.SetUint64(100), true, AZ::JsonSerializationResult::Outcomes::Success);
        Load(m_jsonValue.SetInt64(-100), true, AZ::JsonSerializationResult::Outcomes::Success);
        Load(m_jsonValue.SetInt64(2), true, AZ::JsonSerializationResult::Outcomes::Success);
        Load(m_jsonValue.SetUint64(2), true, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_FloatingPointZero_ReturnsSuccessAndValueFalse)
    {
        Load(m_jsonValue.SetDouble(0.0), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_NegativeFloatingPointZero_ReturnsSuccessAndValueFalse)
    {
        Load(m_jsonValue.SetDouble(-0.0), false, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_FloatingPointOne_ReturnsSuccessAndValueTrue)
    {
        Load(m_jsonValue.SetDouble(1.0), true, AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonBoolSerializerTests, Load_AnyFloatingPoint_ReturnsSuccessAndValueTrue)
    {
        Load(m_jsonValue.SetDouble(1.00001), true, AZ::JsonSerializationResult::Outcomes::Success);
        Load(m_jsonValue.SetDouble(-0.1), true, AZ::JsonSerializationResult::Outcomes::Success);
        Load(m_jsonValue.SetDouble(0.0001), true, AZ::JsonSerializationResult::Outcomes::Success);
        Load(m_jsonValue.SetDouble(-1.0f), true, AZ::JsonSerializationResult::Outcomes::Success);
        Load(m_jsonValue.SetDouble(2.0), true, AZ::JsonSerializationResult::Outcomes::Success);
    }
} // namespace JsonSerializationTests
