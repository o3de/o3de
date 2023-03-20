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
        Data::Asset<ImageAsset> m_testAttachmentImageAsset;

        void SetUp() override
        {
            RPITestFixture::SetUp();

            auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();

            // Since this test doesn't actually instantiate a Material, it won't need to instantiate this ImageAsset, so all we
            // need is an asset reference with a valid ID.
            m_testImageAsset = Data::Asset<ImageAsset>{ Uuid::CreateRandom(), azrtti_typeid<StreamingImageAsset>() };
            m_testAttachmentImageAsset = Data::Asset<AttachmentImageAsset>{ Uuid::CreateRandom(), azrtti_typeid<AttachmentImageAsset>() };

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
            materialTypeCreator.SetPropertyValue(Name{ "MyAttachmentImage" }, m_testAttachmentImageAsset);
            EXPECT_TRUE(materialTypeCreator.End(m_testMaterialTypeAsset));
        }

        void TearDown() override
        {
            m_testMaterialTypeAsset.Reset();

            RPITestFixture::TearDown();
        }
    };

    TEST_F(MaterialAssetTests, Basic)
    {
        auto validate = [this](Data::Asset<MaterialAsset> materialAsset)
        {
            EXPECT_EQ(m_testMaterialTypeAsset, materialAsset->GetMaterialTypeAsset());
            EXPECT_EQ(materialAsset->GetPropertyValues().size(), 11);
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
            EXPECT_EQ(materialAsset->GetPropertyValues()[10].GetValue<Data::Asset<ImageAsset>>(), m_testAttachmentImageAsset);
        };

        // Test basic process of creating a valid asset...

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialAssetCreator creator;
        creator.Begin(assetId, m_testMaterialTypeAsset);
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
        creator.SetPropertyValue(Name{ "MyAttachmentImage"  }, m_testAttachmentImageAsset);

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
        creator.Begin(assetId, m_testMaterialTypeAsset);
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

        EXPECT_EQ(materialAsset->GetPropertyValues().size(), 11);
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
        EXPECT_EQ(materialAsset->GetPropertyValues()[10].GetValue<Data::Asset<ImageAsset>>(), m_testAttachmentImageAsset);
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
        materialCreator.Begin(Uuid::CreateRandom(), emptyMaterialTypeAsset);
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
        creator.Begin(assetId, m_testMaterialTypeAsset);
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
        // both rename properties, set new values *and* change the order in which properties appear in the layout.
        // Various permutations of the ordering of 'rename' and 'setValue' actions are tested.

        auto materialSrgLayout = CreateCommonTestMaterialSrgLayout();

        auto shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout);

        // Construct the material asset with materialTypeAsset version 1
        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        // Prepare material type asset version 3 with update actions
        materialTypeCreator.SetVersion(3);
        {
            MaterialVersionUpdate versionUpdate(2);
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{ "rename" },
                {
                    { Name{ "from" }, AZStd::string("MyInt") },
                    { Name{ "to"   }, AZStd::string("MyIntIntermediateRename") }
                } ));
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{ "setValue" },
                {
                    { Name("name"),  AZStd::string("MyFloat") },
                    { Name("value"), 3.14f }
                } ));
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{ "setValue" },
                {
                    { Name("name"),  AZStd::string("MyFloat2") },
                    { Name("value"), 2.0f }
                } ));
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{ "setValue" },
                {
                    { Name("name"),  AZStd::string("MyUInt") },
                    { Name("value"), 314u }
                } ));
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }
        {
            MaterialVersionUpdate versionUpdate(3);
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{ "setValue" },
                {
                    { Name("name"),  AZStd::string("MyFloat3") },
                    { Name("value"), 3.0f }
                } ));
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{ "rename" },
                {
                    { Name{ "from" }, AZStd::string("MyIntIntermediateRename") },
                    { Name{ "to"   }, AZStd::string("MyIntFinalRename") }
                } ));
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{ "rename" },
                {
                    { Name{ "from" }, AZStd::string("MyUInt") },
                    { Name{ "to"   }, AZStd::string("MyUIntRenamed") }
                } ));
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{ "rename" },
                {
                    { Name{ "from" }, AZStd::string("MyFloat") },
                    { Name{ "to"   }, AZStd::string("MyFloatRenamed") }
                } ));
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }
        materialTypeCreator.AddShader(shaderAsset);
        // Now we add the properties in a different order from before, and use the new names.
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyUIntRenamed" }, MaterialPropertyDataType::UInt, Name{ "m_uint" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyFloatRenamed" }, MaterialPropertyDataType::Float, Name{ "m_float" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyIntFinalRename" }, MaterialPropertyDataType::Int, Name{ "m_int" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyFloat2" }, MaterialPropertyDataType::Float, Name{ "m_float2" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyFloat3" }, MaterialPropertyDataType::Float, Name{ "m_float3" });

        Data::Asset<MaterialTypeAsset> testMaterialTypeAssetV3;
        EXPECT_TRUE(materialTypeCreator.End(testMaterialTypeAssetV3));

        // Expected warning messages
        ErrorMessageFinder warningFinder; 
        auto ExpectOverwriteMessage = [&warningFinder](uint32_t version, const char *name, const char *finalName)
        {
            if (finalName == nullptr)
            {
                warningFinder.AddExpectedErrorMessage(AZStd::string::format(
                    "SetValue operation of update to version %u has detected (and overwritten) a previous value for '%s'.",
                    version, name));
            }
            else
            {
                warningFinder.AddExpectedErrorMessage(AZStd::string::format(
                    "SetValue operation of update to version %u has detected (and overwritten) a previous value for '%s' "
                    "(final name of this property: '%s').",
                    version, name, finalName));
            }
        };
        warningFinder.AddExpectedErrorMessage("Automatic updates have been applied. Consider updating the .material source file");
        warningFinder.AddExpectedErrorMessage("This material is based on version '1'");
        warningFinder.AddExpectedErrorMessage("material type is now at version '3'");
        ExpectOverwriteMessage(2, "MyFloat", "MyFloatRenamed");
        ExpectOverwriteMessage(2, "MyFloat2", nullptr);
        ExpectOverwriteMessage(2, "MyUInt", "MyUIntRenamed");

        MaterialAssetCreator creator;
        creator.Begin(assetId, testMaterialTypeAssetV3);
        creator.SetMaterialTypeVersion(1);
        // Set some properties to non-default values
        creator.SetPropertyValue(Name{ "MyInt" }, 7);
        creator.SetPropertyValue(Name{ "MyUInt" }, 8u);
        creator.SetPropertyValue(Name{ "MyFloat" }, 9.0f);
        creator.SetPropertyValue(Name{ "MyFloat2" }, 10.0f);

        Data::Asset<MaterialAsset> materialAsset;
        EXPECT_TRUE(creator.End(materialAsset));

        warningFinder.CheckExpectedErrorsFound();

        // Since the MaterialAsset has already been updated, and the warnings reported once, we
        // should not see any warnings reported again on subsequent property accesses.
        warningFinder.Reset();

        // Check that the properties have been properly updated, and that their index corresponds to the latest property layout.
        auto FindIndex = [&materialAsset](const Name &propertyId)
        {
            return materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyId);
        };
        EXPECT_FALSE(FindIndex(Name{"MyUInt"}).IsValid());
        MaterialPropertyIndex myUIntIndex = FindIndex(Name{"MyUIntRenamed"});
        EXPECT_EQ(0, myUIntIndex.GetIndex());
        EXPECT_EQ(314u, materialAsset->GetPropertyValues()[myUIntIndex.GetIndex()].GetValue<uint32_t>());

        EXPECT_FALSE(FindIndex(Name{"MyFloat"}).IsValid());
        MaterialPropertyIndex myFloatIndex = FindIndex(Name{"MyFloatRenamed"});
        EXPECT_EQ(1, myFloatIndex.GetIndex());
        EXPECT_EQ(3.14f, materialAsset->GetPropertyValues()[myFloatIndex.GetIndex()].GetValue<float>());

        EXPECT_FALSE(FindIndex(Name{"MyInt"}).IsValid());
        EXPECT_FALSE(FindIndex(Name{"MyIntIntermediateRename"}).IsValid());
        MaterialPropertyIndex myIntIndex = FindIndex(Name{"MyIntFinalRename"});
        EXPECT_EQ(2, myIntIndex.GetIndex());
        EXPECT_EQ(7, materialAsset->GetPropertyValues()[myIntIndex.GetIndex()].GetValue<int32_t>());

        MaterialPropertyIndex myFloat2Index = FindIndex(Name{"MyFloat2"});
        EXPECT_EQ(3, myFloat2Index.GetIndex());
        EXPECT_EQ(2.0f, materialAsset->GetPropertyValues()[myFloat2Index.GetIndex()].GetValue<float>());

        MaterialPropertyIndex myFloat3Index = FindIndex(Name{"MyFloat3"});
        EXPECT_EQ(4, myFloat3Index.GetIndex());
        EXPECT_EQ(3.0f, materialAsset->GetPropertyValues()[myFloat3Index.GetIndex()].GetValue<float>());
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

        auto expectCreatorError = [this](const char* expectedErrorMessage, AZStd::function<void(MaterialAssetCreator& creator)> passBadInput)
        {
            MaterialAssetCreator creator;
            creator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);

            ErrorMessageFinder errorMessageFinder;
            errorMessageFinder.AddExpectedErrorMessage(expectedErrorMessage);
            errorMessageFinder.AddIgnoredErrorMessage("Failed to build", true);

            passBadInput(creator);

            Data::Asset<MaterialAsset> materialAsset;
            EXPECT_FALSE(creator.End(materialAsset));

            errorMessageFinder.CheckExpectedErrorsFound();

            EXPECT_TRUE(creator.GetErrorCount() > 0);
        };

        auto expectCreatorWarning = [this](AZStd::function<void(MaterialAssetCreator& creator)> passBadInput)
        {
            MaterialAssetCreator creator;
            creator.Begin(Uuid::CreateRandom(), m_testMaterialTypeAsset);

            passBadInput(creator);

            Data::Asset<MaterialAsset> material;
            creator.End(material);

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

        expectCreatorError("Type mismatch",
            [this](MaterialAssetCreator& creator)
            {
                creator.SetPropertyValue(Name{ "MyBool" }, m_testImageAsset);
            });

        expectCreatorError("Type mismatch",
            [](MaterialAssetCreator& creator)
            {
                creator.SetPropertyValue(Name{ "MyFloat" }, AZ::Vector4{});
            });

        expectCreatorError("Type mismatch",
            [](MaterialAssetCreator& creator)
            {
                creator.SetPropertyValue(Name{ "MyColor" }, MaterialPropertyValue(false));
            });

        expectCreatorError("Type mismatch",
            [](MaterialAssetCreator& creator)
            {
                creator.SetPropertyValue(Name{ "MyImage" }, true);
            });

        expectCreatorError("can only accept UInt value",
            [](MaterialAssetCreator& creator)
            {
                creator.SetPropertyValue(Name{ "MyEnum" }, -1);
            });
    }
}

