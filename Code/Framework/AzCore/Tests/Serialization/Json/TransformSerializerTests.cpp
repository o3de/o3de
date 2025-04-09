/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/TransformSerializer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    class JsonTransformSerializerTestDescription : public JsonSerializerConformityTestDescriptor<AZ::Transform>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonTransformSerializer>();
        }

        AZStd::shared_ptr<AZ::Transform> CreateDefaultInstance() override
        {
            return AZStd::make_shared<AZ::Transform>(AZ::Transform::CreateIdentity());
        }

        AZStd::shared_ptr<AZ::Transform> CreatePartialDefaultInstance() override
        {
            return AZStd::make_shared<AZ::Transform>(AZ::Transform::CreateTranslation(AZ::Vector3(1.0f, 2.0f, 3.0f)));
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return "{ \"Translation\" : [ 1.0, 2.0, 3.0 ] }";
        }


        AZStd::shared_ptr<AZ::Transform> CreateFullySetInstance() override
        {
            return AZStd::make_shared<AZ::Transform>(
                AZ::Vector3(1.0f, 2.0f, 3.0f), AZ::Quaternion(0.25f, 0.5f, 0.75f, 1.0f), 9.0f);
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "{ \"Translation\" : [ 1.0, 2.0, 3.0 ], \"Rotation\" : [ 0.25, 0.5, 0.75, 1.0 ], \"Scale\" : 9.0 }";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_supportsPartialInitialization = true;
            features.m_supportsInjection = false;
            features.m_requiresTypeIdLookups = true;
        }

        bool AreEqual(const AZ::Transform& lhs, const AZ::Transform& rhs) override
        {
            return lhs == rhs;
        }
    };

    using JsonTransformSerializerConformityTestTypes = ::testing::Types<JsonTransformSerializerTestDescription>;
    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_SUITE_P(JsonTransformSerializer, JsonSerializerConformityTests, JsonTransformSerializerConformityTestTypes));

    class JsonTransformSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_transformSerializer = AZStd::make_unique<AZ::JsonTransformSerializer>();
        }

        void TearDown() override
        {
            m_transformSerializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::JsonTransformSerializer> m_transformSerializer;
    };

    TEST_F(JsonTransformSerializerTests, Load_FullySetTransform_ReturnsSuccessWithAllValuesSetCorrectly)
    {
        AZ::Transform testTransform = AZ::Transform::CreateIdentity();
        AZ::Transform expectedTransform(
            AZ::Vector3(2.25f, 3.5f, 4.75f),
            AZ::Quaternion(0.25f, 0.5f, 0.75f, 1.0f),
            5.5f);

        rapidjson::Document json;
        json.Parse(R"({ "Translation": [ 2.25, 3.5, 4.75 ], "Rotation": [ 0.25, 0.5, 0.75, 1.0 ], "Scale": 5.5 })");

        ResultCode result =
            m_transformSerializer->Load(&testTransform, azrtti_typeid<decltype(testTransform)>(), json, *m_jsonDeserializationContext);

        EXPECT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::Success);
        EXPECT_EQ(testTransform, expectedTransform);
    }

    TEST_F(JsonTransformSerializerTests, Load_FullySetTransform_ReturnsSuccessWithDefaultTranslation)
    {
        AZ::Transform testTransform = AZ::Transform::CreateIdentity();
        AZ::Transform expectedTransform = 
            AZ::Transform::CreateFromQuaternion(AZ::Quaternion(0.25f, 0.5f, 0.75f, 1.0f));
        expectedTransform.SetUniformScale(5.5f);

        rapidjson::Document json;
        json.Parse(R"({ "Rotation": [ 0.25, 0.5, 0.75, 1.0 ], "Scale": 5.5 })");

        ResultCode result =
            m_transformSerializer->Load(&testTransform, azrtti_typeid<decltype(testTransform)>(), json, *m_jsonDeserializationContext);

        EXPECT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::PartialDefaults);
        EXPECT_EQ(testTransform, expectedTransform);
    }

    TEST_F(JsonTransformSerializerTests, Load_FullySetTransform_ReturnsSuccessWithDefaultRotation)
    {
        AZ::Transform testTransform = AZ::Transform::CreateIdentity();
        AZ::Transform expectedTransform = AZ::Transform::CreateTranslation(AZ::Vector3(2.25f, 3.5f, 4.75f));
        expectedTransform.SetUniformScale(5.5f);

        rapidjson::Document json;
        json.Parse(R"({ "Translation": [ 2.25, 3.5, 4.75 ], "Scale": 5.5 })");

        ResultCode result =
            m_transformSerializer->Load(&testTransform, azrtti_typeid<decltype(testTransform)>(), json, *m_jsonDeserializationContext);

        EXPECT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::PartialDefaults);
        EXPECT_EQ(testTransform, expectedTransform);
    }

    TEST_F(JsonTransformSerializerTests, Load_FullySetTransform_ReturnsSuccessWithDefaultScale)
    {
        AZ::Transform testTransform = AZ::Transform::CreateIdentity();
        AZ::Transform expectedTransform =
            AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion(0.25f, 0.5f, 0.75f, 1.0f), AZ::Vector3(2.25f, 3.5f, 4.75f));

        rapidjson::Document json;
        json.Parse(R"({ "Translation": [ 2.25, 3.5, 4.75 ], "Rotation": [ 0.25, 0.5, 0.75, 1.0 ] })");

        ResultCode result =
            m_transformSerializer->Load(&testTransform, azrtti_typeid<decltype(testTransform)>(), json, *m_jsonDeserializationContext);

        EXPECT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::PartialDefaults);
        EXPECT_EQ(testTransform, expectedTransform);
    }

    TEST_F(JsonTransformSerializerTests, Load_FullySetTransform_ReturnsSuccessWithOnlyTranslation)
    {
        AZ::Transform testTransform = AZ::Transform::CreateIdentity();
        AZ::Transform expectedTransform = AZ::Transform::CreateTranslation(AZ::Vector3(2.25f, 3.5f, 4.75f));

        rapidjson::Document json;
        json.Parse(R"({ "Translation": [ 2.25, 3.5, 4.75 ] })");

        ResultCode result =
            m_transformSerializer->Load(&testTransform, azrtti_typeid<decltype(testTransform)>(), json, *m_jsonDeserializationContext);

        EXPECT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::PartialDefaults);
        EXPECT_EQ(testTransform, expectedTransform);
    }

    TEST_F(JsonTransformSerializerTests, Load_FullySetTransform_ReturnsSuccessWithOnlyRotation)
    {
        AZ::Transform testTransform = AZ::Transform::CreateIdentity();
        AZ::Transform expectedTransform = AZ::Transform::CreateFromQuaternion(AZ::Quaternion(0.25f, 0.5f, 0.75f, 1.0f));

        rapidjson::Document json;
        json.Parse(R"({ "Rotation": [ 0.25, 0.5, 0.75, 1.0 ] })");

        ResultCode result =
            m_transformSerializer->Load(&testTransform, azrtti_typeid<decltype(testTransform)>(), json, *m_jsonDeserializationContext);

        EXPECT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::PartialDefaults);
        EXPECT_EQ(testTransform, expectedTransform);
    }

    TEST_F(JsonTransformSerializerTests, Load_FullySetTransform_ReturnsSuccessWithOnlyScale)
    {
        AZ::Transform testTransform = AZ::Transform::CreateIdentity();
        AZ::Transform expectedTransform = AZ::Transform::CreateUniformScale(5.5f);

        rapidjson::Document json;
        json.Parse(R"({ "Scale" : 5.5 })");

        ResultCode result =
            m_transformSerializer->Load(&testTransform, azrtti_typeid<decltype(testTransform)>(), json, *m_jsonDeserializationContext);

        EXPECT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::PartialDefaults);
        EXPECT_EQ(testTransform, expectedTransform);
    }

    TEST_F(JsonTransformSerializerTests, Load_FullySetTransform_ReturnsSuccessWithAllDefaults)
    {
        AZ::Transform testTransform = AZ::Transform::CreateIdentity();
        AZ::Transform expectedTransform = AZ::Transform::CreateIdentity();

        rapidjson::Document json;
        json.Parse(R"({})");

        ResultCode result =
            m_transformSerializer->Load(&testTransform, azrtti_typeid<decltype(testTransform)>(), json, *m_jsonDeserializationContext);

        EXPECT_EQ(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::DefaultsUsed);
        EXPECT_EQ(testTransform, expectedTransform);
    }
} // namespace JsonSerializationTests
