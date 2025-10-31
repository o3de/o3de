/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <initializer_list>
#include <AzCore/JSON/pointer.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/ColorSerializer.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
namespace JsonSerializationTests
{
    class ColorSerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<AZ::Color>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonColorSerializer>();
        }

        AZStd::shared_ptr<AZ::Color> CreateDefaultInstance() override
        {
            return AZStd::make_shared<AZ::Color>(AZ::Color::CreateZero());
        }

        AZStd::shared_ptr<AZ::Color> CreatePartialDefaultInstance() override
        {
            return AZStd::make_shared<AZ::Color>(0.5f, 0.5f, 0.5f, 0.0f);
        }

        AZStd::shared_ptr<AZ::Color> CreateFullySetInstance() override
        {
            return AZStd::make_shared<AZ::Color>(AZ::Color::CreateOne());
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return "[0.5, 0.5, 0.5]";
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "[1.0, 1.0, 1.0, 1.0]";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kObjectType);
            features.EnableJsonType(rapidjson::kArrayType);
            features.m_supportsInjection = false;
            features.m_fixedSizeArray = true;
        }

        bool AreEqual(const AZ::Color& lhs, const AZ::Color& rhs) override
        {
            return lhs == rhs;
        }
    };

    using ColorSerializerConformityTestTypes = ::testing::Types<ColorSerializerTestDescription>;
    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_SUITE_P(JsonColorSerializer, JsonSerializerConformityTests, ColorSerializerConformityTestTypes));

    class JsonColorSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_colorSerializer = AZStd::make_unique<AZ::JsonColorSerializer>();
        }

        void TearDown() override
        {
            m_colorSerializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        rapidjson::Document CreateArray(std::initializer_list<float> values)
        {
            rapidjson::Document document;
            rapidjson::Value& jsonValues = document.SetArray();
            for (float value : values)
            {
                jsonValues.PushBack(value, document.GetAllocator());
            }
            return document;
        }

        rapidjson::Document CreateArray(const char* fieldName, std::initializer_list<float> fieldValues)
        {
            rapidjson::Document document;
            rapidjson::Value& documentObject = document.SetObject();
            rapidjson::Value jsonValues(rapidjson::kArrayType);
            for (float value : fieldValues)
            {
                jsonValues.PushBack(value, document.GetAllocator());
            }
            documentObject.AddMember(rapidjson::StringRef(fieldName), AZStd::move(jsonValues), document.GetAllocator());
            return document;
        }

        rapidjson::Document CreateArray(const char* fieldName, std::initializer_list<int> fieldValues)
        {
            rapidjson::Document document;
            rapidjson::Value& documentObject = document.SetObject();
            rapidjson::Value jsonValues(rapidjson::kArrayType);
            for (int value : fieldValues)
            {
                jsonValues.PushBack(value, document.GetAllocator());
            }
            documentObject.AddMember(rapidjson::StringRef(fieldName), AZStd::move(jsonValues), document.GetAllocator());
            return document;
        }

        rapidjson::Document CreateString(const char* fieldName, const char* fieldValue)
        {
            rapidjson::Document document;
            rapidjson::Value& documentObject = document.SetObject();
            documentObject.AddMember(rapidjson::StringRef(fieldName), rapidjson::StringRef(fieldValue), document.GetAllocator());
            return document;
        }

        void ExpectColorEq(const AZ::Color& color, float r, float g, float b, float a)
        {
            EXPECT_FLOAT_EQ(r, color.GetR());
            EXPECT_FLOAT_EQ(g, color.GetG());
            EXPECT_FLOAT_EQ(b, color.GetB());
            EXPECT_FLOAT_EQ(a, color.GetA());
        }

    protected:
        AZStd::unique_ptr<AZ::JsonColorSerializer> m_colorSerializer;
    };

    // Load - base form

    TEST_F(JsonColorSerializerTests, Load_FloatRgbArray_LoadsChannels)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray({ 1.0f, 0.1f, 0.5f });
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.1f, 0.5f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_MoreThanFourArrayElement_LoadsChannelsAndReturnsPartialConvert)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray({ 1.0f, 0.1f, 0.5f, 0.8f, 0.2f, 0.3f });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.1f, 0.5f, 0.8f);
    }

    TEST_F(JsonColorSerializerTests, Load_InvalidRedInFloatArray_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document(rapidjson::kArrayType);
        document.PushBack("Hello", document.GetAllocator());
        document.PushBack(0.5f, document.GetAllocator());
        document.PushBack(0.5f, document.GetAllocator());
        document.PushBack(0.5f, document.GetAllocator());
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Color::CreateZero(), color);
    }

    TEST_F(JsonColorSerializerTests, Load_InvalidGreenInFloatArray_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document(rapidjson::kArrayType);
        document.PushBack(0.5f, document.GetAllocator());
        document.PushBack("Hello", document.GetAllocator());
        document.PushBack(0.5f, document.GetAllocator());
        document.PushBack(0.5f, document.GetAllocator());
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Color::CreateZero(), color);
    }

    TEST_F(JsonColorSerializerTests, Load_InvalidBlueInFloatArray_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document(rapidjson::kArrayType);
        document.PushBack(0.5f, document.GetAllocator());
        document.PushBack(0.5f, document.GetAllocator());
        document.PushBack("Hello", document.GetAllocator());
        document.PushBack(0.5f, document.GetAllocator());
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Color::CreateZero(), color);
    }

    TEST_F(JsonColorSerializerTests, Load_InvalidAlphaInFloatArray_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document(rapidjson::kArrayType);
        document.PushBack(0.5f, document.GetAllocator());
        document.PushBack(0.5f, document.GetAllocator());
        document.PushBack(0.5f, document.GetAllocator());
        document.PushBack("Hello", document.GetAllocator());
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Color::CreateZero(), color);
    }

    // Load - RGB Object

    TEST_F(JsonColorSerializerTests, Load_RgbObject_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGB", { 1.0f, 0.1f, 0.5f });
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.1f, 0.5f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_RgbObjectRandomCase_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("rGb", { 1.0f, 0.1f, 0.5f });
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.1f, 0.5f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_RgbObjectTooShort_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGB", { 1.0f, 0.1f });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonColorSerializerTests, Load_RgbObjectTooLong_ReturnsPartialConvertAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGB", { 1.0f, 0.1f, 0.5f, 0.8f });
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.1f, 0.5f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_RgbObjectWithInvalidEntry_ReturnsUnsupportedLeavesColorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGB", { 1.0f, 0.1f, 0.5f, 0.8f });
        rapidjson::Pointer pointer("/RGB/1");
        rapidjson::Value* arrayValue = pointer.Get(document);
        ASSERT_NE(nullptr, arrayValue);
        arrayValue->SetString("Hello world");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Color::CreateZero(), color);
    }

    TEST_F(JsonColorSerializerTests, Load_RgbObjectWrongType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("RGB", "FF00FF00");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    // Load - RGBA Object

    TEST_F(JsonColorSerializerTests, Load_RgbaObject_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGBA", { 1.0f, 0.1f, 0.5f, 0.8f });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.1f, 0.5f, 0.8f);
    }

    TEST_F(JsonColorSerializerTests, Load_RgbaObjectRandomCase_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("rGbA", { 1.0f, 0.1f, 0.5f, 0.8f });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.1f, 0.5f, 0.8f);
    }

    TEST_F(JsonColorSerializerTests, Load_RgbaObjectTooShort_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGBA", { 1.0f, 0.1f });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonColorSerializerTests, Load_RgbaObjectTooLong_ReturnsPartialConvertAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGBA", { 1.0f, 0.1f, 0.5f, 0.8f, 0.3f });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.1f, 0.5f, 0.8f);
    }

    TEST_F(JsonColorSerializerTests, Load_RgbaObjectWrongType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("RGBA", "FF00FF00");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    // Load - RGB8 Object

    TEST_F(JsonColorSerializerTests, Load_Rgb8Object_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGB8", { 255, 51, 204 });
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_Rgb8ObjectRandomCase_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("rGb8", { 255, 51, 204 });
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_Rgb8ObjectTooShort_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGB8", { 255, 51 });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonColorSerializerTests, Load_Rgb8ObjectTooLong_ReturnsPartialConvertAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGB8", { 255, 51, 204, 153 });
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_Rgb8ObjectWrongType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("RGB8", "FF00FF00");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    // Load - RGBA8 Object

    TEST_F(JsonColorSerializerTests, Load_Rgba8Object_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGBA8", { 255, 51, 204, 153 });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.6f);
    }

    TEST_F(JsonColorSerializerTests, Load_Rgba8ObjectRandomCase_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("rGbA8", { 255, 51, 204, 153 });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.6f);
    }

    TEST_F(JsonColorSerializerTests, Load_Rgba8ObjectTooShort_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGBA8", { 255, 51, 204 });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonColorSerializerTests, Load_Rgba8ObjectTooLong_ReturnsPartialConvertAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGBA8", { 255, 51, 204, 153, 102 });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.6f);
    }

    TEST_F(JsonColorSerializerTests, Load_Rgba8ObjectWithInvalidRed_ReturnsUnsupportedLeavesColorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGBA8", { 255, 51, 204, 153, 102 });
        rapidjson::Pointer("/RGBA8/0").Get(document)->SetString("Hello world");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Color::CreateZero(), color);
    }

    TEST_F(JsonColorSerializerTests, Load_Rgba8ObjectWithInvalidGreen_ReturnsUnsupportedLeavesColorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGBA8", { 255, 51, 204, 153, 102 });
        rapidjson::Pointer("/RGBA8/1").Get(document)->SetString("Hello world");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Color::CreateZero(), color);
    }

    TEST_F(JsonColorSerializerTests, Load_Rgba8ObjectWithInvalidBlue_ReturnsUnsupportedLeavesColorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGBA8", { 255, 51, 204, 153, 102 });
        rapidjson::Pointer("/RGBA8/2").Get(document)->SetString("Hello world");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Color::CreateZero(), color);
    }

    TEST_F(JsonColorSerializerTests, Load_Rgba8ObjectWithInvalidAlpha_ReturnsUnsupportedLeavesColorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("RGBA8", { 255, 51, 204, 153, 102 });
        rapidjson::Pointer("/RGBA8/3").Get(document)->SetString("Hello world");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Color::CreateZero(), color);
    }

    TEST_F(JsonColorSerializerTests, Load_Rgba8ObjectWrongType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("RGBA8", "FF00FF00");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    // Load - HEX Object

    TEST_F(JsonColorSerializerTests, Load_HexObject_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("HEX", "FF33CC");
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_HexObjectRandomCaseInName_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("hEx", "FF33CC");
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_HexObjectRandomCaseInValue_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("hEx", "Ff33cC");
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_HexObjectTooShort_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("HEX", "FF33");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonColorSerializerTests, Load_HexObjectTooLong_ReturnsPartialConvertAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("HEX", "FF33CC99FF33CC and a comment");
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.3f);
    }

    TEST_F(JsonColorSerializerTests, Load_HexObjectWithInvalidChars_ReturnsUnsupportedLeavesColorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("HEX", "FFXXCC");
        AZ::Color color = AZ::Color::CreateZero();
        color.SetA(0.3f);
        AZ::Color reference = color;
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(reference, color);
    }

    TEST_F(JsonColorSerializerTests, Load_HexObjectWrongType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("HEX", { 255, 51, 204, 153 });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    // Load - HEXA Object

    TEST_F(JsonColorSerializerTests, Load_HexaObject_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("HEXA", "FF33CC99");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.6f);
    }

    TEST_F(JsonColorSerializerTests, Load_HexaObjectRandomCase_ReturnsSuccessAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("hExA", "FF33CC99");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.6f);
    }

    TEST_F(JsonColorSerializerTests, Load_HexaObjectTooShort_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("HEXA", "FF33CC");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonColorSerializerTests, Load_HexaObjectTooLong_ReturnsPartialConvertAndLoadsColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("HEXA", "FF33CC99FF33CC and a comment");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.2f, 0.8f, 0.6f);
    }

    TEST_F(JsonColorSerializerTests, Load_HexaObjectWithInvalidChars_ReturnsUnsupportedLeavesColorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateString("HEXA", "FFXXCC99");
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(AZ::Color::CreateZero(), color);
    }

    TEST_F(JsonColorSerializerTests, Load_HexaObjectWrongType_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document = CreateArray("HEXA", { 255, 51, 204, 153 });
        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), document, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    //  Load - Object misc.

    TEST_F(JsonColorSerializerTests, Load_MiscObjectUnsupportedName_ReturnsUnsupported)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document;
        rapidjson::Value& testVal = document.SetObject();
        testVal.AddMember(rapidjson::StringRef("Hello"), rapidjson::StringRef("World"), document.GetAllocator());

        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), testVal, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());
    }

    TEST_F(JsonColorSerializerTests, Load_MiscObjectMultipleObjects_ReturnsPartialConvertAndLoadsLastColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document;

        rapidjson::Value testArray1(rapidjson::kArrayType);
        testArray1.PushBack(1.0f, document.GetAllocator());
        testArray1.PushBack(0.9f, document.GetAllocator());
        testArray1.PushBack(0.8f, document.GetAllocator());

        rapidjson::Value testArray2(rapidjson::kArrayType);
        testArray2.PushBack(1.0f, document.GetAllocator());
        testArray2.PushBack(0.1f, document.GetAllocator());
        testArray2.PushBack(0.5f, document.GetAllocator());
        testArray2.PushBack(0.8f, document.GetAllocator());

        rapidjson::Value& testVal = document.SetObject();
        testVal.AddMember(rapidjson::StringRef("RGB"), AZStd::move(testArray1), document.GetAllocator());
        testVal.AddMember(rapidjson::StringRef("RGBA"), AZStd::move(testArray2), document.GetAllocator());

        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), testVal, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.1f, 0.5f, 0.8f);
    }

    TEST_F(JsonColorSerializerTests, Load_MiscObjectMultipleObjectsWithLastNotSupported_ReturnsPartialConvertAndLoadsLastSupportedColor)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document;

        rapidjson::Value testArray1(rapidjson::kArrayType);
        testArray1.PushBack(1.0f, document.GetAllocator());
        testArray1.PushBack(0.9f, document.GetAllocator());
        testArray1.PushBack(0.8f, document.GetAllocator());

        rapidjson::Value testArray2(rapidjson::kArrayType);
        testArray2.PushBack(1.0f, document.GetAllocator());
        testArray2.PushBack(0.1f, document.GetAllocator());
        testArray2.PushBack(0.5f, document.GetAllocator());
        testArray2.PushBack(0.8f, document.GetAllocator());

        rapidjson::Value& testVal = document.SetObject();
        testVal.AddMember(rapidjson::StringRef("RGB"), AZStd::move(testArray1), document.GetAllocator());
        testVal.AddMember(rapidjson::StringRef("RGBA"), AZStd::move(testArray2), document.GetAllocator());
        testVal.AddMember(rapidjson::StringRef("Hello"), rapidjson::StringRef("World"), document.GetAllocator());

        AZ::Color color = AZ::Color::CreateZero();
        ResultCode result = m_colorSerializer->Load(&color, azrtti_typeid<AZ::Color>(), testVal, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        ExpectColorEq(color, 1.0f, 0.1f, 0.5f, 0.8f);
    }


    // Store

    TEST_F(JsonColorSerializerTests, Store_OnlyAlphaIsDefault_ReturnsPartialDefaultValueAndDoesNotStoreAlpha)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document;
        float values[] = { 1.0f, 0.2f, 0.8f, 0.6f };
        AZ::Color color = AZ::Color::CreateFromFloat4(values);
        float defaultValues[] = { 0.3f, 0.6f, 0.9f, 0.6f };
        AZ::Color defaultColor = AZ::Color::CreateFromFloat4(defaultValues);

        ResultCode result = m_colorSerializer->Store(document, &color, &defaultColor, azrtti_typeid<AZ::Color>(), *m_jsonSerializationContext);
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        
        ASSERT_TRUE(document.IsArray());
        ASSERT_EQ(3, document.Size());
        for (rapidjson::SizeType i = 0; i < 3; ++i)
        {
            ASSERT_TRUE(document[i].IsDouble());
            EXPECT_DOUBLE_EQ(document[i].GetDouble(), values[i]);
        }
    }

    TEST_F(JsonColorSerializerTests, Store_OnlyGreenIsDefault_ReturnsSuccessAndAllValuesAreStored)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document;
        float values[] = { 1.0f, 0.2f, 0.8f, 0.6f };
        AZ::Color color = AZ::Color::CreateFromFloat4(values);
        float defaultValues[] = { 0.3f, 0.2f, 0.9f, 1.0f };
        AZ::Color defaultColor = AZ::Color::CreateFromFloat4(defaultValues);

        ResultCode result = m_colorSerializer->Store(document, &color, &defaultColor, azrtti_typeid<AZ::Color>(), *m_jsonSerializationContext);
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());

        ASSERT_TRUE(document.IsArray());
        ASSERT_EQ(4, document.Size());
        for (rapidjson::SizeType i = 0; i < 4; ++i)
        {
            ASSERT_TRUE(document[i].IsDouble());
            EXPECT_DOUBLE_EQ(document[i].GetDouble(), values[i]);
        }
    }

    TEST_F(JsonColorSerializerTests, Store_GreenAndAlphaAreDefault_ReturnsPartialDefaultValueAndRgbIsStored)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Document document;
        float values[] = { 1.0f, 0.2f, 0.8f, 0.6f };
        AZ::Color color = AZ::Color::CreateFromFloat4(values);
        float defaultValues[] = { 0.3f, 0.2f, 0.9f, 0.6f };
        AZ::Color defaultColor = AZ::Color::CreateFromFloat4(defaultValues);

        ResultCode result = m_colorSerializer->Store(document, &color, &defaultColor, azrtti_typeid<AZ::Color>(), *m_jsonSerializationContext);
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());

        ASSERT_TRUE(document.IsArray());
        ASSERT_EQ(3, document.Size());
        for (rapidjson::SizeType i = 0; i < 3; ++i)
        {
            ASSERT_TRUE(document[i].IsDouble());
            EXPECT_DOUBLE_EQ(document[i].GetDouble(), values[i]);
        }
    }
} // namespace JsonSerializationTests
