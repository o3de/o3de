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
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Material/Material.h>

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

        RHI::Ptr<RHI::ShaderResourceGroupLayout> m_testMaterialSrgLayout;
        Data::Asset<ShaderAsset> m_testShaderAsset;
        Data::Asset<ShaderAsset> m_testShaderAsset2;
        Data::Asset<ImageAsset> m_testImageAsset;
        Data::Asset<ImageAsset> m_testImageAsset2; // For relative path testing.
        static constexpr const char* TestShaderFilename             = "test.shader";
        static constexpr const char* TestShaderFilename2            = "extra.shader";
        static constexpr const char* TestImageFilename              = "test.streamingimage";

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
        }

        void SetUp() override
        {
            RPITestFixture::SetUp();

            m_functorRegistration.Init();

            AZ::RPI::MaterialFunctorSourceDataRegistration::Get()->RegisterMaterialFunctor("Splat3", azrtti_typeid<Splat3FunctorSourceData>());
            AZ::RPI::MaterialFunctorSourceDataRegistration::Get()->RegisterMaterialFunctor("EnableShader", azrtti_typeid<EnableShaderFunctorSourceData>());
            AZ::RPI::MaterialFunctorSourceDataRegistration::Get()->RegisterMaterialFunctor("SetShaderOption", azrtti_typeid<SetShaderOptionFunctorSourceData>());

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
            EXPECT_TRUE(m_testMaterialSrgLayout->Finalize());

            AZStd::vector<RPI::ShaderOptionValuePair> optionValues;
            optionValues.push_back({Name("Low"),  RPI::ShaderOptionValue(0)});
            optionValues.push_back({Name("Med"), RPI::ShaderOptionValue(1)});
            optionValues.push_back({Name("High"), RPI::ShaderOptionValue(2)});

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

            Data::AssetInfo testShaderAssetInfo;
            testShaderAssetInfo.m_assetId = m_testShaderAsset.GetId();

            Data::AssetInfo testShaderAssetInfo2;
            testShaderAssetInfo2.m_assetId = m_testShaderAsset2.GetId();

            Data::AssetInfo testImageAssetInfo;
            testImageAssetInfo.m_assetId = m_testImageAsset.GetId();

            Data::AssetInfo testImageAssetInfo2;
            testImageAssetInfo2.m_assetId = m_testImageAsset2.GetId();
            testImageAssetInfo2.m_assetType = azrtti_typeid<StreamingImageAsset>();

            m_assetSystemStub.RegisterSourceInfo(TestShaderFilename, testShaderAssetInfo, "");
            m_assetSystemStub.RegisterSourceInfo(TestShaderFilename2, testShaderAssetInfo2, "");
            m_assetSystemStub.RegisterSourceInfo(TestImageFilename, testImageAssetInfo, "");
            // We need to normalize the path because AssetSystemStub uses it as a key to look up.
            AZStd::string testImageFilepathAbsolute(TestImageFilepathAbsolute);
            AzFramework::StringFunc::Path::Normalize(testImageFilepathAbsolute);
            m_assetSystemStub.RegisterSourceInfo(testImageFilepathAbsolute.c_str(), testImageAssetInfo2, "");
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

        AZStd::vector<RPI::ShaderOptionValuePair> optionValues;
        optionValues.push_back({Name("Low"),  RPI::ShaderOptionValue(0)});
        optionValues.push_back({Name("Med"), RPI::ShaderOptionValue(1)});
        optionValues.push_back({Name("High"), RPI::ShaderOptionValue(2)});

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
        sourceData.m_shaderCollection.back().m_shaderOptionValues[Name{"o_foo"}] = Name{"Low"};

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
        EXPECT_EQ(shaderCOptions.GetValue(fooOption).GetIndex(), 0);
        EXPECT_EQ(shaderCOptions.GetValue(barOption).GetIndex(), -1);
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

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_ShaderListWithInvalidShaderOptionValue)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});
        sourceData.m_shaderCollection.back().m_shaderOptionValues[Name{"o_quality"}] = Name{"DoesNotExist"};

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_BoolPropertyConnectedToShaderConstant)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ TestShaderFilename });

        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_name = "MyBool";
        propertySource.m_displayName = "My Bool";
        propertySource.m_description = "This is a bool";
        propertySource.m_dataType = MaterialPropertyDataType::Bool;
        propertySource.m_value = true;
        propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string("m_bool") });
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{ "general.MyBool" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

        ValidateCommonDescriptorFields(propertySource, propertyDescriptor);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex(), 7);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_FloatPropertyConnectedToShaderConstant)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ TestShaderFilename });

        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_name = "MyFloat";
        propertySource.m_displayName = "My Float";
        propertySource.m_description = "This is a float";
        propertySource.m_min = 0.0f;
        propertySource.m_max = 1.0f;
        propertySource.m_softMin = 0.2f;
        propertySource.m_softMax = 1.0f;
        propertySource.m_step = 0.01f;
        propertySource.m_dataType = MaterialPropertyDataType::Float;
        propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string("m_float") });
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"general.MyFloat" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

        ValidateCommonDescriptorFields(propertySource, propertyDescriptor);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex(), 1);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_ImagePropertyConnectedToShaderInput)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ TestShaderFilename });

        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_name = "MyImage";
        propertySource.m_displayName = "My Image";
        propertySource.m_description = "This is an image";
        propertySource.m_dataType = MaterialPropertyDataType::Image;
        propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string("m_image") });
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"general.MyImage" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

        ValidateCommonDescriptorFields(propertySource, propertyDescriptor);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex(), 0);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_IntPropertyConnectedToShaderOption)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});

        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_name = "MyInt";
        propertySource.m_displayName = "My Integer";
        propertySource.m_dataType = MaterialPropertyDataType::Int;
        propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{MaterialPropertyOutputType::ShaderOption, AZStd::string("o_foo"), 0});
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(MaterialPropertyIndex{0});

        ValidateCommonDescriptorFields(propertySource, propertyDescriptor);
        EXPECT_EQ(propertyDescriptor->GetOutputConnections()[0].m_itemIndex.GetIndex(), 1);
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_PropertyConnectedToInvalidShaderOptionId)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});

        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_name = "MyInt";
        propertySource.m_dataType = MaterialPropertyDataType::Int;
        propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{MaterialPropertyOutputType::ShaderOption, AZStd::string("DoesNotExist"), 0});
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

        AZ_TEST_START_TRACE_SUPPRESSION;
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        AZ_TEST_STOP_TRACE_SUPPRESSION(2); // There happens to be an extra assert for "Cannot continue building MaterialAsset because 1 error(s) reported"

        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_InvalidGroupNameId)
    {
        MaterialTypeSourceData sourceData;
        
        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_dataType = MaterialPropertyDataType::Int;

        propertySource.m_name = "a";
        sourceData.m_propertyLayout.m_properties["not a valid name because it has spaces"].push_back(propertySource);

        // Expected errors:
        // Group name 'not a valid name because it has spaces' is not a valid identifier.
        // Warning: Cannot create material property with invalid ID 'not a valid name because it has spaces'.
        // Failed to build MaterialAsset because 1 warning(s) reported
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_InvalidPropertyNameId)
    {
        MaterialTypeSourceData sourceData;

        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_dataType = MaterialPropertyDataType::Int;

        propertySource.m_name = "not a valid name because it has spaces";
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

        // Expected errors:
        // Property name 'not a valid name because it has spaces' is not a valid identifier.
        // Warning: Cannot create material property with invalid ID 'not a valid name because it has spaces'.
        // Failed to build MaterialAsset because 1 warning(s) reported
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_DuplicatePropertyId)
    {
        MaterialTypeSourceData sourceData;

        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_dataType = MaterialPropertyDataType::Int;
        propertySource.m_name = "a";
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

        // Expected errors:
        // Material property 'general.a': A property with this ID already exists.
        // Cannot continue building MaterialAsset because 1 error(s) reported
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        EXPECT_FALSE(materialTypeOutcome.IsSuccess());
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_PropertyConnectedToMultipleOutputs)
    {
        // Setup the shader...

        AZStd::vector<RPI::ShaderOptionValuePair> optionValues;
        optionValues.push_back({ Name("Low"),  RPI::ShaderOptionValue(0) });
        optionValues.push_back({ Name("Med"), RPI::ShaderOptionValue(1) });
        optionValues.push_back({ Name("High"), RPI::ShaderOptionValue(2) });

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

        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_name = "MyInt";
        propertySource.m_displayName = "Integer";
        propertySource.m_description = "Integer property that is connected to multiple shader settings";
        propertySource.m_dataType = MaterialPropertyDataType::Int;

        // The value maps to m_int in the SRG
        propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string("m_int") });
        // The value also maps to m_uint in the SRG
        propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string("m_uint") });
        // The value also maps to the first shader's "o_speed" option
        propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderOption, AZStd::string("o_speed"), 0 });
        // The value also maps to the second shader's "o_speed" option
        propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderOption, AZStd::string("o_speed"), 1 });
        // This case doesn't specify an index, so it will apply to all shaders that have a "o_efficiency", which means it will create two outputs in the property descriptor.
        propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderOption, AZStd::string("o_efficiency") });

        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

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

        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_name = "NonAliasFloat";
        propertySource.m_displayName = "Non-Alias Float";
        propertySource.m_description = "This float is processed by a functor, not with a direct alias";
        propertySource.m_dataType = MaterialPropertyDataType::Float;
        // Note that we don't fill propertySource.m_aliasOutputId because this is not an aliased property
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{TestShaderFilename});

        sourceData.m_materialFunctorSourceData.push_back(
            Ptr<MaterialFunctorSourceDataHolder>
            (
                aznew MaterialFunctorSourceDataHolder
                (
                    aznew Splat3FunctorSourceData{ "general.NonAliasFloat", "m_float3" }
                )
            )
        );

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_TRUE(materialTypeOutcome.IsSuccess());
        Data::Asset<MaterialTypeAsset> materialTypeAsset = materialTypeOutcome.GetValue();

        const MaterialPropertyIndex propertyIndex = materialTypeAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(Name{"general.NonAliasFloat" });
        const MaterialPropertyDescriptor* propertyDescriptor = materialTypeAsset->GetMaterialPropertiesLayout()->GetPropertyDescriptor(propertyIndex);

        ValidateCommonDescriptorFields(propertySource, propertyDescriptor);

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
        
        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_name = "EnableSpecialPassA";
        propertySource.m_displayName = "Enable Special Pass";
        propertySource.m_description = "This is a bool to enable an extra shader/pass";
        propertySource.m_dataType = MaterialPropertyDataType::Bool;
        // Note that we don't fill propertySource.m_outputConnections because this is not a direct-connected property
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);
        propertySource.m_name = "EnableSpecialPassB";
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

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

        ValidateCommonDescriptorFields(propertySource, propertyADescriptor);
        ValidateCommonDescriptorFields(propertySource, propertyBDescriptor);

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

        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_name = "MyProperty";
        propertySource.m_dataType = MaterialPropertyDataType::Bool;
        // Note that we don't fill propertySource.m_outputConnections because this is not a direct-connected property
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

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

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_PropertyValues_AllTypes)
    {
        MaterialTypeSourceData sourceData;

        sourceData.m_shaderCollection.push_back(MaterialTypeSourceData::ShaderVariantReferenceData{ TestShaderFilename });

        auto addProperty = [&sourceData](MaterialPropertyDataType dateType, const char* propertyName, const char* srgConstantName, const AZ::RPI::MaterialPropertyValue& value)
        {
            MaterialTypeSourceData::PropertyDefinition propertySource;
            propertySource.m_name = propertyName;
            propertySource.m_dataType = dateType;
            propertySource.m_outputConnections.push_back(MaterialTypeSourceData::PropertyConnection{ MaterialPropertyOutputType::ShaderInput, AZStd::string(srgConstantName) });
            propertySource.m_value = value;
            sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);
        };

        addProperty(MaterialPropertyDataType::Bool,    "MyBool",   "m_bool",    true);
        addProperty(MaterialPropertyDataType::Float,   "MyFloat",  "m_float",   1.2f);
        addProperty(MaterialPropertyDataType::Int,     "MyInt",    "m_int",     -12);
        addProperty(MaterialPropertyDataType::UInt,    "MyUInt",   "m_uint",    12u);
        addProperty(MaterialPropertyDataType::Vector2, "MyFloat2", "m_float2",  AZ::Vector2{1.1f, 2.2f});
        addProperty(MaterialPropertyDataType::Vector3, "MyFloat3", "m_float3",  AZ::Vector3{3.3f, 4.4f, 5.5f});
        addProperty(MaterialPropertyDataType::Vector4, "MyFloat4", "m_float4",  AZ::Vector4{6.6f, 7.7f, 8.8f, 9.9f});
        addProperty(MaterialPropertyDataType::Color,   "MyColor",  "m_color",   AZ::Color{0.1f, 0.2f, 0.3f, 0.4f});
        addProperty(MaterialPropertyDataType::Image,   "MyImage",  "m_image",   AZStd::string{TestImageFilename});

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
    }
    
    TEST_F(MaterialTypeSourceDataTests, LoadAndStoreJson_AllFields)
    {
        // Note that serialization of individual fields within material properties is thoroughly tested in
        // MaterialPropertySerializerTests, so the sample property data used here is cursory.

        const AZStd::string inputJson = R"(
            {
                "description": "This is a general description about the material",
                "version": 2,
                "versionUpdates": [
                    {
                        "toVersion": 2,
                        "actions": [
                            { "op": "rename", "from": "groupA.fooPrev", "to": "groupA.foo" }
                        ]
                    }
                ],
                "propertyLayout": {
                    "groups": [
                        {
                            "name": "groupA",
                            "displayName": "Property Group A",
                            "description": "Description of property group A"
                        },
                        {
                            "name": "groupB",
                            "displayName": "Property Group B",
                            "description": "Description of property group B"
                        }
                    ],
                    "properties": {
                        "groupA": [
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
                        "groupB": [
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

        EXPECT_EQ(material.m_description, "This is a general description about the material");

        EXPECT_EQ(material.m_version, 2);
        EXPECT_EQ(material.m_versionUpdates.size(), 1);
        EXPECT_EQ(material.m_versionUpdates[0].m_toVersion, 2);
        EXPECT_EQ(material.m_versionUpdates[0].m_actions[0].m_operation, "rename");
        EXPECT_EQ(material.m_versionUpdates[0].m_actions[0].m_renameFrom, "groupA.fooPrev");
        EXPECT_EQ(material.m_versionUpdates[0].m_actions[0].m_renameTo, "groupA.foo");

        EXPECT_EQ(material.m_propertyLayout.m_groups.size(), 2);
        EXPECT_TRUE(material.FindGroup("groupA") != nullptr);
        EXPECT_TRUE(material.FindGroup("groupB") != nullptr);
        EXPECT_EQ(material.FindGroup("groupA")->m_displayName, "Property Group A");
        EXPECT_EQ(material.FindGroup("groupB")->m_displayName, "Property Group B");
        EXPECT_EQ(material.FindGroup("groupA")->m_description, "Description of property group A");
        EXPECT_EQ(material.FindGroup("groupB")->m_description, "Description of property group B");

        EXPECT_EQ(material.m_propertyLayout.m_properties.size(), 2);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"].size(), 2);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"].size(), 2);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][0].m_name, "foo");
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][1].m_name, "bar");
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][0].m_name, "foo");
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][1].m_name, "bar");
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][0].m_dataType, MaterialPropertyDataType::Bool);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][1].m_dataType, MaterialPropertyDataType::Image);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][0].m_dataType, MaterialPropertyDataType::Float);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][1].m_dataType, MaterialPropertyDataType::Color);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][0].m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][1].m_visibility, MaterialPropertyVisibility::Hidden);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][0].m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][1].m_visibility, MaterialPropertyVisibility::Disabled);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][0].m_value, true);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][1].m_value, AZStd::string{"Default.png"});
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][0].m_value, 0.5f);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][1].m_value, AZ::Color(0.5f, 0.5f, 0.5f, 1.0f));

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
        
        AZStd::string outputJson;
        JsonTestResult storeResult = StoreTestDataToJson(material, outputJson);
        ExpectSimilarJson(inputJson, outputJson);
    }
    
    TEST_F(MaterialTypeSourceDataTests, LoadAllFieldsUsingOldFormat)
    {
        // The content of this test was copied from LoadAndStoreJson_AllFields to prove backward compatibility.
        // (The "store" part of the test was not included because the saved data will be the new format).

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

        EXPECT_EQ(material.m_description, "This is a general description about the material");

        EXPECT_EQ(material.m_propertyLayout.m_groups.size(), 2);
        EXPECT_TRUE(material.FindGroup("groupA") != nullptr);
        EXPECT_TRUE(material.FindGroup("groupB") != nullptr);
        EXPECT_EQ(material.FindGroup("groupA")->m_displayName, "Property Group A");
        EXPECT_EQ(material.FindGroup("groupB")->m_displayName, "Property Group B");
        EXPECT_EQ(material.FindGroup("groupA")->m_description, "Description of property group A");
        EXPECT_EQ(material.FindGroup("groupB")->m_description, "Description of property group B");

        EXPECT_EQ(material.m_propertyLayout.m_properties.size(), 2);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"].size(), 2);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"].size(), 2);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][0].m_name, "foo");
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][1].m_name, "bar");
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][0].m_name, "foo");
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][1].m_name, "bar");
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][0].m_dataType, MaterialPropertyDataType::Bool);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][1].m_dataType, MaterialPropertyDataType::Image);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][0].m_dataType, MaterialPropertyDataType::Float);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][1].m_dataType, MaterialPropertyDataType::Color);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][0].m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][1].m_visibility, MaterialPropertyVisibility::Hidden);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][0].m_visibility, MaterialPropertyVisibility::Enabled);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][1].m_visibility, MaterialPropertyVisibility::Disabled);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][0].m_value, true);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupA"][1].m_value, AZStd::string{"Default.png"});
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][0].m_value, 0.5f);
        EXPECT_EQ(material.m_propertyLayout.m_properties["groupB"][1].m_value, AZ::Color(0.5f, 0.5f, 0.5f, 1.0f));

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
                        "groups": [
                            {
                                "name": "general",
                                "displayName": "General",
                                "description": ""
                            }
                        ],
                        "properties": {
                            "general": [
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
                    }
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


    TEST_F(MaterialTypeSourceDataTests, FindPropertyUsingOldName)
    {
        const AZStd::string inputJson = R"(
            {
                "version": 10,
                "versionUpdates": [
                    {
                        "toVersion": 2,
                        "actions": [
                            { "op": "rename", "from": "general.fooA", "to": "general.fooB" }
                        ]
                    },
                    {
                        "toVersion": 4,
                        "actions": [
                            { "op": "rename", "from": "general.barA", "to": "general.barB" }
                        ]
                    },
                    {
                        "toVersion": 6,
                        "actions": [
                            { "op": "rename", "from": "general.fooB", "to": "general.fooC" },
                            { "op": "rename", "from": "general.barB", "to": "general.barC" }
                        ]
                    },
                    {
                        "toVersion": 7,
                        "actions": [
                            { "op": "rename", "from": "general.bazA", "to": "otherGroup.bazB" },
                            { "op": "rename", "from": "onlyOneProperty.bopA", "to": "otherGroup.bopB" } // This tests a group 'onlyOneProperty' that no longer exists in the material type
                        ]
                    }
                ],
                "propertyLayout": {
                    "properties": {
                        "general": [
                            {
                                "name": "fooC",
                                "type": "Bool"
                            },
                            {
                                "name": "barC",
                                "type": "Float"
                            }
                        ],
                        "otherGroup": [
                            {
                                "name": "dontMindMe",
                                "type": "Bool"
                            },
                            {
                                "name": "bazB",
                                "type": "Float"
                            },
                            {
                                "name": "bopB",
                                "type": "Float"
                            }
                        ]
                    }
                }
            }
        )";

        MaterialTypeSourceData materialType;
        JsonTestResult loadResult = LoadTestDataFromJson(materialType, inputJson);

        EXPECT_EQ(materialType.m_version, 10);

        // First find the properties using their correct current names
        const MaterialTypeSourceData::PropertyDefinition* foo = materialType.FindProperty("general", "fooC");
        const MaterialTypeSourceData::PropertyDefinition* bar = materialType.FindProperty("general", "barC");
        const MaterialTypeSourceData::PropertyDefinition* baz = materialType.FindProperty("otherGroup", "bazB");
        const MaterialTypeSourceData::PropertyDefinition* bop = materialType.FindProperty("otherGroup", "bopB");
        
        EXPECT_TRUE(foo);
        EXPECT_TRUE(bar);
        EXPECT_TRUE(baz);
        EXPECT_TRUE(bop);
        EXPECT_EQ(foo->m_name, "fooC");
        EXPECT_EQ(bar->m_name, "barC");
        EXPECT_EQ(baz->m_name, "bazB");
        EXPECT_EQ(bop->m_name, "bopB");

        // Now try doing the property lookup using old versions of the name and make sure the same property can be found

        EXPECT_EQ(foo, materialType.FindProperty("general", "fooA"));
        EXPECT_EQ(foo, materialType.FindProperty("general", "fooB"));
        EXPECT_EQ(bar, materialType.FindProperty("general", "barA"));
        EXPECT_EQ(bar, materialType.FindProperty("general", "barB"));
        EXPECT_EQ(baz, materialType.FindProperty("general", "bazA"));
        EXPECT_EQ(bop, materialType.FindProperty("onlyOneProperty", "bopA"));
        
        EXPECT_EQ(nullptr, materialType.FindProperty("general", "fooX"));
        EXPECT_EQ(nullptr, materialType.FindProperty("general", "barX"));
        EXPECT_EQ(nullptr, materialType.FindProperty("general", "bazX"));
        EXPECT_EQ(nullptr, materialType.FindProperty("general", "bazB"));
        EXPECT_EQ(nullptr, materialType.FindProperty("otherGroup", "bazA"));
        EXPECT_EQ(nullptr, materialType.FindProperty("onlyOneProperty", "bopB"));
        EXPECT_EQ(nullptr, materialType.FindProperty("otherGroup", "bopA"));
    }
    
    TEST_F(MaterialTypeSourceDataTests, FindPropertyUsingOldName_Error_UnsupportedVersionUpdate)
    {
        const AZStd::string inputJson = R"(
            {
                "version": 10,
                "versionUpdates": [
                    {
                        "toVersion": 2,
                        "actions": [
                            { "op": "notRename", "from": "general.fooA", "to": "general.fooB" }
                        ]
                    }
                ],
                "propertyLayout": {
                    "properties": {
                        "general": [
                            {
                                "name": "fooB",
                                "type": "Bool"
                            }
                        ]
                    }
                }
            }
        )";

        MaterialTypeSourceData materialType;
        JsonTestResult loadResult = LoadTestDataFromJson(materialType, inputJson);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Unsupported material version update operation 'notRename'");


        const MaterialTypeSourceData::PropertyDefinition* foo = materialType.FindProperty("general", "fooA");

        EXPECT_EQ(nullptr, foo);

        errorMessageFinder.CheckExpectedErrorsFound();
    }

    TEST_F(MaterialTypeSourceDataTests, CreateMaterialTypeAsset_Error_UnsupportedVersionUpdate)
    {
        MaterialTypeSourceData sourceData;
        
        MaterialTypeSourceData::PropertyDefinition propertySource;
        propertySource.m_name = "a";
        propertySource.m_dataType = MaterialPropertyDataType::Int;
        propertySource.m_value = 0;
        sourceData.m_propertyLayout.m_properties["general"].push_back(propertySource);

        sourceData.m_version = 2;

        MaterialTypeSourceData::VersionUpdateDefinition versionUpdate;
        versionUpdate.m_toVersion = 2;
        MaterialTypeSourceData::VersionUpdatesRenameOperationDefinition updateAction;
        updateAction.m_operation = "operationNotKnown";
        versionUpdate.m_actions.push_back(updateAction);
        sourceData.m_versionUpdates.push_back(versionUpdate);

        ErrorMessageFinder errorMessageFinder;
        errorMessageFinder.AddExpectedErrorMessage("Unsupported material version update operation 'operationNotKnown'");
        errorMessageFinder.AddIgnoredErrorMessage("Failed to build MaterialTypeAsset", true);

        auto materialTypeOutcome = sourceData.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_FALSE(materialTypeOutcome.IsSuccess());

        errorMessageFinder.CheckExpectedErrorsFound();
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
                }
            }
        )";

        MaterialTypeSourceData materialType;
        JsonTestResult loadResult = LoadTestDataFromJson(materialType, inputJson);

        auto materialTypeOutcome = materialType.CreateMaterialTypeAsset(Uuid::CreateRandom());
        EXPECT_FALSE(materialTypeOutcome.IsSuccess());

        errorMessageFinder.CheckExpectedErrorsFound();
    }
}
