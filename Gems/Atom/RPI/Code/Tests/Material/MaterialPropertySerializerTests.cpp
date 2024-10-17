/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>
#include <Common/JsonTestUtils.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertySerializer.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <Common/TestUtils.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

extern "C" void CleanUpRpiPublicGenericClassInfo();
extern "C" void CleanUpRpiEditGenericClassInfo();

namespace JsonSerializationTests
{
    class MaterialPropertySerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<AZ::RPI::MaterialPropertySourceData>
    {
    public:
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            AZ::RPI::MaterialTypeSourceData::Reflect(context.get());
            AZ::RPI::MaterialPropertySourceData::Reflect(context.get());
            AZ::RPI::MaterialPropertyDescriptor::Reflect(context.get());
            AZ::RPI::ReflectMaterialDynamicMetadata(context.get());
        }

        void Reflect(AZStd::unique_ptr<AZ::JsonRegistrationContext>& context) override
        {
            AZ::RPI::MaterialPropertySourceData::Reflect(context.get());
            AZ::RPI::MaterialTypeSourceData::Reflect(context.get());
        }

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::RPI::JsonMaterialPropertySerializer>();
        }

        AZStd::shared_ptr<AZ::RPI::MaterialPropertySourceData> CreateDefaultInstance() override
        {
            return AZStd::make_shared<AZ::RPI::MaterialPropertySourceData>();
        }

        AZStd::shared_ptr<AZ::RPI::MaterialPropertySourceData> CreatePartialDefaultInstance() override
        {
            auto result = AZStd::make_shared<AZ::RPI::MaterialPropertySourceData>("testProperty");
            result->m_dataType = AZ::RPI::MaterialPropertyDataType::Float;
            result->m_step = 1.0f;
            result->m_value = 0.0f;
            return result;
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
            {
                "name": "testProperty",
                "type": "Float",
                "step": 1.0
            })";
        }

        AZStd::shared_ptr<AZ::RPI::MaterialPropertySourceData> CreateFullySetInstance() override
        {
            auto result = AZStd::make_shared<AZ::RPI::MaterialPropertySourceData>("testProperty");
            result->m_description = "description";
            result->m_displayName = "display_name";
            result->m_dataType = AZ::RPI::MaterialPropertyDataType::Float;
            result->m_value = 2.0f;
            result->m_enumIsUv = true;
            result->m_min = 1.0f;
            result->m_max = 10.0f;
            result->m_softMin = 2.0f;
            result->m_softMax = 9.0f;
            result->m_step = 1.5f;
            result->m_visibility = AZ::RPI::MaterialPropertyVisibility::Hidden;
            result->m_outputConnections.emplace_back(AZ::RPI::MaterialPropertyOutputType::ShaderOption, "o_foo");

            return result;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
            {
                "name": "testProperty",
                "displayName": "display_name",
                "description": "description",
                "type": "Float",
                "defaultValue": 2.0,
                "min": 1.0,
                "max": 10.0,
                "softMin": 2.0,
                "softMax": 9.0,
                "step": 1.5,
                "visibility": "Hidden",
                "connection":
                {
                    "type": "ShaderOption",
                    "name": "o_foo"
                },
                "enumIsUv": true
            })";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kObjectType);
        }

        bool AreEqual(
            const AZ::RPI::MaterialPropertySourceData& lhs,
            const AZ::RPI::MaterialPropertySourceData& rhs) override
        {
            if (lhs.GetName() != rhs.GetName()) { return false; }
            if (lhs.m_description != rhs.m_description) { return false; }
            if (lhs.m_displayName != rhs.m_displayName) { return false; }
            if (lhs.m_dataType != rhs.m_dataType) { return false; }
            if (lhs.m_value != rhs.m_value) { return false; }
            if (lhs.m_enumIsUv != rhs.m_enumIsUv) { return false; }
            if (lhs.m_min != rhs.m_min) { return false; }
            if (lhs.m_max != rhs.m_max) { return false; }
            if (lhs.m_softMin != rhs.m_softMin) { return false; }
            if (lhs.m_softMax != rhs.m_softMax) { return false; }
            if (lhs.m_step != rhs.m_step) { return false; }
            if (lhs.m_visibility != rhs.m_visibility) { return false; }
            if (lhs.m_outputConnections.size() != rhs.m_outputConnections.size()) { return false; }
            for (size_t i = 0; i < lhs.m_outputConnections.size(); ++i)
            {
                auto& leftConnection = lhs.m_outputConnections[i];
                auto& rightConnection = rhs.m_outputConnections[i];
                if (leftConnection.m_type != rightConnection.m_type) { return false; }
                if (leftConnection.m_name!= rightConnection.m_name) { return false; }
            }
            return true;
        }

        void TearDown() override
        {
            CleanUpRpiPublicGenericClassInfo();
            CleanUpRpiEditGenericClassInfo();

            JsonSerializerConformityTestDescriptor::TearDown();
        }
    };

    using MaterialPropertySerializerTestTypes = ::testing::Types<MaterialPropertySerializerTestDescription>;
    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_CASE_P(MaterialPropertySerializerTests, JsonSerializerConformityTests, MaterialPropertySerializerTestTypes));
} // namespace JsonSerializationTests

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class MaterialPropertySerializerTests
        : public RPITestFixture
    {
    protected:

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        void Reflect(ReflectContext* context) override
        {
            RPITestFixture::Reflect(context);
            MaterialPropertySourceData::Reflect(context);
            MaterialTypeSourceData::Reflect(context);
        }
        
        template<typename T>
        void TestStoreToJson(const T& object, AZStd::string_view expectedJson)
        {
            AZStd::string outputJson;
            JsonTestResult storeResult = StoreTestDataToJson(object, outputJson);

            EXPECT_EQ(AZ::JsonSerializationResult::Tasks::WriteValue, storeResult.m_jsonResultCode.GetTask());
            EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, storeResult.m_jsonResultCode.GetProcessing());
            EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::PartialDefaults, storeResult.m_jsonResultCode.GetOutcome());

            ExpectSimilarJson(expectedJson, outputJson);
        }
    };


    // "GeneraData" refers to data that isn't dependent on the "type" field
    TEST_F(MaterialPropertySerializerTests, LoadAndStoreJson_GeneralData)
    {
        const AZStd::string inputJson = R"(
        {
            "name": "testProperty",
            "displayName": "Test Property",
            "description": "This is a property description",
            "type": "Float"
        }
        )";

        MaterialPropertySourceData propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::PartialDefaults, loadResult.m_jsonResultCode.GetOutcome());

        EXPECT_EQ("testProperty", propertyData.GetName());
        EXPECT_EQ("Test Property", propertyData.m_displayName);
        EXPECT_EQ("This is a property description", propertyData.m_description);
        EXPECT_EQ(MaterialPropertyDataType::Float, propertyData.m_dataType);

        EXPECT_TRUE(loadResult.ContainsMessage("/name", "Success"));
        EXPECT_TRUE(loadResult.ContainsMessage("/displayName", "Success"));
        EXPECT_TRUE(loadResult.ContainsMessage("/description", "Success"));
        EXPECT_TRUE(loadResult.ContainsMessage("/type", "Success"));

        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));

        TestStoreToJson(propertyData, inputJson);
    }

    // "GeneraData" refers to data that isn't dependent on the "type" field
    TEST_F(MaterialPropertySerializerTests, LoadAndStoreJson_DefaultGeneralData)
    {
        // Note we are keeping id and type because they are required fields
        const AZStd::string inputJson = R"(
        {
            "name": "testProperty",
            "type": "Float"
        }
        )";

        MaterialPropertySourceData propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::PartialDefaults, loadResult.m_jsonResultCode.GetOutcome());

        EXPECT_TRUE(propertyData.m_displayName.empty());
        EXPECT_TRUE(propertyData.m_description.empty());

        EXPECT_TRUE(loadResult.ContainsMessage("/name", "Success"));
        EXPECT_TRUE(loadResult.ContainsMessage("/type", "Success"));

        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));

        TestStoreToJson(propertyData, inputJson);
    }

    TEST_F(MaterialPropertySerializerTests, Load_Error_NotAnObject)
    {
        const AZStd::string inputJson = R"(
        []
        )";

        MaterialPropertySourceData propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Altered, loadResult.m_jsonResultCode.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::Unsupported, loadResult.m_jsonResultCode.GetOutcome());

        EXPECT_TRUE(loadResult.ContainsMessage("", "Property definition must be a JSON object"));
    }
    
    TEST_F(MaterialPropertySerializerTests, Load_Error_InvalidDataType)
    {
        const AZStd::string inputJson = R"(
        {
            "name": "testProperty",
            "type": "foo"
        }
        )";

        MaterialPropertySourceData propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);
        
        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::PartialAlter, loadResult.m_jsonResultCode.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::Unsupported, loadResult.m_jsonResultCode.GetOutcome());

        EXPECT_EQ(AZ::RPI::MaterialPropertyDataType::Invalid, propertyData.m_dataType);

        EXPECT_TRUE(loadResult.ContainsMessage("/name", "Success"));
        EXPECT_TRUE(loadResult.ContainsMessage("/type", "Enum value could not read"));
    }

    TEST_F(MaterialPropertySerializerTests, LoadAndStoreJson_NumericType_AllValues)
    {
        const AZStd::string inputJson = R"(
        [
            {
                "name": "testProperty1",
                "type": "Float",
                "defaultValue": 0.5,
                "min": 0.1,
                "max": 1.5,
                "softMin": 0.2,
                "softMax": 1.0,
                "step": 0.05
            },
            {
                "name": "testProperty2",
                "type": "Int",
                "defaultValue": -1,
                "min": -5,
                "max": 5,
                "softMin": -4,
                "softMax": 4,
                "step": 1
            },
            {
                "name": "testProperty3",
                "type": "UInt",
                "defaultValue": 4294901761,
                "min": 4294901760,
                "max": 4294901775,
                "softMin": 4294901761,
                "softMax": 4294901774,
                "step": 1
            }
        ]
        )";

        AZStd::vector<MaterialPropertySourceData> propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        EXPECT_EQ(MaterialPropertyDataType::Float, propertyData[0].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0.5f), propertyData[0].m_value);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0.1f), propertyData[0].m_min);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(1.5f), propertyData[0].m_max);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0.2f), propertyData[0].m_softMin);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(1.0f), propertyData[0].m_softMax);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0.05f), propertyData[0].m_step);

        EXPECT_EQ(MaterialPropertyDataType::Int, propertyData[1].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(-1), propertyData[1].m_value);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(-5), propertyData[1].m_min);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(5), propertyData[1].m_max);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(-4), propertyData[1].m_softMin);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(4), propertyData[1].m_softMax);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(1), propertyData[1].m_step);

        EXPECT_EQ(MaterialPropertyDataType::UInt, propertyData[2].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0xFFFF0001u), propertyData[2].m_value);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0xFFFF0000u), propertyData[2].m_min);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0xFFFF000Fu), propertyData[2].m_max);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0xFFFF0001u), propertyData[2].m_softMin);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0xFFFF000Eu), propertyData[2].m_softMax);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(1u), propertyData[2].m_step);

        for (int i = 0; i < propertyData.size(); ++i)
        {
            AZStd::string prefix = AZStd::string::format("/%d", i);
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/name", "Success"));
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/type", "Success"));
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/defaultValue", "Success"));
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/min", "Success"));
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/max", "Success"));
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/softMin", "Success"));
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/softMax", "Success"));
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/step", "Success"));
        }

        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));

        TestStoreToJson(propertyData, inputJson);
    }

    TEST_F(MaterialPropertySerializerTests, LoadAndStoreJson_NumericType_DefaultValues)
    {
        const AZStd::string inputJson = R"(
        [
            {
                "name": "testProperty1",
                "displayName": "Test Property 1",
                "description": "Test",
                "type": "Float"
            },
            {
                "name": "testProperty2",
                "displayName": "Test Property 2",
                "description": "Test",
                "type": "Int"
            },
            {
                "name": "testProperty3",
                "displayName": "Test Property 3",
                "description": "Test",
                "type": "UInt"
            }
        ]
        )";

        AZStd::vector<MaterialPropertySourceData> propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::PartialDefaults, loadResult.m_jsonResultCode.GetOutcome());

        EXPECT_EQ(MaterialPropertyDataType::Float, propertyData[0].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0.0f), propertyData[0].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Int, propertyData[1].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0), propertyData[1].m_value);

        EXPECT_EQ(MaterialPropertyDataType::UInt, propertyData[2].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0u), propertyData[2].m_value);

        for (const MaterialPropertySourceData& property : propertyData)
        {
            EXPECT_FALSE(property.m_min.IsValid());
            EXPECT_FALSE(property.m_max.IsValid());
            EXPECT_FALSE(property.m_softMin.IsValid());
            EXPECT_FALSE(property.m_softMax.IsValid());
            EXPECT_FALSE(property.m_step.IsValid());
        }

        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));

        TestStoreToJson(propertyData, inputJson);
    }

    TEST_F(MaterialPropertySerializerTests, LoadAndStoreJson_VectorLabels_LabelValues)
    {
        const AZStd::string inputJson = R"(
        [
            {
                "name": "testProperty1",
                "type": "Vector2",
                "vectorLabels": ["U", "V"],
                "defaultValue": [0.6, 0.5]
            },
            {
                "name": "testProperty2",
                "type": "Vector4",
                "vectorLabels": ["A", "B", "C", "D"],
                "defaultValue": [0.3, 0.4, 0.5, 0.6]
            }
        ]
        )";

        AZStd::vector<MaterialPropertySourceData> propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        EXPECT_EQ(MaterialPropertyDataType::Vector2, propertyData[0].m_dataType);
        EXPECT_TRUE(propertyData[0].m_vectorLabels.size() == 2);
        EXPECT_EQ("U", propertyData[0].m_vectorLabels[0]);
        EXPECT_EQ("V", propertyData[0].m_vectorLabels[1]);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector2{ 0.6f, 0.5f }), propertyData[0].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Vector4, propertyData[1].m_dataType);
        EXPECT_TRUE(propertyData[1].m_vectorLabels.size() == 4);
        EXPECT_EQ("A", propertyData[1].m_vectorLabels[0]);
        EXPECT_EQ("B", propertyData[1].m_vectorLabels[1]);
        EXPECT_EQ("C", propertyData[1].m_vectorLabels[2]);
        EXPECT_EQ("D", propertyData[1].m_vectorLabels[3]);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector4{ 0.3f, 0.4f, 0.5f, 0.6f }), propertyData[1].m_value);

        TestStoreToJson(propertyData, inputJson);
    }

    TEST_F(MaterialPropertySerializerTests, LoadAndStoreJson_Visibility)
    {
        const AZStd::string inputJson = R"(
        [
            {
                "name": "visibilityIsDefault",
                "type": "Float"
            },
            {
                "name": "visibilityIsEditable",
                "type": "Float",
                "visibility": "Enabled"
            },
            {
                "name": "visibilityIsDisabled",
                "type": "Float",
                "visibility": "Disabled"
            },
            {
                "name": "visibilityIsHidden",
                "type": "Float",
                "visibility": "Hidden"
            }
        ]
        )";

        const AZStd::string expectedOutputJson = R"(
        [
            {
                "name": "visibilityIsDefault",
                "type": "Float"
            },
            {
                "name": "visibilityIsEditable",
                "type": "Float"
            },
            {
                "name": "visibilityIsDisabled",
                "type": "Float",
                "visibility": "Disabled"
            },
            {
                "name": "visibilityIsHidden",
                "type": "Float",
                "visibility": "Hidden"
            }
        ]
        )";

        AZStd::vector<MaterialPropertySourceData> propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::PartialDefaults, loadResult.m_jsonResultCode.GetOutcome()); // Because other fields like description are not included

        EXPECT_EQ(propertyData[0].m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(propertyData[1].m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(propertyData[2].m_visibility, MaterialPropertyVisibility::Disabled);
        EXPECT_EQ(propertyData[3].m_visibility, MaterialPropertyVisibility::Hidden);

        TestStoreToJson(propertyData, expectedOutputJson);
    }

    TEST_F(MaterialPropertySerializerTests, Load_NumericType_AlternateValueRepresentation)
    {
        // These alternate representations are supported by the fact that default JSON serializers
        // for numeric values use a flexible "best-effort" paradigm

        const AZStd::string inputJson = R"(
        [
            {
                "name": "testProperty1",
                "type": "Float",
                "defaultValue": true,
                "min": -1,
                "max": "100.5",
                "step": "1"
            },
            {
                "name": "testProperty2",
                "type": "Int",
                "defaultValue": true,
                "min": -1.5,
                "max": "100",
                "step": "1"
            },
            {
                "name": "testProperty3",
                "type": "UInt",
                "defaultValue": "4294963200",
                "min": true,
                "max": "0xFFFFFF00",
                "step": 2.5
            }
        ]
        )";

        AZStd::vector<MaterialPropertySourceData> propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        EXPECT_EQ(MaterialPropertyDataType::Float, propertyData[0].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(1.0f), propertyData[0].m_value);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(-1.0f), propertyData[0].m_min);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(100.5f), propertyData[0].m_max);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(1.0f), propertyData[0].m_step);

        EXPECT_EQ(MaterialPropertyDataType::Int, propertyData[1].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(1), propertyData[1].m_value);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(-1), propertyData[1].m_min);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(100), propertyData[1].m_max);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(1), propertyData[1].m_step);

        EXPECT_EQ(MaterialPropertyDataType::UInt, propertyData[2].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0xFFFFF000u), propertyData[2].m_value);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(1u), propertyData[2].m_min);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(0xFFFFFF00u), propertyData[2].m_max);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(2u), propertyData[2].m_step);

        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));
    }

    TEST_F(MaterialPropertySerializerTests, LoadAndStoreJson_NonNumericType_AllValues)
    {
        const AZStd::string inputJson = R"(
        [
            {
                "name": "testProperty1",
                "type": "Bool",
                "defaultValue": true
            },
            {
                "name": "testProperty2",
                "type": "Vector2",
                "defaultValue": [0.1, 0.2]
            },
            {
                "name": "testProperty3",
                "type": "Vector3",
                "defaultValue": [0.3, 0.4, 0.5]
            },
            {
                "name": "testProperty4",
                "type": "Vector4",
                "defaultValue": [0.6, 0.5, 0.8, 0.4]
            },
            {
                "name": "testProperty5",
                "type": "Color",
                "defaultValue": [0.1, 0.2, 0.3]
            },
            {
                "name": "testProperty6",
                "type": "Image",
                "defaultValue": "Default.png"
            }
        ]
        )";

        AZStd::vector<MaterialPropertySourceData> propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        EXPECT_EQ(MaterialPropertyDataType::Bool, propertyData[0].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(true), propertyData[0].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Vector2, propertyData[1].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector2{0.1f, 0.2f}), propertyData[1].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Vector3, propertyData[2].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector3{0.3f, 0.4f, 0.5f}), propertyData[2].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Vector4, propertyData[3].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector4{0.6f, 0.5f, 0.8f, 0.4f}), propertyData[3].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Color, propertyData[4].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Color{0.1f, 0.2f, 0.3f, 1.0f}), propertyData[4].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Image, propertyData[5].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZStd::string{"Default.png"}), propertyData[5].m_value);

        for (int i = 0; i < propertyData.size(); ++i)
        {
            AZStd::string prefix = AZStd::string::format("/%d", i);
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/name", "Success"));
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/type", "Success"));
            EXPECT_TRUE(loadResult.ContainsMessage(prefix + "/defaultValue", "Success"));
        }

        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));

        TestStoreToJson(propertyData, inputJson);
    }

    TEST_F(MaterialPropertySerializerTests, LoadAndStoreJson_NonNumericType_DefaultValues)
    {
        const AZStd::string inputJson = R"(
        [
            {
                "name": "testProperty1",
                "type": "Bool"
            },
            {
                "name": "testProperty2",
                "type": "Vector2"
            },
            {
                "name": "testProperty3",
                "type": "Vector3"
            },
            {
                "name": "testProperty4",
                "type": "Vector4"
            },
            {
                "name": "testProperty5",
                "type": "Color"
            },
            {
                "name": "testProperty6",
                "type": "Image"
            }
        ]
        )";

        AZStd::vector<MaterialPropertySourceData> propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::PartialDefaults, loadResult.m_jsonResultCode.GetOutcome());

        EXPECT_EQ(MaterialPropertyDataType::Bool, propertyData[0].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(false), propertyData[0].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Vector2, propertyData[1].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector2{0.0f, 0.0f}), propertyData[1].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Vector3, propertyData[2].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector3{0.0f, 0.0f, 0.0f}), propertyData[2].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Vector4, propertyData[3].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector4{0.0f, 0.0f, 0.0f, 0.0f}), propertyData[3].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Color, propertyData[4].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Color{1.0f, 1.0f, 1.0f, 1.0f}), propertyData[4].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Image, propertyData[5].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZStd::string{""}), propertyData[5].m_value);

        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));

        TestStoreToJson(propertyData, inputJson);
    }

    TEST_F(MaterialPropertySerializerTests, Load_NonNumericType_AlternateValueRepresentation)
    {
        const AZStd::string inputJson = R"(
        [
            {
                "name": "testProperty1",
                "type": "Bool",
                "defaultValue": 1
            },
            {
                "name": "testProperty2",
                "type": "Vector2",
                "defaultValue": { "x": 0.4, "y": 0.1 }
            },
            {
                "name": "testProperty3",
                "type": "Vector3",
                "defaultValue": { "x": 0.4, "y": 0.1, "z": 0.5 }
            },
            {
                "name": "testProperty4",
                "type": "Vector4",
                "defaultValue": { "x": 0.4, "y": 0.1, "z": 0.5, "w": 0.6 }
            },
            {
                "name": "testProperty5",
                "type": "Color",
                "defaultValue": { "hex": "FF00FF" }
            }
        ]
        )";

        AZStd::vector<MaterialPropertySourceData> propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        EXPECT_EQ(MaterialPropertyDataType::Bool, propertyData[0].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(true), propertyData[0].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Vector2, propertyData[1].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector2{0.4f, 0.1f}), propertyData[1].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Vector3, propertyData[2].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector3{0.4f, 0.1f, 0.5f}), propertyData[2].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Vector4, propertyData[3].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Vector4{0.4f, 0.1f, 0.5f, 0.6f}), propertyData[3].m_value);

        EXPECT_EQ(MaterialPropertyDataType::Color, propertyData[4].m_dataType);
        EXPECT_EQ(AZ::RPI::MaterialPropertyValue(AZ::Color{1.0f, 0.0f, 1.0f, 1.0f}), propertyData[4].m_value);

        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));
    }

    TEST_F(MaterialPropertySerializerTests, LoadAndStoreJson_OneConnection)
    {
        const AZStd::string inputJson = R"(
        {
            "name": "testProperty",
            "type": "Float",
            "connection": {
                "type": "ShaderOption",
                "name": "o_foo"
            }
        }
        )";

        MaterialPropertySourceData propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        EXPECT_EQ(1, propertyData.m_outputConnections.size());
        EXPECT_EQ(MaterialPropertyOutputType::ShaderOption, propertyData.m_outputConnections[0].m_type);
        EXPECT_EQ("o_foo", propertyData.m_outputConnections[0].m_name);

        EXPECT_TRUE(loadResult.ContainsMessage("/connection/type", "Success"));
        EXPECT_TRUE(loadResult.ContainsMessage("/connection/name", "Success"));
        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));

        TestStoreToJson(propertyData, inputJson);
    }
    
    TEST_F(MaterialPropertySerializerTests, LoadUsingOldFormat)
    {
        // Tests backward compatibility for when "id" was the key instead of "name", for both the property and its connections.

        const AZStd::string inputJson = R"(
        {
            "id": "testProperty",
            "type": "Float",
            "connection": {
                "type": "ShaderOption",
                "id": "o_foo"
            }
        }
        )";

        MaterialPropertySourceData propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());
        
        EXPECT_EQ("testProperty", propertyData.GetName());

        EXPECT_EQ(1, propertyData.m_outputConnections.size());
        EXPECT_EQ(MaterialPropertyOutputType::ShaderOption, propertyData.m_outputConnections[0].m_type);
        EXPECT_EQ("o_foo", propertyData.m_outputConnections[0].m_name);

        EXPECT_TRUE(loadResult.ContainsMessage("/connection/type", "Success"));
        EXPECT_TRUE(loadResult.ContainsMessage("/connection/id", "Success"));
        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));
    }

    TEST_F(MaterialPropertySerializerTests, LoadAndStoreJson_MultipleConnections)
    {
        const AZStd::string inputJson = R"(
        {
            "name": "testProperty",
            "type": "Float",
            "connection": [
                {
                    "type": "ShaderInput",
                    "name": "o_foo"
                },
                {
                    "type": "ShaderOption",
                    "name": "o_bar"
                }
            ]
        }
        )";

        MaterialPropertySourceData propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        EXPECT_EQ(2, propertyData.m_outputConnections.size());
        EXPECT_EQ(MaterialPropertyOutputType::ShaderInput, propertyData.m_outputConnections[0].m_type);
        EXPECT_EQ("o_foo", propertyData.m_outputConnections[0].m_name);

        EXPECT_EQ(MaterialPropertyOutputType::ShaderOption, propertyData.m_outputConnections[1].m_type);
        EXPECT_EQ("o_bar", propertyData.m_outputConnections[1].m_name);

        EXPECT_TRUE(loadResult.ContainsMessage("/connection/0/type", "Success"));
        EXPECT_TRUE(loadResult.ContainsMessage("/connection/0/name", "Success"));
        EXPECT_TRUE(loadResult.ContainsMessage("/connection/1/type", "Success"));
        EXPECT_TRUE(loadResult.ContainsMessage("/connection/1/name", "Success"));
        EXPECT_FALSE(loadResult.ContainsOutcome(JsonSerializationResult::Outcomes::Skipped));

        TestStoreToJson(propertyData, inputJson);
    }

    TEST_F(MaterialPropertySerializerTests, Load_Warning_SkippedTopLevelField)
    {
        // "conection" is misspelled
        const AZStd::string inputJson = R"(
        {
            "name": "testProperty",
            "type": "Float",
            "conection": [
                {
                    "type": "ShaderInput",
                    "name": "o_foo"
                }
            ]
        }
        )";

        MaterialPropertySourceData propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        EXPECT_EQ(propertyData.GetName(), "testProperty");
        EXPECT_EQ(propertyData.m_dataType, MaterialPropertyDataType::Float);
        EXPECT_EQ(propertyData.m_outputConnections.size(), 0);

        EXPECT_TRUE(loadResult.ContainsMessage("/conection", "skip"));
    }

    TEST_F(MaterialPropertySerializerTests, Load_Warning_SkippedConnectionField)
    {
        // "nam" is misspelled
        const AZStd::string inputJson = R"(
        {
            "name": "testProperty",
            "type": "Float",
            "connection": [
                {
                    "type": "ShaderInput",
                    "nam": "o_foo"
                }
            ]
        }
        )";

        MaterialPropertySourceData propertyData;
        JsonTestResult loadResult = LoadTestDataFromJson(propertyData, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        EXPECT_EQ(propertyData.GetName(), "testProperty");
        EXPECT_EQ(propertyData.m_dataType, MaterialPropertyDataType::Float);
        EXPECT_EQ(propertyData.m_outputConnections.size(), 1);
        EXPECT_EQ(propertyData.m_outputConnections[0].m_name, "");
        EXPECT_EQ(propertyData.m_outputConnections[0].m_type, MaterialPropertyOutputType::ShaderInput);

        EXPECT_TRUE(loadResult.ContainsMessage("/connection/0/nam", "skip"));
    }
}

