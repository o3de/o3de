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
#include <Common/ErrorMessageFinder.h>
#include <Common/SerializeTester.h>
#include <Material/MaterialAssetTestUtils.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>

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
            MaterialPropertySourceData::Reflect(context);
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
            EXPECT_NE(nullptr, m_testMaterialSrgLayout);

            m_testShaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout);
            EXPECT_TRUE(m_testShaderAsset.GetId().IsValid());
            EXPECT_TRUE(m_testShaderAsset.IsReady());

            m_assetSystemStub.RegisterSourceInfo(DeAliasPath("@exefolder@/Temp/test.shader"), m_testShaderAsset.GetId());

            m_testMaterialTypeAsset = CreateTestMaterialTypeAsset(Uuid::CreateRandom());
            EXPECT_TRUE(m_testMaterialTypeAsset.GetId().IsValid());
            EXPECT_TRUE(m_testMaterialTypeAsset.IsReady());

            // Since this test doesn't actually instantiate a Material, it won't need to instantiate this ImageAsset, so all we
            // need is an asset reference with a valid ID.
            m_testImageAsset = Data::Asset<ImageAsset>{ Data::AssetId{Uuid::CreateRandom(), StreamingImageAsset::GetImageAssetSubId()}, azrtti_typeid<StreamingImageAsset>() };
            EXPECT_TRUE(m_testImageAsset.GetId().IsValid());

            // Register the test assets with the AssetSystemStub so CreateMaterialAsset() can use AssetUtils.
            m_assetSystemStub.RegisterSourceInfo(DeAliasPath("@exefolder@/Temp/test.materialtype"), m_testMaterialTypeAsset.GetId());
            m_assetSystemStub.RegisterSourceInfo(DeAliasPath("@exefolder@/Temp/test.streamingimage"), m_testImageAsset.GetId());
        }

        void TearDown() override
        {
            m_testMaterialTypeAsset.Reset();
            m_testMaterialSrgLayout = nullptr;
            m_testShaderAsset.Reset();
            m_testImageAsset.Reset();

            RPITestFixture::TearDown();
        }

        AZStd::string DeAliasPath(const AZStd::string& sourcePath) const
        {
            AZ::IO::FixedMaxPath sourcePathNoAlias;
            AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(sourcePathNoAlias, AZ::IO::PathView{ sourcePath });
            return sourcePathNoAlias.LexicallyNormal().String();
        }

        AZStd::string GetTestMaterialTypeJson()
        {
            const char* materialTypeJson = R"(
                    {
                        "version": 10,
                        "propertyLayout": {
                            "propertyGroups": [
                                {
                                    "name": "general",
                                    "properties": [
                                        {"name": "MyBool", "type": "bool"},
                                        {"name": "MyInt", "type": "Int"},
                                        {"name": "MyUInt", "type": "UInt"},
                                        {"name": "MyFloat", "type": "Float"},
                                        {"name": "MyFloat2", "type": "Vector2"},
                                        {"name": "MyFloat3", "type": "Vector3"},
                                        {"name": "MyFloat4", "type": "Vector4"},
                                        {"name": "MyColor", "type": "Color"},
                                        {"name": "MyImage", "type": "Image"},
                                        {"name": "MyEnum", "type": "Enum", "enumValues": ["Enum0", "Enum1", "Enum2"], "defaultValue": "Enum0"}
                                    ]
                                }
                            ]
                        },
                        "shaders": [
                            {
                                "file": "@exefolder@/Temp/test.shader"
                            }
                        ],
                        "versionUpdates": [
                            {
                                "toVersion": 2,
                                "actions": [
                                    {"op": "rename", "from": "general.testColorNameA", "to": "general.testColorNameB"}
                                ]
                            },
                            {
                                "toVersion": 4,
                                "actions": [
                                    {"op": "rename", "from": "general.testColorNameB", "to": "general.testColorNameC"}
                                ]
                            },
                            {
                                "toVersion": 6,
                                "actions": [
                                    {"op": "rename", "from": "oldGroup.MyFloat", "to": "general.MyFloat"},
                                    {"op": "rename", "from": "oldGroup.MyIntOldName", "to": "general.MyInt"}
                                ]
                            },
                            {
                                "toVersion": 10,
                                "actions": [
                                    {"op": "rename", "from": "general.testColorNameC", "to": "general.MyColor"}
                                ]
                            }
                        ]
                    }
                )";

            return materialTypeJson;
        }

        Data::Asset<MaterialTypeAsset> CreateTestMaterialTypeAsset(Data::AssetId assetId)
        {
            MaterialTypeSourceData materialTypeSourceData;
            LoadTestDataFromJson(materialTypeSourceData, GetTestMaterialTypeJson());
            return materialTypeSourceData.CreateMaterialTypeAsset(assetId).TakeValue();
        }
    };

    void AddPropertyGroup(MaterialSourceData&, AZStd::string_view)
    {
        // Old function, left blank intentionally
    }

    void AddProperty(MaterialSourceData& material, AZStd::string_view groupName, AZStd::string_view propertyName, const MaterialPropertyValue& value)
    {
        MaterialPropertyId id{groupName, propertyName};
        material.SetPropertyValue(id, value);
    }

    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_BasicProperties)
    {
        MaterialSourceData sourceData;

        sourceData.m_materialType = "@exefolder@/Temp/test.materialtype";
        AddPropertyGroup(sourceData, "general");
        AddProperty(sourceData, "general", "MyBool", true);
        AddProperty(sourceData, "general", "MyInt", -10);
        AddProperty(sourceData, "general", "MyUInt", 25u);
        AddProperty(sourceData, "general", "MyFloat", 1.5f);
        AddProperty(sourceData, "general", "MyColor", AZ::Color{0.1f, 0.2f, 0.3f, 0.4f});
        AddProperty(sourceData, "general", "MyFloat2", AZ::Vector2(2.1f, 2.2f));
        AddProperty(sourceData, "general", "MyFloat3", AZ::Vector3(3.1f, 3.2f, 3.3f));
        AddProperty(sourceData, "general", "MyFloat4", AZ::Vector4(4.1f, 4.2f, 4.3f, 4.4f));
        AddProperty(sourceData, "general", "MyImage", AZStd::string("@exefolder@/Temp/test.streamingimage"));
        AddProperty(sourceData, "general", "MyEnum", AZStd::string("Enum1"));

        auto materialAssetOutcome = sourceData.CreateMaterialAsset(Uuid::CreateRandom(), "", true);
        EXPECT_TRUE(materialAssetOutcome.IsSuccess());

        Data::Asset<MaterialAsset> materialAsset = materialAssetOutcome.GetValue();

        // The order here is based on the order in the MaterialTypeSourceData, as added to the MaterialTypeAssetCreator.
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
    
    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_VersionUpdate_ReportTheSpecifiedMaterialTypeVersion)
    {
        // This is in response to a specific issue where the material type version update reported the wrong version
        // because MaterialSourceData was not feeding it to the MaterialAsset 

        AZ::Utils::WriteFile(GetTestMaterialTypeJson(), "@exefolder@/Temp/test.materialtype");

        MaterialSourceData sourceData;

        sourceData.m_materialType = "@exefolder@/Temp/test.materialtype";
        sourceData.m_materialTypeVersion = 5;
        AddPropertyGroup(sourceData, "oldGroup");
        AddProperty(sourceData, "oldGroup", "MyFloat", 1.2f);

        ErrorMessageFinder findVersionWarning;
        findVersionWarning.AddExpectedErrorMessage("This material is based on version '5'");
        findVersionWarning.AddExpectedErrorMessage("the material type is now at version '10'");
        findVersionWarning.AddExpectedErrorMessage("Consider updating the .material source file");

        findVersionWarning.ResetCounts();
        sourceData.CreateMaterialAsset(Uuid::CreateRandom(), "");
        findVersionWarning.CheckExpectedErrorsFound();
        
        findVersionWarning.ResetCounts();
        sourceData.CreateMaterialAssetFromSourceData(Uuid::CreateRandom());
        findVersionWarning.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_VersionUpdate_ReportUnspecifiedMaterialTypeVersion)
    {
        // This is in response to a specific issue where the material type version update reported the wrong version
        // because MaterialSourceData was not feeding it to the MaterialAsset.
        // It's the same as CreateMaterialAsset_VersionUpdate_ReportTheSpecifiedMaterialTypeVersion except it looks
        // for "<Unspecified>" in the warning message.

        AZ::Utils::WriteFile(GetTestMaterialTypeJson(), "@exefolder@/Temp/test.materialtype");

        MaterialSourceData sourceData;

        sourceData.m_materialType = "@exefolder@/Temp/test.materialtype";
        // We intentionally do not set sourceData.m_materialTypeVersion here
        AddPropertyGroup(sourceData, "oldGroup");
        AddProperty(sourceData, "oldGroup", "MyFloat", 1.2f);

        ErrorMessageFinder findVersionWarning;
        findVersionWarning.AddExpectedErrorMessage("This material is based on version <Unspecified>");
        findVersionWarning.AddExpectedErrorMessage("the material type is now at version '10'");
        findVersionWarning.AddExpectedErrorMessage("Consider updating the .material source file");

        findVersionWarning.ResetCounts();
        sourceData.CreateMaterialAsset(Uuid::CreateRandom(), "");
        findVersionWarning.CheckExpectedErrorsFound();
        
        findVersionWarning.ResetCounts();
        sourceData.CreateMaterialAssetFromSourceData(Uuid::CreateRandom());
        findVersionWarning.CheckExpectedErrorsFound();
    }

    // Can return a Vector4 or a Color as a Vector4
    Vector4 GetAsVector4(const MaterialPropertyValue& value)
    {
        if (value.GetTypeId() == azrtti_typeid<Vector4>())
        {
            return value.GetValue<Vector4>();
        }
        else if (value.GetTypeId() == azrtti_typeid<Color>())
        {
            return value.GetValue<Color>().GetAsVector4();
        }
        else
        {
            return Vector4::CreateZero();
        }
    }
    
    // Can return a Int or a UInt as a Int
    int32_t GetAsInt(const MaterialPropertyValue& value)
    {
        if (value.GetTypeId() == azrtti_typeid<int32_t>())
        {
            return value.GetValue<int32_t>();
        }
        else if (value.GetTypeId() == azrtti_typeid<uint32_t>())
        {
            return aznumeric_cast<int32_t>(value.GetValue<uint32_t>());
        }
        else
        {
            return 0;
        }
    }

    template<typename TargetTypeT>
    bool AreTypesCompatible(const MaterialPropertyValue& a, const MaterialPropertyValue& b)
    {
        auto fixupType = [](TypeId t)
        {
            if (t == azrtti_typeid<uint32_t>())
            {
                return azrtti_typeid<int32_t>();
            }

            if (t == azrtti_typeid<Color>())
            {
                return azrtti_typeid<Vector4>();
            }

            return t;
        };

        TypeId targetTypeId = azrtti_typeid<TargetTypeT>();

        return fixupType(a.GetTypeId()) == fixupType(targetTypeId) && fixupType(b.GetTypeId()) == fixupType(targetTypeId);
    }

    void CheckEqual(const MaterialSourceData& a, const MaterialSourceData& b)
    {
        EXPECT_STREQ(a.m_materialType.c_str(), b.m_materialType.c_str());
        EXPECT_STREQ(a.m_description.c_str(), b.m_description.c_str());
        EXPECT_STREQ(a.m_parentMaterial.c_str(), b.m_parentMaterial.c_str());
        EXPECT_EQ(a.m_materialTypeVersion, b.m_materialTypeVersion);

        EXPECT_EQ(a.GetPropertyValues().size(), b.GetPropertyValues().size());

        for (auto& [propertyId, propertyValue] : a.GetPropertyValues())
        {
            if (!b.HasPropertyValue(propertyId))
            {
                EXPECT_TRUE(false) << "Property '" << propertyId.GetCStr() << "' not found in material B";
                continue;
            }

            auto& propertyA = propertyValue;
            auto& propertyB = b.GetPropertyValue(propertyId);

            AZStd::string propertyReference = AZStd::string::format(" for property '%s'", propertyId.GetCStr());
                
            // We allow some types like Vector4 and Color or Int and UInt to be interchangeable since they serialize the same and can be converted when the MaterialAsset is finalized.

            if (AreTypesCompatible<bool>(propertyA, propertyB))
            {
                EXPECT_EQ(propertyA.GetValue<bool>(), propertyB.GetValue<bool>()) << propertyReference.c_str();
            }
            else if (AreTypesCompatible<int32_t>(propertyA, propertyB))
            {
                EXPECT_EQ(GetAsInt(propertyA), GetAsInt(propertyB)) << propertyReference.c_str();
            }
            else if (AreTypesCompatible<float>(propertyA, propertyB))
            {
                EXPECT_NEAR(propertyA.GetValue<float>(),     propertyB.GetValue<float>(), 0.01) << propertyReference.c_str();
            }
            else if (AreTypesCompatible<Vector2>(propertyA, propertyB))
            {
                EXPECT_TRUE(propertyA.GetValue<Vector2>().IsClose(propertyB.GetValue<Vector2>())) << propertyReference.c_str();
            }
            else if (AreTypesCompatible<Vector3>(propertyA, propertyB))
            {
                EXPECT_TRUE(propertyA.GetValue<Vector3>().IsClose(propertyB.GetValue<Vector3>())) << propertyReference.c_str();
            }
            else if (AreTypesCompatible<Vector4>(propertyA, propertyB))
            {
                EXPECT_TRUE(GetAsVector4(propertyA).IsClose(GetAsVector4(propertyB))) << propertyReference.c_str();
            }
            else if (AreTypesCompatible<AZStd::string>(propertyA, propertyB))
            {
                EXPECT_STREQ(propertyA.GetValue<AZStd::string>().c_str(), propertyB.GetValue<AZStd::string>().c_str()) << propertyReference.c_str();
            }
            else
            {
                ADD_FAILURE();
            }
        }
    }

    TEST_F(MaterialSourceDataTests, TestJsonRoundTrip)
    {
        const char* materialTypeFilePath = "@exefolder@/Temp/roundTripTest.materialtype";

        MaterialSourceData sourceDataOriginal;
        sourceDataOriginal.m_materialType = materialTypeFilePath;
        sourceDataOriginal.m_parentMaterial = materialTypeFilePath;
        sourceDataOriginal.m_description = "This is a description";
        sourceDataOriginal.m_materialTypeVersion = 7;
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
        AddProperty(sourceDataOriginal, "groupC", "MyImage", AZStd::string("@exefolder@/Temp/test.streamingimage"));

        AZStd::string sourceDataSerialized;
        JsonTestResult storeResult = StoreTestDataToJson(sourceDataOriginal, sourceDataSerialized);

        MaterialSourceData sourceDataCopy;
        JsonTestResult loadResult = LoadTestDataFromJson(sourceDataCopy, sourceDataSerialized);

        CheckEqual(sourceDataOriginal, sourceDataCopy);
    }
    
    TEST_F(MaterialSourceDataTests, TestLoadLegacyFormat)
    {
        const AZStd::string inputJson = R"(
            {
                "materialType": "test.materialtype", // Doesn't matter, this isn't loaded
                "properties": {
                    "groupA": {
                        "myBool": true,
                        "myInt": 5,
                        "myFloat": 0.5
                    },
                    "groupB": {
                        "myFloat2": [0.1, 0.2],
                        "myFloat3": [0.3, 0.4, 0.5],
                        "myFloat4": [0.6, 0.7, 0.8, 0.9],
                        "myString": "Hello"
                    }
                }
            }
        )";

        MaterialSourceData material;
        LoadTestDataFromJson(material, inputJson);
        material.UpgradeLegacyFormat();

        MaterialSourceData expectedMaterial;
        expectedMaterial.m_materialType = "test.materialtype";
        expectedMaterial.SetPropertyValue(Name{"groupA.myBool"}, true);
        expectedMaterial.SetPropertyValue(Name{"groupA.myInt"}, 5);
        expectedMaterial.SetPropertyValue(Name{"groupA.myFloat"}, 0.5f);
        expectedMaterial.SetPropertyValue(Name{"groupB.myFloat2"}, Vector2(0.1f, 0.2f));
        expectedMaterial.SetPropertyValue(Name{"groupB.myFloat3"}, Vector3(0.3f, 0.4f, 0.5f));
        expectedMaterial.SetPropertyValue(Name{"groupB.myFloat4"}, Vector4(0.6f, 0.7f, 0.8f, 0.9f));
        expectedMaterial.SetPropertyValue(Name{"groupB.myString"}, AZStd::string{"Hello"});

        CheckEqual(expectedMaterial, material);
    }
    
    TEST_F(MaterialSourceDataTests, TestPropertyValues)
    {
        MaterialSourceData material;
        
        Name foo{"foo"};
        Name bar{"bar"};
        Name baz{"baz"};

        EXPECT_EQ(0, material.GetPropertyValues().size());
        EXPECT_FALSE(material.HasPropertyValue(foo));
        EXPECT_FALSE(material.HasPropertyValue(bar));
        EXPECT_FALSE(material.HasPropertyValue(baz));
        EXPECT_FALSE(material.GetPropertyValue(foo).IsValid());
        EXPECT_FALSE(material.GetPropertyValue(bar).IsValid());
        EXPECT_FALSE(material.GetPropertyValue(baz).IsValid());

        material.SetPropertyValue(Name{"foo"}, 2);
        material.SetPropertyValue(Name{"bar"}, true);
        material.SetPropertyValue(Name{"baz"}, 0.5f);
        
        EXPECT_EQ(3, material.GetPropertyValues().size());
        EXPECT_TRUE(material.HasPropertyValue(foo));
        EXPECT_TRUE(material.HasPropertyValue(bar));
        EXPECT_TRUE(material.HasPropertyValue(baz));
        EXPECT_TRUE(material.GetPropertyValue(foo).IsValid());
        EXPECT_TRUE(material.GetPropertyValue(bar).IsValid());
        EXPECT_TRUE(material.GetPropertyValue(baz).IsValid());
        EXPECT_EQ(material.GetPropertyValue(foo).GetValue<int32_t>(), 2);
        EXPECT_EQ(material.GetPropertyValue(bar).GetValue<bool>(), true);
        EXPECT_EQ(material.GetPropertyValue(baz).GetValue<float>(), 0.5f);

        material.RemovePropertyValue(bar);
        
        EXPECT_EQ(2, material.GetPropertyValues().size());
        EXPECT_TRUE(material.HasPropertyValue(foo));
        EXPECT_FALSE(material.HasPropertyValue(bar));
        EXPECT_TRUE(material.HasPropertyValue(baz));
        EXPECT_TRUE(material.GetPropertyValue(foo).IsValid());
        EXPECT_FALSE(material.GetPropertyValue(bar).IsValid());
        EXPECT_TRUE(material.GetPropertyValue(baz).IsValid());
        EXPECT_EQ(material.GetPropertyValue(foo).GetValue<int32_t>(), 2);
        EXPECT_EQ(material.GetPropertyValue(baz).GetValue<float>(), 0.5f);
    }

    TEST_F(MaterialSourceDataTests, Load_MaterialTypeAfterPropertyList)
    {
        const AZStd::string simpleMaterialTypeJson = R"(
            {
                "propertyLayout": {
                    "propertyGroups":
                    [
                        {
                            "name": "general",
                            "properties": [
                                {
                                    "name": "testValue",
                                    "type": "Float"
                                }
                            ]
                        }
                    ]
                }
            }
        )";

        AZ::Utils::WriteFile(simpleMaterialTypeJson, "@exefolder@/Temp/simpleMaterialType.materialtype");

        // It shouldn't matter whether the materialType field appears before the property value list. This allows for the possibility
        // that customer scripts generate material data and happen to use an unexpected order.
        const AZStd::string inputJson = R"(
        {
            "propertyValues": {
                "general.testValue": 1.2
            },
            "materialType": "@exefolder@/Temp/simpleMaterialType.materialtype"
        }
        )";

        MaterialSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        EXPECT_EQ(AZ::JsonSerializationResult::Tasks::ReadField, loadResult.m_jsonResultCode.GetTask());
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, loadResult.m_jsonResultCode.GetProcessing());

        float testValue = material.GetPropertyValue(Name{"general.testValue"}).GetValue<float>();
        EXPECT_FLOAT_EQ(1.2f, testValue);
    }
    
    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_NoMaterialType)
    {
        const AZStd::string inputJson = R"(
            {
                "materialTypeVersion": 1,
                "propertyValues": {
                    "baseColor.color": [1.0,1.0,1.0]
                }
            }
        )";

        MaterialSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        const bool elevateWarnings = false;

        ErrorMessageFinder errorMessageFinder;

        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("materialType was not specified");
        auto result = material.CreateMaterialAsset(AZ::Uuid::CreateRandom(), "test.material", elevateWarnings);
        EXPECT_FALSE(result.IsSuccess());
        errorMessageFinder.CheckExpectedErrorsFound();
        
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("materialType was not specified");
        result = material.CreateMaterialAssetFromSourceData(AZ::Uuid::CreateRandom(), "test.material", elevateWarnings);
        EXPECT_FALSE(result.IsSuccess());
        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_MaterialTypeDoesNotExist)
    {
        const AZStd::string inputJson = R"(
            {
                "materialType": "DoesNotExist.materialtype",
                "materialTypeVersion": 1,
                "propertyValues": {
                    "baseColor.color": [1.0,1.0,1.0]
                }
            }
        )";

        MaterialSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        const bool elevateWarnings = false;

        ErrorMessageFinder errorMessageFinder;

        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("Could not find asset for source file [DoesNotExist.materialtype]");
        auto result = material.CreateMaterialAsset(AZ::Uuid::CreateRandom(), "test.material", elevateWarnings);
        EXPECT_FALSE(result.IsSuccess());
        errorMessageFinder.CheckExpectedErrorsFound();
        
        errorMessageFinder.Reset();
        errorMessageFinder.AddExpectedErrorMessage("Could not find asset for source file [DoesNotExist.materialtype]");
        errorMessageFinder.AddIgnoredErrorMessage("Could not find material type file", true);
        errorMessageFinder.AddIgnoredErrorMessage("Failed to create material type asset ID", true);
        result = material.CreateMaterialAssetFromSourceData(AZ::Uuid::CreateRandom(), "test.material", elevateWarnings);
        EXPECT_FALSE(result.IsSuccess());
        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_MaterialPropertyNotFound)
    {
        MaterialSourceData material;
        material.m_materialType = "@exefolder@/Temp/test.materialtype";
        AddPropertyGroup(material, "general");
        AddProperty(material, "general", "FieldDoesNotExist", 1.5f);
        
        const bool elevateWarnings = true;

        ErrorMessageFinder errorMessageFinder("\"general.FieldDoesNotExist\" is not found");
        errorMessageFinder.AddIgnoredErrorMessage("Failed to build MaterialAsset", true);
        auto result = material.CreateMaterialAsset(AZ::Uuid::CreateRandom(), "test.material", elevateWarnings);
        EXPECT_FALSE(result.IsSuccess());
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialSourceDataTests, CreateMaterialAsset_MultiLevelDataInheritance)
    {
        MaterialSourceData sourceDataLevel1;
        sourceDataLevel1.m_materialType = "@exefolder@/Temp/test.materialtype";
        AddPropertyGroup(sourceDataLevel1, "general");
        AddProperty(sourceDataLevel1, "general", "MyFloat", 1.5f);
        AddProperty(sourceDataLevel1, "general", "MyColor", AZ::Color{0.1f, 0.2f, 0.3f, 0.4f});

        MaterialSourceData sourceDataLevel2;
        sourceDataLevel2.m_materialType = "@exefolder@/Temp/test.materialtype";
        sourceDataLevel2.m_parentMaterial = "level1.material";
        AddPropertyGroup(sourceDataLevel2, "general");
        AddProperty(sourceDataLevel2, "general", "MyColor", AZ::Color{0.15f, 0.25f, 0.35f, 0.45f});
        AddProperty(sourceDataLevel2, "general", "MyFloat2", AZ::Vector2{4.1f, 4.2f});

        MaterialSourceData sourceDataLevel3;
        sourceDataLevel3.m_materialType = "@exefolder@/Temp/test.materialtype";
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

        AZStd::span<const MaterialPropertyValue> properties;

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
        sourceDataLevel1.m_materialType = "@exefolder@/Temp/test.materialtype";

        MaterialSourceData sourceDataLevel2;
        sourceDataLevel2.m_materialType = "@exefolder@/Temp/test.materialtype";
        sourceDataLevel2.m_parentMaterial = "level1.material";

        MaterialSourceData sourceDataLevel3;
        sourceDataLevel3.m_materialType = "@exefolder@/Temp/otherBase.materialtype";
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

        auto expectWarning = [](const char* expectedErrorMessage, const char* secondExpectedErrorMessage, AZStd::function<void(MaterialSourceData& materialSourceData)> setOneBadInput)
        {
            MaterialSourceData sourceData;

            sourceData.m_materialType = "@exefolder@/Temp/test.materialtype";

            AddPropertyGroup(sourceData, "general");

            setOneBadInput(sourceData);

            ErrorMessageFinder errorFinder;
            errorFinder.AddExpectedErrorMessage(expectedErrorMessage);
            if (secondExpectedErrorMessage)
            {
                errorFinder.AddExpectedErrorMessage(secondExpectedErrorMessage);
            }
            errorFinder.AddIgnoredErrorMessage("Failed to build", true);
            auto materialAssetOutcome = sourceData.CreateMaterialAsset(Uuid::CreateRandom(), "", true);
            errorFinder.CheckExpectedErrorsFound();

            EXPECT_FALSE(materialAssetOutcome.IsSuccess());
        };

        // Test property does not exist...

        expectWarning("\"general.DoesNotExist\" is not found in the material properties layout", nullptr,
            [](MaterialSourceData& materialSourceData)
            {
                AddProperty(materialSourceData, "general", "DoesNotExist", true);
            });

        expectWarning("\"general.DoesNotExist\" is not found in the material properties layout", nullptr,
            [](MaterialSourceData& materialSourceData)
            {
                AddProperty(materialSourceData, "general", "DoesNotExist", -10);
            });

        expectWarning("\"general.DoesNotExist\" is not found in the material properties layout", nullptr,
            [](MaterialSourceData& materialSourceData)
            {
                AddProperty(materialSourceData, "general", "DoesNotExist", 25u);
            });

        expectWarning("\"general.DoesNotExist\" is not found in the material properties layout", nullptr,
            [](MaterialSourceData& materialSourceData)
            {
                AddProperty(materialSourceData, "general", "DoesNotExist", 1.5f);
            });

        expectWarning("\"general.DoesNotExist\" is not found in the material properties layout", nullptr,
            [](MaterialSourceData& materialSourceData)
            {
                AddProperty(materialSourceData, "general", "DoesNotExist", AZ::Color{ 0.1f, 0.2f, 0.3f, 0.4f });
            });

        expectWarning("\"general.DoesNotExist\" is not found in the material properties layout", nullptr,
            [](MaterialSourceData& materialSourceData)
            {
                AddProperty(materialSourceData, "general", "DoesNotExist", AZStd::string("@exefolder@/Temp/test.streamingimage"));
            });

        // Missing image reference
        expectWarning("Could not find the image 'doesNotExist.streamingimage'",
            "Material at path  could not resolve image doesNotExist.streamingimage, using invalid UUID {00000BAD-0BAD-0BAD-0BAD-000000000BAD}. To resolve this, verify the image exists at the relative path to a scan folder matching this reference. Verify a portion of the scan folder is not in the relative path, which is a common cause of this issue.",
            [](MaterialSourceData& materialSourceData)
            {
                AddProperty(materialSourceData, "general", "MyImage", AZStd::string("doesNotExist.streamingimage"));
            }); // In this case, the warning does happen even when the asset is not finalized, because the image path is checked earlier than that
    }
    
    template<typename PropertyTypeT>
    void CheckSimilar(PropertyTypeT a, PropertyTypeT b);
    
    template<> void CheckSimilar<float>(float a, float b) { EXPECT_FLOAT_EQ(a, b); }
    template<> void CheckSimilar<Vector2>(Vector2 a, Vector2 b) { EXPECT_TRUE(a.IsClose(b)); }
    template<> void CheckSimilar<Vector3>(Vector3 a, Vector3 b) { EXPECT_TRUE(a.IsClose(b)); }
    template<> void CheckSimilar<Vector4>(Vector4 a, Vector4 b) { EXPECT_TRUE(a.IsClose(b)); }
    template<> void CheckSimilar<Color>(Color a, Color b) { EXPECT_TRUE(a.IsClose(b)); }

    template<typename PropertyTypeT> void CheckSimilar(PropertyTypeT a, PropertyTypeT b) { EXPECT_EQ(a, b); }

    template<typename PropertyTypeT>
    void CheckEndToEndDataTypeResolution(const char* propertyName, const char* jsonValue, PropertyTypeT expectedFinalValue)
    {
        const char* groupName = "general";

        const AZStd::string inputJson = AZStd::string::format(R"(
            {
                "materialType": "@exefolder@/Temp/test.materialtype",
                "propertyValues": {
                    "%s.%s": %s
                }
            }
        )", groupName, propertyName, jsonValue);

        MaterialSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);
        auto materialAssetResult = material.CreateMaterialAsset(Uuid::CreateRandom(), "test.material");
        EXPECT_TRUE(materialAssetResult);
        MaterialPropertyIndex propertyIndex = materialAssetResult.GetValue()->GetMaterialPropertiesLayout()->FindPropertyIndex(MaterialPropertyId{groupName, propertyName});
        CheckSimilar(expectedFinalValue, materialAssetResult.GetValue()->GetPropertyValues()[propertyIndex.GetIndex()].GetValue<PropertyTypeT>());
    }

    TEST_F(MaterialSourceDataTests, TestEndToEndDataTypeResolution)
    {
        // Data types in .material files don't have to exactly match the types in .materialtype files as specified in the properties layout.
        // The exact location of the data type resolution has moved around over the life of the project, but the important thing is that
        // the data type in the source .material file gets applied correctly by the time a finalized MaterialAsset comes out the other side.
        
        CheckEndToEndDataTypeResolution("MyBool", "true", true);
        CheckEndToEndDataTypeResolution("MyBool", "false", false);
        CheckEndToEndDataTypeResolution("MyBool", "1", true);
        CheckEndToEndDataTypeResolution("MyBool", "0", false);
        CheckEndToEndDataTypeResolution("MyBool", "1.0", true);
        CheckEndToEndDataTypeResolution("MyBool", "0.0", false);
        
        CheckEndToEndDataTypeResolution("MyInt", "5", 5);
        CheckEndToEndDataTypeResolution("MyInt", "-6", -6);
        CheckEndToEndDataTypeResolution("MyInt", "-7.0", -7);
        CheckEndToEndDataTypeResolution("MyInt", "false", 0);
        CheckEndToEndDataTypeResolution("MyInt", "true", 1);
        
        CheckEndToEndDataTypeResolution("MyUInt", "8", 8u);
        CheckEndToEndDataTypeResolution("MyUInt", "9.0", 9u);
        CheckEndToEndDataTypeResolution("MyUInt", "false", 0u);
        CheckEndToEndDataTypeResolution("MyUInt", "true", 1u);
        
        CheckEndToEndDataTypeResolution("MyFloat", "2", 2.0f);
        CheckEndToEndDataTypeResolution("MyFloat", "-2", -2.0f);
        CheckEndToEndDataTypeResolution("MyFloat", "2.1", 2.1f);
        CheckEndToEndDataTypeResolution("MyFloat", "false", 0.0f);
        CheckEndToEndDataTypeResolution("MyFloat", "true", 1.0f);
        
        CheckEndToEndDataTypeResolution("MyColor", "[0.1,0.2,0.3]", Color{0.1f, 0.2f, 0.3f, 1.0});
        CheckEndToEndDataTypeResolution("MyColor", "[0.1, 0.2, 0.3, 0.5]", Color{0.1f, 0.2f, 0.3f, 0.5f});
        CheckEndToEndDataTypeResolution("MyColor", "{\"RGB8\": [255, 0, 255, 0]}", Color{1.0f, 0.0f, 1.0f, 0.0f});
        
        CheckEndToEndDataTypeResolution("MyFloat2", "[0.1,0.2]", Vector2{0.1f, 0.2f});
        CheckEndToEndDataTypeResolution("MyFloat2", "{\"y\":0.2, \"x\":0.1}", Vector2{0.1f, 0.2f});
        CheckEndToEndDataTypeResolution("MyFloat2", "{\"y\":0.2, \"x\":0.1, \"Z\":0.3}", Vector2{0.1f, 0.2f});
        CheckEndToEndDataTypeResolution("MyFloat2", "{\"y\":0.2, \"W\":0.4, \"x\":0.1, \"Z\":0.3}", Vector2{0.1f, 0.2f});
        
        CheckEndToEndDataTypeResolution("MyFloat3", "[0.1,0.2,0.3]", Vector3{0.1f, 0.2f, 0.3f});
        CheckEndToEndDataTypeResolution("MyFloat3", "{\"y\":0.2, \"x\":0.1}", Vector3{0.1f, 0.2f, 0.0f});
        CheckEndToEndDataTypeResolution("MyFloat3", "{\"y\":0.2, \"x\":0.1, \"Z\":0.3}", Vector3{0.1f, 0.2f, 0.3f});
        CheckEndToEndDataTypeResolution("MyFloat3", "{\"y\":0.2, \"W\":0.4, \"x\":0.1, \"Z\":0.3}", Vector3{0.1f, 0.2f, 0.3f});
        
        CheckEndToEndDataTypeResolution("MyFloat4", "[0.1,0.2,0.3,0.4]", Vector4{0.1f, 0.2f, 0.3f, 0.4f});
        CheckEndToEndDataTypeResolution("MyFloat4", "{\"y\":0.2, \"x\":0.1}", Vector4{0.1f, 0.2f, 0.0f, 0.0f});
        CheckEndToEndDataTypeResolution("MyFloat4", "{\"y\":0.2, \"x\":0.1, \"Z\":0.3}", Vector4{0.1f, 0.2f, 0.3f, 0.0f});
        CheckEndToEndDataTypeResolution("MyFloat4", "{\"y\":0.2, \"W\":0.4, \"x\":0.1, \"Z\":0.3}", Vector4{0.1f, 0.2f, 0.3f, 0.4f});
    }

    TEST_F(MaterialSourceDataTests, CreateMaterialAssetFromSourceData_MultiLevelDataInheritance)
    {
        // Note the data being tested here is based on CreateMaterialAsset_MultiLevelDataInheritance()

        const AZStd::string simpleMaterialTypeJson = R"(
            {
                "propertyLayout": {
                    "propertyGroups":
                    [
                        {
                            "name": "general",
                            "properties": [
                                {
                                    "name": "MyFloat",
                                    "type": "Float"
                                },
                                {
                                    "name": "MyFloat2",
                                    "type": "Vector2"
                                },
                                {
                                    "name": "MyColor",
                                    "type": "Color"
                                }
                            ]
                        }
                    ]
                },
                "shaders": [
                    {
                        "file": "test.shader"
                    }
                ]
            }
        )";

        AZ::Utils::WriteFile(simpleMaterialTypeJson, "@exefolder@/Temp/test.materialtype");

        const AZStd::string material1Json = R"(
            {
                "materialType": "@exefolder@/Temp/test.materialtype",
                "propertyValues": {
                    "general.MyFloat": 1.5,
                    "general.MyColor": [0.1, 0.2, 0.3, 0.4]
                }
            }
        )";
        
        AZ::Utils::WriteFile(material1Json, "@exefolder@/Temp/m1.material");
        
        const AZStd::string material2Json = R"(
            {
                "materialType": "@exefolder@/Temp/test.materialtype",
                "parentMaterial": "@exefolder@/Temp/m1.material",
                "propertyValues": {
                    "general.MyFloat2": [4.1, 4.2],
                    "general.MyColor": [0.15, 0.25, 0.35, 0.45]
                }
            }
        )";
        
        AZ::Utils::WriteFile(material2Json, "@exefolder@/Temp/m2.material");
        
        const AZStd::string material3Json = R"(
            {
                "materialType": "@exefolder@/Temp/test.materialtype",
                "parentMaterial": "@exefolder@/Temp/m2.material",
                "propertyValues": {
                    "general.MyFloat": 3.5
                }
            }
        )";
        
        AZ::Utils::WriteFile(material3Json, "@exefolder@/Temp/m3.material");
        
        MaterialSourceData sourceDataLevel1 = MaterialUtils::LoadMaterialSourceData("@exefolder@/Temp/m1.material").TakeValue();
        MaterialSourceData sourceDataLevel2 = MaterialUtils::LoadMaterialSourceData("@exefolder@/Temp/m2.material").TakeValue();
        MaterialSourceData sourceDataLevel3 = MaterialUtils::LoadMaterialSourceData("@exefolder@/Temp/m3.material").TakeValue();
        
        auto materialAssetLevel1 = sourceDataLevel1.CreateMaterialAssetFromSourceData(Uuid::CreateRandom());
        ASSERT_TRUE(materialAssetLevel1.IsSuccess());

        auto materialAssetLevel2 = sourceDataLevel2.CreateMaterialAssetFromSourceData(Uuid::CreateRandom());
        ASSERT_TRUE(materialAssetLevel2.IsSuccess());

        auto materialAssetLevel3 = sourceDataLevel3.CreateMaterialAssetFromSourceData(Uuid::CreateRandom());
        ASSERT_TRUE(materialAssetLevel3.IsSuccess());

        auto layout = materialAssetLevel1.GetValue()->GetMaterialPropertiesLayout();
        MaterialPropertyIndex myFloat = layout->FindPropertyIndex(Name("general.MyFloat"));
        MaterialPropertyIndex myFloat2 = layout->FindPropertyIndex(Name("general.MyFloat2"));
        MaterialPropertyIndex myColor = layout->FindPropertyIndex(Name("general.MyColor"));

        AZStd::span<const MaterialPropertyValue> properties;

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
    
    TEST_F(MaterialSourceDataTests, CreateMaterialAssetFromSourceData_MultiLevelDataInheritance_OldFormat)
    {
        // This test is the same as CreateMaterialAssetFromSourceData_MultiLevelDataInheritance except it uses the old format
        // where material property values in the .material file were nested, with properties listed under a group object,
        // rather than using a flat list of property values.
        // Basically, we are making sure that MaterialSourceData::UpgradeLegacyFormat() is getting called.

        const AZStd::string simpleMaterialTypeJson = R"(
            {
                "propertyLayout": {
                    "propertyGroups":
                    [
                        {
                            "name": "general",
                            "properties": [
                                {
                                    "name": "MyFloat",
                                    "type": "Float"
                                },
                                {
                                    "name": "MyFloat2",
                                    "type": "Vector2"
                                },
                                {
                                    "name": "MyColor",
                                    "type": "Color"
                                }
                            ]
                        }
                    ]
                },
                "shaders": [
                    {
                        "file": "test.shader"
                    }
                ]
            }
        )";

        AZ::Utils::WriteFile(simpleMaterialTypeJson, "@exefolder@/Temp/test.materialtype");

        const AZStd::string material1Json = R"(
            {
                "materialType": "@exefolder@/Temp/test.materialtype",
                "properties": {
                    "general": {
                        "MyFloat": 1.5,
                        "MyColor": [0.1, 0.2, 0.3, 0.4]
                    }
                }
            }
        )";
        
        AZ::Utils::WriteFile(material1Json, "@exefolder@/Temp/m1.material");
        
        const AZStd::string material2Json = R"(
            {
                "materialType": "@exefolder@/Temp/test.materialtype",
                "parentMaterial": "@exefolder@/Temp/m1.material",
                "properties": {
                    "general": {
                        "MyFloat2": [4.1, 4.2],
                        "MyColor": [0.15, 0.25, 0.35, 0.45]
                    }
                }
            }
        )";
        
        AZ::Utils::WriteFile(material2Json, "@exefolder@/Temp/m2.material");
        
        const AZStd::string material3Json = R"(
            {
                "materialType": "@exefolder@/Temp/test.materialtype",
                "parentMaterial": "@exefolder@/Temp/m2.material",
                "properties": {
                    "general": {
                        "MyFloat": 3.5
                    }
                }
            }
        )";
        
        AZ::Utils::WriteFile(material3Json, "@exefolder@/Temp/m3.material");
        
        MaterialSourceData sourceDataLevel1 = MaterialUtils::LoadMaterialSourceData("@exefolder@/Temp/m1.material").TakeValue();
        MaterialSourceData sourceDataLevel2 = MaterialUtils::LoadMaterialSourceData("@exefolder@/Temp/m2.material").TakeValue();
        MaterialSourceData sourceDataLevel3 = MaterialUtils::LoadMaterialSourceData("@exefolder@/Temp/m3.material").TakeValue();
        
        auto materialAssetLevel1 = sourceDataLevel1.CreateMaterialAssetFromSourceData(Uuid::CreateRandom());
        EXPECT_TRUE(materialAssetLevel1.IsSuccess());

        auto materialAssetLevel2 = sourceDataLevel2.CreateMaterialAssetFromSourceData(Uuid::CreateRandom());
        EXPECT_TRUE(materialAssetLevel2.IsSuccess());

        auto materialAssetLevel3 = sourceDataLevel3.CreateMaterialAssetFromSourceData(Uuid::CreateRandom());
        EXPECT_TRUE(materialAssetLevel3.IsSuccess());

        auto layout = materialAssetLevel1.GetValue()->GetMaterialPropertiesLayout();
        MaterialPropertyIndex myFloat = layout->FindPropertyIndex(Name("general.MyFloat"));
        MaterialPropertyIndex myFloat2 = layout->FindPropertyIndex(Name("general.MyFloat2"));
        MaterialPropertyIndex myColor = layout->FindPropertyIndex(Name("general.MyColor"));

        AZStd::span<const MaterialPropertyValue> properties;

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


    TEST_F(MaterialSourceDataTests, CreateAllPropertyDefaultsMaterial)
    {
        const char* materialTypeJson = R"(
                {
                    "version": 3,
                    "propertyLayout": {
                        "propertyGroups": [
                            {
                                "name": "general",
                                "properties": [
                                    {"name": "MyBool", "type": "bool", "defaultValue": true},
                                    {"name": "MyInt", "type": "Int", "defaultValue": -7},
                                    {"name": "MyUInt", "type": "UInt", "defaultValue": 78},
                                    {"name": "MyFloat", "type": "Float", "defaultValue": 1.5},
                                    {"name": "MyFloat2", "type": "Vector2", "defaultValue": [0.1,0.2]},
                                    {"name": "MyFloat3", "type": "Vector3", "defaultValue": [0.1,0.2,0.3]},
                                    {"name": "MyFloat4", "type": "Vector4", "defaultValue": [0.1,0.2,0.3,0.4]},
                                    {"name": "MyColor", "type": "Color", "defaultValue": [0.1,0.2,0.3,0.5]},
                                    {"name": "MyImage1", "type": "Image"},
                                    {"name": "MyImage2", "type": "Image", "defaultValue": "@exefolder@/Temp/test.streamingimage"},
                                    {"name": "MyEnum", "type": "Enum", "enumValues": ["Enum0", "Enum1", "Enum2"], "defaultValue": "Enum1"}
                                ]
                            }
                        ]
                    },
                    "shaders": [
                        {
                            "file": "@exefolder@/Temp/test.shader"
                        }
                    ]
                }
            )";

        MaterialTypeSourceData materialTypeSourceData;
        LoadTestDataFromJson(materialTypeSourceData, materialTypeJson);
        Data::Asset<MaterialTypeAsset> materialType = materialTypeSourceData.CreateMaterialTypeAsset(Uuid::CreateRandom()).TakeValue();

        MaterialSourceData material = MaterialSourceData::CreateAllPropertyDefaultsMaterial(materialType, "@exefolder@/Temp/test.materialtype");

        MaterialSourceData expecteMaterial;
        expecteMaterial.m_materialType = "@exefolder@/Temp/test.materialtype";
        expecteMaterial.m_description = "For reference, lists the default values for every available property in '@exefolder@/Temp/test.materialtype'";
        expecteMaterial.m_materialTypeVersion = 3;
        expecteMaterial.SetPropertyValue(Name{"general.MyBool"}, true);
        expecteMaterial.SetPropertyValue(Name{"general.MyInt"}, -7);
        expecteMaterial.SetPropertyValue(Name{"general.MyUInt"}, 78);
        expecteMaterial.SetPropertyValue(Name{"general.MyFloat"}, 1.5f);
        expecteMaterial.SetPropertyValue(Name{"general.MyFloat2"}, Vector2{0.1f,0.2f});
        expecteMaterial.SetPropertyValue(Name{"general.MyFloat3"}, Vector3{0.1f,0.2f,0.3f});
        expecteMaterial.SetPropertyValue(Name{"general.MyFloat4"}, Vector4{0.1f,0.2f,0.3f,0.4f});
        expecteMaterial.SetPropertyValue(Name{"general.MyColor"}, Color{0.1f,0.2f,0.3f,0.5f});
        expecteMaterial.SetPropertyValue(Name{"general.MyImage1"}, AZStd::string{});
        expecteMaterial.SetPropertyValue(Name{"general.MyImage2"}, DeAliasPath("@exefolder@/Temp/test.streamingimage"));
        expecteMaterial.SetPropertyValue(Name{"general.MyEnum"}, AZStd::string{"Enum1"});

        CheckEqual(expecteMaterial, material);
    }
}


