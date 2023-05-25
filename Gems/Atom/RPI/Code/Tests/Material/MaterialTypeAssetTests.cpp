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
#include <Common/ErrorMessageFinder.h>
#include <Common/ShaderAssetTestUtils.h>
#include <Material/MaterialAssetTestUtils.h>

#include <Atom/RPI.Reflect/Material/MaterialAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAssetCreator.h>
#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Material/Material.h>

#include <Atom/RPI.Edit/Material/MaterialUtils.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class MaterialTypeAssetTests
        : public RPITestFixture
    {
    protected:

        /// Sample used for testing a MaterialFunctor that updates shader inputs
        class Splat3Functor final
            : public AZ::RPI::MaterialFunctor
        {
        public:
            AZ_CLASS_ALLOCATOR(Splat3Functor, SystemAllocator)
            AZ_RTTI(Splat3Functor, "{4719BBAD-21A1-4909-88E9-C190208BDD00}", AZ::RPI::MaterialFunctor);

            static void Reflect(AZ::SerializeContext* serializeContext)
            {
                serializeContext->Class<Splat3Functor, AZ::RPI::MaterialFunctor>()
                    ->Version(1)
                    ->Field("m_floatIndex", &Splat3Functor::m_floatIndex)
                    ->Field("m_vector3Index", &Splat3Functor::m_vector3Index)
                    ;
            }

            using AZ::RPI::MaterialFunctor::Process;
            void Process(AZ::RPI::MaterialFunctorAPI::RuntimeContext& context) override
            {
                // This code isn't actually called in the unit test, but we include it here just to demonstrate what a real functor might look like.
                float f = context.GetMaterialPropertyValue(m_floatIndex).GetValue<float>();
                float f3[3] = { f,f,f };
                context.GetShaderResourceGroup()->SetConstantRaw(m_vector3Index, f3, sizeof(float) * 3);
            }

            AZ::RPI::MaterialPropertyIndex m_floatIndex;
            RHI::ShaderInputConstantIndex m_vector3Index;
        };

        /// Sample used for testing a MaterialFunctor that updates the shader collection
        class DummyShaderCollectionFunctor final
            : public AZ::RPI::MaterialFunctor
        {
        public:
            AZ_CLASS_ALLOCATOR(DummyShaderCollectionFunctor, SystemAllocator)
            AZ_RTTI(DummyShaderCollectionFunctor, "{6ED031DC-DADC-4A47-B858-DDA9748700A6}", AZ::RPI::MaterialFunctor);

            static void Reflect(AZ::SerializeContext* serializeContext)
            {
                serializeContext->Class<DummyShaderCollectionFunctor, AZ::RPI::MaterialFunctor>()
                    ->Version(1)
                    ->Field("m_enableIndex", &DummyShaderCollectionFunctor::m_enableIndex)
                    ;
            }

            using AZ::RPI::MaterialFunctor::Process;
            void Process(AZ::RPI::MaterialFunctorAPI::RuntimeContext& context) override
            {
                // This code isn't actually called in the unit test, but we include it here just to demonstrate what a real functor might look like.

                const bool enable = context.GetMaterialPropertyValue(m_enableIndex).GetValue<bool>();
                context.SetShaderEnabled(0, enable);
            }

            AZ::RPI::MaterialPropertyIndex m_enableIndex;
        };

        RHI::Ptr<RHI::ShaderResourceGroupLayout> m_testMaterialSrgLayout;
        Ptr<ShaderOptionGroupLayout> m_testShaderOptionsLayout;
        Data::Asset<ShaderAsset> m_testShaderAsset;
        Data::Asset<ImageAsset> m_testImageAsset;
        Data::Asset<ImageAsset> m_testImageAsset2;
        const char* m_testImageFilename = "test.streamingimage";
        const char* m_testImageFilename2 = "test2.streamingimage";

        static void AddRenameAction(MaterialVersionUpdate& versionUpdate, const char* from, const char* to)
        {
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{ "rename" },
                {
                    { Name{ "from" }, AZStd::string(from) },
                    { Name{ "to"   }, AZStd::string(to)   }
                } ));
        };

        static void AddRenamePrefixAction(MaterialVersionUpdate& versionUpdate, const char* from, const char* to)
        {
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{"renamePrefix"},
                {
                    { Name{ "from" }, AZStd::string(from) },
                    { Name{ "to"   }, AZStd::string(to)   }
                }));
        };

        static void AddSetValueAction(MaterialVersionUpdate& versionUpdate, const char* name, const MaterialPropertyValue& val)
        {
            versionUpdate.AddAction(MaterialVersionUpdate::Action(
                AZ::Name{ "setValue" },
                {
                    { Name{ "name"  }, AZStd::string(name) },
                    { Name{ "value" }, val }
                } ));
        };

        void SetUp() override
        {
            RPITestFixture::SetUp();

            Splat3Functor::Reflect(GetSerializeContext());
            DummyShaderCollectionFunctor::Reflect(GetSerializeContext());

            m_testMaterialSrgLayout = CreateCommonTestMaterialSrgLayout();

            AZStd::vector<RPI::ShaderOptionValuePair> boolOptionValues = CreateBoolShaderOptionValues();
            AZStd::vector<RPI::ShaderOptionValuePair> enumOptionValues = CreateEnumShaderOptionValues({"Low", "Med", "High"});
            AZStd::vector<RPI::ShaderOptionValuePair> intOptionRange = CreateIntRangeShaderOptionValues(0, 8);

            m_testShaderOptionsLayout = ShaderOptionGroupLayout::Create();
            uint32_t order = 0;
            m_testShaderOptionsLayout->AddShaderOption(ShaderOptionDescriptor{Name{"o_debug"}, ShaderOptionType::Boolean, 0, order++, boolOptionValues, Name{"False"}});
            m_testShaderOptionsLayout->AddShaderOption(ShaderOptionDescriptor{Name{"o_quality"}, ShaderOptionType::Enumeration, 1, order++, enumOptionValues, Name{"Low"}});
            m_testShaderOptionsLayout->AddShaderOption(ShaderOptionDescriptor{Name{"o_lightCount"}, ShaderOptionType::IntegerRange, 3, order++, intOptionRange, Name{"0"}});
            m_testShaderOptionsLayout->Finalize();

            m_testShaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, m_testShaderOptionsLayout);

            // Since this test doesn't actually instantiate a Material, it won't need to instantiate this ImageAsset, so all we
            // need is an asset reference with a valid ID.
            m_testImageAsset = Data::Asset<ImageAsset>{ Data::AssetId{ Uuid::CreateRandom(), StreamingImageAsset::GetImageAssetSubId() }, azrtti_typeid<StreamingImageAsset>() };
            Data::AssetInfo imageAssetInfo;
            imageAssetInfo.m_assetId = m_testImageAsset.GetId();
            m_assetSystemStub.RegisterSourceInfo(m_testImageFilename, imageAssetInfo, "");

            m_testImageAsset2 = Data::Asset<ImageAsset>{ Data::AssetId{ Uuid::CreateRandom(), StreamingImageAsset::GetImageAssetSubId() }, azrtti_typeid<StreamingImageAsset>() };
            Data::AssetInfo imageAssetInfo2;
            imageAssetInfo2.m_assetId = m_testImageAsset2.GetId();
            m_assetSystemStub.RegisterSourceInfo(m_testImageFilename2, imageAssetInfo2, "");
        }

        void TearDown() override
        {
            m_testMaterialSrgLayout = nullptr;
            m_testShaderAsset.Reset();
            m_testShaderOptionsLayout = nullptr;

            RPITestFixture::TearDown();
        }
    };

    TEST_F(MaterialTypeAssetTests, Basic)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        // Test basic process of creating a valid asset
        {
            Data::AssetId assetId(Uuid::CreateRandom());

            MaterialTypeAssetCreator materialTypeCreator;
            materialTypeCreator.Begin(assetId);

            // Version updates
            MaterialVersionUpdate versionUpdate(2);
            AddRenameAction(versionUpdate, "EnableSpecialPassPrev", "EnableSpecialPass");
            AddSetValueAction(versionUpdate, "FloatThatGetsSet", 1.234f);
            materialTypeCreator.SetVersion(versionUpdate.GetVersion());
            materialTypeCreator.AddVersionUpdate(versionUpdate);

            // Property for the setValue update

            materialTypeCreator.BeginMaterialProperty(Name{ "FloatThatGetsSet" }, MaterialPropertyDataType::Float);
            materialTypeCreator.EndMaterialProperty();

            // Built-in shader

            materialTypeCreator.AddShader(m_testShaderAsset);

            // Functor-driven shader

            materialTypeCreator.BeginMaterialProperty(Name{ "EnableSpecialPass" }, MaterialPropertyDataType::Bool);
            materialTypeCreator.EndMaterialProperty();

            Ptr<DummyShaderCollectionFunctor> shaderCollectionFunctor = aznew DummyShaderCollectionFunctor;
            shaderCollectionFunctor->m_enableIndex = materialTypeCreator.GetMaterialPropertiesLayout()->FindPropertyIndex(Name{ "EnableSpecialPass" });
            materialTypeCreator.AddMaterialFunctor(shaderCollectionFunctor);

            // Aliased settings

            AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyColor" }, MaterialPropertyDataType::Color, Name{ "m_color" });
            AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyImage" }, MaterialPropertyDataType::Image, Name{ "m_image" });
            AddMaterialPropertyForSrg(materialTypeCreator, Name{ "MyFloat" }, MaterialPropertyDataType::Float, Name{ "m_float" });

            // Functor-driven settings

            materialTypeCreator.BeginMaterialProperty(Name{ "NonAliasFloat" }, MaterialPropertyDataType::Float);
            materialTypeCreator.EndMaterialProperty();

            Ptr<Splat3Functor> shaderInputFunctor = aznew Splat3Functor;
            shaderInputFunctor->m_floatIndex = materialTypeCreator.GetMaterialPropertiesLayout()->FindPropertyIndex(Name{ "NonAliasFloat" });
            shaderInputFunctor->m_vector3Index = m_testMaterialSrgLayout->FindShaderInputConstantIndex(Name{ "m_float3" });
            materialTypeCreator.AddMaterialFunctor(shaderInputFunctor);

            EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));
            EXPECT_EQ(assetId, materialTypeAsset->GetId());
        }

        // Run the asset through the serializer to make sure we have the proper reflection set up
        {
            SerializeTester<RPI::MaterialTypeAsset> tester(GetSerializeContext());
            tester.SerializeOut(materialTypeAsset.Get());

            materialTypeAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));
        }

        // Validate the results
        {
            EXPECT_EQ(m_testMaterialSrgLayout, materialTypeAsset->GetMaterialSrgLayout());
            EXPECT_EQ(6, materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyCount());
            EXPECT_EQ(2, materialTypeAsset->GetVersion());
            // Check aliased properties

            const MaterialPropertyIndex colorIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{ "MyColor" });
            const MaterialPropertyIndex floatIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{ "MyFloat" });
            const MaterialPropertyIndex imageIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{ "MyImage" });

            const MaterialPropertyDescriptor* colorDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(colorIndex);
            const MaterialPropertyDescriptor* floatDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(floatIndex);
            const MaterialPropertyDescriptor* imageDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(imageIndex);

            EXPECT_EQ(1, colorDescriptor->GetOutputConnections().size());
            EXPECT_EQ(1, floatDescriptor->GetOutputConnections().size());
            EXPECT_EQ(1, imageDescriptor->GetOutputConnections().size());

            EXPECT_EQ(1, colorDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex());
            EXPECT_EQ(2, floatDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex());
            EXPECT_EQ(1, imageDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex());

            EXPECT_EQ(MaterialPropertyOutputType::ShaderInput, colorDescriptor->GetOutputConnections()[0].m_type);
            EXPECT_EQ(MaterialPropertyOutputType::ShaderInput, floatDescriptor->GetOutputConnections()[0].m_type);
            EXPECT_EQ(MaterialPropertyOutputType::ShaderInput, imageDescriptor->GetOutputConnections()[0].m_type);

            // Check non-aliased, functor-based properties

            const MaterialPropertyIndex enableSpecialPassIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{ "EnableSpecialPass" });
            const MaterialPropertyDescriptor* enableSpecialPassDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(enableSpecialPassIndex);
            EXPECT_EQ(0, enableSpecialPassDescriptor->GetOutputConnections().size());

            const MaterialPropertyIndex nonAliasFloatIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{ "NonAliasFloat" });
            const MaterialPropertyDescriptor* nonAliasFloatDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(nonAliasFloatIndex);
            EXPECT_EQ(0, nonAliasFloatDescriptor->GetOutputConnections().size());

            // Check the functors

            const RHI::ShaderInputConstantIndex expectedVector3Index = materialTypeAsset->GetMaterialSrgLayout()->FindShaderInputConstantIndex(Name{ "m_float3" });

            EXPECT_EQ(2, materialTypeAsset->GetMaterialFunctors().size());
            const DummyShaderCollectionFunctor* shaderCollectionFunctor = azrtti_cast<DummyShaderCollectionFunctor*>(materialTypeAsset->GetMaterialFunctors()[0].get());
            EXPECT_TRUE(nullptr != shaderCollectionFunctor);
            EXPECT_EQ(enableSpecialPassIndex, shaderCollectionFunctor->m_enableIndex);

            const Splat3Functor* shaderInputFunctor = azrtti_cast<Splat3Functor*>(materialTypeAsset->GetMaterialFunctors()[1].get());
            EXPECT_TRUE(nullptr != shaderInputFunctor);
            EXPECT_EQ(nonAliasFloatIndex, shaderInputFunctor->m_floatIndex);
            EXPECT_EQ(expectedVector3Index, shaderInputFunctor->m_vector3Index);
        };

    }

    TEST_F(MaterialTypeAssetTests, DefaultPropertyValues)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset);

        AddCommonTestMaterialProperties(materialTypeCreator);

        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        CheckPropertyValue<bool>    (materialTypeAsset, Name{"MyBool"}, false);
        CheckPropertyValue<float>   (materialTypeAsset, Name{"MyFloat"}, 0.0f);
        CheckPropertyValue<int32_t> (materialTypeAsset, Name{"MyInt"}, 0);
        CheckPropertyValue<uint32_t>(materialTypeAsset, Name{"MyUInt"}, 0u);
        CheckPropertyValue<Vector2> (materialTypeAsset, Name{"MyFloat2"}, Vector2{ 0.0f, 0.0f });
        CheckPropertyValue<Vector3> (materialTypeAsset, Name{"MyFloat3"}, Vector3{ 0.0f, 0.0f, 0.0f });
        CheckPropertyValue<Vector4> (materialTypeAsset, Name{"MyFloat4"}, Vector4{ 0.0f, 0.0f, 0.0f, 0.0f });
        CheckPropertyValue<Color>   (materialTypeAsset, Name{"MyColor"}, Color{ 1.0f, 1.0f, 1.0f, 1.0f });
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{"MyImage"}, Data::Asset<ImageAsset>({}));
        CheckPropertyValue<uint32_t>(materialTypeAsset, Name{"MyEnum"}, 0u);
    }

    TEST_F(MaterialTypeAssetTests, SetPropertyValues)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset);

        AddCommonTestMaterialProperties(materialTypeCreator);

        materialTypeCreator.SetPropertyValue(Name{"MyBool"},   true);
        materialTypeCreator.SetPropertyValue(Name{"MyFloat"},  1.2f);
        materialTypeCreator.SetPropertyValue(Name{"MyInt"},    -12);
        materialTypeCreator.SetPropertyValue(Name{"MyUInt"},   12u);
        materialTypeCreator.SetPropertyValue(Name{"MyFloat2"}, Vector2{ 1.1f, 2.2f });
        materialTypeCreator.SetPropertyValue(Name{"MyFloat3"}, Vector3{ 3.3f, 4.4f, 5.5f });
        materialTypeCreator.SetPropertyValue(Name{"MyFloat4"}, Vector4{ 6.6f, 7.7f, 8.8f, 9.9f });
        materialTypeCreator.SetPropertyValue(Name{"MyColor"},  Color{ 0.1f, 0.2f, 0.3f, 0.4f });
        materialTypeCreator.SetPropertyValue(Name{"MyImage"},  m_testImageAsset);
        materialTypeCreator.SetPropertyValue(Name{"MyEnum"}, 1u);

        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        CheckPropertyValue<bool>    (materialTypeAsset, Name{"MyBool"}, true);
        CheckPropertyValue<float>   (materialTypeAsset, Name{"MyFloat"}, 1.2f);
        CheckPropertyValue<int32_t> (materialTypeAsset, Name{"MyInt"}, -12);
        CheckPropertyValue<uint32_t>(materialTypeAsset, Name{"MyUInt"}, 12u);
        CheckPropertyValue<Vector2> (materialTypeAsset, Name{"MyFloat2"}, Vector2{ 1.1f, 2.2f });
        CheckPropertyValue<Vector3> (materialTypeAsset, Name{"MyFloat3"}, Vector3{ 3.3f, 4.4f, 5.5f });
        CheckPropertyValue<Vector4> (materialTypeAsset, Name{"MyFloat4"}, Vector4{ 6.6f, 7.7f, 8.8f, 9.9f });
        CheckPropertyValue<Color>   (materialTypeAsset, Name{"MyColor"}, Color{ 0.1f, 0.2f, 0.3f, 0.4f });
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{"MyImage"}, m_testImageAsset);
        CheckPropertyValue<uint32_t>(materialTypeAsset, Name{"MyEnum"}, 1u);
    }

    TEST_F(MaterialTypeAssetTests, EnumPropertyValues)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        MaterialTypeAssetCreator materialTypeCreator;

        const AZStd::vector<AZStd::string> enumNames = { "Enum0", "Enum1", "Enum2" };

        materialTypeCreator.Begin(Data::AssetId(Uuid::CreateRandom()));
        materialTypeCreator.BeginMaterialProperty(AZ::Name{ "MyEnum" }, MaterialPropertyDataType::Enum);
        materialTypeCreator.SetMaterialPropertyEnumNames(enumNames);
        materialTypeCreator.EndMaterialProperty();

        MaterialPropertyIndex propertyIndex = materialTypeCreator.GetMaterialPropertiesLayout()->FindPropertyIndex(AZ::Name{ "MyEnum" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeCreator.GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);
        for (uint32_t i = 0u; i < enumNames.size(); ++i)
        {
            AZ::Name enumName = AZ::Name(enumNames[i]);
            EXPECT_EQ(propertyDescriptor->GetEnumValue(enumName), i);

            // Also test utilities, though they have the same call.
            AZ::RPI::MaterialPropertyValue enumValue;
            MaterialUtils::ResolveMaterialPropertyEnumValue(propertyDescriptor, enumName, enumValue);
            EXPECT_TRUE(enumValue == i);
        }

        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));
    }

    TEST_F(MaterialTypeAssetTests, ConnectToShaderOptions)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        // Add the same shader twice to test making the property target options in multiple shaders
        materialTypeCreator.AddShader(m_testShaderAsset);
        materialTypeCreator.AddShader(m_testShaderAsset);
        // Add another shader that doesn't have shader options to demonstrate connecting to all 
        // shaders with a given option simply skips shaders that don't have that option
        materialTypeCreator.AddShader(CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout));

        materialTypeCreator.BeginMaterialProperty(Name{"Debug"}, MaterialPropertyDataType::Bool);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_debug"}); // Connects to both shaders automatically
        materialTypeCreator.EndMaterialProperty();

        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        const MaterialPropertiesLayout* propertyLayout = materialTypeAsset->GetMaterialPropertiesLayout();

        EXPECT_EQ(1, propertyLayout->GetPropertyCount());

        EXPECT_EQ(propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{0})->GetOutputConnections().size(), 2);
        EXPECT_EQ(propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{0})->GetOutputConnections()[0].m_type, MaterialPropertyOutputType::ShaderOption);
        EXPECT_EQ(propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{0})->GetOutputConnections()[0].m_containerIndex.GetIndex(), 0);
        EXPECT_EQ(propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{0})->GetOutputConnections()[0].m_itemIndex.GetIndex(), 0);
        EXPECT_EQ(propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{0})->GetOutputConnections()[1].m_type, MaterialPropertyOutputType::ShaderOption);
        EXPECT_EQ(propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{0})->GetOutputConnections()[1].m_containerIndex.GetIndex(), 1);
        EXPECT_EQ(propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{0})->GetOutputConnections()[1].m_itemIndex.GetIndex(), 0);
    }

    TEST_F(MaterialTypeAssetTests, ConnectToShaderEnabled)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.AddShader(m_testShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"first"});
        materialTypeCreator.AddShader(m_testShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"second"});
        materialTypeCreator.AddShader(m_testShaderAsset, AZ::RPI::ShaderVariantId{}, Name{"third"});

        materialTypeCreator.BeginMaterialProperty(Name{"SecondShaderEnabled"}, MaterialPropertyDataType::Bool);
        materialTypeCreator.ConnectMaterialPropertyToShaderEnabled(Name{"second"});
        materialTypeCreator.EndMaterialProperty();

        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        const MaterialPropertiesLayout* propertyLayout = materialTypeAsset->GetMaterialPropertiesLayout();

        EXPECT_EQ(1, propertyLayout->GetPropertyCount());

        EXPECT_EQ(propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{0})->GetOutputConnections().size(), 1);
        EXPECT_EQ(propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{0})->GetOutputConnections()[0].m_type, MaterialPropertyOutputType::ShaderEnabled);
        EXPECT_EQ(propertyLayout->GetPropertyDescriptor(MaterialPropertyIndex{0})->GetOutputConnections()[0].m_containerIndex.GetIndex(), 1);
    }

    TEST_F(MaterialTypeAssetTests, Error_SetPropertyInvalidInputs)
    {
        Data::AssetId assetId(Uuid::CreateRandom());

        // We use local functions to easily start a new MaterialAssetCreator for each test case because
        // the AssetCreator would just skip subsequent operations after the first failure is detected.

        auto expectCreatorError = [this](AZStd::function<void(MaterialTypeAssetCreator& creator)> passBadInput)
        {
            MaterialTypeAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());

            creator.AddShader(m_testShaderAsset);
            AddCommonTestMaterialProperties(creator);

            AZ_TEST_START_ASSERTTEST;
            passBadInput(creator);
            AZ_TEST_STOP_ASSERTTEST(1);

            EXPECT_EQ(1, creator.GetErrorCount());
        };

        auto expectCreatorWarning = [](AZStd::function<void(MaterialTypeAssetCreator& creator)> passBadInput)
        {
            MaterialTypeAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());

            passBadInput(creator);

            EXPECT_EQ(1, creator.GetWarningCount());
        };

        // Invalid input ID
        expectCreatorWarning([](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "BoolDoesNotExist" }, MaterialPropertyValue(false));
        });

        // Invalid image input ID
        expectCreatorWarning([this](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "ImageDoesNotExist" }, m_testImageAsset);
        });

        // Test data type mismatches...

        expectCreatorError([this](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyBool" }, m_testImageAsset);
        });

        expectCreatorError([](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyInt" }, 0.0f);
        });

        expectCreatorError([](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyUInt" }, -1);
        });

        expectCreatorError([](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyFloat" }, 10u);
        });

        expectCreatorError([](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyFloat2" }, 1.0f);
        });

        expectCreatorError([](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyFloat3" }, AZ::Vector4{});
        });

        expectCreatorError([](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyFloat4" }, AZ::Vector3{});
        });

        expectCreatorError([](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyColor" }, MaterialPropertyValue(false));
        });

        expectCreatorError([](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyImage" }, true);
        });

        expectCreatorError([](MaterialTypeAssetCreator& creator)
        {
            creator.SetPropertyValue(Name{ "MyEnum" }, -1);
        });
    }

    TEST_F(MaterialTypeAssetTests, ApplySetValues)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset);
        AddCommonTestMaterialProperties(materialTypeCreator);

        // Set some default values
        materialTypeCreator.SetPropertyValue(Name{"MyBool"},   true);
        materialTypeCreator.SetPropertyValue(Name{"MyFloat"},  1.2f);
        materialTypeCreator.SetPropertyValue(Name{"MyInt"},    -12);
        materialTypeCreator.SetPropertyValue(Name{"MyUInt"},   12u);
        materialTypeCreator.SetPropertyValue(Name{"MyFloat2"}, Vector2{ 1.1f, 2.2f });
        materialTypeCreator.SetPropertyValue(Name{"MyFloat3"}, Vector3{ 3.3f, 4.4f, 5.5f });
        materialTypeCreator.SetPropertyValue(Name{"MyFloat4"}, Vector4{ 6.6f, 7.7f, 8.8f, 9.9f });
        materialTypeCreator.SetPropertyValue(Name{"MyColor"},  Color{ 0.1f, 0.2f, 0.3f, 0.4f });
        materialTypeCreator.SetPropertyValue(Name{"MyEnum"}, 1u);
        materialTypeCreator.SetPropertyValue(Name{"MyImage"},  m_testImageAsset);

        // Add update rules to new values that aren't the original default
        {
            MaterialVersionUpdate versionUpdate(2);
            AddSetValueAction(versionUpdate, "MyBool",   false);
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }
        {
            MaterialVersionUpdate versionUpdate(3);
            AddSetValueAction(versionUpdate, "MyFloat",  2.0f);
            AddSetValueAction(versionUpdate, "MyInt",    3);
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }
        {
            MaterialVersionUpdate versionUpdate(4);
            AddSetValueAction(versionUpdate, "MyUInt",   4u);
            AddSetValueAction(versionUpdate, "MyFloat2", Vector2{ 5.1f, 5.2f });
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }
        {
            MaterialVersionUpdate versionUpdate(5);
            AddSetValueAction(versionUpdate, "MyFloat3", Vector3{ 6.1f, 6.2f, 6.3f });
            AddSetValueAction(versionUpdate, "MyFloat4", Vector4{ 7.1f, 7.2f, 7.3f, 7.4f });
            AddSetValueAction(versionUpdate, "MyColor",  Color{ 1.0f, 0.9f, 0.8f, 0.6f });
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }
        {
            MaterialVersionUpdate versionUpdate(7);
            AddSetValueAction(versionUpdate, "MyEnum",   2u);
            AddSetValueAction(versionUpdate, "MyImage",  m_testImageAsset2);
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }

        materialTypeCreator.SetVersion(10);
        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        // Add the MaterialTypeAsset to a MaterialAssets that are based on a range of
        // materialTypeAsset versions to trigger the value updates
        MaterialAssetCreator materialCreator;

        Data::Asset<MaterialAsset> materialAssetV1;
        materialCreator.Begin(Uuid::CreateRandom(), materialTypeAsset);
        materialCreator.SetMaterialTypeVersion(1);
        EXPECT_TRUE(materialCreator.End(materialAssetV1));

        Data::Asset<MaterialAsset> materialAssetV3;
        materialCreator.Begin(Uuid::CreateRandom(), materialTypeAsset);
        materialCreator.SetMaterialTypeVersion(3);
        EXPECT_TRUE(materialCreator.End(materialAssetV3));

        Data::Asset<MaterialAsset> materialAssetV6;
        materialCreator.Begin(Uuid::CreateRandom(), materialTypeAsset);
        materialCreator.SetMaterialTypeVersion(6);
        EXPECT_TRUE(materialCreator.End(materialAssetV6));

        Data::Asset<MaterialAsset> materialAssetV9;
        materialCreator.Begin(Uuid::CreateRandom(), materialTypeAsset);
        materialCreator.SetMaterialTypeVersion(9);
        EXPECT_TRUE(materialCreator.End(materialAssetV9));

        // Check that the correct values have been updated and resolved
        CheckMaterialPropertyValue<bool>    (materialAssetV1, Name{"MyBool"},   false);
        CheckMaterialPropertyValue<float>   (materialAssetV1, Name{"MyFloat"},  2.0f);
        CheckMaterialPropertyValue<int32_t> (materialAssetV1, Name{"MyInt"},    3);
        CheckMaterialPropertyValue<uint32_t>(materialAssetV1, Name{"MyUInt"},   4u);
        CheckMaterialPropertyValue<Vector2> (materialAssetV1, Name{"MyFloat2"}, Vector2{ 5.1f, 5.2f });
        CheckMaterialPropertyValue<Vector3> (materialAssetV1, Name{"MyFloat3"}, Vector3{ 6.1f, 6.2f, 6.3f });
        CheckMaterialPropertyValue<Vector4> (materialAssetV1, Name{"MyFloat4"}, Vector4{ 7.1f, 7.2f, 7.3f, 7.4f });
        CheckMaterialPropertyValue<Color>   (materialAssetV1, Name{"MyColor"},  Color{ 1.0f, 0.9f, 0.8f, 0.6f });
        CheckMaterialPropertyValue<uint32_t>(materialAssetV1, Name{"MyEnum"}, 2u);
        CheckMaterialPropertyValue<Data::Asset<ImageAsset>>(materialAssetV1, Name{"MyImage"}, m_testImageAsset2);

        CheckMaterialPropertyValue<bool>    (materialAssetV3, Name{"MyBool"},   true);
        CheckMaterialPropertyValue<float>   (materialAssetV3, Name{"MyFloat"},  1.2f);
        CheckMaterialPropertyValue<int32_t> (materialAssetV3, Name{"MyInt"},    -12);
        CheckMaterialPropertyValue<uint32_t>(materialAssetV3, Name{"MyUInt"},   4u);
        CheckMaterialPropertyValue<Vector2> (materialAssetV3, Name{"MyFloat2"}, Vector2{ 5.1f, 5.2f });
        CheckMaterialPropertyValue<Vector3> (materialAssetV3, Name{"MyFloat3"}, Vector3{ 6.1f, 6.2f, 6.3f });
        CheckMaterialPropertyValue<Vector4> (materialAssetV3, Name{"MyFloat4"}, Vector4{ 7.1f, 7.2f, 7.3f, 7.4f });
        CheckMaterialPropertyValue<Color>   (materialAssetV3, Name{"MyColor"},  Color{ 1.0f, 0.9f, 0.8f, 0.6f });
        CheckMaterialPropertyValue<uint32_t>(materialAssetV3, Name{"MyEnum"}, 2u);
        CheckMaterialPropertyValue<Data::Asset<ImageAsset>>(materialAssetV3, Name{"MyImage"}, m_testImageAsset2);

        CheckMaterialPropertyValue<bool>    (materialAssetV6, Name{"MyBool"},   true);
        CheckMaterialPropertyValue<float>   (materialAssetV6, Name{"MyFloat"},  1.2f);
        CheckMaterialPropertyValue<int32_t> (materialAssetV6, Name{"MyInt"},    -12);
        CheckMaterialPropertyValue<uint32_t>(materialAssetV6, Name{"MyUInt"},   12u);
        CheckMaterialPropertyValue<Vector2> (materialAssetV6, Name{"MyFloat2"}, Vector2{ 1.1f, 2.2f });
        CheckMaterialPropertyValue<Vector3> (materialAssetV6, Name{"MyFloat3"}, Vector3{ 3.3f, 4.4f, 5.5f });
        CheckMaterialPropertyValue<Vector4> (materialAssetV6, Name{"MyFloat4"}, Vector4{ 6.6f, 7.7f, 8.8f, 9.9f });
        CheckMaterialPropertyValue<Color>   (materialAssetV6, Name{"MyColor"},  Color{ 0.1f, 0.2f, 0.3f, 0.4f });
        CheckMaterialPropertyValue<uint32_t>(materialAssetV6, Name{"MyEnum"}, 2u);
        CheckMaterialPropertyValue<Data::Asset<ImageAsset>>(materialAssetV6, Name{"MyImage"}, m_testImageAsset2);

        CheckMaterialPropertyValue<bool>    (materialAssetV9, Name{"MyBool"},   true);
        CheckMaterialPropertyValue<float>   (materialAssetV9, Name{"MyFloat"},  1.2f);
        CheckMaterialPropertyValue<int32_t> (materialAssetV9, Name{"MyInt"},    -12);
        CheckMaterialPropertyValue<uint32_t>(materialAssetV9, Name{"MyUInt"},   12u);
        CheckMaterialPropertyValue<Vector2> (materialAssetV9, Name{"MyFloat2"}, Vector2{ 1.1f, 2.2f });
        CheckMaterialPropertyValue<Vector3> (materialAssetV9, Name{"MyFloat3"}, Vector3{ 3.3f, 4.4f, 5.5f });
        CheckMaterialPropertyValue<Vector4> (materialAssetV9, Name{"MyFloat4"}, Vector4{ 6.6f, 7.7f, 8.8f, 9.9f });
        CheckMaterialPropertyValue<Color>   (materialAssetV9, Name{"MyColor"},  Color{ 0.1f, 0.2f, 0.3f, 0.4f });
        CheckMaterialPropertyValue<uint32_t>(materialAssetV9, Name{"MyEnum"}, 1u);
        CheckMaterialPropertyValue<Data::Asset<ImageAsset>>(materialAssetV9, Name{"MyImage"}, m_testImageAsset);
    }

    TEST_F(MaterialTypeAssetTests, ApplySetValues_FuzzyCast)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset);
        AddCommonTestMaterialProperties(materialTypeCreator);

        // Set some default values
        materialTypeCreator.SetPropertyValue(Name{"MyBool"},   true);
        materialTypeCreator.SetPropertyValue(Name{"MyFloat"},  1.2f);
        materialTypeCreator.SetPropertyValue(Name{"MyInt"},    -12);
        materialTypeCreator.SetPropertyValue(Name{"MyUInt"},   12u);
        materialTypeCreator.SetPropertyValue(Name{"MyFloat3"}, Vector3{ 3.3f, 4.4f, 5.5f });
        materialTypeCreator.SetPropertyValue(Name{"MyFloat4"}, Vector4{ 6.6f, 7.7f, 8.8f, 9.9f });
        materialTypeCreator.SetPropertyValue(Name{"MyColor"},  Color{ 0.1f, 0.2f, 0.3f, 0.4f });

        // Set update rules to new values that aren't the original default
        // Use 'wrong' types that can nonetheless be fuzzily cast to the correct type
        MaterialVersionUpdate versionUpdate(2);
        AddSetValueAction(versionUpdate, "MyBool",   0);
        AddSetValueAction(versionUpdate, "MyFloat",  8u);    // powers of 2 should be preserved exactly in float<->int
        AddSetValueAction(versionUpdate, "MyInt",    -4.0f); // powers of 2 should be preserved exactly in float<->int
        AddSetValueAction(versionUpdate, "MyUInt",   4);
        AddSetValueAction(versionUpdate, "MyFloat3", Vector2{ 6.1f, 6.2f });
        AddSetValueAction(versionUpdate, "MyFloat4", Color{ 0.1f, 0.2f, 0.3f, 0.4f });
        AddSetValueAction(versionUpdate, "MyColor",  Vector3{ 1.0f, 0.9f, 0.8f });

        materialTypeCreator.SetVersion(versionUpdate.GetVersion());
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        // Add the MaterialTypeAsset to a MaterialAsset to trigger the value updates
        Data::Asset<MaterialAsset> materialAsset;
        MaterialAssetCreator materialCreator;
        materialCreator.Begin(Uuid::CreateRandom(), materialTypeAsset);
        materialCreator.SetMaterialTypeVersion(1);
        EXPECT_TRUE(materialCreator.End(materialAsset));

        // Check that the defaults have been updated and resolved
        CheckMaterialPropertyValue<bool>    (materialAsset, Name{"MyBool"},   false);
        CheckMaterialPropertyValue<float>   (materialAsset, Name{"MyFloat"},  8.0f);
        CheckMaterialPropertyValue<int32_t> (materialAsset, Name{"MyInt"},    -4);
        CheckMaterialPropertyValue<uint32_t>(materialAsset, Name{"MyUInt"},   4u);
        CheckMaterialPropertyValue<Vector3> (materialAsset, Name{"MyFloat3"}, Vector3{ 6.1f, 6.2f, 0.0f });
        CheckMaterialPropertyValue<Vector4> (materialAsset, Name{"MyFloat4"}, Vector4{ 0.1f, 0.2f, 0.3f, 0.4f });
        CheckMaterialPropertyValue<Color>   (materialAsset, Name{"MyColor"},  Color{ 1.0f, 0.9f, 0.8f, 1.0f });
    }

    TEST_F(MaterialTypeAssetTests, ApplyPropertyRenames)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        // Version updates
        materialTypeCreator.SetVersion(10);

        MaterialVersionUpdate versionUpdate2(2);
        AddRenameAction(versionUpdate2, "general.fooA", "general.fooB");
        materialTypeCreator.AddVersionUpdate(versionUpdate2);

        MaterialVersionUpdate versionUpdate4(4);
        AddRenameAction(versionUpdate4, "general.barA", "general.barB");
        materialTypeCreator.AddVersionUpdate(versionUpdate4);

        MaterialVersionUpdate versionUpdate6(6);
        AddRenameAction(versionUpdate6, "general.fooB", "general.fooC");
        AddRenameAction(versionUpdate6, "general.barB", "general.barC");
        materialTypeCreator.AddVersionUpdate(versionUpdate6);

        MaterialVersionUpdate versionUpdate7(7);
        AddRenameAction(versionUpdate7, "general.bazA", "otherGroup.bazB");
        materialTypeCreator.AddVersionUpdate(versionUpdate7);

        materialTypeCreator.BeginMaterialProperty(Name{ "general.fooC" }, MaterialPropertyDataType::Bool);
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{ "general.barC" }, MaterialPropertyDataType::Bool);
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{ "otherGroup.bazB" }, MaterialPropertyDataType::Bool);
        materialTypeCreator.EndMaterialProperty();

        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        AZ::Name propertyId;

        propertyId = AZ::Name{"doesNotExist"};
        EXPECT_FALSE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "doesNotExist");

        propertyId = AZ::Name{"general.fooA"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "general.fooC");

        propertyId = AZ::Name{"general.fooB"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "general.fooC");

        propertyId = AZ::Name{"general.fooC"};
        EXPECT_FALSE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "general.fooC");

        propertyId = AZ::Name{"general.barA"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "general.barC");

        propertyId = AZ::Name{"general.barB"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "general.barC");

        propertyId = AZ::Name{"general.barC"};
        EXPECT_FALSE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "general.barC");

        propertyId = AZ::Name{"general.bazA"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "otherGroup.bazB");

        propertyId = AZ::Name{"otherGroup.bazB"};
        EXPECT_FALSE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "otherGroup.bazB");
    }

    TEST_F(MaterialTypeAssetTests, ApplyPropertyRenamePrefixes)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        // Version updates
        materialTypeCreator.SetVersion(10);

        MaterialVersionUpdate versionUpdate2(2);
        AddRenamePrefixAction(versionUpdate2, "layer1_", "layer1.");
        AddRenamePrefixAction(versionUpdate2, "layer2_", "layer2.");
        materialTypeCreator.AddVersionUpdate(versionUpdate2);

        MaterialVersionUpdate versionUpdate4(4);
        AddRenamePrefixAction(versionUpdate4, "layer1.", "layerA.");
        AddRenamePrefixAction(versionUpdate4, "layer2.", "layerB.");
        materialTypeCreator.AddVersionUpdate(versionUpdate4);

        materialTypeCreator.BeginMaterialProperty(Name{"blend.factor"}, MaterialPropertyDataType::Float);
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"layerA.baseColor.color"}, MaterialPropertyDataType::Color);
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"layerA.baseColor.factor"}, MaterialPropertyDataType::Float);
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"layerB.baseColor.color"}, MaterialPropertyDataType::Color);
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"layerB.baseColor.factor"}, MaterialPropertyDataType::Float);
        materialTypeCreator.EndMaterialProperty();

        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        AZ::Name propertyId;

        // Handle version 1 style names

        propertyId = AZ::Name{"layer1_baseColor.color"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "layerA.baseColor.color");

        propertyId = AZ::Name{"layer1_baseColor.factor"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "layerA.baseColor.factor");

        propertyId = AZ::Name{"layer2_baseColor.color"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "layerB.baseColor.color");

        propertyId = AZ::Name{"layer2_baseColor.factor"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "layerB.baseColor.factor");

        // Handle version 2 style names

        propertyId = AZ::Name{"layer1.baseColor.color"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "layerA.baseColor.color");

        propertyId = AZ::Name{"layer1.baseColor.factor"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "layerA.baseColor.factor");

        propertyId = AZ::Name{"layer2.baseColor.color"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "layerB.baseColor.color");

        propertyId = AZ::Name{"layer2.baseColor.factor"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "layerB.baseColor.factor");

        // Other cases...

        propertyId = AZ::Name{"doesNotExist"};
        EXPECT_FALSE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "doesNotExist");

        propertyId = AZ::Name{"blend.factor"};
        EXPECT_FALSE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "blend.factor");

        propertyId = AZ::Name{"shouldNotBeRenamed_layer1_isNotAPrefix"};
        EXPECT_FALSE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "shouldNotBeRenamed_layer1_isNotAPrefix");

        // The first "layer1_" is a prefix but the other is not
        propertyId = AZ::Name{"layer1_theNext_layer1_isNotAPrefix"};
        EXPECT_TRUE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "layerA.theNext_layer1_isNotAPrefix");

        // The replacement is case sensitive
        propertyId = AZ::Name{"Layer1_foo"};
        EXPECT_FALSE(materialTypeAsset->ApplyPropertyRenames(propertyId));
        EXPECT_STREQ(propertyId.GetCStr(), "Layer1_foo");
    }

    TEST_F(MaterialTypeAssetTests, Error_InternalPipelineProperty_ConnectToSRG)
    {
        // Internal properties of the material pipeline do not have access to the MaterialSRG because
        // the material type and material pipeline are decoupled.

        using namespace AZ;
        using namespace AZ::RPI;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        // Include a shader for both MaterialPipelineNone and "TestPipeline" because it doesn't matter where the ShaderResourceGroup
        // appears, the material pipeline should not have access to it. 
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, MaterialPipelineNone);
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"TestPipeline"});

        materialTypeCreator.BeginMaterialProperty(Name{"materialPipelineBoolProperty"}, MaterialPropertyDataType::Bool, Name{"TestPipeline"});

        ErrorMessageFinder errorMessageFinder("Material property 'materialPipelineBoolProperty': Connection type 'ShaderInput' is not supported by internal material pipeline properties.");
        errorMessageFinder.AddIgnoredErrorMessage("Cannot continue building", true);
        materialTypeCreator.ConnectMaterialPropertyToShaderInput(Name{"m_bool"});
        materialTypeCreator.EndMaterialProperty();
        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(materialTypeCreator.GetMaterialPropertiesLayout(Name{"TestPipeline"})->GetPropertyCount(), 0);
    }

    TEST_F(MaterialTypeAssetTests, MaterialProperty_ConnectToShaderOption_AccessesMaterialPipelineShaders)
    {
        // Normal material property connections to ShaderOption will apply to every shader in the material type,
        // including any shaders that are inside MaterialPipeline(s).

        using namespace AZ;
        using namespace AZ::RPI;

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, MaterialPipelineNone);
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, MaterialPipelineNone);
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineA"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineA"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineB"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineB"});

        materialTypeCreator.BeginMaterialProperty(Name{"debug"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_debug"});
        materialTypeCreator.EndMaterialProperty();

        materialTypeCreator.End(materialTypeAsset);

        EXPECT_EQ(materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyCount(), 1);
        const MaterialPropertyDescriptor* property = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(MaterialPropertyIndex{0});
        EXPECT_EQ(property->GetOutputConnections().size(), 6);

        ShaderOptionIndex shaderOptionIndex = m_testShaderAsset->GetShaderOptionGroupLayout()->FindShaderOptionIndex(Name{"o_debug"});

        auto checkShaderOption = [property, shaderOptionIndex](size_t connectionIndex, Name materialPipelineName, uint32_t shaderIndex)
        {
            EXPECT_EQ(property->GetOutputConnections()[connectionIndex].m_type, MaterialPropertyOutputType::ShaderOption);
            EXPECT_EQ(property->GetOutputConnections()[connectionIndex].m_materialPipelineName, materialPipelineName);
            EXPECT_EQ(property->GetOutputConnections()[connectionIndex].m_containerIndex.GetIndex(), shaderIndex);
            EXPECT_EQ(property->GetOutputConnections()[connectionIndex].m_itemIndex.GetIndex(), shaderOptionIndex.GetIndex());
        };

        checkShaderOption(0, MaterialPipelineNone, 0);
        checkShaderOption(1, MaterialPipelineNone, 1);
        checkShaderOption(2, Name{"PipelineA"}, 0);
        checkShaderOption(3, Name{"PipelineA"}, 1);
        checkShaderOption(4, Name{"PipelineB"}, 0);
        checkShaderOption(5, Name{"PipelineB"}, 1);
    }

    TEST_F(MaterialTypeAssetTests, InternalPipelineProperty_ConnectToShaderOption_AccessesLocalShadersOnly)
    {
        // Internal material properties that are part of a material pipeline should only set shader options
        // on the shaders that are part of that pipeline.

        using namespace AZ;
        using namespace AZ::RPI;

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, MaterialPipelineNone);
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, MaterialPipelineNone);
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineA"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineA"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineB"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineB"});

        materialTypeCreator.BeginMaterialProperty(Name{"debug"}, MaterialPropertyDataType::Bool, Name{"PipelineA"});
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_debug"});
        materialTypeCreator.EndMaterialProperty();

        materialTypeCreator.End(materialTypeAsset);

        auto pipelineIter = materialTypeAsset->GetMaterialPipelinePayloads().find(Name{"PipelineA"});
        EXPECT_TRUE(pipelineIter != materialTypeAsset->GetMaterialPipelinePayloads().end());
        EXPECT_EQ(pipelineIter->second.m_materialPropertiesLayout->GetPropertyCount(), 1);
        const MaterialPropertyDescriptor* property = pipelineIter->second.m_materialPropertiesLayout->GetPropertyDescriptor(MaterialPropertyIndex{0});
        EXPECT_EQ(property->GetOutputConnections().size(), 2);

        ShaderOptionIndex shaderOptionIndex = m_testShaderAsset->GetShaderOptionGroupLayout()->FindShaderOptionIndex(Name{"o_debug"});

        auto checkShaderOption = [property, shaderOptionIndex](size_t connectionIndex, Name materialPipelineName, uint32_t shaderIndex)
        {
            EXPECT_EQ(property->GetOutputConnections()[connectionIndex].m_type, MaterialPropertyOutputType::ShaderOption);
            EXPECT_EQ(property->GetOutputConnections()[connectionIndex].m_materialPipelineName, materialPipelineName);
            EXPECT_EQ(property->GetOutputConnections()[connectionIndex].m_containerIndex.GetIndex(), shaderIndex);
            EXPECT_EQ(property->GetOutputConnections()[connectionIndex].m_itemIndex.GetIndex(), shaderOptionIndex.GetIndex());
        };

        checkShaderOption(0, Name{"PipelineA"}, 0);
        checkShaderOption(1, Name{"PipelineA"}, 1);
    }

    TEST_F(MaterialTypeAssetTests, MaterialProperty_ConnectToShaderEnable_AccessesGeneralShadersOnly)
    {
        // Normal material property connections to ShaderEnable will only apply to the general ShaderCollection,
        // not any of the shaders within individual material pipelines.

        using namespace AZ;
        using namespace AZ::RPI;

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderA"}, MaterialPipelineNone);
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderB"}, MaterialPipelineNone);
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderA"}, Name{"PipelineA"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderB"}, Name{"PipelineA"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderA"}, Name{"PipelineB"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderB"}, Name{"PipelineB"});

        materialTypeCreator.BeginMaterialProperty(Name{"enable"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.ConnectMaterialPropertyToShaderEnabled(Name{"shaderB"});
        materialTypeCreator.EndMaterialProperty();

        materialTypeCreator.End(materialTypeAsset);

        EXPECT_EQ(materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyCount(), 1);
        const MaterialPropertyDescriptor* property = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(MaterialPropertyIndex{0});
        EXPECT_EQ(property->GetOutputConnections().size(), 1);
        EXPECT_EQ(property->GetOutputConnections()[0].m_type, MaterialPropertyOutputType::ShaderEnabled);
        EXPECT_EQ(property->GetOutputConnections()[0].m_materialPipelineName, MaterialPipelineNone);
        EXPECT_EQ(property->GetOutputConnections()[0].m_containerIndex.GetIndex(), 1);
        EXPECT_FALSE(property->GetOutputConnections()[0].m_itemIndex.IsValid());
    }

    TEST_F(MaterialTypeAssetTests, InternalPipelineProperty_ConnectToShaderEnable_AccessesLocalShadersOnly)
    {
        // Internal material properties that are part of a material pipeline should only enable/disable
        // shaders that are part of that pipeline.

        using namespace AZ;
        using namespace AZ::RPI;

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderA"}, MaterialPipelineNone);
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderB"}, MaterialPipelineNone);
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderA"}, Name{"PipelineA"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderB"}, Name{"PipelineA"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderA"}, Name{"PipelineB"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shaderB"}, Name{"PipelineB"});

        materialTypeCreator.BeginMaterialProperty(Name{"enable"}, MaterialPropertyDataType::Bool, Name{"PipelineA"});
        materialTypeCreator.ConnectMaterialPropertyToShaderEnabled(Name{"shaderB"});
        materialTypeCreator.EndMaterialProperty();

        materialTypeCreator.End(materialTypeAsset);

        auto pipelineIter = materialTypeAsset->GetMaterialPipelinePayloads().find(Name{"PipelineA"});
        EXPECT_TRUE(pipelineIter != materialTypeAsset->GetMaterialPipelinePayloads().end());
        EXPECT_EQ(pipelineIter->second.m_materialPropertiesLayout->GetPropertyCount(), 1);
        const MaterialPropertyDescriptor* property = pipelineIter->second.m_materialPropertiesLayout->GetPropertyDescriptor(MaterialPropertyIndex {0});
        EXPECT_EQ(property->GetOutputConnections().size(), 1);
        EXPECT_EQ(property->GetOutputConnections()[0].m_type, MaterialPropertyOutputType::ShaderEnabled);
        EXPECT_EQ(property->GetOutputConnections()[0].m_materialPipelineName, Name {"PipelineA"});
        EXPECT_EQ(property->GetOutputConnections()[0].m_containerIndex.GetIndex(), 1);
        EXPECT_FALSE(property->GetOutputConnections()[0].m_itemIndex.IsValid());
    }

    TEST_F(MaterialTypeAssetTests, MaterialProperty_ConnectToInternalProperties)
    {
        // Material properties can connect to internal properties to pass data along to the MaterialPipelinePayload(s).

        using namespace AZ;
        using namespace AZ::RPI;

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineA"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineB"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineC"});

        // PipelineA properties
        materialTypeCreator.BeginMaterialProperty(Name{"unused1"}, MaterialPropertyDataType::Bool, Name{"PipelineA"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"unused2"}, MaterialPropertyDataType::Bool, Name{"PipelineA"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"castShadows"}, MaterialPropertyDataType::Bool, Name{"PipelineA"});
        materialTypeCreator.EndMaterialProperty();

        // PipelineB properties
        materialTypeCreator.BeginMaterialProperty(Name{"unused1"}, MaterialPropertyDataType::Bool, Name{"PipelineB"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"unused2"}, MaterialPropertyDataType::Bool, Name{"PipelineB"});
        materialTypeCreator.EndMaterialProperty();

        // PipelineC properties
        materialTypeCreator.BeginMaterialProperty(Name{"unused1"}, MaterialPropertyDataType::Bool, Name{"PipelineC"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"castShadows"}, MaterialPropertyDataType::Bool, Name{"PipelineC"});
        materialTypeCreator.EndMaterialProperty();
        materialTypeCreator.BeginMaterialProperty(Name{"unused2"}, MaterialPropertyDataType::Bool, Name{"PipelineC"});
        materialTypeCreator.EndMaterialProperty();

        // This main property has the same name as the ones in the material pipelines, but will not be connected because it is not
        // an "internal" property.
        materialTypeCreator.BeginMaterialProperty(Name{"castShadows"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.EndMaterialProperty();

        // This is the main enableShadows property that will connect to the others
        materialTypeCreator.BeginMaterialProperty(Name{"general.enableShadows"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.ConnectMaterialPropertyToInternalProperty(Name{"castShadows"});
        materialTypeCreator.EndMaterialProperty();

        materialTypeCreator.End(materialTypeAsset);

        EXPECT_EQ(materialTypeAsset->GetMaterialPipelinePayloads().size(), 3);

        EXPECT_EQ(materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyCount(), 2);
        const MaterialPropertyDescriptor* property = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(MaterialPropertyIndex{1});
        EXPECT_EQ(property->GetName(), Name("general.enableShadows"));
        EXPECT_EQ(property->GetOutputConnections().size(), 2);
        EXPECT_EQ(property->GetOutputConnections()[0].m_type, MaterialPropertyOutputType::InternalProperty);
        EXPECT_EQ(property->GetOutputConnections()[0].m_materialPipelineName, Name {"PipelineA"});
        EXPECT_FALSE(property->GetOutputConnections()[0].m_containerIndex.IsValid());
        EXPECT_EQ(property->GetOutputConnections()[0].m_itemIndex.GetIndex(), 2);
        EXPECT_EQ(property->GetOutputConnections()[1].m_type, MaterialPropertyOutputType::InternalProperty);
        EXPECT_EQ(property->GetOutputConnections()[1].m_materialPipelineName, Name {"PipelineC"});
        EXPECT_FALSE(property->GetOutputConnections()[1].m_containerIndex.IsValid());
        EXPECT_EQ(property->GetOutputConnections()[1].m_itemIndex.GetIndex(), 1);
    }

    TEST_F(MaterialTypeAssetTests, Error_InternalPropertyCannotConnectToInternalProperties)
    {
        // Internal material properties can connect to other internal properties.

        using namespace AZ;
        using namespace AZ::RPI;

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineA"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineB"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineC"});

        // PipelineA property
        materialTypeCreator.BeginMaterialProperty(Name{"castShadows"}, MaterialPropertyDataType::Bool, Name{"PipelineA"});
        materialTypeCreator.EndMaterialProperty();

        ErrorMessageFinder errorMessageFinder("Material property 'otherCastShadows': Internal properties cannot be connected to other internal properties.");
        errorMessageFinder.AddIgnoredErrorMessage("Cannot continue building", true);

        // PipelineB property tries to connect to pipelineA's property
        materialTypeCreator.BeginMaterialProperty(Name{"otherCastShadows"}, MaterialPropertyDataType::Bool, Name{"PipelineB"});
        materialTypeCreator.ConnectMaterialPropertyToInternalProperty(Name{"castShadows"});
        materialTypeCreator.EndMaterialProperty();

        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTypeAssetTests, Error_MaterialProperty_ConnectToInternalPropertiesWithWrongDataType)
    {
        using namespace AZ;
        using namespace AZ::RPI;

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineA"});

        // PipelineA property is a int
        materialTypeCreator.BeginMaterialProperty(Name{"someInt"}, MaterialPropertyDataType::UInt, Name{"PipelineA"});
        materialTypeCreator.EndMaterialProperty();

        ErrorMessageFinder errorMessageFinder("Material property 'someFloat': Cannot connect to internal property 'someInt' because the data types do not match.");
        errorMessageFinder.AddIgnoredErrorMessage("Cannot continue building", true);

        // This main property is a float, so it cannot connect
        materialTypeCreator.BeginMaterialProperty(Name{"someFloat"}, MaterialPropertyDataType::Float, MaterialPipelineNone);
        materialTypeCreator.ConnectMaterialPropertyToInternalProperty(Name{"someInt"});
        materialTypeCreator.EndMaterialProperty();

        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTypeAssetTests, Error_MaterialProperty_ConnectToInternalPropertyThatDoesNotExist)
    {
        using namespace AZ;
        using namespace AZ::RPI;

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{}, Name{"PipelineA"});

        // PipelineA property
        materialTypeCreator.BeginMaterialProperty(Name{"enableSomething"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.EndMaterialProperty();

        ErrorMessageFinder errorMessageFinder("Material property 'someBool': Material contains no internal property 'doesNotExist'");
        errorMessageFinder.AddIgnoredErrorMessage("Cannot continue building", true);

        // This property connects to something that doers not exist
        materialTypeCreator.BeginMaterialProperty(Name{"someBool"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.ConnectMaterialPropertyToInternalProperty(Name{"doesNotExist"});
        materialTypeCreator.EndMaterialProperty();

        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTypeAssetTests, CreateWithMaterialPipelines)
    {
        using namespace AZ;
        using namespace AZ::RPI;

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        // The test will set up a structure like this:
        //   Properties
        //       "general.enableShadows" connects to "castShadows" in each pipeline
        //   Material pipelines
        //       "MainPipeline"
        //           Properties
        //               "castShadows" connects to enable the local "shadows" shader
        //           Shaders
        //               "depth"
        //               "shadows"
        //               "forward"
        //       "DeferredPipeline"
        //           Properties
        //               "unused"
        //               "castShadows" doesn't have any direct connections
        //           Functors
        //               DummyShaderCollectionFunctor reads "castShadows" to enable the local "shadows" shader
        //           Shaders
        //               "shadows"
        //               "deferred"
        //       "LowEndPipeline"
        //           Properties are empty
        //           Shaders
        //               "forward"

        // Note we just use the same shader asset repeatedly for simplicity, not realism.

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"depth"}, Name{"MainPipeline"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shadows"}, Name{"MainPipeline"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"forward"}, Name{"MainPipeline"});

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"shadows"}, Name{"DeferredPipeline"});
        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"deferred"}, Name{"DeferredPipeline"});

        materialTypeCreator.AddShader(m_testShaderAsset, ShaderVariantId{}, Name{"forward"}, Name{"LowEndPipeline"});

        // This internal property enables the shadow shader via direct connection
        materialTypeCreator.BeginMaterialProperty(Name{"castShadows"}, MaterialPropertyDataType::Bool, Name{"MainPipeline"});
        materialTypeCreator.ConnectMaterialPropertyToShaderEnabled(Name{"shadows"});
        materialTypeCreator.EndMaterialProperty();

        // This property is just shifting the property index for more interesting testing
        materialTypeCreator.BeginMaterialProperty(Name{"unused"}, MaterialPropertyDataType::Bool, Name{"DeferredPipeline"});
        materialTypeCreator.EndMaterialProperty();

        // This internal property enables the shadow shader via material functor
        materialTypeCreator.BeginMaterialProperty(Name{"castShadows"}, MaterialPropertyDataType::Bool, Name{"DeferredPipeline"});
        materialTypeCreator.EndMaterialProperty();
        Ptr<DummyShaderCollectionFunctor> shaderCollectionFunctor = aznew DummyShaderCollectionFunctor;
        shaderCollectionFunctor->m_enableIndex = materialTypeCreator.GetMaterialPropertiesLayout(Name{"DeferredPipeline"})->FindPropertyIndex(Name{"castShadows"});
        materialTypeCreator.AddMaterialFunctor(shaderCollectionFunctor, Name{"DeferredPipeline"});

        // This external property connects to the ther internal properties via direct "InternalProperty" connection
        materialTypeCreator.BeginMaterialProperty(Name{"general.enableShadows"}, MaterialPropertyDataType::Bool, MaterialPipelineNone);
        materialTypeCreator.ConnectMaterialPropertyToInternalProperty(Name{"castShadows"});
        materialTypeCreator.EndMaterialProperty();

        materialTypeCreator.SetPropertyValue(Name{"castShadows"}, MaterialPropertyValue{true}, Name{"MainPipeline"});
        materialTypeCreator.SetPropertyValue(Name{"castShadows"}, MaterialPropertyValue{false}, Name{"DeferredPipeline"});
        materialTypeCreator.SetPropertyValue(Name{"general.enableShadows"}, MaterialPropertyValue{true}, MaterialPipelineNone);

        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        EXPECT_EQ(materialTypeAsset->GetMaterialPipelinePayloads().size(), 3);

        auto mainPipelineIter = materialTypeAsset->GetMaterialPipelinePayloads().find(Name{"MainPipeline"});
        auto deferredPipelineIter = materialTypeAsset->GetMaterialPipelinePayloads().find(Name{"DeferredPipeline"});
        auto lowEndPipelineIter = materialTypeAsset->GetMaterialPipelinePayloads().find(Name{"LowEndPipeline"});
        EXPECT_TRUE(mainPipelineIter != materialTypeAsset->GetMaterialPipelinePayloads().end());
        EXPECT_TRUE(deferredPipelineIter != materialTypeAsset->GetMaterialPipelinePayloads().end());
        EXPECT_TRUE(lowEndPipelineIter != materialTypeAsset->GetMaterialPipelinePayloads().end());

        // Check the "MainPipeline" internal shadow property
        const MaterialPropertiesLayout* layout = mainPipelineIter->second.m_materialPropertiesLayout.get();
        EXPECT_EQ(layout->GetPropertyCount(), 1);
        MaterialPropertyIndex propertyIndex = layout->FindPropertyIndex(Name("castShadows"));
        EXPECT_EQ(propertyIndex.GetIndex(), 0);
        EXPECT_EQ(layout->GetPropertyDescriptor(propertyIndex)->GetName(), Name("castShadows"));
        EXPECT_EQ(mainPipelineIter->second.m_defaultPropertyValues[propertyIndex.GetIndex()], MaterialPropertyValue(true));
        EXPECT_EQ(mainPipelineIter->second.m_materialFunctors.size(), 0);

        // Check the "MainPipeline" internal shadow property connections
        MaterialPropertyDescriptor::OutputList connections = layout->GetPropertyDescriptor(propertyIndex)->GetOutputConnections();
        EXPECT_EQ(connections.size(), 1);
        EXPECT_EQ(connections[0].m_materialPipelineName, Name("MainPipeline"));
        EXPECT_EQ(connections[0].m_type, MaterialPropertyOutputType::ShaderEnabled);
        EXPECT_EQ(connections[0].m_containerIndex.GetIndex(), 1);
        EXPECT_FALSE(connections[0].m_itemIndex.IsValid());

        // Check the "DeferredPipeline" internal shadow property
        layout = deferredPipelineIter->second.m_materialPropertiesLayout.get();
        EXPECT_EQ(layout->GetPropertyCount(), 2);
        propertyIndex = layout->FindPropertyIndex(Name("castShadows"));
        EXPECT_EQ(propertyIndex.GetIndex(), 1);
        EXPECT_EQ(layout->GetPropertyDescriptor(propertyIndex)->GetName(), Name("castShadows"));
        EXPECT_EQ(deferredPipelineIter->second.m_defaultPropertyValues[propertyIndex.GetIndex()], MaterialPropertyValue(false));
        EXPECT_EQ(layout->GetPropertyDescriptor(propertyIndex)->GetOutputConnections().size(), 0);
        EXPECT_EQ(deferredPipelineIter->second.m_materialFunctors.size(), 1);

        // Check the "LowEndPipeline" internal properties empty
        EXPECT_EQ(lowEndPipelineIter->second.m_materialPropertiesLayout->GetPropertyCount(), 0);

        // Check the external shadow property
        layout = materialTypeAsset->GetMaterialPropertiesLayout();
        EXPECT_EQ(layout->GetPropertyCount(), 1);
        propertyIndex = layout->FindPropertyIndex(Name("general.enableShadows"));
        EXPECT_EQ(propertyIndex.GetIndex(), 0);
        EXPECT_EQ(layout->GetPropertyDescriptor(propertyIndex)->GetName(), Name("general.enableShadows"));
        EXPECT_EQ(materialTypeAsset->GetDefaultPropertyValues()[propertyIndex.GetIndex()], MaterialPropertyValue(true));

        // Check the external shadow property connections
        connections = layout->GetPropertyDescriptor(propertyIndex)->GetOutputConnections();
        EXPECT_EQ(connections.size(), 2);
        EXPECT_EQ(connections[0].m_type, MaterialPropertyOutputType::InternalProperty);
        EXPECT_EQ(connections[1].m_type, MaterialPropertyOutputType::InternalProperty);
        EXPECT_EQ(connections[0].m_materialPipelineName, Name("MainPipeline"));
        EXPECT_EQ(connections[1].m_materialPipelineName, Name("DeferredPipeline"));
        EXPECT_EQ(connections[0].m_itemIndex.GetIndex(), 0);
        EXPECT_EQ(connections[1].m_itemIndex.GetIndex(), 1);
        EXPECT_FALSE(connections[0].m_containerIndex.IsValid());
        EXPECT_FALSE(connections[1].m_containerIndex.IsValid());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_NoOperation)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("The operation type under the 'op' key was missing or empty");
        errorMessageFinder.AddExpectedErrorMessage("Material version update action was not properly initialized: empty operation");

        materialTypeCreator.Begin(assetId);

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(
            {
                { AZStd::string("name"),  AZStd::string("MyInt") },
                { AZStd::string("value"), 123 }
            } ));
        materialTypeCreator.SetVersion(versionUpdate.GetVersion());
        materialTypeCreator.AddVersionUpdate(versionUpdate);
        materialTypeCreator.AddShader(m_testShaderAsset);

        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();
        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_WrongOperationType)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("The operation type under the 'op' key should be a string");
        errorMessageFinder.AddExpectedErrorMessage("Material version update action was not properly initialized: empty operation");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action({ { AZStd::string("op"), 123 } }));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_UnknownOperation)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Unknown operation 'UnknownOperation' in material version update action");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(AZ::Name{ "UnknownOperation" }, { } ));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_Rename_WrongName)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        // Invalid version updates
        MaterialVersionUpdate versionUpdate(2);
        AddRenameAction(versionUpdate, "EnableSpecialPassPrev", "InvalidPropertyName");
        materialTypeCreator.SetVersion(versionUpdate.GetVersion());
        materialTypeCreator.AddVersionUpdate(versionUpdate);
        materialTypeCreator.AddShader(m_testShaderAsset);

        materialTypeCreator.BeginMaterialProperty(Name{ "EnableSpecialPass" }, MaterialPropertyDataType::Bool);
        materialTypeCreator.EndMaterialProperty();

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_Rename_WrongOrder)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(4);
        materialTypeCreator.AddShader(m_testShaderAsset);
        materialTypeCreator.BeginMaterialProperty(Name{ "d" }, MaterialPropertyDataType::Bool);
        materialTypeCreator.EndMaterialProperty();

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Version updates are not sequential. See version update '3'");

        {
            MaterialVersionUpdate versionUpdate(2);
            AddRenameAction(versionUpdate, "a", "b");
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }

        {
            MaterialVersionUpdate versionUpdate(4);
            AddRenameAction(versionUpdate, "b", "c");
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }

        {
            MaterialVersionUpdate versionUpdate(3);
            AddRenameAction(versionUpdate, "c", "d");
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_Rename_NotEnoughArgs)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Expected 2 arguments in 'rename' version update ('from', 'to'), but found 1");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(
            AZ::Name{ "rename" },
            {
                { Name{ "to" }, Name{ "newName" } }
            } ));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_Rename_NoFromArg)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Expected a 'from' field in 'rename' of type string");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(
            AZ::Name{ "rename" },
            {
                { Name{ "notFrom" }, Name{ "oldName" } },
                { Name{ "to" }, Name{ "newName" } }
            } ));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_Rename_NoToArg)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Expected a 'to' field in 'rename' of type string");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(
            AZ::Name{ "rename" },
            {
                { Name{ "from" }, Name{ "oldName" } },
                { Name{ "notTo" }, Name{ "newName" } }
            } ));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_Rename_WrongFromType)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Expected a 'from' field in 'rename' of type string");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(
            AZ::Name{ "rename" },
            {
                { Name{ "from" }, 123 },
                { Name{ "to" }, Name{ "newName" } }
            } ));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_Rename_WrongToType)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Expected a 'to' field in 'rename' of type string");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(
            AZ::Name{ "rename" },
            {
                { Name{ "from" }, Name{ "oldName" } },
                { Name{ "to" }, 123 }
            } ));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_SetValue_UnknownProperty)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset);
        AddCommonTestMaterialProperties(materialTypeCreator);

        MaterialVersionUpdate versionUpdate(2);
        AddSetValueAction(versionUpdate, "InvalidPropertyName", 123);

        materialTypeCreator.SetVersion(versionUpdate.GetVersion());
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage(
            "Could not find property 'InvalidPropertyName' in the material properties layout");

        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_SetValue_InvalidType)
    {
        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        materialTypeCreator.AddShader(m_testShaderAsset);
        AddCommonTestMaterialProperties(materialTypeCreator);

        MaterialVersionUpdate versionUpdate(2);
        AddSetValueAction(versionUpdate, "MyFloat", AZStd::string("ThisIsNotAFloatingPointNumber"));

        materialTypeCreator.SetVersion(versionUpdate.GetVersion());
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Unexpected type for property 'MyFloat'");

        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_SetValue_NotEnoughArgs)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Expected 2 arguments in 'setValue' version update ('name', 'value'), but found 1");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(
            AZ::Name{ "setValue" },
            {
                { Name{ "name" }, AZStd::string("MyInt") },
            } ));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_SetValue_NoNameArg)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Expected a 'name' field in 'setValue' of type string");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(
            AZ::Name{ "setValue" },
            {
                { Name{ "notName" }, AZStd::string("MyInt") },
                { Name{ "value" }, 123 }
            } ));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_SetValue_NoValueArg)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);
        AddCommonTestMaterialProperties(materialTypeCreator);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Expected a 'value' field in 'setValue'");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(
            AZ::Name{ "setValue" },
            {
                { Name{ "name" }, AZStd::string("MyInt") },
                { Name{ "notValue" }, 123 }
            } ));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_SetValue_WrongNameType)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());

        materialTypeCreator.SetVersion(2);
        materialTypeCreator.AddShader(m_testShaderAsset);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Expected a 'name' field in 'setValue' of type string");

        MaterialVersionUpdate versionUpdate(2);
        versionUpdate.AddAction(MaterialVersionUpdate::Action(
            AZ::Name{ "setValue" },
            {
                { Name{ "name" }, 123 },
                { Name{ "value" }, 123 }
            } ));
        materialTypeCreator.AddVersionUpdate(versionUpdate);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidMaterialVersionUpdate_GoesTooFar)
    {
        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        
        materialTypeCreator.SetVersion(3);
        materialTypeCreator.AddShader(m_testShaderAsset);
        materialTypeCreator.BeginMaterialProperty(Name{ "d" }, MaterialPropertyDataType::Bool);
        materialTypeCreator.EndMaterialProperty();

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Version updates go beyond the current material type version. See version update '4'");

        {
            MaterialVersionUpdate versionUpdate(2);
            AddRenameAction(versionUpdate, "a", "b");
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }
        
        {
            MaterialVersionUpdate versionUpdate(4);
            AddRenameAction(versionUpdate, "b", "c");
            materialTypeCreator.AddVersionUpdate(versionUpdate);
        }

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_FALSE(materialTypeCreator.End(materialTypeAsset));

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(1, materialTypeCreator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, MaterialTypeWithNoSRGOrProperties)
    {
        // Making a material type with no properties and no SRG allows us to create simple shaders
        // that don't need any input, for example a debug shader that just renders surface normals.

        Data::AssetId assetId(Uuid::CreateRandom());

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(assetId);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;
        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));
        EXPECT_EQ(assetId, materialTypeAsset->GetId());

        // Also test serialization...
        SerializeTester<RPI::MaterialTypeAsset> tester(GetSerializeContext());
        tester.SerializeOut(materialTypeAsset.Get(), AZ::DataStream::ST_XML);
        materialTypeAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));

        EXPECT_FALSE(materialTypeAsset->GetMaterialSrgLayout());
        EXPECT_EQ(0, materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyCount());
        EXPECT_EQ(0, materialTypeAsset->GetMaterialFunctors().size());
    }

    TEST_F(MaterialTypeAssetTests, TestWithMultipleShaders)
    {
        Data::Asset<ShaderAsset> shaderA = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, m_testShaderOptionsLayout);
        Data::Asset<ShaderAsset> shaderB = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, m_testShaderOptionsLayout);
        Data::Asset<ShaderAsset> shaderC = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, m_testShaderOptionsLayout);

        ShaderOptionGroup optionsA{m_testShaderOptionsLayout};
        ShaderOptionGroup optionsB{m_testShaderOptionsLayout};

        optionsA.SetValue(Name{"o_quality"}, Name{"Med"});
        optionsA.SetValue(Name{"o_lightCount"}, ShaderOptionValue{5});
        optionsB.SetValue(Name{"o_quality"}, Name{"High"});
        optionsB.SetValue(Name{"o_lightCount"}, ShaderOptionValue{3});

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(shaderA, optionsA.GetShaderVariantId());
        materialTypeCreator.AddShader(shaderB, optionsB.GetShaderVariantId());
        materialTypeCreator.AddShader(shaderC);
        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));
        
        // Also test serialization...
        SerializeTester<RPI::MaterialTypeAsset> tester(GetSerializeContext());
        tester.SerializeOut(materialTypeAsset.Get());
        materialTypeAsset = tester.SerializeIn(Data::AssetId(Uuid::CreateRandom()));

        const ShaderCollection& shaderCollection = materialTypeAsset->GetGeneralShaderCollection();

        EXPECT_EQ(3, shaderCollection.size());
        EXPECT_EQ(shaderA, shaderCollection[0].GetShaderAsset());
        EXPECT_EQ(shaderB, shaderCollection[1].GetShaderAsset());
        EXPECT_EQ(shaderC, shaderCollection[2].GetShaderAsset());

        EXPECT_EQ(shaderCollection[0].GetShaderOptions()->GetValue(Name{"o_quality"}).GetIndex(),
            m_testShaderOptionsLayout->FindValue(Name{"o_quality"}, Name{"Med"}).GetIndex());
        EXPECT_EQ(shaderCollection[0].GetShaderOptions()->GetValue(Name{"o_lightCount"}).GetIndex(), 5);
        EXPECT_EQ(shaderCollection[1].GetShaderOptions()->GetValue(Name{"o_quality"}).GetIndex(),
            m_testShaderOptionsLayout->FindValue(Name{"o_quality"}, Name{"High"}).GetIndex());
        EXPECT_EQ(shaderCollection[1].GetShaderOptions()->GetValue(Name{"o_lightCount"}).GetIndex(), 3);
        EXPECT_FALSE(shaderCollection[2].GetShaderOptions()->GetValue(Name{"o_quality"}).IsValid());
        EXPECT_FALSE(shaderCollection[2].GetShaderOptions()->GetValue(Name{"o_lightCount"}).IsValid());

        EXPECT_EQ(m_testMaterialSrgLayout, materialTypeAsset->GetMaterialSrgLayout());

    }

    TEST_F(MaterialTypeAssetTests, TestWithMultipleShaders_OnlyOneUsesSRG)
    {
        Data::Asset<ShaderAsset> shaderA = CreateTestShaderAsset(Uuid::CreateRandom(), {});
        Data::Asset<ShaderAsset> shaderB = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout);
        Data::Asset<ShaderAsset> shaderC = CreateTestShaderAsset(Uuid::CreateRandom(), {});

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(shaderA);
        materialTypeCreator.AddShader(shaderB);
        materialTypeCreator.AddShader(shaderC);
        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        EXPECT_EQ(m_testMaterialSrgLayout, materialTypeAsset->GetMaterialSrgLayout());
    }

    TEST_F(MaterialTypeAssetTests, Error_NoBegin_BeforeBeginMaterialProperty)
    {
        MaterialTypeAssetCreator creator;

        AZ_TEST_START_ASSERTTEST;
        creator.BeginMaterialProperty(Name{ "MyColor" }, MaterialPropertyDataType::Color);
        creator.EndMaterialProperty();

        Data::Asset<MaterialTypeAsset> asset;
        EXPECT_FALSE(creator.End(asset));

        AZ_TEST_STOP_ASSERTTEST(3);
    }

    TEST_F(MaterialTypeAssetTests, Error_NoBegin_BeforeAddShader)
    {
        MaterialTypeAssetCreator creator;

        AZ_TEST_START_ASSERTTEST;
        creator.AddShader(m_testShaderAsset);

        Data::Asset<MaterialTypeAsset> asset;
        EXPECT_FALSE(creator.End(asset));

        AZ_TEST_STOP_ASSERTTEST(2);
    }

    TEST_F(MaterialTypeAssetTests, Error_NullShader)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());

        AZ_TEST_START_ASSERTTEST;
        creator.AddShader(AZ::Data::Asset<ShaderAsset>{});
        AZ_TEST_STOP_ASSERTTEST(1);
    }

    TEST_F(MaterialTypeAssetTests, Error_NullFunctor)
    {
        {
            MaterialTypeAssetCreator creator;
            creator.Begin(Uuid::CreateRandom());

            AZ_TEST_START_ASSERTTEST;
            creator.AddMaterialFunctor(nullptr);
            AZ_TEST_STOP_ASSERTTEST(1);
        }
    }

    TEST_F(MaterialTypeAssetTests, Error_MultipleShadersUsingDifferentSRGs)
    {
        RHI::Ptr<RHI::ShaderResourceGroupLayout> otherPerMaterialSRGLayout = RHI::ShaderResourceGroupLayout::Create();
        otherPerMaterialSRGLayout->SetName(Name("MaterialSrg"));
        otherPerMaterialSRGLayout->SetBindingSlot(SrgBindingSlot::Material);
        otherPerMaterialSRGLayout->Finalize();

        Data::Asset<ShaderAsset> shaderA = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout);
        Data::Asset<ShaderAsset> shaderB = CreateTestShaderAsset(Uuid::CreateRandom(), otherPerMaterialSRGLayout);

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());
        creator.AddShader(shaderA);

        AZ_TEST_START_ASSERTTEST;
        creator.AddShader(shaderB);
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_ShaderInputNotFound)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());
        creator.AddShader(m_testShaderAsset);

        creator.BeginMaterialProperty(Name{ "MyColor" }, MaterialPropertyDataType::Color);

        AZ_TEST_START_ASSERTTEST;
        creator.ConnectMaterialPropertyToShaderInput(Name{ "doesNotExist" });
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_NoShaderResourceGroup)
    {
        // This is very similar to Error_ShaderInputNotFound above, but makes sure the creator doesn't try to access a null SRG

        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());

        creator.BeginMaterialProperty(Name{ "MyColor" }, MaterialPropertyDataType::Color);

        AZ_TEST_START_ASSERTTEST;
        creator.ConnectMaterialPropertyToShaderInput(Name{ "m_color" });
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_ImageMappedToShaderConstant)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());
        creator.AddShader(m_testShaderAsset);

        creator.BeginMaterialProperty(Name{ "MyImage" }, MaterialPropertyDataType::Image);

        AZ_TEST_START_ASSERTTEST;
        creator.ConnectMaterialPropertyToShaderInput(Name{ "m_float" });
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_StandardPropertyMappedToImage)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());
        creator.AddShader(m_testShaderAsset);

        creator.BeginMaterialProperty(Name{ "MyFloat" }, MaterialPropertyDataType::Float);

        AZ_TEST_START_ASSERTTEST;
        creator.ConnectMaterialPropertyToShaderInput(Name{ "m_image" });
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_EndMaterialPropertyNotCalled_BeforeEnd)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());

        creator.BeginMaterialProperty(Name{ "MyColor" }, MaterialPropertyDataType::Color);

        Data::Asset<MaterialTypeAsset> asset;

        AZ_TEST_START_ASSERTTEST;
        EXPECT_FALSE(creator.End(asset));
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_EndMaterialPropertyNotCalled_BeforeBeginMaterial)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());

        creator.BeginMaterialProperty(Name{ "MyColor" }, MaterialPropertyDataType::Color);

        AZ_TEST_START_ASSERTTEST;
        creator.BeginMaterialProperty(Name{ "AnotherColor" }, MaterialPropertyDataType::Color);
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_PropertyNameIdAlreadyExists)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());

        creator.BeginMaterialProperty(Name{ "MyColor" }, MaterialPropertyDataType::Color);
        creator.EndMaterialProperty();

        AZ_TEST_START_ASSERTTEST;
        creator.BeginMaterialProperty(Name{ "MyColor" }, MaterialPropertyDataType::Color);
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_InvalidPropertyDataType)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());

        AZ_TEST_START_ASSERTTEST;
        creator.BeginMaterialProperty(Name{ "MyColor" }, MaterialPropertyDataType::Invalid);
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_NoBeginMaterialProperty_BeforeConnectMaterialPropertyToShaderInput)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());
        creator.AddShader(m_testShaderAsset);

        AZ_TEST_START_ASSERTTEST;
        creator.ConnectMaterialPropertyToShaderInput(Name{"m_bool"});
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_NoBeginMaterialProperty_BeforeConnectMaterialPropertyToShaderOptions)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());
        creator.AddShader(m_testShaderAsset);

        AZ_TEST_START_ASSERTTEST;
        creator.ConnectMaterialPropertyToShaderOptions(Name{"o_debug"});
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_NoBeginMaterialProperty_BeforeEndMaterialProperty)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());

        AZ_TEST_START_ASSERTTEST;
        creator.EndMaterialProperty();
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_PropertyTypeIncompatibleWithShaderOption)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());
        creator.AddShader(m_testShaderAsset);
        creator.BeginMaterialProperty(Name{"color"}, MaterialPropertyDataType::Color);

        AZ_TEST_START_ASSERTTEST;
        creator.ConnectMaterialPropertyToShaderOptions(Name{"o_debug"});
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, Error_ConnectMaterialPropertyToShaderOption_ShaderOptionDoesNoExist)
    {
        MaterialTypeAssetCreator creator;
        creator.Begin(Uuid::CreateRandom());
        creator.AddShader(m_testShaderAsset);
        creator.BeginMaterialProperty(Name{"bool"}, MaterialPropertyDataType::Bool);

        AZ_TEST_START_ASSERTTEST;
        creator.ConnectMaterialPropertyToShaderOptions(Name{"DoesNotExist"});
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, creator.GetErrorCount());
    }

    TEST_F(MaterialTypeAssetTests, ShaderOptionOwnership)
    {
        // Create shaders...

        AZStd::vector<RPI::ShaderOptionValuePair> boolOptionValues = CreateBoolShaderOptionValues();

        Ptr<ShaderOptionGroupLayout> optionsForShaderA = ShaderOptionGroupLayout::Create();
        optionsForShaderA->AddShaderOption(ShaderOptionDescriptor{Name{"o_globalOption_inBothShaders"}, ShaderOptionType::Boolean, 0, 0, boolOptionValues, Name{"False"}});
        optionsForShaderA->AddShaderOption(ShaderOptionDescriptor{Name{"o_globalOption_inShaderA"}, ShaderOptionType::Boolean, 1, 1, boolOptionValues, Name{"False"}});
        optionsForShaderA->AddShaderOption(ShaderOptionDescriptor{Name{"o_materialOption_inBothShaders"}, ShaderOptionType::Boolean, 2, 2, boolOptionValues, Name{"False"}});
        optionsForShaderA->AddShaderOption(ShaderOptionDescriptor{Name{"o_materialOption_inShaderA"}, ShaderOptionType::Boolean, 3, 3, boolOptionValues, Name{"False"}});
        optionsForShaderA->Finalize();

        Data::Asset<ShaderAsset> shaderAssetA = CreateTestShaderAsset(Uuid::CreateRandom(), {}, optionsForShaderA);

        Ptr<ShaderOptionGroupLayout> optionsForShaderB = ShaderOptionGroupLayout::Create();
        optionsForShaderB->AddShaderOption(ShaderOptionDescriptor{Name{"o_materialOption_inBothShaders"}, ShaderOptionType::Boolean, 0, 0, boolOptionValues, Name{"False"}});
        optionsForShaderB->AddShaderOption(ShaderOptionDescriptor{Name{"o_materialOption_inShaderB"}, ShaderOptionType::Boolean, 1, 1, boolOptionValues, Name{"False"}});
        optionsForShaderB->AddShaderOption(ShaderOptionDescriptor{Name{"o_globalOption_inBothShaders"}, ShaderOptionType::Boolean, 2, 2, boolOptionValues, Name{"False"}});
        optionsForShaderB->AddShaderOption(ShaderOptionDescriptor{Name{"o_globalOption_inShaderB"}, ShaderOptionType::Boolean, 3, 3, boolOptionValues, Name{"False"}});
        optionsForShaderB->Finalize();

        Data::Asset<ShaderAsset> shaderAssetB = CreateTestShaderAsset(Uuid::CreateRandom(), {}, optionsForShaderB);

        // Create material type...

        Data::Asset<MaterialTypeAsset> materialTypeAsset;

        MaterialTypeAssetCreator materialTypeCreator;
        materialTypeCreator.Begin(Uuid::CreateRandom());
        materialTypeCreator.AddShader(shaderAssetA);
        materialTypeCreator.AddShader(shaderAssetB);

        // Shader options can be claimed via property connections

        materialTypeCreator.BeginMaterialProperty(Name{"Property1"}, MaterialPropertyDataType::Bool);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_materialOption_inBothShaders"});
        materialTypeCreator.EndMaterialProperty();

        materialTypeCreator.BeginMaterialProperty(Name{"Property2"}, MaterialPropertyDataType::Bool);
        materialTypeCreator.ConnectMaterialPropertyToShaderOptions(Name{"o_materialOption_inShaderA"});
        materialTypeCreator.EndMaterialProperty();

        // Shader options can be claimed directly. For examplek, this can be used when registering material functors or to reserve unused shader options for future use.

        materialTypeCreator.ClaimShaderOptionOwnership(Name{"o_materialOption_inShaderB"});

        EXPECT_TRUE(materialTypeCreator.End(materialTypeAsset));

        // Check ownership results...

        const ShaderCollection& shaderCollection = materialTypeAsset->GetGeneralShaderCollection();

        EXPECT_TRUE(shaderCollection[0].MaterialOwnsShaderOption(Name{"o_materialOption_inBothShaders"}));
        EXPECT_TRUE(shaderCollection[0].MaterialOwnsShaderOption(Name{"o_materialOption_inShaderA"}));
        EXPECT_FALSE(shaderCollection[0].MaterialOwnsShaderOption(Name{"o_materialOption_inShaderB"}));
        EXPECT_FALSE(shaderCollection[0].MaterialOwnsShaderOption(Name{"o_globalOption_inBothShaders"}));
        EXPECT_FALSE(shaderCollection[0].MaterialOwnsShaderOption(Name{"o_globalOption_inShaderA"}));
        EXPECT_FALSE(shaderCollection[0].MaterialOwnsShaderOption(Name{"o_globalOption_inShaderB"}));

        EXPECT_TRUE(shaderCollection[1].MaterialOwnsShaderOption(Name{"o_materialOption_inBothShaders"}));
        EXPECT_FALSE(shaderCollection[1].MaterialOwnsShaderOption(Name{"o_materialOption_inShaderA"}));
        EXPECT_TRUE(shaderCollection[1].MaterialOwnsShaderOption(Name{"o_materialOption_inShaderB"}));
        EXPECT_FALSE(shaderCollection[1].MaterialOwnsShaderOption(Name{"o_globalOption_inBothShaders"}));
        EXPECT_FALSE(shaderCollection[1].MaterialOwnsShaderOption(Name{"o_globalOption_inShaderA"}));
        EXPECT_FALSE(shaderCollection[1].MaterialOwnsShaderOption(Name{"o_globalOption_inShaderB"}));
    }

}

