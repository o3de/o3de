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
#include <Material/MaterialAssetTestUtils.h>

#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceDataRegistration.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Material/Material.h>

#include <AzCore/Utils/Utils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class MaterialTypeSourceDataTests
        : public RPITestFixture
    {
    protected:

        AZ::IO::FixedMaxPath m_tempFolder;
        RHI::Ptr<RHI::ShaderResourceGroupLayout> m_testMaterialSrgLayout;
        Data::Asset<ShaderAsset> m_testShaderAsset;
        Data::Asset<ShaderAsset> m_testShaderAsset2;
        Data::Asset<ImageAsset> m_testImageAsset;
        Data::Asset<ImageAsset> m_testImageAsset2; // For relative path testing.
        Data::Asset<ImageAsset> m_testAttachmentImageAsset;
        static constexpr const char* TestShaderFilename             = "test.shader";
        static constexpr const char* TestShaderFilename2            = "extra.shader";
        static constexpr const char* TestImageFilename              = "test.streamingimage";
        static constexpr const char* TestAttImageFilename           = "test.attimage";

        static constexpr const char* TestImageFilepathAbsolute      = "Folder/test.png";
        static constexpr const char* TestImageFilepathRelative      = "test.png";
        static constexpr const char* TestMaterialFilepathAbsolute   = "Folder/test.material";

        MaterialFunctorSourceDataRegistration m_functorRegistration; // Used for functor source data serialization

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Samples used for testing MaterialFunctor

        class Splat3Functor final
            : public AZ::RPI::MaterialFunctor
        {
        public:
            AZ_RTTI(Splat3Functor, "{4719BBAD-21A1-4909-88E9-C190208BDD00}", AZ::RPI::MaterialFunctor);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<Splat3Functor, AZ::RPI::MaterialFunctor>()
                        ->Version(1)
                        ->Field("m_floatIndex", &Splat3Functor::m_floatIndex)
                        ->Field("m_vector3Index", &Splat3Functor::m_vector3Index)
                        ;
                }
            }

            using AZ::RPI::MaterialFunctor::Process;
            void Process(AZ::RPI::MaterialFunctor::RuntimeContext& context) override
            {
                // This code isn't actually called in the unit test, but we include it here just to demonstrate what a real functor might look like.
                float f = context.GetMaterialPropertyValue(m_floatIndex).GetValue<float>();
                float f3[3] = { f,f,f };
                context.GetShaderResourceGroup()->SetConstantRaw(m_vector3Index, f3, sizeof(float) * 3);
            }

            AZ::RPI::MaterialPropertyIndex m_floatIndex;
            RHI::ShaderInputConstantIndex m_vector3Index;
        };

        class Splat3FunctorSourceData final
            : public MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(Splat3FunctorSourceData, "{658D56CC-D754-471D-BF83-4007FE05C691}", MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<Splat3FunctorSourceData>()
                        ->Version(1)
                        ->Field("floatPropertyInput", &Splat3FunctorSourceData::m_floatPropertyInputId)
                        ->Field("float3ShaderSettingOutput", &Splat3FunctorSourceData::m_float3ShaderSettingOutputId)
                        ;
                }
            }


            Splat3FunctorSourceData() = default;
            Splat3FunctorSourceData(AZStd::string_view floatInputFieldName, AZStd::string_view float3OutputFieldName) :
                m_floatPropertyInputId{floatInputFieldName},
                m_float3ShaderSettingOutputId{float3OutputFieldName}
            {}

            AZStd::string m_floatPropertyInputId;
            AZStd::string m_float3ShaderSettingOutputId;

            using MaterialFunctorSourceData::CreateFunctor;
            FunctorResult CreateFunctor(const RuntimeContext& context) const override
            {
                Ptr<Splat3Functor> functor = aznew Splat3Functor;
                functor->m_floatIndex = context.FindMaterialPropertyIndex(Name(m_floatPropertyInputId));
                functor->m_vector3Index = context.GetShaderResourceGroupLayout()->FindShaderInputConstantIndex(Name{ m_float3ShaderSettingOutputId });
                return  Success(Ptr<MaterialFunctor>(functor));
            }

        };

        class EnableShaderFunctor final
            : public AZ::RPI::MaterialFunctor
        {
        public:
            AZ_RTTI(EnableShaderFunctor, "{6ED031DC-DADC-4A47-B858-DDA9748700A6}", AZ::RPI::MaterialFunctor);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<EnableShaderFunctor, AZ::RPI::MaterialFunctor>()
                        ->Version(1)
                        ->Field("m_enablePropertyIndex", &EnableShaderFunctor::m_enablePropertyIndex)
                        ->Field("m_shaderIndex", &EnableShaderFunctor::m_shaderIndex)
                        ;
                }
            }

            using AZ::RPI::MaterialFunctor::Process;
            void Process(AZ::RPI::MaterialFunctor::RuntimeContext& context) override
            {
                // This code isn't actually called in the unit test, but we include it here just to demonstrate what a real functor might look like.

                const bool enable = context.GetMaterialPropertyValue(m_enablePropertyIndex).GetValue<bool>();
                context.SetShaderEnabled(0, enable);
            }

            AZ::RPI::MaterialPropertyIndex m_enablePropertyIndex;
            int32_t m_shaderIndex = 0;
        };

        class EnableShaderFunctorSourceData final
            : public MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(EnableShaderFunctorSourceData, "{3BEBEB62-6341-4F56-8500-8745BF4A9744}", MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<EnableShaderFunctorSourceData>()
                        ->Version(1)
                        ->Field("enablePassProperty", &EnableShaderFunctorSourceData::m_enablePassPropertyId)
                        ->Field("shaderIndex", &EnableShaderFunctorSourceData::m_shaderIndex)
                        ;
                }
            }

            EnableShaderFunctorSourceData() = default;
            EnableShaderFunctorSourceData(AZStd::string_view enablePassPropertyId, int32_t shaderIndex) :
                m_enablePassPropertyId{enablePassPropertyId},
                m_shaderIndex{shaderIndex}
            {}

            using MaterialFunctorSourceData::CreateFunctor;
            FunctorResult CreateFunctor(const RuntimeContext& context) const override
            {
                Ptr<EnableShaderFunctor> functor = aznew EnableShaderFunctor;
                functor->m_enablePropertyIndex = context.FindMaterialPropertyIndex(Name(m_enablePassPropertyId));
                functor->m_shaderIndex = m_shaderIndex;
                return Success(Ptr<MaterialFunctor>(functor));
            }

            AZStd::string m_enablePassPropertyId;
            int32_t m_shaderIndex = 0;
        };

        class SetShaderOptionFunctor final
            : public AZ::RPI::MaterialFunctor
        {
        public:
            AZ_RTTI(SetShaderOptionFunctor, "{26DBDF5E-F3AC-4394-8D28-D1CD44623429}", AZ::RPI::MaterialFunctor);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<SetShaderOptionFunctor, AZ::RPI::MaterialFunctor>();
                }
            }

            using AZ::RPI::MaterialFunctor::Process;
            void Process(AZ::RPI::MaterialFunctor::RuntimeContext& context) override
            {
                // This code isn't actually called in the unit test, but we include it here just to demonstrate what a real functor might look like.
                context.SetShaderOptionValue(0, ShaderOptionIndex{1}/*o_foo*/, ShaderOptionValue{1});
                context.SetShaderOptionValue(0, ShaderOptionIndex{2}/*o_bar*/, ShaderOptionValue{2});
            }
        };

        class SetShaderOptionFunctorSourceData final
            : public MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(SetShaderOptionFunctorSourceData, "{051CD884-FE93-403B-8E27-EC3237BF6CAE}", MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<SetShaderOptionFunctorSourceData>()
                        ->Version(1)
                        ->Field("enableProperty", &SetShaderOptionFunctorSourceData::m_enablePropertyName)
                        ;
                }
            }

            AZStd::vector<AZ::Name> GetShaderOptionDependencies() const override
            {
                AZStd::vector<AZ::Name> options;
                options.push_back(AZ::Name{"o_foo"});
                options.push_back(AZ::Name{"o_bar"});
                return options;
            }

            using MaterialFunctorSourceData::CreateFunctor;
            FunctorResult CreateFunctor([[maybe_unused]] const RuntimeContext& context) const override
            {
                Ptr<SetShaderOptionFunctor> functor = aznew SetShaderOptionFunctor;
                return Success(Ptr<MaterialFunctor>(functor));
            }

            AZStd::string m_enablePropertyName;
        };

        // All this functor does is save the MaterialNameContext
        class SaveNameContextTestFunctor final
            : public AZ::RPI::MaterialFunctor
        {
        public:
            AZ_RTTI(SaveNameContextTestFunctor, "{FD680069-B430-4278-9E5B-A2B9617627D5}", AZ::RPI::MaterialFunctor);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<SaveNameContextTestFunctor, AZ::RPI::MaterialFunctor>()
                    ->Version(1)
                    ->Field("nameContext", &SaveNameContextTestFunctor::m_nameContext);
                }
            }

            using AZ::RPI::MaterialFunctor::Process;
            void Process(AZ::RPI::MaterialFunctor::RuntimeContext&) override
            {
                // Intentionally empty, this is where the functor would do it's normal processing,
                // but all this test functor does is store the MaterialNameContext.
            }
            
            MaterialNameContext m_nameContext;
        };
        
        // All this functor does is save the MaterialNameContext
        class SaveNameContextTestFunctorSourceData final
            : public MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(SaveNameContextTestFunctorSourceData, "{4261A2EC-4AB6-420E-884A-18D1A36500BE}", MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<SaveNameContextTestFunctorSourceData>()
                        ->Version(1)
                        ;
                }
            }

            using MaterialFunctorSourceData::CreateFunctor;
            FunctorResult CreateFunctor([[maybe_unused]] const RuntimeContext& context) const override
            {
                Ptr<SaveNameContextTestFunctor> functor = aznew SaveNameContextTestFunctor;
                functor->m_nameContext = *context.GetNameContext();
                return Success(Ptr<MaterialFunctor>(functor));
            }
        };

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        void Reflect(ReflectContext* context) override
        {
            RPITestFixture::Reflect(context);

            MaterialTypeSourceData::Reflect(context);

            MaterialFunctorSourceDataHolder::Reflect(context);

            Splat3FunctorSourceData::Reflect(context);
            EnableShaderFunctorSourceData::Reflect(context);
            SetShaderOptionFunctorSourceData::Reflect(context);
            SaveNameContextTestFunctorSourceData::Reflect(context);
        }

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_functorRegistration.Init();

            AZ::RPI::MaterialFunctorSourceDataRegistration::Get()->RegisterMaterialFunctor("Splat3", azrtti_typeid<Splat3FunctorSourceData>());
            AZ::RPI::MaterialFunctorSourceDataRegistration::Get()->RegisterMaterialFunctor("EnableShader", azrtti_typeid<EnableShaderFunctorSourceData>());
            AZ::RPI::MaterialFunctorSourceDataRegistration::Get()->RegisterMaterialFunctor("SetShaderOption", azrtti_typeid<SetShaderOptionFunctorSourceData>());
            AZ::RPI::MaterialFunctorSourceDataRegistration::Get()->RegisterMaterialFunctor("SaveNameContext", azrtti_typeid<SaveNameContextTestFunctorSourceData>());

            const Name materialSrgId{"MaterialSrg"};
            m_testMaterialSrgLayout = RHI::ShaderResourceGroupLayout::Create();
            m_testMaterialSrgLayout->SetName(materialSrgId);
            m_testMaterialSrgLayout->SetBindingSlot(SrgBindingSlot::Material);
            m_testMaterialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_color" }, 4, 16, 0 });
            m_testMaterialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_float" }, 20, 4, 0 });
            m_testMaterialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_int" }, 24, 4, 0 });
            m_testMaterialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_uint" }, 28, 4, 0 });
            m_testMaterialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_float2" }, 32, 8, 0 });
            m_testMaterialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_float3" }, 40, 12, 0 });
            m_testMaterialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_float4" }, 52, 16, 0 });
            m_testMaterialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_bool" }, 68, 1, 0 });
            m_testMaterialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_image" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
            m_testMaterialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_attachmentImage" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
            EXPECT_TRUE(m_testMaterialSrgLayout->Finalize());

            AZStd::vector<RPI::ShaderOptionValuePair> optionValues = CreateEnumShaderOptionValues({"Low", "Med", "High"});

            Ptr<ShaderOptionGroupLayout> shaderOptions = ShaderOptionGroupLayout::Create();
            uint32_t order = 0;
            shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_quality"}, ShaderOptionType::Enumeration, 0, order++, optionValues, Name{"Low"}});
            shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_foo"}, ShaderOptionType::Enumeration, 2, order++, optionValues, Name{"Low"}});
            shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_bar"}, ShaderOptionType::Enumeration, 4, order++, optionValues, Name{"Low"}});
            shaderOptions->Finalize();

            m_testShaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, shaderOptions);
            m_testShaderAsset2 = CreateTestShaderAsset(Uuid::CreateRandom());
            
            // Since this test doesn't actually instantiate a Material, it won't need to instantiate this ImageAsset, so all we
            // need is an asset reference with a valid ID.
            m_testImageAsset = Data::Asset<ImageAsset>{ Data::AssetId{ Uuid::CreateRandom(), StreamingImageAsset::GetImageAssetSubId() }, azrtti_typeid<AZ::RPI::StreamingImageAsset>() };
            m_testImageAsset2 = Data::Asset<ImageAsset>{ Data::AssetId{ Uuid::CreateRandom(), StreamingImageAsset::GetImageAssetSubId() }, azrtti_typeid<StreamingImageAsset>() };

            m_testAttachmentImageAsset = Data::Asset<ImageAsset>{ Data::AssetId{ Uuid::CreateRandom(), 0 }, azrtti_typeid<AZ::RPI::AttachmentImageAsset>() };

            Data::AssetInfo testShaderAssetInfo;
            testShaderAssetInfo.m_assetId = m_testShaderAsset.GetId();

            Data::AssetInfo testShaderAssetInfo2;
            testShaderAssetInfo2.m_assetId = m_testShaderAsset2.GetId();

            Data::AssetInfo testImageAssetInfo;
            testImageAssetInfo.m_assetId = m_testImageAsset.GetId();

            Data::AssetInfo testImageAssetInfo2;
            testImageAssetInfo2.m_assetId = m_testImageAsset2.GetId();
            testImageAssetInfo2.m_assetType = azrtti_typeid<StreamingImageAsset>();
                        
            Data::AssetInfo testAttImageAssetInfo;
            testAttImageAssetInfo.m_assetId = m_testAttachmentImageAsset.GetId();

            m_assetSystemStub.RegisterSourceInfo(TestShaderFilename, testShaderAssetInfo, "");
            m_assetSystemStub.RegisterSourceInfo(TestShaderFilename2, testShaderAssetInfo2, "");
            m_assetSystemStub.RegisterSourceInfo(TestImageFilename, testImageAssetInfo, "");
            m_assetSystemStub.RegisterSourceInfo(TestAttImageFilename, testAttImageAssetInfo, "");
            // We need to normalize the path because AssetSystemStub uses it as a key to look up.
            AZStd::string testImageFilepathAbsolute(TestImageFilepathAbsolute);
            AzFramework::StringFunc::Path::Normalize(testImageFilepathAbsolute);
            m_assetSystemStub.RegisterSourceInfo(testImageFilepathAbsolute.c_str(), testImageAssetInfo2, "");

            m_tempFolder = AZ::Utils::GetExecutableDirectory();
            m_tempFolder = m_tempFolder/"temp"/"MaterialTypeSourceDataTest";
        }

        void TearDown() override
        {
            m_testMaterialSrgLayout = nullptr;
            m_testShaderAsset.Reset();
            m_testShaderAsset2.Reset();

            m_functorRegistration.Shutdown();

            RPITestFixture::TearDown();
        }

        //! Checks for a match between source data and MaterialPropertyDescriptor, for only the fields that correspond 1:1.
        //! (Note this function can't be used in every case, because there are cases where output connections will not correspond 1:1)
        void ValidateCommonDescriptorFields(const MaterialTypeSourceData::PropertyDefinition& expectedValues, const MaterialPropertyDescriptor* propertyDescriptor)
        {
            EXPECT_EQ(propertyDescriptor->GetDataType(), expectedValues.m_dataType);
            EXPECT_EQ(propertyDescriptor->GetOutputConnections().size(), expectedValues.m_outputConnections.size());
            for (int i = 0; i < expectedValues.m_outputConnections.size() && i < propertyDescriptor->GetOutputConnections().size(); ++i)
            {
                EXPECT_EQ(propertyDescriptor->GetOutputConnections()[i].m_type, expectedValues.m_outputConnections[i].m_type);
                EXPECT_EQ(propertyDescriptor->GetOutputConnections()[i].m_containerIndex.GetIndex(), expectedValues.m_outputConnections[i].m_shaderIndex);
            }
        }
    };

    TEST_F(MaterialTypeSourceDataTests, PopulateAndSearchPropertyLayout)
    {
        MaterialTypeSourceData sourceData;

        // Here we are building up multiple layers of property groups and properties, using a variety of different Add functions,
        // going through the MaterialTypeSourceData or going to the PropertyGroup directly.

        MaterialTypeSourceData::PropertyGroup* layer1 = sourceData.AddPropertyGroup("layer1");
        MaterialTypeSourceData::PropertyGroup* layer2 = sourceData.AddPropertyGroup("layer2");
        MaterialTypeSourceData::PropertyGroup* blend = sourceData.AddPropertyGroup("blend");
        
        MaterialTypeSourceData::PropertyGroup* layer1_baseColor = layer1->AddPropertyGroup("baseColor");
        MaterialTypeSourceData::PropertyGroup* layer2_baseColor = layer2->AddPropertyGroup("baseColor");
        
        MaterialTypeSourceData::PropertyGroup* layer1_roughness = sourceData.AddPropertyGroup("layer1.roughness");
        MaterialTypeSourceData::PropertyGroup* layer2_roughness = sourceData.AddPropertyGroup("layer2.roughness");

        MaterialTypeSourceData::PropertyDefinition* layer1_baseColor_texture = layer1_baseColor->AddProperty("texture");
        MaterialTypeSourceData::PropertyDefinition* layer2_baseColor_texture = layer2_baseColor->AddProperty("texture");
        
        MaterialTypeSourceData::PropertyDefinition* layer1_roughness_texture = sourceData.AddProperty("layer1.roughness.texture");
        MaterialTypeSourceData::PropertyDefinition* layer2_roughness_texture = sourceData.AddProperty("layer2.roughness.texture");
        
        // We're doing clear coat only on layer2, for brevity
        MaterialTypeSourceData::PropertyGroup* layer2_clearCoat = layer2->AddPropertyGroup("clearCoat");
        MaterialTypeSourceData::PropertyGroup* layer2_clearCoat_roughness = layer2_clearCoat->AddPropertyGroup("roughness");
        MaterialTypeSourceData::PropertyGroup* layer2_clearCoat_normal = layer2_clearCoat->AddPropertyGroup("normal");
        MaterialTypeSourceData::PropertyDefinition* layer2_clearCoat_enabled = layer2_clearCoat->AddProperty("enabled");
        MaterialTypeSourceData::PropertyDefinition* layer2_clearCoat_roughness_texture = layer2_clearCoat_roughness->AddProperty("texture");
        MaterialTypeSourceData::PropertyDefinition* layer2_clearCoat_normal_texture = layer2_clearCoat_normal->AddProperty("texture");
        MaterialTypeSourceData::PropertyDefinition* layer2_clearCoat_normal_factor = layer2_clearCoat_normal->AddProperty("factor");

        MaterialTypeSourceData::PropertyDefinition* blend_factor = blend->AddProperty("factor");

        // Check the available Find functions

        EXPECT_EQ(nullptr, sourceData.FindProperty("DoesNotExist"));
        EXPECT_EQ(nullptr, sourceData.FindProperty("layer1.DoesNotExist"));
        EXPECT_EQ(nullptr, sourceData.FindProperty("layer1.baseColor.DoesNotExist"));
        EXPECT_EQ(nullptr, sourceData.FindProperty("baseColor.texture"));
        EXPECT_EQ(nullptr, sourceData.FindProperty("baseColor")); // This is a property group, not a property
        EXPECT_EQ(nullptr, sourceData.FindPropertyGroup("baseColor.texture")); // This is a property, not a property group
        
        EXPECT_EQ(layer1, sourceData.FindPropertyGroup("layer1"));
        EXPECT_EQ(layer2, sourceData.FindPropertyGroup("layer2"));
        EXPECT_EQ(blend, sourceData.FindPropertyGroup("blend"));
        
        EXPECT_EQ(layer1_baseColor, sourceData.FindPropertyGroup("layer1.baseColor"));
        EXPECT_EQ(layer2_baseColor, sourceData.FindPropertyGroup("layer2.baseColor"));

        EXPECT_EQ(layer1_roughness, sourceData.FindPropertyGroup("layer1.roughness"));
        EXPECT_EQ(layer2_roughness, sourceData.FindPropertyGroup("layer2.roughness"));
        
        EXPECT_EQ(layer1_baseColor_texture, sourceData.FindProperty("layer1.baseColor.texture"));
        EXPECT_EQ(layer2_baseColor_texture, sourceData.FindProperty("layer2.baseColor.texture"));
        EXPECT_EQ(layer1_roughness_texture, sourceData.FindProperty("layer1.roughness.texture"));
        EXPECT_EQ(layer2_roughness_texture, sourceData.FindProperty("layer2.roughness.texture"));
        
        EXPECT_EQ(layer2_clearCoat, sourceData.FindPropertyGroup("layer2.clearCoat"));
        EXPECT_EQ(layer2_clearCoat_roughness, sourceData.FindPropertyGroup("layer2.clearCoat.roughness"));
        EXPECT_EQ(layer2_clearCoat_normal, sourceData.FindPropertyGroup("layer2.clearCoat.normal"));
        
        EXPECT_EQ(layer2_clearCoat_enabled, sourceData.FindProperty("layer2.clearCoat.enabled"));
        EXPECT_EQ(layer2_clearCoat_roughness_texture, sourceData.FindProperty("layer2.clearCoat.roughness.texture"));
        EXPECT_EQ(layer2_clearCoat_normal_texture, sourceData.FindProperty("layer2.clearCoat.normal.texture"));
        EXPECT_EQ(layer2_clearCoat_normal_factor, sourceData.FindProperty("layer2.clearCoat.normal.factor"));

        EXPECT_EQ(blend_factor, sourceData.FindProperty("blend.factor"));
        
        // Check EnumeratePropertyGroups

        struct EnumeratePropertyGroupsResult
        {
            MaterialNameContext m_nameContext;

            void Check(AZStd::string expectedGroupId)
            {
                Name imaginaryProperty{"someChildProperty"};
                m_nameContext.ContextualizeProperty(imaginaryProperty);

                EXPECT_EQ(expectedGroupId + ".someChildProperty", imaginaryProperty.GetStringView());
            }
        };
        AZStd::vector<EnumeratePropertyGroupsResult> enumeratePropertyGroupsResults;

        sourceData.EnumeratePropertyGroups([&enumeratePropertyGroupsResults](
            const MaterialTypeSourceData::PropertyGroupStack& propertyGroupStack)
            {
                MaterialNameContext nameContext = MaterialTypeSourceData::MakeMaterialNameContext(propertyGroupStack);
                enumeratePropertyGroupsResults.push_back(EnumeratePropertyGroupsResult{nameContext});
                return true;
            });

        int resultIndex = 0;
        enumeratePropertyGroupsResults[resultIndex++].Check("layer1");
        enumeratePropertyGroupsResults[resultIndex++].Check("layer1.baseColor");
        enumeratePropertyGroupsResults[resultIndex++].Check("layer1.roughness");
        enumeratePropertyGroupsResults[resultIndex++].Check("layer2");
        enumeratePropertyGroupsResults[resultIndex++].Check("layer2.baseColor");
        enumeratePropertyGroupsResults[resultIndex++].Check("layer2.roughness");
        enumeratePropertyGroupsResults[resultIndex++].Check("layer2.clearCoat");
        enumeratePropertyGroupsResults[resultIndex++].Check("layer2.clearCoat.roughness");
        enumeratePropertyGroupsResults[resultIndex++].Check("layer2.clearCoat.normal");
        enumeratePropertyGroupsResults[resultIndex++].Check("blend");
        EXPECT_EQ(resultIndex, enumeratePropertyGroupsResults.size());

        // Check EnumerateProperties
        
        struct EnumeratePropertiesResult
        {
            const MaterialTypeSourceData::PropertyDefinition* m_propertyDefinition;
            MaterialNameContext m_materialNameContext;

            void Check(AZStd::string expectedIdContext, const MaterialTypeSourceData::PropertyDefinition* expectedPropertyDefinition)
            {
                Name propertyFullId{m_propertyDefinition->GetName()};
                m_materialNameContext.ContextualizeProperty(propertyFullId);

                AZStd::string expectedPropertyId = expectedIdContext + expectedPropertyDefinition->GetName();

                EXPECT_EQ(expectedPropertyId, propertyFullId.GetStringView());
                EXPECT_EQ(expectedPropertyDefinition, m_propertyDefinition);
            }
        };
        AZStd::vector<EnumeratePropertiesResult> enumeratePropertiesResults;

        sourceData.EnumerateProperties([&enumeratePropertiesResults](const MaterialTypeSourceData::PropertyDefinition* propertyDefinition, const MaterialNameContext& nameContext)
            {
                enumeratePropertiesResults.push_back(EnumeratePropertiesResult{propertyDefinition, nameContext});
                return true;
            });
        
        resultIndex = 0;
        enumeratePropertiesResults[resultIndex++].Check("layer1.baseColor.", layer1_baseColor_texture);
        enumeratePropertiesResults[resultIndex++].Check("layer1.roughness.", layer1_roughness_texture);
        enumeratePropertiesResults[resultIndex++].Check("layer2.baseColor.", layer2_baseColor_texture);
        enumeratePropertiesResults[resultIndex++].Check("layer2.roughness.", layer2_roughness_texture);
        enumeratePropertiesResults[resultIndex++].Check("layer2.clearCoat.", layer2_clearCoat_enabled);
        enumeratePropertiesResults[resultIndex++].Check("layer2.clearCoat.roughness.", layer2_clearCoat_roughness_texture);
        enumeratePropertiesResults[resultIndex++].Check("layer2.clearCoat.normal.", layer2_clearCoat_normal_texture);
        enumeratePropertiesResults[resultIndex++].Check("layer2.clearCoat.normal.", layer2_clearCoat_normal_factor);
        enumeratePropertiesResults[resultIndex++].Check("blend.", blend_factor);
        EXPECT_EQ(resultIndex, enumeratePropertiesResults.size());
    }
    
    TEST_F(MaterialTypeSourceDataTests, AddProperty_Error_AddPropertyWithInvalidName)
    {
        MaterialTypeSourceData sourceData;

        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("main");

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("'' is not a valid identifier");
        errorMessageFinder.AddExpectedErrorMessage("'main.' is not a valid identifier");
        errorMessageFinder.AddExpectedErrorMessage("'base-color' is not a valid identifier");
        
        EXPECT_FALSE(propertyGroup->AddProperty(""));
        EXPECT_FALSE(propertyGroup->AddProperty("main."));
        EXPECT_FALSE(sourceData.AddProperty("main.base-color"));
        
        EXPECT_TRUE(propertyGroup->GetProperties().empty());

        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialTypeSourceDataTests, AddPropertyGroup_Error_InvalidName)
    {
        MaterialTypeSourceData sourceData;

        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("'' is not a valid identifier", 2);
        errorMessageFinder.AddExpectedErrorMessage("'base-color' is not a valid identifier");
        errorMessageFinder.AddExpectedErrorMessage("'look@it' is not a valid identifier");

        EXPECT_FALSE(propertyGroup->AddPropertyGroup(""));
        EXPECT_FALSE(sourceData.AddPropertyGroup(""));
        EXPECT_FALSE(sourceData.AddPropertyGroup("base-color"));
        EXPECT_FALSE(sourceData.AddPropertyGroup("general.look@it"));
        
        EXPECT_TRUE(propertyGroup->GetProperties().empty());

        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialTypeSourceDataTests, AddProperty_Error_AddDuplicateProperty)
    {
        MaterialTypeSourceData sourceData;

        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("main");

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("PropertyGroup 'main' already contains a property named 'foo'", 2);

        EXPECT_TRUE(propertyGroup->AddProperty("foo"));
        EXPECT_FALSE(propertyGroup->AddProperty("foo"));
        EXPECT_FALSE(sourceData.AddProperty("main.foo"));
        
        EXPECT_EQ(propertyGroup->GetProperties().size(), 1);

        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialTypeSourceDataTests, AddProperty_Error_AddLooseProperty)
    {
        MaterialTypeSourceData sourceData;
        ErrorMessageFinder errorMessageFinder("Property id 'foo' is invalid. Properties must be added to a PropertyGroup");
        EXPECT_FALSE(sourceData.AddProperty("foo"));
        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialTypeSourceDataTests, AddProperty_Error_PropertyGroupDoesNotExist   )
    {
        MaterialTypeSourceData sourceData;
        ErrorMessageFinder errorMessageFinder("PropertyGroup 'DNE' does not exists");
        EXPECT_FALSE(sourceData.AddProperty("DNE.foo"));
        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialTypeSourceDataTests, AddPropertyGroup_Error_PropertyGroupDoesNotExist   )
    {
        MaterialTypeSourceData sourceData;
        ErrorMessageFinder errorMessageFinder("PropertyGroup 'DNE' does not exists");
        EXPECT_FALSE(sourceData.AddPropertyGroup("DNE.foo"));
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTypeSourceDataTests, AddPropertyGroup_Error_AddDuplicatePropertyGroup)
    {
        MaterialTypeSourceData sourceData;
        
        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("main");
        sourceData.AddPropertyGroup("main.level2");

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("PropertyGroup named 'main' already exists", 1);
        errorMessageFinder.AddExpectedErrorMessage("PropertyGroup named 'level2' already exists", 2);
        
        EXPECT_FALSE(sourceData.AddPropertyGroup("main"));
        EXPECT_FALSE(sourceData.AddPropertyGroup("main.level2"));
        EXPECT_FALSE(propertyGroup->AddPropertyGroup("level2"));
        
        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_EQ(sourceData.GetPropertyLayout().m_propertyGroups.size(), 1);
        EXPECT_EQ(propertyGroup->GetPropertyGroups().size(), 1);
    }
    
    TEST_F(MaterialTypeSourceDataTests, AddPropertyGroup_Error_NameCollidesWithProperty   )
    {
        MaterialTypeSourceData sourceData;
        sourceData.AddPropertyGroup("main");
        sourceData.AddProperty("main.foo");

        ErrorMessageFinder errorMessageFinder("PropertyGroup name 'foo' collides with a Property of the same name");
        EXPECT_FALSE(sourceData.AddPropertyGroup("main.foo"));
        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialTypeSourceDataTests, AddProperty_Error_NameCollidesWithPropertyGroup   )
    {
        MaterialTypeSourceData sourceData;
        sourceData.AddPropertyGroup("main");
        sourceData.AddPropertyGroup("main.foo");

        ErrorMessageFinder errorMessageFinder("Property name 'foo' collides with a PropertyGroup of the same name");
        EXPECT_FALSE(sourceData.AddProperty("main.foo"));
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTypeSourceDataTests, ResolveUvStreamAsEnum)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_uvNameMap["UV0"] = "Tiled";
        sourceData.m_uvNameMap["UV1"] = "Unwrapped";
        sourceData.m_uvNameMap["UV2"] = "Other";

        sourceData.AddPropertyGroup("a");
        sourceData.AddPropertyGroup("a.b");
        sourceData.AddPropertyGroup("c");
        sourceData.AddPropertyGroup("c.d");
        sourceData.AddPropertyGroup("c.d.e");

        MaterialTypeSourceData::PropertyDefinition* enum1 = sourceData.AddProperty("a.enum1");
        MaterialTypeSourceData::PropertyDefinition* enum2 = sourceData.AddProperty("a.b.enum2");
        MaterialTypeSourceData::PropertyDefinition* enum3 = sourceData.AddProperty("c.d.e.enum3");
        MaterialTypeSourceData::PropertyDefinition* notEnum = sourceData.AddProperty("c.d.myFloat");

        enum1->m_dataType = MaterialPropertyDataType::Enum;
        enum2->m_dataType = MaterialPropertyDataType::Enum;
        enum3->m_dataType = MaterialPropertyDataType::Enum;
        notEnum->m_dataType = MaterialPropertyDataType::Float;

        enum1->m_enumIsUv = true;
        enum2->m_enumIsUv = false;
        enum3->m_enumIsUv = true;

        sourceData.ResolveUvEnums();

        EXPECT_STREQ(enum1->m_enumValues[0].c_str(), "Tiled");
        EXPECT_STREQ(enum1->m_enumValues[1].c_str(), "Unwrapped");
        EXPECT_STREQ(enum1->m_enumValues[2].c_str(), "Other");
        
        EXPECT_STREQ(enum3->m_enumValues[0].c_str(), "Tiled");
        EXPECT_STREQ(enum3->m_enumValues[1].c_str(), "Unwrapped");
        EXPECT_STREQ(enum3->m_enumValues[2].c_str(), "Other");

        // enum2 is not a UV stream enum
        EXPECT_EQ(enum2->m_enumValues.size(), 0);
        
        // myFloat is not even an enum
        EXPECT_EQ(notEnum->m_enumValues.size(), 0);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_GetMaterialSrgAsset)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ TestShaderFilename });

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());

        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        EXPECT_EQ(m_testMaterialSrgLayout, materialTypeAsset->GetMaterialSrgLayout());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_NoMaterialSrgAsset)
    {
        MaterialTypeSourceData sourceData;

        Data::Asset<ShaderAsset> shaderAssetWithNoMaterialSrg = CreateTestShaderAsset(Uuid::CreateRandom(), {});

        Data::AssetInfo shaderAssetInfo;
        shaderAssetInfo.m_assetId = shaderAssetWithNoMaterialSrg.GetId();

        m_assetSystemStub.RegisterSourceInfo("noMaterialSrg.shader", shaderAssetInfo, "");

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ "noMaterialSrg.shader" });

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());

        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        EXPECT_FALSE(materialTypeAsset->GetMaterialSrgLayout());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_WithMultipleShaders)
    {
        // Set up the shaders...

        AZStd::vector<RPI::ShaderOptionValuePair> optionValues = CreateEnumShaderOptionValues({"Low", "Med", "High"});

        Ptr<ShaderOptionGroupLayout> shaderOptions = ShaderOptionGroupLayout::Create();
        uint32_t order = 0;
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_foo"}, ShaderOptionType::Enumeration, 0, order++, optionValues, Name{"Low"}});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_bar"}, ShaderOptionType::Enumeration, 2, order++, optionValues, Name{"Low"}});
        shaderOptions->Finalize();

        Data::Asset<ShaderAsset> shaderAssetA = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, shaderOptions);
        Data::Asset<ShaderAsset> shaderAssetB = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, shaderOptions);
        Data::Asset<ShaderAsset> shaderAssetC = CreateTestShaderAsset(Uuid::CreateRandom(), {}, shaderOptions);

        Data::AssetInfo shaderAssetAInfo;
        shaderAssetAInfo.m_assetId = shaderAssetA.GetId();

        Data::AssetInfo shaderAssetBInfo;
        shaderAssetBInfo.m_assetId = shaderAssetB.GetId();

        Data::AssetInfo shaderAssetCInfo;
        shaderAssetCInfo.m_assetId = shaderAssetC.GetId();

        m_assetSystemStub.RegisterSourceInfo("a.shader", shaderAssetAInfo, "");
        m_assetSystemStub.RegisterSourceInfo("b.shader", shaderAssetBInfo, "");
        m_assetSystemStub.RegisterSourceInfo("c.shader", shaderAssetCInfo, "");

        // Set up the material...

        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ "a.shader" });
        sourceData.m_shaderCollection.back().m_shaderOptionValues[Name{"o_foo"}] = Name{"Med"};
        sourceData.m_shaderCollection.back().m_shaderOptionValues[Name{"o_bar"}] = Name{"High"};

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ "b.shader" });
        sourceData.m_shaderCollection.back().m_shaderOptionValues[Name{"o_foo"}] = Name{"High"};
        sourceData.m_shaderCollection.back().m_shaderOptionValues[Name{"o_bar"}] = Name{"Low"};

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ "c.shader" });
        // Only setting one option here, leaving the other unspecified
        sourceData.m_shaderCollection.back().m_shaderOptionValues[Name{"o_foo"}] = Name{"Med"};

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());

        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        // Check the results...

        EXPECT_EQ(m_testMaterialSrgLayout, materialTypeAsset->GetMaterialSrgLayout());
        EXPECT_EQ(3, materialTypeAsset->GetShaderCollection().size());
        EXPECT_EQ(shaderAssetA, materialTypeAsset->GetShaderCollection()[0].GetShaderAsset());
        EXPECT_EQ(shaderAssetB, materialTypeAsset->GetShaderCollection()[1].GetShaderAsset());
        EXPECT_EQ(shaderAssetC, materialTypeAsset->GetShaderCollection()[2].GetShaderAsset());

        ShaderOptionGroup shaderAOptions{shaderOptions, materialTypeAsset->GetShaderCollection()[0].GetShaderVariantId()};
        ShaderOptionGroup shaderBOptions{shaderOptions, materialTypeAsset->GetShaderCollection()[1].GetShaderVariantId()};
        ShaderOptionGroup shaderCOptions{shaderOptions, materialTypeAsset->GetShaderCollection()[2].GetShaderVariantId()};
        ShaderOptionIndex fooOption = shaderOptions->FindShaderOptionIndex(Name{"o_foo"});
        ShaderOptionIndex barOption = shaderOptions->FindShaderOptionIndex(Name{"o_bar"});
        EXPECT_EQ(shaderAOptions.GetValue(fooOption).GetIndex(), 1);
        EXPECT_EQ(shaderAOptions.GetValue(barOption).GetIndex(), 2);
        EXPECT_EQ(shaderBOptions.GetValue(fooOption).GetIndex(), 2);
        EXPECT_EQ(shaderBOptions.GetValue(barOption).GetIndex(), 0);
        EXPECT_EQ(shaderCOptions.GetValue(fooOption).GetIndex(), 1);
        EXPECT_EQ(shaderCOptions.GetValue(barOption).GetIndex(), 0);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_ShaderAssetNotFound)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ "doesNotExist.shader" });

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        AZ_TEST_STOP_TRACE_SUPPRESSION(2); // One for CreateMaterialTypeAsset() and one in AssetUtils

        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_ShaderListWithInvalidShaderOptionId)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});
        sourceData.m_shaderCollection.back().m_shaderOptionValues[Name{"DoesNotExist"}] = Name{"High"};

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("ShaderOption 'DoesNotExist' does not exist"); // This is from ShaderOptionGroup
        errorMessageFinder.AddExpectedErrorMessage("Could not set shader option 'DoesNotExist' to 'High'."); // This is from MaterialTypeSourceData
        errorMessageFinder.AddIgnoredErrorMessage("Failed to build MaterialTypeAsset", true);

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_ShaderListWithInvalidShaderOptionValue)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});
        sourceData.m_shaderCollection.back().m_shaderOptionValues[Name{"o_quality"}] = Name{"DoesNotExist"};

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("ShaderOption value 'DoesNotExist' does not exist"); // This is from ShaderOptionGroup
        errorMessageFinder.AddExpectedErrorMessage("Could not set shader option 'o_quality' to 'DoesNotExist'."); // This is from MaterialTypeSourceData
        errorMessageFinder.AddIgnoredErrorMessage("Failed to build MaterialTypeAsset", true);

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());

        errorMessageFinder.CheckExpectedErrorsFound();

        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_BoolPropertyConnectedToShaderConstant)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ TestShaderFilename });

        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");
        MaterialTypeSourceData::PropertyDefinition* property = propertyGroup->AddProperty("MyBool");
        property->m_displayName = "My Bool";
        property->m_description = "This is a bool";
        property->m_dataType = MaterialPropertyDataType::Bool;
        property->m_value = true;
        property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string("m_bool") });
        
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{ "general.MyBool" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

        ValidateCommonDescriptorFields(*property, propertyDescriptor);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex(), 7);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_FloatPropertyConnectedToShaderConstant)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ TestShaderFilename });
        
        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");
        MaterialTypeSourceData::PropertyDefinition* property = propertyGroup->AddProperty("MyFloat");
        property->m_displayName = "My Float";
        property->m_description = "This is a float";
        property->m_min = 0.0f;
        property->m_max = 1.0f;
        property->m_softMin = 0.2f;
        property->m_softMax = 1.0f;
        property->m_value = 0.0f; 
        property->m_step = 0.01f;
        property->m_dataType = MaterialPropertyDataType::Float;
        property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string("m_float") });
        
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"general.MyFloat" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

        ValidateCommonDescriptorFields(*property, propertyDescriptor);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex(), 1);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_ImagePropertyConnectedToShaderInput)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ TestShaderFilename });
        
        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");
        MaterialTypeSourceData::PropertyDefinition* property = propertyGroup->AddProperty("MyImage");
        property->m_displayName = "My Image";
        property->m_description = "This is an image";
        property->m_dataType = MaterialPropertyDataType::Image;
        property->m_value = AZStd::string{};
        property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string("m_image") });
        
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"general.MyImage" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

        ValidateCommonDescriptorFields(*property, propertyDescriptor);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex(), 0);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_IntPropertyConnectedToShaderOption)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});
        
        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");
        MaterialTypeSourceData::PropertyDefinition* property = propertyGroup->AddProperty("MyInt");
        property->m_displayName = "My Integer";
        property->m_dataType = MaterialPropertyDataType::Int;
        property->m_value = 0;
        property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{MaterialPropertyOutputType::ShaderOption, AZStd::string("o_foo"), 0});
        
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(MaterialPropertyIndex{0});

        ValidateCommonDescriptorFields(*property, propertyDescriptor);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex(), 1);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_PropertyConnectedToInvalidShaderOptionId)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});
        
        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");
        MaterialTypeSourceData::PropertyDefinition* property = propertyGroup->AddProperty("MyInt");
        property->m_dataType = MaterialPropertyDataType::Int;
        property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{MaterialPropertyOutputType::ShaderOption, AZStd::string("DoesNotExist"), 0});
        
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        AZ_TEST_STOP_TRACE_SUPPRESSION(2); // There happens to be an extra assert for "Cannot continue building MaterialAsset because 1 error(s) reported"

        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_InvalidGroupName)
    {
        const AZStd::string inputJson = R"(
            {
                "propertyLayout": {
                    "propertyGroups": [
                        {
                            "name": "not a valid name because it has spaces",
                            "properties": [
                                {
                                    "name": "foo",
                                    "type": "Bool"
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

        MaterialTypeSourceData sourceData;
        JsonTestResult loadResult = LoadTestDataFromJson(sourceData, inputJson);
        EXPECT_EQ(loadResult.m_jsonResultCode.GetProcessing(), JsonSerializationResult::Processing::Completed);
                        
        ErrorMessageFinder errorMessageFinder{"'not a valid name because it has spaces' is not a valid identifier"};
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_InvalidPropertyName)
    {
        const AZStd::string inputJson = R"(
            {
                "propertyLayout": {
                    "propertyGroups": [
                        {
                            "name": "general",
                            "properties": [
                                {
                                    "name": "not a valid name because it has spaces",
                                    "type": "Bool"
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

        MaterialTypeSourceData sourceData;
        JsonTestResult loadResult = LoadTestDataFromJson(sourceData, inputJson);
        EXPECT_EQ(loadResult.m_jsonResultCode.GetProcessing(), JsonSerializationResult::Processing::Completed);
                
        ErrorMessageFinder errorMessageFinder{"'not a valid name because it has spaces' is not a valid identifier"};
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_DuplicatePropertyId)
    {
            const AZStd::string inputJson = R"(
            {
                "propertyLayout": {
                    "propertyGroups": [
                        {
                            "name": "general",
                            "properties": [
                                {
                                    "name": "foo",
                                    "type": "Bool"
                                },
                                {
                                    "name": "foo",
                                    "type": "Bool"
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

        MaterialTypeSourceData sourceData;
        JsonTestResult loadResult = LoadTestDataFromJson(sourceData, inputJson);
        EXPECT_EQ(loadResult.m_jsonResultCode.GetProcessing(), JsonSerializationResult::Processing::Completed);
        
        ErrorMessageFinder errorMessageFinder("Material property 'general.foo': A property with this ID already exists");
        errorMessageFinder.AddExpectedErrorMessage("Cannot continue building MaterialTypeAsset");
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
        errorMessageFinder.CheckExpectedErrorsFound();
    }
    
    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_PropertyAndPropertyGroupNameCollision)
    {
            const AZStd::string inputJson = R"(
            {
                "propertyLayout": {
                    "propertyGroups": [
                        {
                            "name": "general",
                            "properties": [
                                {
                                    "name": "foo",
                                    "type": "Bool"
                                }
                            ],
                            "propertyGroups": [
                                {
                                    "name": "foo",
                                    "properties": [
                                        {
                                            "name": "bar",
                                            "type": "Bool"
                                        }
                                    ]
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

        MaterialTypeSourceData sourceData;
        JsonTestResult loadResult = LoadTestDataFromJson(sourceData, inputJson);
        EXPECT_EQ(loadResult.m_jsonResultCode.GetProcessing(), JsonSerializationResult::Processing::Completed);
        
        ErrorMessageFinder errorMessageFinder("Material property 'general.foo' collides with a PropertyGroup with the same ID");
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_PropertyConnectedToMultipleOutputs)
    {
        // Setup the shader...

        AZStd::vector<RPI::ShaderOptionValuePair> optionValues = CreateEnumShaderOptionValues({"Low", "Med", "High"});

        uint32_t order = 0;

        Ptr<ShaderOptionGroupLayout> shaderAOptions = ShaderOptionGroupLayout::Create();
        order = 0;
        shaderAOptions->AddShaderOption(ShaderOptionDescriptor{ Name{"o_quality"}, ShaderOptionType::Enumeration, 0, order++, optionValues, Name{"Low"} });
        shaderAOptions->AddShaderOption(ShaderOptionDescriptor{ Name{"o_speed"}, ShaderOptionType::Enumeration, 2, order++, optionValues, Name{"Low"} });
        shaderAOptions->Finalize();
        auto shaderA = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, shaderAOptions);

        Ptr<ShaderOptionGroupLayout> shaderBOptions = ShaderOptionGroupLayout::Create();
        order = 0;
        shaderBOptions->AddShaderOption(ShaderOptionDescriptor{ Name{"o_efficiency"}, ShaderOptionType::Enumeration, 0, order++, optionValues, Name{"Low"} });
        shaderBOptions->AddShaderOption(ShaderOptionDescriptor{ Name{"o_quality"}, ShaderOptionType::Enumeration, 2, order++, optionValues, Name{"Low"} });
        shaderBOptions->AddShaderOption(ShaderOptionDescriptor{ Name{"o_speed"}, ShaderOptionType::Enumeration, 4, order++, optionValues, Name{"Low"} });
        shaderBOptions->Finalize();
        auto shaderB = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, shaderBOptions);

        Ptr<ShaderOptionGroupLayout> shaderCOptions = ShaderOptionGroupLayout::Create();
        order = 0;
        shaderCOptions->AddShaderOption(ShaderOptionDescriptor{ Name{"o_accuracy"}, ShaderOptionType::Enumeration, 0, order++, optionValues, Name{"Low"} });
        shaderCOptions->AddShaderOption(ShaderOptionDescriptor{ Name{"o_efficiency"}, ShaderOptionType::Enumeration, 2, order++, optionValues, Name{"Low"} });
        shaderCOptions->Finalize();
        auto shaderC = CreateTestShaderAsset(Uuid::CreateRandom(), m_testMaterialSrgLayout, shaderCOptions);

        Data::AssetInfo testShaderAssetInfo;
        testShaderAssetInfo.m_assetId = shaderA.GetId();
        m_assetSystemStub.RegisterSourceInfo("shaderA.shader", testShaderAssetInfo, "");
        testShaderAssetInfo.m_assetId = shaderB.GetId();
        m_assetSystemStub.RegisterSourceInfo("shaderB.shader", testShaderAssetInfo, "");
        testShaderAssetInfo.m_assetId = shaderC.GetId();
        m_assetSystemStub.RegisterSourceInfo("shaderC.shader", testShaderAssetInfo, "");

        // Setup the material...

        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ "shaderA.shader" });
        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ "shaderB.shader" });
        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ "shaderC.shader" });
        
        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");
        MaterialTypeSourceData::PropertyDefinition* property = propertyGroup->AddProperty("MyInt");

        property->m_displayName = "Integer";
        property->m_description = "Integer property that is connected to multiple shader settings";
        property->m_dataType = MaterialPropertyDataType::Int;
        property->m_value = 0;

        // The value maps to m_int in the SRG
        property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string("m_int") });
        // The value also maps to m_uint in the SRG
        property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string("m_uint") });
        // The value also maps to the first shader's "o_speed" option
        property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderOption, AZStd::string("o_speed"), 0 });
        // The value also maps to the second shader's "o_speed" option
        property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderOption, AZStd::string("o_speed"), 1 });
        // This case doesn't specify an index, so it will apply to all shaders that have a "o_efficiency", which means it will create two outputs in the property descriptor.
        property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderOption, AZStd::string("o_efficiency") });
        

        // Do the actual test...

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"general.MyInt" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

        EXPECT_EQ(propertyDescriptor->GetOutputConnections().size(), 6);

        // m_int
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[0].m_containerIndex.GetIndex(), -1);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex(), 2);

        // m_uint
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[1].m_containerIndex.GetIndex(), -1);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[1].m_itemIndex.GetIndex(), 3); 

        // shaderA's Speed option
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[2].m_containerIndex.GetIndex(), 0);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[2].m_itemIndex.GetIndex(), shaderAOptions->FindShaderOptionIndex(Name{"o_speed"}).GetIndex());

        // shaderB's Speed option
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[3].m_containerIndex.GetIndex(), 1);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[3].m_itemIndex.GetIndex(), shaderBOptions->FindShaderOptionIndex(Name{ "o_speed" }).GetIndex());

        // shaderB's Efficiency option
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[4].m_containerIndex.GetIndex(), 1);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[4].m_itemIndex.GetIndex(), shaderBOptions->FindShaderOptionIndex(Name{ "o_efficiency" }).GetIndex());

        // shaderC's Efficiency option
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[5].m_containerIndex.GetIndex(), 2);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[5].m_itemIndex.GetIndex(), shaderCOptions->FindShaderOptionIndex(Name{ "o_efficiency" }).GetIndex());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_PropertyWithShaderInputFunctor)
    {
        MaterialTypeSourceData sourceData;
        
        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");
        MaterialTypeSourceData::PropertyDefinition* property = propertyGroup->AddProperty("floatForFunctor");

        property->m_displayName = "Float for Functor";
        property->m_description = "This float is processed by a functor, not with a direct connection";
        property->m_dataType = MaterialPropertyDataType::Float;
        property->m_value = 0.0f;
        // Note that we don't fill property->m_outputConnections because this is not an aliased property
        
        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});

        sourceData.m_materialFunctorSourceData.push_back(
            Ptr<MaterialFunctorSourceDataHolder>
            (
                aznew MaterialFunctorSourceDataHolder
                (
                    aznew Splat3FunctorSourceData{ "general.floatForFunctor", "m_float3" }
                )
            )
        );

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"general.floatForFunctor" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

        ValidateCommonDescriptorFields(*property, propertyDescriptor);

        EXPECT_EQ(1, materialTypeAsset->GetMaterialFunctors().size());
        auto shaderInputFunctor = azrtti_cast<Splat3Functor*>(materialTypeAsset->GetMaterialFunctors()[0].get());
        EXPECT_TRUE(nullptr != shaderInputFunctor);
        EXPECT_EQ(propertyIndex, shaderInputFunctor->m_floatIndex);

        const RHI::ShaderInputConstantIndex expectedVector3Index = materialTypeAsset->GetMaterialSrgLayout()->FindShaderInputConstantIndex(Name{ "m_float3" });
        EXPECT_EQ(expectedVector3Index, shaderInputFunctor->m_vector3Index);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_PropertyWithShaderEnabledFunctor)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});
        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});
        
        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");
        MaterialTypeSourceData::PropertyDefinition* property1 = propertyGroup->AddProperty("EnableSpecialPassA");
        MaterialTypeSourceData::PropertyDefinition* property2 = propertyGroup->AddProperty("EnableSpecialPassB");

        property1->m_displayName = property2->m_displayName = "Enable Special Pass";
        property1->m_description = property2->m_description = "This is a bool to enable an extra shader/pass";
        property1->m_dataType    = property2->m_dataType    = MaterialPropertyDataType::Bool;
        property1->m_value       = property2->m_value       = false;

        sourceData.m_materialFunctorSourceData.push_back(
            Ptr<MaterialFunctorSourceDataHolder>
            (
                aznew MaterialFunctorSourceDataHolder
                (
                    aznew EnableShaderFunctorSourceData{ "general.EnableSpecialPassA", 0 }
                )
            )
        );

        sourceData.m_materialFunctorSourceData.push_back(
            Ptr<MaterialFunctorSourceDataHolder>
            (
                aznew MaterialFunctorSourceDataHolder
                (
                    aznew EnableShaderFunctorSourceData{ "general.EnableSpecialPassB", 1 }
                )
            )
        );

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyAIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"general.EnableSpecialPassA" });
        const MaterialPropertyDescriptor* propertyADescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyAIndex);
        const MaterialPropertyIndex propertyBIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"general.EnableSpecialPassB"});
        const MaterialPropertyDescriptor* propertyBDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyBIndex);

        ValidateCommonDescriptorFields(*sourceData.FindProperty("general.EnableSpecialPassA"), propertyADescriptor);
        ValidateCommonDescriptorFields(*sourceData.FindProperty("general.EnableSpecialPassB"), propertyBDescriptor);

        EXPECT_EQ(2, materialTypeAsset->GetMaterialFunctors().size());
        auto functorA = azrtti_cast<EnableShaderFunctor*>(materialTypeAsset->GetMaterialFunctors()[0].get());
        auto functorB = azrtti_cast<EnableShaderFunctor*>(materialTypeAsset->GetMaterialFunctors()[1].get());
        EXPECT_NE(functorA, nullptr);
        EXPECT_EQ(functorA->m_enablePropertyIndex, propertyAIndex);
        EXPECT_EQ(functorA->m_shaderIndex, 0);
        EXPECT_NE(functorB, nullptr);
        EXPECT_EQ(functorB->m_enablePropertyIndex, propertyBIndex);
        EXPECT_EQ(functorB->m_shaderIndex, 1);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_FunctorSetsShaderOptions)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});
        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});
        
        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");
        MaterialTypeSourceData::PropertyDefinition* property = propertyGroup->AddProperty("MyProperty");

        property->m_dataType = MaterialPropertyDataType::Bool;
        property->m_value = false;
        // Note that we don't fill property->m_outputConnections because this is not a direct-connected property
        
        sourceData.m_materialFunctorSourceData.push_back(
            Ptr<MaterialFunctorSourceDataHolder>
            (
                aznew MaterialFunctorSourceDataHolder
                (
                    aznew SetShaderOptionFunctorSourceData{}
                )
            )
        );

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        // This option is not a dependency of the functor and therefore is not owned by the material
        EXPECT_FALSE(materialTypeAsset->GetShaderCollection()[0].MaterialOwnsShaderOption(Name{"o_quality"}));

        // These options are listed as dependencies of the functor, so the material owns them
        EXPECT_TRUE(materialTypeAsset->GetShaderCollection()[0].MaterialOwnsShaderOption(Name{"o_foo"}));
        EXPECT_TRUE(materialTypeAsset->GetShaderCollection()[0].MaterialOwnsShaderOption(Name{"o_bar"}));
    }
    
    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_FunctorIsInsidePropertyGroup)
    {
        MaterialTypeSourceData sourceData;
        
        MaterialTypeSourceData::PropertyGroup* propertyGroup = sourceData.AddPropertyGroup("general");
        MaterialTypeSourceData::PropertyDefinition* property = propertyGroup->AddProperty("floatForFunctor");

        property->m_dataType = MaterialPropertyDataType::Float;
        property->m_value = 0.0f;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});

        sourceData.m_materialFunctorSourceData.push_back(
            Ptr<MaterialFunctorSourceDataHolder>
            (
                aznew MaterialFunctorSourceDataHolder
                (
                    aznew Splat3FunctorSourceData{ "general.floatForFunctor", "m_float3" }
                )
            )
        );

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"general.floatForFunctor" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

        ValidateCommonDescriptorFields(*property, propertyDescriptor);

        EXPECT_EQ(1, materialTypeAsset->GetMaterialFunctors().size());
        auto shaderInputFunctor = azrtti_cast<Splat3Functor*>(materialTypeAsset->GetMaterialFunctors()[0].get());
        EXPECT_TRUE(nullptr != shaderInputFunctor);
        EXPECT_EQ(propertyIndex, shaderInputFunctor->m_floatIndex);

        const RHI::ShaderInputConstantIndex expectedVector3Index = materialTypeAsset->GetMaterialSrgLayout()->FindShaderInputConstantIndex(Name{ "m_float3" });
        EXPECT_EQ(expectedVector3Index, shaderInputFunctor->m_vector3Index);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_PropertyValues_AllTypes)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ TestShaderFilename });

        auto addProperty = [&sourceData](MaterialPropertyDataType dateType, const char* propertyName, const char* srgConstantName, const AZ::RPI::MaterialPropertyValue& value)
        {
            MaterialTypeSourceData::PropertyDefinition* property = sourceData.AddProperty(propertyName);
            property->m_dataType = dateType;
            property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string(srgConstantName) });
            property->m_value = value;
        };
        
        sourceData.AddPropertyGroup("general");

        addProperty(MaterialPropertyDataType::Bool,    "general.MyBool",   "m_bool",    true);
        addProperty(MaterialPropertyDataType::Float,   "general.MyFloat",  "m_float",   1.2f);
        addProperty(MaterialPropertyDataType::Int,     "general.MyInt",    "m_int",     -12);
        addProperty(MaterialPropertyDataType::UInt,    "general.MyUInt",   "m_uint",    12u);
        addProperty(MaterialPropertyDataType::Vector2, "general.MyFloat2", "m_float2",  AZ::Vector2{1.1f, 2.2f});
        addProperty(MaterialPropertyDataType::Vector3, "general.MyFloat3", "m_float3",  AZ::Vector3{3.3f, 4.4f, 5.5f});
        addProperty(MaterialPropertyDataType::Vector4, "general.MyFloat4", "m_float4",  AZ::Vector4{6.6f, 7.7f, 8.8f, 9.9f});
        addProperty(MaterialPropertyDataType::Color,   "general.MyColor",  "m_color",   AZ::Color{0.1f, 0.2f, 0.3f, 0.4f});
        addProperty(MaterialPropertyDataType::Image,   "general.MyImage",  "m_image",   AZStd::string{TestImageFilename});
        addProperty(MaterialPropertyDataType::Image,   "general.MyAttachmentImage",  "m_attachmentImage",   AZStd::string{TestAttImageFilename});

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        CheckPropertyValue<bool>    (materialTypeAsset,  Name{"general.MyBool"},   true);
        CheckPropertyValue<float>   (materialTypeAsset,  Name{"general.MyFloat"},  1.2f);
        CheckPropertyValue<int32_t> (materialTypeAsset,  Name{"general.MyInt"},    -12);
        CheckPropertyValue<uint32_t>(materialTypeAsset,  Name{"general.MyUInt"},   12u);
        CheckPropertyValue<Vector2> (materialTypeAsset,  Name{"general.MyFloat2"}, Vector2{ 1.1f, 2.2f });
        CheckPropertyValue<Vector3> (materialTypeAsset,  Name{"general.MyFloat3"}, Vector3{ 3.3f, 4.4f, 5.5f });
        CheckPropertyValue<Vector4> (materialTypeAsset,  Name{"general.MyFloat4"}, Vector4{ 6.6f, 7.7f, 8.8f, 9.9f });
        CheckPropertyValue<Color>   (materialTypeAsset,  Name{"general.MyColor"},  Color{ 0.1f, 0.2f, 0.3f, 0.4f });
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{"general.MyImage"}, m_testImageAsset);
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{"general.MyAttachmentImage"}, m_testAttachmentImageAsset);
    }
    
    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_NestedPropertyGroups)
    {
        RHI::Ptr<RHI::ShaderResourceGroupLayout> layeredMaterialSrgLayout = RHI::ShaderResourceGroupLayout::Create();
        layeredMaterialSrgLayout->SetName(Name{"MaterialSrg"});
        layeredMaterialSrgLayout->SetBindingSlot(SrgBindingSlot::Material);
        layeredMaterialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_layer1_baseColor_texture" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
        layeredMaterialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_layer1_roughness_texture" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
        layeredMaterialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_layer2_baseColor_texture" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
        layeredMaterialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_layer2_roughness_texture" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
        layeredMaterialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_layer2_clearCoat_roughness_texture" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
        layeredMaterialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_layer2_clearCoat_normal_texture" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
        layeredMaterialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_layer2_clearCoat_normal_factor" }, 0, 4, 0 });
        layeredMaterialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_blendFactor" }, 4, 4, 0 });
        layeredMaterialSrgLayout->Finalize();
        
        AZStd::vector<RPI::ShaderOptionValuePair> boolOptionValues = CreateBoolShaderOptionValues();
        Ptr<ShaderOptionGroupLayout> shaderOptionsLayout = ShaderOptionGroupLayout::Create();
        uint32_t order = 0;
        shaderOptionsLayout->AddShaderOption(ShaderOptionDescriptor{Name{"o_layer2_clearCoat_enable"}, ShaderOptionType::Boolean, 0, order++, boolOptionValues, Name{"False"}});
        shaderOptionsLayout->Finalize();

        Data::Asset<ShaderAsset> layeredMaterialShaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), layeredMaterialSrgLayout, shaderOptionsLayout);
        
        Data::AssetInfo testShaderAssetInfo;
        testShaderAssetInfo.m_assetId = layeredMaterialShaderAsset.GetId();
        m_assetSystemStub.RegisterSourceInfo("layeredMaterial.shader", testShaderAssetInfo, "");

        MaterialTypeSourceData sourceData;
        
        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ "layeredMaterial.shader" });

        auto addSrgProperty = [&sourceData](MaterialPropertyDataType dateType, MaterialPropertyOutputType connectionType, const char* propertyName, const char* srgConstantName, const AZ::RPI::MaterialPropertyValue& value)
        {
            MaterialTypeSourceData::PropertyDefinition* property = sourceData.AddProperty(propertyName);
            property->m_dataType = dateType;
            property->m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ connectionType, AZStd::string(srgConstantName) });
            property->m_value = value;
        };
        
        sourceData.AddPropertyGroup("layer1");
        sourceData.AddPropertyGroup("layer2");
        sourceData.AddPropertyGroup("blend");
        sourceData.AddPropertyGroup("layer1.baseColor");
        sourceData.AddPropertyGroup("layer2.baseColor");
        sourceData.AddPropertyGroup("layer1.roughness");
        sourceData.AddPropertyGroup("layer2.roughness");
        sourceData.AddPropertyGroup("layer2.clearCoat");
        sourceData.AddPropertyGroup("layer2.clearCoat.roughness");
        sourceData.AddPropertyGroup("layer2.clearCoat.normal");
        
        addSrgProperty(MaterialPropertyDataType::Image, MaterialPropertyOutputType::ShaderInput,  "layer1.baseColor.texture", "m_layer1_baseColor_texture", AZStd::string{TestImageFilename});
        addSrgProperty(MaterialPropertyDataType::Image, MaterialPropertyOutputType::ShaderInput,  "layer1.roughness.texture", "m_layer1_roughness_texture", AZStd::string{TestImageFilename});
        addSrgProperty(MaterialPropertyDataType::Image, MaterialPropertyOutputType::ShaderInput,  "layer2.baseColor.texture", "m_layer2_baseColor_texture", AZStd::string{TestImageFilename});
        addSrgProperty(MaterialPropertyDataType::Image, MaterialPropertyOutputType::ShaderInput,  "layer2.roughness.texture", "m_layer2_roughness_texture", AZStd::string{TestImageFilename});
        addSrgProperty(MaterialPropertyDataType::Bool,  MaterialPropertyOutputType::ShaderOption, "layer2.clearCoat.enabled", "o_layer2_clearCoat_enable", true);
        addSrgProperty(MaterialPropertyDataType::Image, MaterialPropertyOutputType::ShaderInput,  "layer2.clearCoat.roughness.texture", "m_layer2_clearCoat_roughness_texture", AZStd::string{TestImageFilename});
        addSrgProperty(MaterialPropertyDataType::Image, MaterialPropertyOutputType::ShaderInput,  "layer2.clearCoat.normal.texture", "m_layer2_clearCoat_normal_texture", AZStd::string{TestImageFilename});
        addSrgProperty(MaterialPropertyDataType::Float, MaterialPropertyOutputType::ShaderInput,  "layer2.clearCoat.normal.factor", "m_layer2_clearCoat_normal_factor", 0.4f);
        addSrgProperty(MaterialPropertyDataType::Float, MaterialPropertyOutputType::ShaderInput,  "blend.factor", "m_blendFactor", 0.5f);

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();
        
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{"layer1.baseColor.texture"}, m_testImageAsset);
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{"layer1.roughness.texture"}, m_testImageAsset);
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{"layer2.baseColor.texture"}, m_testImageAsset);
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{"layer2.roughness.texture"}, m_testImageAsset);
        CheckPropertyValue<bool>(materialTypeAsset, Name{"layer2.clearCoat.enabled"}, true);
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{"layer2.clearCoat.roughness.texture"}, m_testImageAsset);
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{"layer2.clearCoat.normal.texture"}, m_testImageAsset);
        CheckPropertyValue<float>(materialTypeAsset, Name{"layer2.clearCoat.normal.factor"}, 0.4f);
        CheckPropertyValue<float>(materialTypeAsset, Name{"blend.factor"}, 0.5f);

        // Note it might be nice to check that the right property connections are prescribed in the final MaterialTypeAsset,
        // but it's not really necessary because CreateMaterialTypeAsset reports errors when a connection target is not found
        // in the shader options layout or SRG layout. If one of the output names like "m_layer2_roughness_texture" is wrong
        // these errors will cause this test to fail.
    }

    TEST_F(MaterialTypeSourceDataTests, LoadAndStoreJson_AllFields)
    {
        // Note that serialization of individual fields within material properties is thoroughly tested in
        // MaterialPropertySerializerTests, so the sample property data used here is cursory.
        // We also don't cover fields related to providing name contexts for nested property groups, like
        // "shaderInputsPrefix" and "shaderOptionsPrefix" as those are covered in CreateMaterialTypeAsset_NestedGroups*.
        //
        // NOTE: The keys in the actions lists of versionUpdates need to be given in alphabetical
        // order to ensure exact json string match after serialization + deserialization.
        const AZStd::string inputJson = R"(
            {
                "description": "This is a general description about the material",
                "version": 2,
                "versionUpdates": [
                    {
                        "toVersion": 2,
                        "actions": [
                            { "from": "groupA.fooPrev", "op": "rename", "to": "groupA.foo" },
                            { "name": "groupB.bar", "op": "setValue", "value": [0.0, 0.5, 1.0] }
                        ]
                    }
                ],
                "propertyLayout": {
                    "propertyGroups": [
                        {
                            "name": "groupA",
                            "displayName": "Property Group A",
                            "description": "Description of property group A",
                            "properties": [
                                {
                                    "name": "foo",
                                    "type": "Bool",
                                    "defaultValue": true
                                },
                                {
                                    "name": "bar",
                                    "type": "Image",
                                    "defaultValue": "Default.png",
                                    "visibility": "Hidden"
                                }
                            ],
                            "functors": [
                                {
                                    "type": "EnableShader",
                                    "args": {
                                        "enablePassProperty": "foo",
                                        "shaderIndex": 1
                                    }
                                }
                            ]
                        },
                        {
                            "name": "groupB",
                            "displayName": "Property Group B",
                            "description": "Description of property group B",
                            "properties": [
                                {
                                    "name": "foo",
                                    "type": "Float",
                                    "defaultValue": 0.5
                                },
                                {
                                    "name": "bar",
                                    "type": "Color",
                                    "defaultValue": [0.5, 0.5, 0.5],
                                    "visibility": "Disabled"
                                }
                            ],
                            "functors": [
                                {
                                    "type": "Splat3",
                                    "args": {
                                        "floatPropertyInput": "foo",
                                        "float3ShaderSettingOutput": "m_someFloat3"
                                    }
                                }
                            ]
                        },
                        {
                            "name": "groupC",
                            "displayName": "Property Group C",
                            "description": "Property group C has a nested property group",
                            "propertyGroups": [
                                {
                                    "name": "groupD",
                                    "displayName": "Property Group D",
                                    "description": "Description of property group D",
                                    "properties": [
                                        {
                                            "name": "foo",
                                            "type": "Int",
                                            "defaultValue": -1
                                        }
                                    ]
                                },
                                {
                                    "name": "groupE",
                                    "displayName": "Property Group E",
                                    "description": "Description of property group E",
                                    "properties": [
                                        {
                                            "name": "bar",
                                            "type": "UInt"
                                        }
                                    ]
                                }
                            ]
                        }
                    ]
                },
                "shaders": [
                    {
                        "file": "ForwardPass.shader",
                        "tag": "ForwardPass",
                        "options": {
                            "o_optionA": "False",
                            "o_optionB": "True"
                        }
                    },
                    {
                        "file": "DepthPass.shader",
                        "options": {
                            "o_optionC": "1",
                            "o_optionD": "2"
                        }
                    }
                ],
                "functors": [
                    {
                        "type": "SetShaderOption",
                        "args": {
                            "enableProperty": "groupA.foo"
                        }
                    }
                ]
            }
        )";

        MaterialTypeSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        EXPECT_EQ(material.m_description, "This is a general description about the material");


        EXPECT_EQ(material.m_version, 2);
        EXPECT_EQ(material.m_versionUpdates.size(), 1);
        EXPECT_EQ(material.m_versionUpdates[0].m_toVersion, 2);
        EXPECT_EQ(material.m_versionUpdates[0].m_actions.size(), 2);

        {
            const auto opIt   = material.m_versionUpdates[0].m_actions[0].find("op");
            const auto fromIt = material.m_versionUpdates[0].m_actions[0].find("from");
            const auto toIt   = material.m_versionUpdates[0].m_actions[0].find("to");
            EXPECT_FALSE(opIt   == material.m_versionUpdates[0].m_actions[0].end());
            EXPECT_FALSE(fromIt == material.m_versionUpdates[0].m_actions[0].end());
            EXPECT_FALSE(toIt   == material.m_versionUpdates[0].m_actions[0].end());
            EXPECT_EQ(opIt->first,    AZStd::string("op"));
            EXPECT_EQ(fromIt->first,  AZStd::string("from"));
            EXPECT_EQ(toIt->first,    AZStd::string("to"));
            EXPECT_EQ(opIt->second,   AZStd::string("rename"));
            EXPECT_EQ(fromIt->second, AZStd::string("groupA.fooPrev"));
            EXPECT_EQ(toIt->second,   AZStd::string("groupA.foo"));
        }

        {
            const auto opIt    = material.m_versionUpdates[0].m_actions[1].find("op");
            const auto nameIt  = material.m_versionUpdates[0].m_actions[1].find("name");
            const auto valueIt = material.m_versionUpdates[0].m_actions[1].find("value");
            EXPECT_FALSE(opIt    == material.m_versionUpdates[0].m_actions[1].end());
            EXPECT_FALSE(nameIt  == material.m_versionUpdates[0].m_actions[1].end());
            EXPECT_FALSE(valueIt == material.m_versionUpdates[0].m_actions[1].end());
            EXPECT_EQ(opIt->first,     AZStd::string("op"));
            EXPECT_EQ(nameIt->first,   AZStd::string("name"));
            EXPECT_EQ(valueIt->first,  AZStd::string("value"));
            EXPECT_EQ(opIt->second,    AZStd::string("setValue"));
            EXPECT_EQ(nameIt->second,  AZStd::string("groupB.bar"));
            const Color colorValue = valueIt->second.CastToType(azrtti_typeid<Color>()).GetValue<Color>();
            EXPECT_EQ(colorValue, Color(0.0f, 0.5f, 1.0f, 1.0f));
        }

        EXPECT_EQ(material.GetPropertyLayout().m_propertyGroups.size(), 3);
        EXPECT_TRUE(material.FindPropertyGroup("groupA") != nullptr);
        EXPECT_TRUE(material.FindPropertyGroup("groupB") != nullptr);
        EXPECT_TRUE(material.FindPropertyGroup("groupC") != nullptr);
        EXPECT_TRUE(material.FindPropertyGroup("groupC.groupD") != nullptr);
        EXPECT_TRUE(material.FindPropertyGroup("groupC.groupE") != nullptr);
        EXPECT_EQ(material.FindPropertyGroup("groupA")->GetDisplayName(), "Property Group A");
        EXPECT_EQ(material.FindPropertyGroup("groupB")->GetDisplayName(), "Property Group B");
        EXPECT_EQ(material.FindPropertyGroup("groupC")->GetDisplayName(), "Property Group C");
        EXPECT_EQ(material.FindPropertyGroup("groupC.groupD")->GetDisplayName(), "Property Group D");
        EXPECT_EQ(material.FindPropertyGroup("groupC.groupE")->GetDisplayName(), "Property Group E");
        EXPECT_EQ(material.FindPropertyGroup("groupA")->GetDescription(), "Description of property group A");
        EXPECT_EQ(material.FindPropertyGroup("groupB")->GetDescription(), "Description of property group B");
        EXPECT_EQ(material.FindPropertyGroup("groupC")->GetDescription(), "Property group C has a nested property group");
        EXPECT_EQ(material.FindPropertyGroup("groupC.groupD")->GetDescription(), "Description of property group D");
        EXPECT_EQ(material.FindPropertyGroup("groupC.groupE")->GetDescription(), "Description of property group E");
        EXPECT_EQ(material.FindPropertyGroup("groupA")->GetProperties().size(), 2);
        EXPECT_EQ(material.FindPropertyGroup("groupB")->GetProperties().size(), 2);
        EXPECT_EQ(material.FindPropertyGroup("groupC")->GetProperties().size(), 0);
        EXPECT_EQ(material.FindPropertyGroup("groupC.groupD")->GetProperties().size(), 1);
        EXPECT_EQ(material.FindPropertyGroup("groupC.groupE")->GetProperties().size(), 1);
        
        EXPECT_NE(material.FindProperty("groupA.foo"), nullptr);
        EXPECT_NE(material.FindProperty("groupA.bar"), nullptr);
        EXPECT_NE(material.FindProperty("groupB.foo"), nullptr);
        EXPECT_NE(material.FindProperty("groupB.bar"), nullptr);
        EXPECT_NE(material.FindProperty("groupC.groupD.foo"), nullptr);
        EXPECT_NE(material.FindProperty("groupC.groupE.bar"), nullptr);

        EXPECT_EQ(material.FindProperty("groupA.foo")->GetName(), "foo");
        EXPECT_EQ(material.FindProperty("groupA.bar")->GetName(), "bar");
        EXPECT_EQ(material.FindProperty("groupB.foo")->GetName(), "foo");
        EXPECT_EQ(material.FindProperty("groupB.bar")->GetName(), "bar");
        EXPECT_EQ(material.FindProperty("groupC.groupD.foo")->GetName(), "foo");
        EXPECT_EQ(material.FindProperty("groupC.groupE.bar")->GetName(), "bar");
        EXPECT_EQ(material.FindProperty("groupA.foo")->m_dataType, MaterialPropertyDataType::Bool);
        EXPECT_EQ(material.FindProperty("groupA.bar")->m_dataType, MaterialPropertyDataType::Image);
        EXPECT_EQ(material.FindProperty("groupB.foo")->m_dataType, MaterialPropertyDataType::Float);
        EXPECT_EQ(material.FindProperty("groupB.bar")->m_dataType, MaterialPropertyDataType::Color);
        EXPECT_EQ(material.FindProperty("groupC.groupD.foo")->m_dataType, MaterialPropertyDataType::Int);
        EXPECT_EQ(material.FindProperty("groupC.groupE.bar")->m_dataType, MaterialPropertyDataType::UInt);
        EXPECT_EQ(material.FindProperty("groupA.foo")->m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(material.FindProperty("groupA.bar")->m_visibility, MaterialPropertyVisibility::Hidden);
        EXPECT_EQ(material.FindProperty("groupB.foo")->m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(material.FindProperty("groupB.bar")->m_visibility, MaterialPropertyVisibility::Disabled);
        EXPECT_EQ(material.FindProperty("groupC.groupD.foo")->m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(material.FindProperty("groupC.groupE.bar")->m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(material.FindProperty("groupA.foo")->m_value, true);
        EXPECT_EQ(material.FindProperty("groupA.bar")->m_value, AZStd::string{"Default.png"});
        EXPECT_EQ(material.FindProperty("groupB.foo")->m_value, 0.5f);
        EXPECT_EQ(material.FindProperty("groupB.bar")->m_value, AZ::Color(0.5f, 0.5f, 0.5f, 1.0f));
        EXPECT_EQ(material.FindProperty("groupC.groupD.foo")->m_value, -1);
        EXPECT_EQ(material.FindProperty("groupC.groupE.bar")->m_value, 0u);
        
        EXPECT_EQ(material.FindPropertyGroup("groupA")->GetFunctors().size(), 1);
        EXPECT_EQ(material.FindPropertyGroup("groupB")->GetFunctors().size(), 1);
        Ptr<MaterialFunctorSourceData> functorA = material.FindPropertyGroup("groupA")->GetFunctors()[0]->GetActualSourceData();
        Ptr<MaterialFunctorSourceData> functorB = material.FindPropertyGroup("groupB")->GetFunctors()[0]->GetActualSourceData();
        EXPECT_TRUE(azrtti_cast<const EnableShaderFunctorSourceData*>(functorA.get()));
        EXPECT_EQ(azrtti_cast<const EnableShaderFunctorSourceData*>(functorA.get())->m_enablePassPropertyId, "foo");
        EXPECT_EQ(azrtti_cast<const EnableShaderFunctorSourceData*>(functorA.get())->m_shaderIndex, 1);
        EXPECT_TRUE(azrtti_cast<const Splat3FunctorSourceData*>(functorB.get()));
        EXPECT_EQ(azrtti_cast<const Splat3FunctorSourceData*>(functorB.get())->m_floatPropertyInputId, "foo");
        EXPECT_EQ(azrtti_cast<const Splat3FunctorSourceData*>(functorB.get())->m_float3ShaderSettingOutputId, "m_someFloat3");

        EXPECT_EQ(material.m_shaderCollection.size(), 2);
        EXPECT_EQ(material.m_shaderCollection[0].m_shaderFilePath, "ForwardPass.shader");
        EXPECT_EQ(material.m_shaderCollection[1].m_shaderFilePath, "DepthPass.shader");
        EXPECT_EQ(material.m_shaderCollection[0].m_shaderOptionValues.size(), 2);
        EXPECT_EQ(material.m_shaderCollection[1].m_shaderOptionValues.size(), 2);
        EXPECT_EQ(material.m_shaderCollection[0].m_shaderOptionValues[Name{"o_optionA"}], Name{"False"});
        EXPECT_EQ(material.m_shaderCollection[0].m_shaderOptionValues[Name{"o_optionB"}], Name{"True"});
        EXPECT_EQ(material.m_shaderCollection[1].m_shaderOptionValues[Name{"o_optionC"}], Name{"1"});
        EXPECT_EQ(material.m_shaderCollection[1].m_shaderOptionValues[Name{"o_optionD"}], Name{"2"});
        EXPECT_EQ(material.m_shaderCollection[0].m_shaderTag, Name{"ForwardPass"});

        EXPECT_EQ(material.m_materialFunctorSourceData.size(), 1);
        Ptr<MaterialFunctorSourceData> functorC = material.m_materialFunctorSourceData[0]->GetActualSourceData();
        EXPECT_TRUE(azrtti_cast<const SetShaderOptionFunctorSourceData*>(functorC.get()));
        EXPECT_EQ(azrtti_cast<const SetShaderOptionFunctorSourceData*>(functorC.get())->m_enablePropertyName, "groupA.foo");
        
        AZStd::string outputJson;
        JsonTestResult storeResult = StoreTestDataToJson(material, outputJson);
        ExpectSimilarJson(inputJson, outputJson);
    }

    TEST_F(MaterialTypeSourceDataTests, LoadAllFieldsUsingOldFormat)
    {
        // The content of this test was copied from LoadAndStoreJson_AllFields to prove backward compatibility.
        // (The "store" part of the test was not included because the saved data will be the new format).
        // Notable differences include:
        // 1) the key "id" is used instead of "name"
        // 2) the group metadata, property definitions, and functors are all defined in different sections rather than in a unified property group definition

        const AZStd::string inputJson = R"(
            {
                "description": "This is a general description about the material",
                "propertyLayout": {
                    "version": 2,
                    "groups": [
                        {
                            "id": "groupA",
                            "displayName": "Property Group A",
                            "description": "Description of property group A"
                        },
                        {
                            "id": "groupB",
                            "displayName": "Property Group B",
                            "description": "Description of property group B"
                        }
                    ],
                    "properties": {
                        "groupA": [
                            {
                                "id": "foo",
                                "type": "Bool",
                                "defaultValue": true
                            },
                            {
                                "id": "bar",
                                "type": "Image",
                                "defaultValue": "Default.png",
                                "visibility": "Hidden"
                            }
                        ],
                        "groupB": [
                            {
                                "id": "foo",
                                "type": "Float",
                                "defaultValue": 0.5
                            },
                            {
                                "id": "bar",
                                "type": "Color",
                                "defaultValue": [0.5, 0.5, 0.5],
                                "visibility": "Disabled"
                            }
                        ]
                    }
                },
                "shaders": [
                    {
                        "file": "ForwardPass.shader",
                        "tag": "ForwardPass",
                        "options": {
                            "o_optionA": "False",
                            "o_optionB": "True"
                        }
                    },
                    {
                        "file": "DepthPass.shader",
                        "options": {
                            "o_optionC": "1",
                            "o_optionD": "2"
                        }
                    }
                ],
                "functors": [
                    {
                        "type": "EnableShader",
                        "args": {
                            "enablePassProperty": "groupA.foo",
                            "shaderIndex": 1
                        }
                    },
                    {
                        "type": "Splat3",
                        "args": {
                            "floatPropertyInput": "groupB.foo",
                            "float3ShaderSettingOutput": "m_someFloat3"
                        }
                    }
                ]
            }
        )";

        MaterialTypeSourceData material;
        JsonTestResult loadResult = LoadTestDataFromJson(material, inputJson);

        // Before conversion to the new format, the data is in the old place
        EXPECT_EQ(material.GetPropertyLayout().m_groupsOld.size(), 2);
        EXPECT_EQ(material.GetPropertyLayout().m_propertiesOld.size(), 2);
        EXPECT_EQ(material.GetPropertyLayout().m_propertyGroups.size(), 0);

        material.UpgradeLegacyFormat();
        
        // After conversion to the new format, the data is in the new place
        EXPECT_EQ(material.GetPropertyLayout().m_groupsOld.size(), 0);
        EXPECT_EQ(material.GetPropertyLayout().m_propertiesOld.size(), 0);
        EXPECT_EQ(material.GetPropertyLayout().m_propertyGroups.size(), 2);

        EXPECT_EQ(material.m_description, "This is a general description about the material");

        EXPECT_TRUE(material.FindPropertyGroup("groupA") != nullptr);
        EXPECT_TRUE(material.FindPropertyGroup("groupB") != nullptr);
        EXPECT_EQ(material.FindPropertyGroup("groupA")->GetDisplayName(), "Property Group A");
        EXPECT_EQ(material.FindPropertyGroup("groupB")->GetDisplayName(), "Property Group B");
        EXPECT_EQ(material.FindPropertyGroup("groupA")->GetDescription(), "Description of property group A");
        EXPECT_EQ(material.FindPropertyGroup("groupB")->GetDescription(), "Description of property group B");
        EXPECT_EQ(material.FindPropertyGroup("groupA")->GetProperties().size(), 2);
        EXPECT_EQ(material.FindPropertyGroup("groupB")->GetProperties().size(), 2);
        
        EXPECT_TRUE(material.FindProperty("groupA.foo") != nullptr);
        EXPECT_TRUE(material.FindProperty("groupA.bar") != nullptr);
        EXPECT_TRUE(material.FindProperty("groupB.foo") != nullptr);
        EXPECT_TRUE(material.FindProperty("groupB.bar") != nullptr);

        EXPECT_EQ(material.FindProperty("groupA.foo")->GetName(), "foo");
        EXPECT_EQ(material.FindProperty("groupA.bar")->GetName(), "bar");
        EXPECT_EQ(material.FindProperty("groupB.foo")->GetName(), "foo");
        EXPECT_EQ(material.FindProperty("groupB.bar")->GetName(), "bar");
        EXPECT_EQ(material.FindProperty("groupA.foo")->m_dataType, MaterialPropertyDataType::Bool);
        EXPECT_EQ(material.FindProperty("groupA.bar")->m_dataType, MaterialPropertyDataType::Image);
        EXPECT_EQ(material.FindProperty("groupB.foo")->m_dataType, MaterialPropertyDataType::Float);
        EXPECT_EQ(material.FindProperty("groupB.bar")->m_dataType, MaterialPropertyDataType::Color);
        EXPECT_EQ(material.FindProperty("groupA.foo")->m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(material.FindProperty("groupA.bar")->m_visibility, MaterialPropertyVisibility::Hidden);
        EXPECT_EQ(material.FindProperty("groupB.foo")->m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(material.FindProperty("groupB.bar")->m_visibility, MaterialPropertyVisibility::Disabled);
        EXPECT_EQ(material.FindProperty("groupA.foo")->m_value, true);
        EXPECT_EQ(material.FindProperty("groupA.bar")->m_value, AZStd::string{"Default.png"});
        EXPECT_EQ(material.FindProperty("groupB.foo")->m_value, 0.5f);
        EXPECT_EQ(material.FindProperty("groupB.bar")->m_value, AZ::Color(0.5f, 0.5f, 0.5f, 1.0f));

        // The functors can appear either at the top level or within each property group. The format conversion
        // function doesn't know how to move the functors, and they will be left at the top level.
        EXPECT_EQ(material.FindPropertyGroup("groupA")->GetFunctors().size(), 0);
        EXPECT_EQ(material.FindPropertyGroup("groupB")->GetFunctors().size(), 0);

        EXPECT_EQ(material.m_shaderCollection.size(), 2);
        EXPECT_EQ(material.m_shaderCollection[0].m_shaderFilePath, "ForwardPass.shader");
        EXPECT_EQ(material.m_shaderCollection[1].m_shaderFilePath, "DepthPass.shader");
        EXPECT_EQ(material.m_shaderCollection[0].m_shaderOptionValues.size(), 2);
        EXPECT_EQ(material.m_shaderCollection[1].m_shaderOptionValues.size(), 2);
        EXPECT_EQ(material.m_shaderCollection[0].m_shaderOptionValues[Name{"o_optionA"}], Name{"False"});
        EXPECT_EQ(material.m_shaderCollection[0].m_shaderOptionValues[Name{"o_optionB"}], Name{"True"});
        EXPECT_EQ(material.m_shaderCollection[1].m_shaderOptionValues[Name{"o_optionC"}], Name{"1"});
        EXPECT_EQ(material.m_shaderCollection[1].m_shaderOptionValues[Name{"o_optionD"}], Name{"2"});
        EXPECT_EQ(material.m_shaderCollection[0].m_shaderTag, Name{"ForwardPass"});

        EXPECT_EQ(material.m_materialFunctorSourceData.size(), 2);
        EXPECT_TRUE(azrtti_cast<const EnableShaderFunctorSourceData*>(material.m_materialFunctorSourceData[0]->GetActualSourceData().get()));
        EXPECT_EQ(azrtti_cast<const EnableShaderFunctorSourceData*>(material.m_materialFunctorSourceData[0]->GetActualSourceData().get())->m_enablePassPropertyId, "groupA.foo");
        EXPECT_EQ(azrtti_cast<const EnableShaderFunctorSourceData*>(material.m_materialFunctorSourceData[0]->GetActualSourceData().get())->m_shaderIndex, 1);
        EXPECT_TRUE(azrtti_cast<const Splat3FunctorSourceData*>(material.m_materialFunctorSourceData[1]->GetActualSourceData().get()));
        EXPECT_EQ(azrtti_cast<const Splat3FunctorSourceData*>(material.m_materialFunctorSourceData[1]->GetActualSourceData().get())->m_floatPropertyInputId, "groupB.foo");
        EXPECT_EQ(azrtti_cast<const Splat3FunctorSourceData*>(material.m_materialFunctorSourceData[1]->GetActualSourceData().get())->m_float3ShaderSettingOutputId, "m_someFloat3");
    }
    
    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_PropertyImagePath)
    {
        char inputJson[2048];
        azsprintf(inputJson,
            R"(
                {
                    "description": "",
                    "propertyLayout": {
                        "propertyGroups": [
                            {
                                "name": "general",
                                "displayName": "General",
                                "description": "",
                                "properties": [
                                    {
                                        "name": "absolute",
                                        "type": "Image",
                                        "defaultValue": "%s"
                                    },
                                    {
                                        "name": "relative",
                                        "type": "Image",
                                        "defaultValue": "%s"
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
            )",
            MaterialTypeSourceDataTests::TestImageFilepathAbsolute,
            MaterialTypeSourceDataTests::TestImageFilepathRelative
        );

        MaterialTypeSourceData material;
        LoadTestDataFromJson(material, inputJson);

        Outcome<Data::Asset<MaterialTypeAsset>> materialTypeOutcome = material.CreateMaterialTypeAsset(Uuid::CreateRandom(), MaterialTypeSourceDataTests::TestMaterialFilepathAbsolute);
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());

        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{ "general.absolute" }, m_testImageAsset2);
        CheckPropertyValue<Data::Asset<ImageAsset>>(materialTypeAsset, Name{ "general.relative" }, m_testImageAsset2);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_ResolveSetValueVersionUpdates)
    {
        char inputJson[2048];
        azsprintf(inputJson,
        R"(
            {
                "description": "",
                "version": 2,
                "versionUpdates": [
                    {
                        "toVersion": 2,
                        "actions": [
                            { "op": "setValue", "name": "grp.myEnum", "value": "Enum1" },
                            { "op": "setValue", "name": "grp.myImage", "value": "%s" }
                        ]
                    }
                ],
                "propertyLayout": {
                    "propertyGroups": [
                        {
                            "name": "grp",
                            "displayName": "",
                            "description": "",
                            "properties": [

                                {
                                    "name": "myEnum",
                                    "type": "Enum",
                                    "enumValues": [ "Enum0", "Enum1", "Enum2", "Enum3"],
                                    "defaultValue": "Enum3"
                                },
                                {
                                    "name": "myImage",
                                    "type": "Image"
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
            )",
            MaterialTypeSourceDataTests::TestImageFilepathAbsolute);

        MaterialTypeSourceData material;
        LoadTestDataFromJson(material, inputJson);

        Outcome<Data::Asset<MaterialTypeAsset>> materialTypeOutcome = material.CreateMaterialTypeAsset(Uuid::CreateRandom(), MaterialTypeSourceDataTests::TestMaterialFilepathAbsolute);
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());

        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        auto materialVersionUpdates = materialTypeAsset->GetMaterialVersionUpdates();
        EXPECT_EQ(materialVersionUpdates.GetVersionUpdateCount(), 1);
        auto actions = materialVersionUpdates.GetVersionUpdate(0).GetActions();
        EXPECT_EQ(actions.size(), 2);

        EXPECT_EQ(actions[0].GetOperation(), Name("setValue"));
        EXPECT_EQ(actions[0].GetArg(Name("name")),  AZStd::string("grp.myEnum"));
        EXPECT_EQ(actions[0].GetArg(Name("value")), 1u);

        EXPECT_EQ(actions[1].GetOperation(), Name("setValue"));
        EXPECT_EQ(actions[1].GetArg(Name("name")),  AZStd::string("grp.myImage"));
        EXPECT_EQ(actions[1].GetArg(Name("value")), m_testImageAsset2);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_VersionInWrongLocation)
    {
        // The version field used to be under the propertyLayout section, but it has been moved up to the top level.
        // If any users have their own custom .materialtype with an older format that has the version in the wrong place
        // then we will report an error with instructions to move it to the correct location.

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("The field '/propertyLayout/version' is deprecated and moved to '/version'. Please edit this material type source file and move the '\"version\": 4' setting up one level");

        const AZStd::string inputJson = R"(
            {
                "propertyLayout": {
                    "version": 4
                },
                "shaders": [
                    {
                        "file": "test.shader"
                    }
                ]
            }
        )";

        MaterialTypeSourceData materialType;
        JsonTestResult loadResult = LoadTestDataFromJson(materialType, inputJson);

        auto materialTypeOutcome = materialType.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_FALSE(materialTypeOutcome.IsSuccess());

        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTypeSourceDataTests, LoadWithImportedJson)
    {
        const AZStd::string propertyGroupJson = R"(
            {
                "name": "myGroup",
                "displayName": "My Group",
                "description": "This group is defined in a separate JSON file",
                "properties": [
                    {
                        "name": "foo",
                        "type": "Bool"
                    },
                    {
                        "name": "bar",
                        "type": "Float"
                    }
                ]
            }
        )";

        IO::FixedMaxPath propertyGroupJsonFilePath = m_tempFolder/"MyPropertyGroup.json";
        AZ::Utils::WriteFile(propertyGroupJson, propertyGroupJsonFilePath.c_str());

        const AZStd::string materialTypeJson = R"(
            {
                "propertyLayout": {
                    "propertyGroups": [
                        { "$import": "MyPropertyGroup.json" }
                    ]
                }
            }
        )";
        
        IO::FixedMaxPath materialTypeJsonFilePath = m_tempFolder/"TestImport.materialtype";
        AZ::Utils::WriteFile(materialTypeJson, materialTypeJsonFilePath.c_str());

        auto loadMaterialTypeResult = MaterialUtils::LoadMaterialTypeSourceData(materialTypeJsonFilePath.c_str());
        EXPECT_TRUE(loadMaterialTypeResult);
        MaterialTypeSourceData materialType = loadMaterialTypeResult.TakeValue();

        EXPECT_EQ(materialType.GetPropertyLayout().m_propertyGroups.size(), 1);
        EXPECT_TRUE(materialType.FindPropertyGroup("myGroup") != nullptr);
        EXPECT_EQ(materialType.FindPropertyGroup("myGroup")->GetDisplayName(), "My Group");
        EXPECT_EQ(materialType.FindPropertyGroup("myGroup")->GetDescription(), "This group is defined in a separate JSON file");
        EXPECT_EQ(materialType.FindPropertyGroup("myGroup")->GetProperties().size(), 2);
        EXPECT_NE(materialType.FindProperty("myGroup.foo"), nullptr);
        EXPECT_NE(materialType.FindProperty("myGroup.bar"), nullptr);
        EXPECT_EQ(materialType.FindProperty("myGroup.foo")->GetName(), "foo");
        EXPECT_EQ(materialType.FindProperty("myGroup.bar")->GetName(), "bar");
        EXPECT_EQ(materialType.FindProperty("myGroup.foo")->m_dataType, MaterialPropertyDataType::Bool);
        EXPECT_EQ(materialType.FindProperty("myGroup.bar")->m_dataType, MaterialPropertyDataType::Float);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_NestedGroupNameContext)
    {
        const Name materialSrgId{"MaterialSrg"};
        RHI::Ptr<RHI::ShaderResourceGroupLayout> materialSrgLayout = RHI::ShaderResourceGroupLayout::Create();
        materialSrgLayout->SetName(materialSrgId);
        materialSrgLayout->SetBindingSlot(SrgBindingSlot::Material);
        materialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_unused1" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
        materialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_unused2" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
        materialSrgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{ Name{ "m_groupA_m_groupB_m_texture" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1 });
        materialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_unused3" }, 0, 4, 0 });
        materialSrgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_groupA_m_groupB_m_number" }, 4, 4, 0 });
        EXPECT_TRUE(materialSrgLayout->Finalize());

        Ptr<ShaderOptionGroupLayout> shaderOptions = ShaderOptionGroupLayout::Create();
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_unused"}, ShaderOptionType::Boolean, 0, 0, CreateBoolShaderOptionValues()});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_groupA_o_groupB_o_useTexture"}, ShaderOptionType::Boolean, 1, 1, CreateBoolShaderOptionValues()});
        shaderOptions->AddShaderOption(ShaderOptionDescriptor{Name{"o_groupA_o_groupB_o_useTextureAlt"}, ShaderOptionType::Boolean, 2, 2, CreateBoolShaderOptionValues()});
        shaderOptions->Finalize();

        Data::Asset<ShaderAsset> shaderAsset = CreateTestShaderAsset(Uuid::CreateRandom(), materialSrgLayout, shaderOptions);

        Data::AssetInfo testShaderAssetInfo;
        testShaderAssetInfo.m_assetId = shaderAsset.GetId();
        m_assetSystemStub.RegisterSourceInfo("NestedGroupNameContext.shader", testShaderAssetInfo, "");

        const AZStd::string materialTypeJson = R"(
            {
                "propertyLayout": {
                    "propertyGroups": [
                        {
                            "name": "groupA",
                            "shaderInputsPrefix": "m_groupA_",
                            "shaderOptionsPrefix": "o_groupA_",
                            "propertyGroups": [
                                {
                                    "name": "groupB",
                                    "shaderInputsPrefix": "m_groupB_",
                                    "shaderOptionsPrefix": "o_groupB_",
                                    "properties": [
                                        {
                                            "name": "number",
                                            "type": "Float",
                                            "connection": {
                                                "type": "ShaderInput",
                                                "name": "m_number"
                                            }
                                        }
                                    ],
                                    "propertyGroups": [
                                        {
                                            "name": "groupC",
                                            "properties": [
                                                {
                                                    "name": "textureMap",
                                                    "type": "Image",
                                                    "connection": {
                                                        "type": "ShaderInput",
                                                        "name": "m_texture"
                                                    }
                                                },
                                                {
                                                    "name": "useTextureMap",
                                                    "type": "Bool",
                                                    "connection": [
                                                        {
                                                            "type": "ShaderOption",
                                                            "name": "o_useTexture"
                                                        },
                                                        {
                                                            "type": "ShaderOption",
                                                            "name": "o_useTextureAlt", 
                                                            "shaderIndex": 0 // Having a specific shaderIndex traverses a different code path
                                                        }
                                                    ]
                                                }
                                            ],
                                            "functors": [
                                                {
                                                    "type": "SaveNameContext"
                                                }
                                            ]
                                        }
                                    ]
                                }
                            ]
                        }
                    ]
                },
                "shaders": [
                    {
                        "file": "NestedGroupNameContext.shader"
                    }
                ]
            }
        )";
        
        MaterialTypeSourceData materialTypeSourceData;
        JsonTestResult loadResult = LoadTestDataFromJson(materialTypeSourceData, materialTypeJson);
        
        auto materialTypeAssetOutcome = materialTypeSourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeAssetOutcome.IsSuccess());

        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeAssetOutcome.TakeValue();
        const MaterialPropertiesLayout* propertiesLayout = materialTypeAsset->GetMaterialPropertiesLayout();

        EXPECT_EQ(3, propertiesLayout->GetPropertyCount());
        
        EXPECT_EQ(0, propertiesLayout->FindPropertyIndex(Name("groupA.groupB.number")).GetIndex());
        EXPECT_EQ(1, propertiesLayout->FindPropertyIndex(Name("groupA.groupB.groupC.textureMap")).GetIndex());
        EXPECT_EQ(2, propertiesLayout->FindPropertyIndex(Name("groupA.groupB.groupC.useTextureMap")).GetIndex());
        
        // groupA.groupB.number has a connection to m_groupA_m_groupB_m_number
        MaterialPropertyIndex numberPropertyIndex{0};
        EXPECT_EQ(1, propertiesLayout->GetPropertyDescriptor(numberPropertyIndex)->GetOutputConnections().size());
        EXPECT_EQ(materialSrgLayout->FindShaderInputConstantIndex(Name("m_groupA_m_groupB_m_number")).GetIndex(),
            propertiesLayout->GetPropertyDescriptor(numberPropertyIndex)->GetOutputConnections()[0].m_itemIndex.GetIndex());

        // groupA.gropuB.groupC.textureMap has a connection to m_groupA_m_groupB_m_texture
        MaterialPropertyIndex texturePropertyIndex{1};
        EXPECT_EQ(1, propertiesLayout->GetPropertyDescriptor(texturePropertyIndex)->GetOutputConnections().size());
        EXPECT_EQ(materialSrgLayout->FindShaderInputImageIndex(Name("m_groupA_m_groupB_m_texture")).GetIndex(),
            propertiesLayout->GetPropertyDescriptor(texturePropertyIndex)->GetOutputConnections()[0].m_itemIndex.GetIndex());
        
        // groupA.gropuB.groupC.useTextureMap has a connection to o_groupA_o_groupB_o_useTexture and o_groupA_o_groupB_o_useTextureAlt
        MaterialPropertyIndex useTexturePropertyIndex{2};
        EXPECT_EQ(2, propertiesLayout->GetPropertyDescriptor(useTexturePropertyIndex)->GetOutputConnections().size());
        EXPECT_EQ(shaderOptions->FindShaderOptionIndex(Name("o_groupA_o_groupB_o_useTexture")).GetIndex(),
            propertiesLayout->GetPropertyDescriptor(useTexturePropertyIndex)->GetOutputConnections()[0].m_itemIndex.GetIndex());
        EXPECT_EQ(shaderOptions->FindShaderOptionIndex(Name("o_groupA_o_groupB_o_useTextureAlt")).GetIndex(),
            propertiesLayout->GetPropertyDescriptor(useTexturePropertyIndex)->GetOutputConnections()[1].m_itemIndex.GetIndex());
        
        // There should be one functor, which processes useTextureMap, and it should have the appropriate name context for constructing the correct full names.
        EXPECT_EQ(1, materialTypeAsset->GetMaterialFunctors().size());

        EXPECT_TRUE(azrtti_istypeof<SaveNameContextTestFunctor>(materialTypeAsset->GetMaterialFunctors()[0].get()));

        auto saveNameContextFunctor = azrtti_cast<SaveNameContextTestFunctor*>(materialTypeAsset->GetMaterialFunctors()[0].get());
        const MaterialNameContext& nameContext = saveNameContextFunctor->m_nameContext;
        
        Name textureMapProperty{"textureMap"};
        Name textureMapShaderInput{"m_texture"};
        Name useTextureMapProperty{"useTextureMap"};
        Name useTextureShaderOption{"o_useTexture"};
        
        nameContext.ContextualizeProperty(textureMapProperty);
        nameContext.ContextualizeProperty(useTextureMapProperty);
        nameContext.ContextualizeSrgInput(textureMapShaderInput);
        nameContext.ContextualizeShaderOption(useTextureShaderOption);

        EXPECT_EQ("groupA.groupB.groupC.useTextureMap", useTextureMapProperty.GetStringView());
        EXPECT_EQ("o_groupA_o_groupB_o_useTexture", useTextureShaderOption.GetStringView());
        EXPECT_EQ("groupA.groupB.groupC.textureMap", textureMapProperty.GetStringView());
        EXPECT_EQ("m_groupA_m_groupB_m_texture", textureMapShaderInput.GetStringView());
    }

}
