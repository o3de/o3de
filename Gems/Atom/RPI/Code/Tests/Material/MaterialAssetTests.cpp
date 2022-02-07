/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>
#include <Common/SerializeTester.h>
#include <Common/ShaderAssetTestUtils.h>
#include <Common/ErrorMessageFinder.h>
#include <Material/MaterialAssetTestUtils.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class MaterialAssetTests
        : public RPITestFixture
    {
    protected:
        Data::Asset<MaterialTypeAsset> m_testMaterialTypeAsset;
        Data::Asset<ImageAsset> m_testImageAsset;

        void SetUp() override
        {
            RPITestFixture::SetUp();

            auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();

            // Since this test doesn't actually instantiate a Material, it won't need to instantiate this ImageAsset, so all we
            // need is an asset reference with a valid ID.
            m_testImageAsset = Data::Asset<ImageAsset>{ Uuid::CreateRandom(), azrtti_typeid<StreamingImageAsset>() };

            auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(Uuid::CreateRandom());
            materialTypeCreator.AddShader(shaderAsset);
            AddCommonTestMaterialProperties(materialTypeCreator);
            materialTypeCreator.SetPropertyValue(Name{ "MyBool" }, true);
            materialTypeCreator.SetPropertyValue(Name{ "MyInt" }, 1);
            materialTypeCreator.SetPropertyValue(Name{ "MyUInt" }, 2u);
            materialTypeCreator.SetPropertyValue(Name{ "MyFloat" }, 3.3f);
            materialTypeCreator.SetPropertyValue(Name{ "MyFloat2" }, Vector2{ 4.4f, 5.5f });
            materialTypeCreator.SetPropertyValue(Name{ "MyFloat3" }, Vector3{ 6.6f, 7.7f, 8.8f });
            materialTypeCreator.SetPropertyValue(Name{ "MyFloat4" }, Vector4{ 9.9f, 10.1f, 11.11f, 12.12f });
            materialTypeCreator.SetPropertyValue(Name{ "MyColor" }, Color{ 0.1f, 0.2f, 0.3f, 0.4f });
            materialTypeCreator.SetPropertyValue(Name{ "MyImage" }, m_testImageAsset);
            materialTypeCreator.SetPropertyValue(Name{ "MyEnum" }, 1u);
            EXPECT_TRUE(materialTypeCreator.End(m_testMaterialTypeAsset));
        }

        void TearDown() override
        {
            m_testMaterialTypeAsset.Reset();

            RPITestFixture::TearDown();
        }

        void ReplaceMaterialType(Data::Asset<MaterialAsset> materialAsset, Data::Asset<MaterialTypeAsset> upgradedMaterialTypeAsset)
        {
            materialAsset->m_materialTypeAsset = upgradedMaterialTypeAsset;
        }
    };

    TEST_F(MaterialAssetTests, Basic)
    {
        auto validate = [this](Data::Asset<MaterialAsset> materialAsset)
        {
            EXPECT_EQ(m_testMaterialTypeAsset, materialAsset->GetMaterialTypeAsset());
            EXPECT_EQ(materialAsset->GetPropertyValues().size(), 10);
            EXPECT_EQ(materialAsset->GetPropertyValues()[0].GetValue<bool>(), true);
            EXPECT_EQ(materialAsset->GetPropertyValues()[1].GetValue<int32_t>(), -2);
            EXPECT_EQ(materialAsset->GetPropertyValues()[2].GetValue<uint32_t>(), 12);
            EXPECT_EQ(materialAsset->GetPropertyValues()[3].GetValue<float>(), 1.5f);
            EXPECT_EQ(materialAsset->GetPropertyValues()[4].GetValue<Vector2>(), Vector2(0.1f, 0.2f));
            EXPECT_EQ(materialAsset->GetPropertyValues()[5].GetValue<Vector3>(), Vector3(1.1f, 1.2f, 1.3f));
            EXPECT_EQ(materialAsset->GetPropertyValues()[6].GetValue<Vector4>(), Vector4(2.1f, 2.2f, 2.3f, 2.4f));
            EXPECT_EQ(materialAsset->GetPropertyValues()[7].GetValue<Color>(), Color(1.0f, 1.0f, 1.0f, 1.0f));
            EXPECT_EQ(materialAsset->GetPropertyValues()[8].GetValue<Data::Asset<ImageAsset>>(), m_testImageAsset);
            EXPECT_EQ(materialAsset->GetPropertyValues()[9].GetValue<uint32_t>(), 1u);
        };

        // Test basic process of creating a valid asset...

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialAssetCreator creator;
        creator.Begin(assetId, *m_testMaterialTypeAsset);
        creator.SetPropertyValue(Name{ "MyFloat2" }, Vector2{ 0.1f, 0.2f });
        creator.SetPropertyValue(Name{ "MyFloat3" }, Vector3{ 1.1f, 1.2f, 1.3f });
        creator.SetPropertyValue(Name{ "MyFloat4" }, Vector4{ 2.1f, 2.2f, 2.3f, 2.4f });
        creator.SetPropertyValue(Name{ "MyColor"  }, Color{ 1.0f, 1.0f, 1.0f, 1.0f });
        creator.SetPropertyValue(Name{ "MyInt"    }, -2);
        creator.SetPropertyValue(Name{ "MyUInt"   }, 12u);
        creator.SetPropertyValue(Name{ "MyFloat"  }, 1.5f);
        creator.SetPropertyValue(Name{ "MyBool"   }, true);
        creator.SetPropertyValue(Name{ "MyImage"  }, m_testImageAsset);
        creator.SetPropertyValue(Name{ "MyEnum"   }, 1u);

        Data::Asset<MaterialAsset> materialAsset;
        EXPECT_TRUE(creator.End(materialAsset));

        EXPECT_EQ(assetId, materialAsset->GetId());
        EXPECT_EQ(Data::AssetData::AssetStatus::Ready, materialAsset->GetStatus());
        validate(materialAsset);

        // Also test serialization...

        SerializeTester<RPI::MaterialAsset> tester(GetSerializeContext());
        tester.SerializeOut(materialAsset.Get());

        // Using a filter that skips loading assets because we are using a dummy image asset
        ObjectStream::FilterDescriptor noAssets{ AZ::Data::AssetFilterNoAssetLoading };
        Data::Asset<RPI::MaterialAsset> serializedAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()), noAssets);
        validate(serializedAsset);
    }

    TEST_F(MaterialAssetTests, PropertyDefaultValuesComeFromParentMaterial)
    {
        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialAssetCreator creator;
        creator.Begin(assetId, *m_testMaterialTypeAsset);
        creator.SetPropertyValue(Name{ "MyFloat" }, 3.14f);

        Data::Asset<MaterialAsset> materialAsset;
        EXPECT_TRUE(creator.End(materialAsset));

        EXPECT_EQ(assetId, materialAsset->GetId());
        EXPECT_EQ(Data::AssetData::AssetStatus::Ready, materialAsset->GetStatus());

        // Also test serialization...
        SerializeTester<RPI::MaterialAsset> tester(GetSerializeContext());
        tester.SerializeOut(materialAsset.Get());

        // Using a filter that skips loading assets because we are using a dummy image asset
        ObjectStream::FilterDescriptor noAssets{ AZ::Data::AssetFilterNoAssetLoading };
        materialAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()), noAssets);

        EXPECT_EQ(materialAsset->GetPropertyValues().size(), 10);
        EXPECT_EQ(materialAsset->GetPropertyValues()[0].GetValue<bool>(), true);
        EXPECT_EQ(materialAsset->GetPropertyValues()[1].GetValue<int32_t>(), 1);
        EXPECT_EQ(materialAsset->GetPropertyValues()[2].GetValue<uint32_t>(), 2);
        EXPECT_EQ(materialAsset->GetPropertyValues()[3].GetValue<float>(), 3.14f);
        EXPECT_EQ(materialAsset->GetPropertyValues()[4].GetValue<Vector2>(), Vector2(4.4f, 5.5f));
        EXPECT_EQ(materialAsset->GetPropertyValues()[5].GetValue<Vector3>(), Vector3(6.6f, 7.7f, 8.8f));
        EXPECT_EQ(materialAsset->GetPropertyValues()[6].GetValue<Vector4>(), Vector4(9.9f, 10.1f, 11.11f, 12.12f));
        EXPECT_EQ(materialAsset->GetPropertyValues()[7].GetValue<Color>(), Color(0.1f, 0.2f, 0.3f, 0.4f));
        EXPECT_EQ(materialAsset->GetPropertyValues()[8].GetValue<Data::Asset<ImageAsset>>(), m_testImageAsset);
        EXPECT_EQ(materialAsset->GetPropertyValues()[9].GetValue<uint32_t>(), 1u);
    }

    TEST_F(MaterialAssetTests, MaterialWithNoSRGOrProperties)
    {
        // Making a material with no properties and no SRG allows us to create simple shaders
        // that don't need any input, for example a debug shader that just renders surface normals.
        
        Data::Asset<MaterialTypeAsset> emptyMaterialTypeAsset;
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeCreator.End(emptyMaterialTypeAsset));

        Data::Asset<MaterialAsset> materialAsset;
        MaterialAssetCreator materialCreator;
        materialCreator.Begin(Uuid::CreateRandom(), *emptyMaterialTypeAsset);
        EXPECT_TRUE(materialCreator.End(materialAsset));
        EXPECT_EQ(emptyMaterialTypeAsset, materialAsset->GetMaterialTypeAsset());
        EXPECT_EQ(materialAsset->GetPropertyValues().size(), 0);
    }

    TEST_F(MaterialAssetTests, SetPropertyWithImageAssetSubclass)
    {
        // In the above test we called SetProperty(m_testImageAsset) which is an ImageAsset. Just to be safe, we also test
        // to make sure it still works when using the leaf type of StreamingImageAsset.

        // Since this test doesn't actually instantiate a Material, it won't need to instantiate this ImageAsset, so all we
        // need is an asset reference with a valid ID.
        Data::Asset<StreamingImageAsset> streamingImageAsset = Data::Asset<StreamingImageAsset>{ Uuid::CreateRandom(), azrtti_typeid<StreamingImageAsset>() };

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialAssetCreator creator;
        creator.Begin(assetId, *m_testMaterialTypeAsset);
        creator.SetPropertyValue(Name{ "MyImage" }, streamingImageAsset);

        Data::Asset<MaterialAsset> materialAsset;
        EXPECT_TRUE(creator.End(materialAsset));

        EXPECT_EQ(materialAsset->GetPropertyValues()[8].GetValue<Data::Asset<ImageAsset>>(), streamingImageAsset);

        // Also test serialization...

        SerializeTester<RPI::MaterialAsset> tester(GetSerializeContext());
        tester.SerializeOut(materialAsset.Get());

        // Using a filter that skips loading assets because we are using a dummy image asset
        ObjectStream::FilterDescriptor noAssets{ AZ::Data::AssetFilterNoAssetLoading };
        Data::Asset<RPI::MaterialAsset> serializedAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()), noAssets);
        EXPECT_EQ(serializedAsset->GetPropertyValues()[8].GetValue<Data::Asset<ImageAsset>>(), streamingImageAsset);
    }

    TEST_F(MaterialAssetTests, UpgradeMaterialAsset)
    {
        // Here we test the main way that a material asset upgrade would be applied at runtime: A material type is updated to
        // both rename a property *and* change the order in which properties appear in the layout. In this case, the new name
        // must be identified and then that new name is used to find the appropriate index in the property layout.

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();

        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        Data::Asset<MaterialTypeAsset> testMaterialTypeAssetV1;
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(shaderAsset);
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyInt" }, MaterialPropertyDataType::Int, Name{ "m_int" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyUInt" }, MaterialPropertyDataType::UInt, Name{ "m_uint" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyFloat" }, MaterialPropertyDataType::Float, Name{ "m_float" });
        EXPECT_TRUE(materialTypeCreator.End(testMaterialTypeAssetV1));

        // Construct the material asset with materialTypeAsset version 1
        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialAssetCreator creator;
        const bool includePropertyNames = true;
        creator.Begin(assetId, *testMaterialTypeAssetV1, includePropertyNames);
        creator.SetPropertyValue(Name{ "MyInt" }, 7);
        creator.SetPropertyValue(Name{ "MyUInt" }, 8u);
        creator.SetPropertyValue(Name{ "MyFloat" }, 9.0f);
        Data::Asset<MaterialAsset> materialAsset;
        EXPECT_TRUE(creator.End(materialAsset));

        // Prepare material type asset version 2 with the update actions
        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::RenamePropertyAction(
            {
                Name{ "MyInt" },
                Name{ "MyIntRenamed" }
            }));

        Data::Asset<MaterialTypeAsset> testMaterialTypeAssetV2;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.SetVersion(versionUpdate.GetVersion());
        materialTypeCreator.AddVersionUpdate(versionUpdate);
        materialTypeCreator.AddShader(shaderAsset);
        // Now we add the properties in a different order from before, and use the new name for MyInt.
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyUInt" }, MaterialPropertyDataType::UInt, Name{ "m_uint" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyFloat" }, MaterialPropertyDataType::Float, Name{ "m_float" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyIntRenamed" }, MaterialPropertyDataType::Int, Name{ "m_int" });
        EXPECT_TRUE(materialTypeCreator.End(testMaterialTypeAssetV2));

        // This is our way of faking the idea that an old version of the MaterialAsset could be loaded with a new version of the MaterialTypeAsset.
        ReplaceMaterialType(materialAsset, testMaterialTypeAssetV2);

        // This can find errors and warnings, we are looking for a warning when the version update is applied
        ErrorMessageFinder warningFinder; 
        warningFinder.AddExpectedErrorMessage("Automatic updates are available. Consider updating the .material source file");
        warningFinder.AddExpectedErrorMessage("This material is based on version '1'");
        warningFinder.AddExpectedErrorMessage("material type is now at version '2'");

        // Even though this material was created using the old version of the material type, it's property values should get automatically
        // updated to align with the new property layout in the latest MaterialTypeAsset.
        MaterialPropertyIndex myIntIndex = materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"MyIntRenamed"});
        EXPECT_EQ(2, myIntIndex.GetIndex());
        EXPECT_EQ(7, materialAsset->GetPropertyValues()[myIntIndex.GetIndex()].GetValue<int32_t>());

        warningFinder.CheckExpectedErrorsFound();

        // Since the MaterialAsset has already been updated, and the warning reported once, we should not see the "consider updating"
        // warning reported again on subsequent property accesses.
        warningFinder.Reset();
        myIntIndex = materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"MyIntRenamed"});
        EXPECT_EQ(2, myIntIndex.GetIndex());
        EXPECT_EQ(7, materialAsset->GetPropertyValues()[myIntIndex.GetIndex()].GetValue<int32_t>());
    }

    TEST_F(MaterialAssetTests, Error_NoBegin)
    {
        Data::AssetId assetId(Uuid::CreateRandom());

        AZ_TEST_START_ASSERTTEST;

        MaterialAssetCreator creator;

        creator.SetPropertyValue(Name{ "MyBool" }, true);
        creator.SetPropertyValue(Name{ "MyImage" }, m_testImageAsset);

        Data::Asset<MaterialAsset> materialAsset;
        EXPECT_FALSE(creator.End(materialAsset));

        AZ_TEST_STOP_ASSERTTEST(3);
    }

    TEST_F(MaterialAssetTests, Error_SetPropertyInvalidInputs)
    {
        Data::AssetId assetId(Uuid::CreateRandom());
        
        // We use local functions to easily start a new MaterialAssetCreator for each test case because
        // the AssetCreator would just skip subsequent operations after the first failure is detected.

        auto expectCreatorError = [this](AZStd::function<void(MaterialAssetCreator& creator)> passBadInput)
        {
            MaterialAssetCreator creator;
            creator.Begin(Uuid::CreateRandom(), *m_testMaterialTypeAsset);

            AZ_TEST_START_ASSERTTEST;
            passBadInput(creator);
            AZ_TEST_STOP_ASSERTTEST(1);

            EXPECT_EQ(1, creator.GetErrorCount());
        };

        auto expectCreatorWarning = [this](AZStd::function<void(MaterialAssetCreator& creator)> passBadInput)
        {
            MaterialAssetCreator creator;
            creator.Begin(Uuid::CreateRandom(), *m_testMaterialTypeAsset);

            passBadInput(creator);

            EXPECT_EQ(1, creator.GetWarningCount());
        };

        // Invalid input ID
        expectCreatorWarning([](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "BoolDoesNotExist" }, MaterialPropertyValue(false));
        });

        // Invalid image input ID
        expectCreatorWarning([this](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "ImageDoesNotExist" }, m_testImageAsset);
        });

        // Test data type mismatches...

        expectCreatorError([this](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyBool" }, m_testImageAsset);
        });

        expectCreatorError([](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyInt" }, 0.0f);
        });

        expectCreatorError([](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyUInt" }, -1);
        });

        expectCreatorError([](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyFloat" }, 10u);
        });

        expectCreatorError([](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyFloat2" }, 1.0f);
        });

        expectCreatorError([](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyFloat3" }, AZ::Vector4{});
        });

        expectCreatorError([](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyFloat4" }, AZ::Vector3{});
        });

        expectCreatorError([](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyColor" }, MaterialPropertyValue(false));
        });

        expectCreatorError([](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyImage" }, true);
        });

        expectCreatorError([](MaterialAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyEnum" }, -1);
        });
    }
}

