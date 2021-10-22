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
#include <Common/ShaderAssetTestUtils.h>
#include <Material/MaterialAssetTestUtils.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/IO/LocalFileIO.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class MaterialSourceDataTests
        : public RPITestFixture
    {
    protected:
        RHI::Ptr<RHI::ShaderResourceGroupLayout> m_testMaterialSrgLayout;
        Data::Asset<ShaderAsset> m_testShaderAsset;
        Data::Asset<MaterialTypeAsset> m_testMaterialTypeAsset;
        Data::Asset<ImageAsset> m_testImageAsset;

        void Reflect(AZ::ReflectContext* context) override
        {
            RPITestFixture::Reflect(context);
            MaterialTypeSourceData::Reflect(context);
            MaterialSourceData::Reflect(context);
        }

        void SetUp() override
        {
            EXPECT_EQ(nullptr, IO::FileIOBase::GetInstance());

            RPITestFixture::SetUp();

            auto localFileIO = AZ::IO::FileIOBase::GetInstance();
            EXPECT_NE(nullptr, localFileIO);
            char rootPath[AZ_MAX_PATH_LEN];
            AZ::Utils::GetExecutableDirectory(rootPath, AZ_MAX_PATH_LEN);
            localFileIO->SetAlias("@exefolder@", rootPath);

            m_testMaterialSrgLayout = CreateCommonTestMaterialSrgLayout();

            m_testShaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout);

            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(Uuid::CreateRandom());
            materialTypeCreator.AddShader(m_testShaderAsset);
            AddCommonTestMaterialProperties(materialTypeCreator, "general.");
            materialTypeCreator.End(m_testMaterialTypeAsset);

            // Since this test doesn't actually instantiate a Material, it won't need to instantiate this ImageAsset, so all we
            // need is an asset reference with a valid ID.
            m_testImageAsset = Data::Asset<ImageAsset>{ Data::AssetId{Uuid::CreateRandom(), StreamingImageAsset::GetImageAssetSubId()}, azrtti_typeid<StreamingImageAsset>() };

            // Register the test assets with the AssetSystemStub so CreateMaterialAsset() can use AssetUtils.
            m_assetSystemStub.RegisterSourceInfo("test.materialtype", m_testMaterialTypeAsset.GetId());
            m_assetSystemStub.RegisterSourceInfo("test.streamingimage", m_testImageAsset.GetId());
        }

        void TearDown() override
        {
            m_testMaterialTypeAsset.Reset();
            m_testMaterialSrgLayout = nullptr;
            m_testShaderAsset.Reset();
            m_testImageAsset.Reset();

            RPITestFixture::TearDown();
        }
    };
    
    void AddPropertyGroup(MaterialSourceData& material, AZStd::string_view groupName)
    {
        material.m_properties.insert(groupName);
    }
    
    void AddProperty(MaterialSourceData& material, AZStd::string_view groupName, AZStd::string_view propertyName, const MaterialPropertyValue& anyValue)
    {
        material.m_properties[groupName][propertyName].m_value = anyValue;
    }

    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_BasicProperties)
    {
        MaterialSourceData sourceData;

        sourceData.m_materialType = "test.materialtype";
        AddPropertyGroup(sourceData, "general");
        AddProperty(sourceData, "general", "MyBool", true);
        AddProperty(sourceData, "general", "MyInt", -10);
        AddProperty(sourceData, "general", "MyUInt", 25u);
        AddProperty(sourceData, "general", "MyFloat", 1.5f);
        AddProperty(sourceData, "general", "MyColor", AZ::Color{0.1f, 0.2f, 0.3f, 0.4f});
        AddProperty(sourceData, "general", "MyFloat2", AZ::Vector2(2.1f, 2.2f));
        AddProperty(sourceData, "general", "MyFloat3", AZ::Vector3(3.1f, 3.2f, 3.3f));
        AddProperty(sourceData, "general", "MyFloat4", AZ::Vector4(4.1f, 4.2f, 4.3f, 4.4f));
        AddProperty(sourceData, "general", "MyImage", AZStd::string("test.streamingimage"));
        AddProperty(sourceData, "general", "MyEnum", AZStd::string("Enum1"));

        auto materialAssetOutcome = sourceData.CreateMaterialAsset(Uuid::CreateRandom(), "", true);
        EXPECT_TRUE(materialAssetOutcome.IsSuccess());

        Data::Asset<MaterialAsset> materialAsset = materialAssetOutcome.GetValue();

        // The order here is based on the order in the MaterialTypeSourceData, as added to the the MaterialTypeAssetCreator.
        EXPECT_EQ(materialAsset->GetPropertyValues()[0].GetValue<bool>(), true);
        EXPECT_EQ(materialAsset->GetPropertyValues()[1].GetValue<int32_t>(), -10);
        EXPECT_EQ(materialAsset->GetPropertyValues()[2].GetValue<uint32_t>(), 25u);
        EXPECT_EQ(materialAsset->GetPropertyValues()[3].GetValue<float>(), 1.5f);
        EXPECT_EQ(materialAsset->GetPropertyValues()[4].GetValue<Vector2>(), Vector2(2.1f, 2.2f));
        EXPECT_EQ(materialAsset->GetPropertyValues()[5].GetValue<Vector3>(), Vector3(3.1f, 3.2f, 3.3f));
        EXPECT_EQ(materialAsset->GetPropertyValues()[6].GetValue<Vector4>(), Vector4(4.1f, 4.2f, 4.3f, 4.4f));
        EXPECT_EQ(materialAsset->GetPropertyValues()[7].GetValue<Color>(), Color(0.1f, 0.2f, 0.3f, 0.4f));
        EXPECT_EQ(materialAsset->GetPropertyValues()[8].GetValue<Data::Asset<ImageAsset>>(), m_testImageAsset);
        EXPECT_EQ(materialAsset->GetPropertyValues()[9].GetValue<uint32_t>(), 1u);
    }

    void CheckEqual(MaterialSourceData& a, MaterialSourceData& b)
    {
        EXPECT_STREQ(a.m_materialType.data(), b.m_materialType.data());
        EXPECT_STREQ(a.m_description.data(), b.m_description.data());
        EXPECT_STREQ(a.m_parentMaterial.data(), b.m_parentMaterial.data());
        EXPECT_EQ(a.m_propertyLayoutVersion, b.m_propertyLayoutVersion);

        EXPECT_EQ(a.m_properties.size(), b.m_properties.size());
        for (auto& groupA : a.m_properties)
        {
            AZStd::string groupName = groupA.first;

            auto groupIterB = b.m_properties.find(groupName);
            if (groupIterB == b.m_properties.end())
            {
                EXPECT_TRUE(false) << "groupB[" << groupName.c_str() << "] not found";
                continue;
            }

            auto& groupB = *groupIterB;

            EXPECT_EQ(groupA.second.size(), groupB.second.size()) << " for group[" << groupName.c_str() << "]";

            for (auto& propertyIterA : groupA.second)
            {
                AZStd::string propertyName = propertyIterA.first;

                auto propertyIterB = groupB.second.find(propertyName);
                if (propertyIterB == groupB.second.end())
                {
                    EXPECT_TRUE(false) << "groupB[" << groupName.c_str() << "][" << propertyName.c_str() << "] not found";
                    continue;
                }

                auto& propertyA = propertyIterA.second;
                auto& propertyB = propertyIterB->second;
                
                bool typesMatch = propertyA.m_value.GetTypeId() == propertyB.m_value.GetTypeId();
                EXPECT_TRUE(typesMatch);
                if (typesMatch)
                {
                    AZStd::string propertyReference = AZStd::string::format(" for property '%s.%s'", groupName.c_str(), propertyName.c_str());

                    auto typeId = propertyA.m_value.GetTypeId();

                    if      (typeId == azrtti_typeid<bool>())          { EXPECT_EQ(propertyA.m_value.GetValue<bool>(),      propertyB.m_value.GetValue<bool>())      << propertyReference.c_str(); }
                    else if (typeId == azrtti_typeid<int32_t>())       { EXPECT_EQ(propertyA.m_value.GetValue<int32_t>(),   propertyB.m_value.GetValue<int32_t>())   << propertyReference.c_str(); }
                    else if (typeId == azrtti_typeid<uint32_t>())      { EXPECT_EQ(propertyA.m_value.GetValue<uint32_t>(),  propertyB.m_value.GetValue<uint32_t>())  << propertyReference.c_str(); }
                    else if (typeId == azrtti_typeid<float>())         { EXPECT_NEAR(propertyA.m_value.GetValue<float>(),     propertyB.m_value.GetValue<float>(), 0.01)   << propertyReference.c_str(); }
                    else if (typeId == azrtti_typeid<Vector2>())       { EXPECT_TRUE(propertyA.m_value.GetValue<Vector2>().IsClose(propertyB.m_value.GetValue<Vector2>())) << propertyReference.c_str(); }
                    else if (typeId == azrtti_typeid<Vector3>())       { EXPECT_TRUE(propertyA.m_value.GetValue<Vector3>().IsClose(propertyB.m_value.GetValue<Vector3>())) << propertyReference.c_str(); }
                    else if (typeId == azrtti_typeid<Vector4>())       { EXPECT_TRUE(propertyA.m_value.GetValue<Vector4>().IsClose(propertyB.m_value.GetValue<Vector4>())) << propertyReference.c_str(); }
                    else if (typeId == azrtti_typeid<Color>())         { EXPECT_TRUE(propertyA.m_value.GetValue<Color>().IsClose(propertyB.m_value.GetValue<Color>())) << propertyReference.c_str(); }
                    else if (typeId == azrtti_typeid<AZStd::string>()) { EXPECT_STREQ(propertyA.m_value.GetValue<AZStd::string>().c_str(), propertyB.m_value.GetValue<AZStd::string>().c_str()) << propertyReference.c_str(); }
                    else
                    {
                        ADD_FAILURE();
                    }
                }
            }
        }

    }

    TEST_F(MaterialSourceDataTests, TestJsonRoundTrip)
    {
        const char* materialTypeJson =
            "{                                                                   \n"
            "    \"propertyLayout\": {                                           \n"
            "        \"version\": 1,                                             \n"
            "        \"groups\": [                                               \n"
            "            { \"name\": \"groupA\" },                               \n"
            "            { \"name\": \"groupB\" },                               \n"
            "            { \"name\": \"groupC\" }                                \n"
            "        ],                                                          \n"
            "        \"properties\": {                                           \n"
            "            \"groupA\": [                                           \n"
            "                {\"name\": \"MyBool\", \"type\": \"bool\"},         \n"
            "                {\"name\": \"MyInt\", \"type\": \"int\"},           \n"
            "                {\"name\": \"MyUInt\", \"type\": \"uint\"}          \n"
            "            ],                                                      \n"
            "            \"groupB\": [                                           \n"
            "                {\"name\": \"MyFloat\", \"type\": \"float\"},       \n"
            "                {\"name\": \"MyFloat2\", \"type\": \"vector2\"},    \n"
            "                {\"name\": \"MyFloat3\", \"type\": \"vector3\"}     \n"
            "            ],                                                      \n"
            "            \"groupC\": [                                           \n"
            "                {\"name\": \"MyFloat4\", \"type\": \"vector4\"},    \n"
            "                {\"name\": \"MyColor\", \"type\": \"color\"},       \n"
            "                {\"name\": \"MyImage\", \"type\": \"image\"}        \n"
            "            ]                                                       \n"
            "        }                                                           \n"
            "    }                                                               \n"
            "}                                                                   \n";

        const char* materialTypeFilePath = "@exefolder@/Gems/Atom/RPI/Code/Tests/Material/Temp/roundTripTest.materialtype";
        
        AZ::IO::FileIOStream file;
        EXPECT_TRUE(file.Open(materialTypeFilePath, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath));
        file.Write(strlen(materialTypeJson), materialTypeJson);
        file.Close();

        MaterialSourceData sourceDataOriginal;
        sourceDataOriginal.m_materialType = materialTypeFilePath;
        sourceDataOriginal.m_parentMaterial = materialTypeFilePath;
        sourceDataOriginal.m_description = "This is a description";
        sourceDataOriginal.m_propertyLayoutVersion = 7;
        AddPropertyGroup(sourceDataOriginal, "groupA");
        AddProperty(sourceDataOriginal, "groupA", "MyBool", true);
        AddProperty(sourceDataOriginal, "groupA", "MyInt", -10);
        AddProperty(sourceDataOriginal, "groupA", "MyUInt", 25u);
        AddPropertyGroup(sourceDataOriginal, "groupB");
        AddProperty(sourceDataOriginal, "groupB", "MyFloat", 1.5f);
        AddProperty(sourceDataOriginal, "groupB", "MyFloat2", AZ::Vector2(2.1f, 2.2f));
        AddProperty(sourceDataOriginal, "groupB", "MyFloat3", AZ::Vector3(3.1f, 3.2f, 3.3f));
        AddPropertyGroup(sourceDataOriginal, "groupC");
        AddProperty(sourceDataOriginal, "groupC", "MyFloat4", AZ::Vector4(4.1f, 4.2f, 4.3f, 4.4f));
        AddProperty(sourceDataOriginal, "groupC", "MyColor", AZ::Color{0.1f, 0.2f, 0.3f, 0.4f});
        AddProperty(sourceDataOriginal, "groupC", "MyImage", AZStd::string("test.streamingimage"));

        AZStd::string sourceDataSerialized;
        JsonTestResult storeResult = StoreTestDataToJson(sourceDataOriginal, sourceDataSerialized);

        MaterialSourceData sourceDataCopy;
        JsonTestResult loadResult = LoadTestDataFromJson(sourceDataCopy, sourceDataSerialized);
        
        CheckEqual(sourceDataOriginal, sourceDataCopy);
    }

    TEST_F(MaterialSourceDataTests, Load_MaterialTypeAfterPropertyList)
    {
        const AZStd::string simpleMaterialTypeJson = R"(
        {
            "propertyLayout": {
                "properties": {
                    "general": [
                        {
                            "name": "testColor",
                            "type": "color"
                        }
                    ]
                }
            }
        } 
        )";

        const char* materialTypeFilePath = "@exefolder@/Gems/Atom/RPI/Code/Tests/Material/Temp/simpleMaterialType.materialtype";

        AZ::IO::FileIOStream file;
        EXPECT_TRUE(file.Open(materialTypeFilePath, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath));
        file.Write(simpleMaterialTypeJson.size(), simpleMaterialTypeJson.data());
        file.Close();

        // It shouldn't matter whether the materialType field appears before the property value list. This allows for the possibility
        // that customer scripts generate material data and happen to use an unexpected order.
        const AZStd::string inputJson = R"(
        {
            "properties": {
                "general": {
                    "testColor": [0.1,0.2,0.3]
                }
            },
            "materialType": "@exefolder@/Gems/Atom/RPI/Code/Tests/Material/Temp/simpleMaterialType.materialtype"
        }
        )";

        MaterialSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        AZ::Color testColor = material.m_properties["general"]["testColor"].m_value.GetValue<AZ::Color>();
        EXPECT_TRUE(AZ::Color(0.1f, 0.2f, 0.3f, 1.0f).IsClose(testColor, 0.01));
    }

    TEST_F(MaterialSourceDataTests, Load_Error_NotAnObject)
    {
        const AZStd::string inputJson = R"(
        []
        )";

        MaterialSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Altered, loadResult.m_jsonResultCode.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::Unsupported, loadResult.m_jsonResultCode.GetOutcome());

        EXPECT_TRUE(loadResult.ContainsMessage("", "Material data must be a JSON object"));
    }

    TEST_F(MaterialSourceDataTests, Load_Error_NoMaterialType)
    {
        const AZStd::string inputJson = R"(
            {
                "propertyLayoutVersion": 1,
                "properties": {
                    "baseColor": {
                        "color": [1.0,1.0,1.0]
                    }
                }
            }
        )";

        MaterialSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Halted, loadResult.m_jsonResultCode.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::Catastrophic, loadResult.m_jsonResultCode.GetOutcome());

        EXPECT_TRUE(loadResult.ContainsMessage("", "Required field 'materialType' is missing"));
    }

    TEST_F(MaterialSourceDataTests, Load_Error_MaterialTypeDoesNotExist)
    {
        const AZStd::string inputJson = R"(
            {
                "materialType": "DoesNotExist.materialtype",
                "propertyLayoutVersion": 1,
                "properties": {
                    "baseColor": {
                        "color": [1.0,1.0,1.0]
                    }
                }
            }
        )";

        MaterialSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Halted, loadResult.m_jsonResultCode.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::Catastrophic, loadResult.m_jsonResultCode.GetOutcome());

        EXPECT_TRUE(loadResult.ContainsMessage("/materialType", "Failed to load material-type file"));
    }

    TEST_F(MaterialSourceDataTests, Load_MaterialTypeMessagesAreReported)
    {
        const AZStd::string simpleMaterialTypeJson = R"(
        {
            "propertyLayout": {
                "properties": {
                    "general": [
                        {
                            "name": "testColor",
                            "type": "color"
                        }
                    ]
                }
            }
        } 
        )";

        const char* materialTypeFilePath = "@exefolder@/Gems/Atom/RPI/Code/Tests/Material/Temp/simpleMaterialType.materialtype";

        AZ::IO::FileIOStream file;
        EXPECT_TRUE(file.Open(materialTypeFilePath, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath));
        file.Write(simpleMaterialTypeJson.size(), simpleMaterialTypeJson.data());
        file.Close();

        const AZStd::string inputJson = R"(
        {
            "materialType": "@exefolder@/Gems/Atom/RPI/Code/Tests/Material/Temp/simpleMaterialType.materialtype",
            "propertyLayoutVersion": 1,
            "properties": {
                "general": {
                    "testColor": [1.0,1.0,1.0]
                }
            }
        }
        )";

        MaterialSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        // propertyLayout is a field in the material type, not the material
        EXPECT_TRUE(loadResult.ContainsMessage("[simpleMaterialType.materialtype]/propertyLayout/properties", "Successfully read"));
    }

    TEST_F(MaterialSourceDataTests, Load_Error_PropertyNotFound)
    {
        const AZStd::string simpleMaterialTypeJson = R"(
        {
            "propertyLayout": {
                "properties": {
                    "general": [
                        {
                            "name": "testColor",
                            "type": "color"
                        }
                    ]
                }
            }
        } 
        )";

        const char* materialTypeFilePath = "@exefolder@/Gems/Atom/RPI/Code/Tests/Material/Temp/simpleMaterialType.materialtype";

        AZ::IO::FileIOStream file;
        EXPECT_TRUE(file.Open(materialTypeFilePath, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath));
        file.Write(simpleMaterialTypeJson.size(), simpleMaterialTypeJson.data());
        file.Close();

        const AZStd::string inputJson = R"(
        {
            "materialType": "@exefolder@/Gems/Atom/RPI/Code/Tests/Material/Temp/simpleMaterialType.materialtype",
            "propertyLayoutVersion": 1,
            "properties": {
                "general": {
                    "doesNotExist": [1.0,1.0,1.0]
                }
            }
        }
        )";

        MaterialSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::PartialAlter, loadResult.m_jsonResultCode.GetProcessing());

        EXPECT_TRUE(loadResult.ContainsMessage("/properties/general/doesNotExist", "Property 'general.doesNotExist' not found in material type."));
    }

    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_MultiLevelDataInheritance)
    {
        MaterialSourceData sourceDataLevel1;
        sourceDataLevel1.m_materialType = "test.materialtype";
        AddPropertyGroup(sourceDataLevel1, "general");
        AddProperty(sourceDataLevel1, "general", "MyFloat", 1.5f);
        AddProperty(sourceDataLevel1, "general", "MyColor", AZ::Color{0.1f, 0.2f, 0.3f, 0.4f});

        MaterialSourceData sourceDataLevel2;
        sourceDataLevel2.m_materialType = "test.materialtype";
        sourceDataLevel2.m_parentMaterial = "level1.material";
        AddPropertyGroup(sourceDataLevel2, "general");
        AddProperty(sourceDataLevel2, "general", "MyColor", AZ::Color{0.15f, 0.25f, 0.35f, 0.45f});
        AddProperty(sourceDataLevel2, "general", "MyFloat2", AZ::Vector2{4.1f, 4.2f});

        MaterialSourceData sourceDataLevel3;
        sourceDataLevel3.m_materialType = "test.materialtype";
        sourceDataLevel3.m_parentMaterial = "level2.material";
        AddPropertyGroup(sourceDataLevel3, "general");
        AddProperty(sourceDataLevel3, "general", "MyFloat", 3.5f);

        auto materialAssetLevel1 = sourceDataLevel1.CreateMaterialAsset(Uuid::CreateRandom(), "", true);
        EXPECT_TRUE(materialAssetLevel1.IsSuccess());

        m_assetSystemStub.RegisterSourceInfo("level1.material", materialAssetLevel1.GetValue().GetId());

        auto materialAssetLevel2 = sourceDataLevel2.CreateMaterialAsset(Uuid::CreateRandom(), "", true);
        EXPECT_TRUE(materialAssetLevel2.IsSuccess());

        m_assetSystemStub.RegisterSourceInfo("level2.material", materialAssetLevel2.GetValue().GetId());

        auto materialAssetLevel3 = sourceDataLevel3.CreateMaterialAsset(Uuid::CreateRandom(), "", true);
        EXPECT_TRUE(materialAssetLevel3.IsSuccess());
        
        auto layout = m_testMaterialTypeAsset->GetMaterialPropertiesLayout();
        MaterialPropertyIndex myFloat = layout->FindPropertyIndex(Name("general.MyFloat"));
        MaterialPropertyIndex myFloat2 = layout->FindPropertyIndex(Name("general.MyFloat2"));
        MaterialPropertyIndex myColor = layout->FindPropertyIndex(Name("general.MyColor"));

        AZStd::array_view<MaterialPropertyValue> properties;

        // Check level 1 properties
        properties = materialAssetLevel1.GetValue()->GetPropertyValues();
        EXPECT_EQ(properties[myFloat.GetIndex()].GetValue<float>(), 1.5f);
        EXPECT_EQ(properties[myFloat2.GetIndex()].GetValue<Vector2>(), Vector2(0.0f, 0.0f));
        EXPECT_EQ(properties[myColor.GetIndex()].GetValue<Color>(), Color(0.1f, 0.2f, 0.3f, 0.4f));

        // Check level 2 properties
        properties = materialAssetLevel2.GetValue()->GetPropertyValues();
        EXPECT_EQ(properties[myFloat.GetIndex()].GetValue<float>(), 1.5f);
        EXPECT_EQ(properties[myFloat2.GetIndex()].GetValue<Vector2>(), Vector2(4.1f, 4.2f));
        EXPECT_EQ(properties[myColor.GetIndex()].GetValue<Color>(), Color(0.15f, 0.25f, 0.35f, 0.45f));

        // Check level 3 properties
        properties = materialAssetLevel3.GetValue()->GetPropertyValues();
        EXPECT_EQ(properties[myFloat.GetIndex()].GetValue<float>(), 3.5f);
        EXPECT_EQ(properties[myFloat2.GetIndex()].GetValue<Vector2>(), Vector2(4.1f, 4.2f));
        EXPECT_EQ(properties[myColor.GetIndex()].GetValue<Color>(), Color(0.15f, 0.25f, 0.35f, 0.45f));
    }

    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_MultiLevelDataInheritance_Error_MaterialTypesDontMatch)
    {
        Data::Asset<MaterialTypeAsset> otherMaterialType;
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(m_testShaderAsset);
        AddCommonTestMaterialProperties(materialTypeCreator, "general.");
        EXPECT_TRUE(materialTypeCreator.End(otherMaterialType));
        m_assetSystemStub.RegisterSourceInfo("otherBase.materialtype", otherMaterialType.GetId());

        MaterialSourceData sourceDataLevel1;
        sourceDataLevel1.m_materialType = "test.materialtype";

        MaterialSourceData sourceDataLevel2;
        sourceDataLevel2.m_materialType = "test.materialtype";
        sourceDataLevel2.m_parentMaterial = "level1.material";

        MaterialSourceData sourceDataLevel3;
        sourceDataLevel3.m_materialType = "otherBase.materialtype";
        sourceDataLevel3.m_parentMaterial = "level2.material";

        auto materialAssetLevel1 = sourceDataLevel1.CreateMaterialAsset(Uuid::CreateRandom(), "", true);
        EXPECT_TRUE(materialAssetLevel1.IsSuccess());

        m_assetSystemStub.RegisterSourceInfo("level1.material", materialAssetLevel1.GetValue().GetId());

        auto materialAssetLevel2 = sourceDataLevel2.CreateMaterialAsset(Uuid::CreateRandom(), "", true);
        EXPECT_TRUE(materialAssetLevel2.IsSuccess());

        m_assetSystemStub.RegisterSourceInfo("level2.material", materialAssetLevel2.GetValue().GetId());

        AZ_TEST_START_ASSERTTEST;
        auto materialAssetLevel3 = sourceDataLevel3.CreateMaterialAsset(Uuid::CreateRandom(), "", true);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_FALSE(materialAssetLevel3.IsSuccess());
    }

    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_Error_BadInput)
    {
        // We use local functions to easily start a new MaterialAssetCreator for each test case because
        // the AssetCreator would just skip subsequent operations after the first failure is detected.

        auto expectError = [](AZStd::function<void(MaterialSourceData& materialSourceData)> setOneBadInput, [[maybe_unused]] uint32_t expectedAsserts = 2)
        {
            MaterialSourceData sourceData;

            sourceData.m_materialType = "test.materialtype";

            AddPropertyGroup(sourceData, "general");

            setOneBadInput(sourceData);

            AZ_TEST_START_ASSERTTEST;
            auto materialAssetOutcome = sourceData.CreateMaterialAsset(Uuid::CreateRandom(), "", false);
            AZ_TEST_STOP_ASSERTTEST(expectedAsserts); // Usually one for the initial error, and one for when End() is called

            EXPECT_FALSE(materialAssetOutcome.IsSuccess());
        };

        auto expectWarning = [](AZStd::function<void(MaterialSourceData& materialSourceData)> setOneBadInput, [[maybe_unused]] uint32_t expectedAsserts = 1)
        {
            MaterialSourceData sourceData;

            sourceData.m_materialType = "test.materialtype";

            AddPropertyGroup(sourceData, "general");

            setOneBadInput(sourceData);

            AZ_TEST_START_ASSERTTEST;
            auto materialAssetOutcome = sourceData.CreateMaterialAsset(Uuid::CreateRandom(), "", true);
            AZ_TEST_STOP_ASSERTTEST(expectedAsserts); // Usually just one for when End() is called

            EXPECT_FALSE(materialAssetOutcome.IsSuccess());
        };

        // Test property does not exist...

        expectWarning([](MaterialSourceData& materialSourceData)
        {
            AddProperty(materialSourceData, "general", "DoesNotExist", true);
        });

        expectWarning([](MaterialSourceData& materialSourceData)
        {
            AddProperty(materialSourceData, "general", "DoesNotExist", -10);
        });

        expectWarning([](MaterialSourceData& materialSourceData)
        {
            AddProperty(materialSourceData, "general", "DoesNotExist", 25u);
        });

        expectWarning([](MaterialSourceData& materialSourceData)
        {
            AddProperty(materialSourceData, "general", "DoesNotExist", 1.5f);
        });

        expectWarning([](MaterialSourceData& materialSourceData)
        {
            AddProperty(materialSourceData, "general", "DoesNotExist", AZ::Color{ 0.1f, 0.2f, 0.3f, 0.4f });
        });

        expectWarning([](MaterialSourceData& materialSourceData)
        {
            AddProperty(materialSourceData, "general", "DoesNotExist", AZStd::string("test.streamingimage"));
        });

        // Missing image reference
        expectError([](MaterialSourceData& materialSourceData)
        {
            AddProperty(materialSourceData, "general", "MyImage", AZStd::string("doesNotExist.streamingimage"));
        }, 3); // Expect a 3rd error because AssetUtils reports its own assertion failure
    }
}


