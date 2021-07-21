/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/MaterialAssetTestUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAssetCreator.h>
#include <Atom/RPI.Reflect/Shader/ShaderAssetCreator.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantAssetCreator.h>
#include <Atom/RHI/Factory.h>
#include <Common/RHI/Stubs.h>

namespace UnitTest
{
    void AddMaterialPropertyForSrg(
        AZ::RPI::MaterialTypeAssetCreator& materialTypeCreator,
        const AZ::Name& propertyName,
        AZ::RPI::MaterialPropertyDataType dataType,
        const AZ::Name& shaderInputName)
    {
        using namespace AZ::RPI;
        materialTypeCreator.BeginMaterialProperty(propertyName, dataType);
        materialTypeCreator.ConnectMaterialPropertyToShaderInput(shaderInputName);
        if (dataType == MaterialPropertyDataType::Enum)
        {
            materialTypeCreator.SetMaterialPropertyEnumNames(AZStd::vector<AZStd::string>({ "Enum0", "Enum1", "Enum2" }));
        }
        materialTypeCreator.EndMaterialProperty();
    }

    void AddCommonTestMaterialProperties(AZ::RPI::MaterialTypeAssetCreator& materialTypeCreator, AZStd::string propertyGroupPrefix)
    {
        using namespace AZ;
        using namespace RPI;
        using namespace RHI;

        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyBool" }, MaterialPropertyDataType::Bool, Name{ "m_bool" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyInt" }, MaterialPropertyDataType::Int, Name{ "m_int" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyUInt" }, MaterialPropertyDataType::UInt, Name{ "m_uint" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyFloat" }, MaterialPropertyDataType::Float, Name{ "m_float" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyFloat2" }, MaterialPropertyDataType::Vector2, Name{ "m_float2" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyFloat3" }, MaterialPropertyDataType::Vector3, Name{ "m_float3" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyFloat4" }, MaterialPropertyDataType::Vector4, Name{ "m_float4" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyColor" }, MaterialPropertyDataType::Color, Name{ "m_color" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyImage" }, MaterialPropertyDataType::Image, Name{ "m_image" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyEnum" }, MaterialPropertyDataType::Enum, Name{ "m_enum" });
    }

    AZ::Data::Asset<AZ::RPI::ShaderResourceGroupAsset> CreateCommonTestMaterialSrgAsset()
    {
        using namespace AZ;
        using namespace RPI;
        using namespace RHI;

        // Note we specify the shader inputs and material properties in a different order so the indexes don't align
        // We also include a couple unused inputs to further make sure shader and material indexes don't align

        ShaderResourceGroupAssetCreator srgCreator;
        srgCreator.Begin(Uuid::CreateRandom(), Name("MaterialSrg"));
        srgCreator.BeginAPI(RHI::Factory::Get().GetType());
        srgCreator.SetBindingSlot(SrgBindingSlot::Material);
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_unused" }, 0, 4, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_color" }, 4, 16, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float" }, 20, 4, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_int" }, 24, 4, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_uint" }, 28, 4, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float2" }, 32, 8, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float3" }, 40, 12, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float4" }, 52, 16, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_enum" }, 68, 4, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_bool" }, 72, 4, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float3x3" }, 76, 44, 0}); // See ConstantsData::SetConstant<Matrix3x3> for packing rules
        srgCreator.AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float4x4" }, 120, 64, 0});
        srgCreator.AddShaderInput(RHI::ShaderInputImageDescriptor{Name{ "m_unusedImage" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1});
        srgCreator.AddShaderInput(RHI::ShaderInputImageDescriptor{Name{ "m_image" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 2});
        srgCreator.EndAPI();

        AZ::Data::Asset<AZ::RPI::ShaderResourceGroupAsset> srgAsset;
        EXPECT_TRUE(srgCreator.End(srgAsset));

        return srgAsset;
    }

    template<class T>
    void AddShaderInputToBindingInfo(AZ::RHI::ShaderResourceGroupBindingInfo& bindingInfo, const T& shaderInputs)
    {
        for (const auto& shaderInput : shaderInputs)
        {
            bindingInfo.m_resourcesRegisterMap.insert({shaderInput.m_name.GetStringView(), AZ::RHI::ResourceBindingInfo{AZ::RHI::ShaderStageMask::Vertex, shaderInput.m_registerId}});
        }
    }

    AZ::RHI::ShaderResourceGroupBindingInfo CreateShaderResourceGroupBindingInfo(const AZ::RHI::ShaderResourceGroupLayout* layout)
    {
        using namespace AZ;

        RHI::ShaderResourceGroupBindingInfo bindingInfo;
        if (!layout)
        {
            return bindingInfo;
        }

        if (layout->GetConstantDataSize())
        {
            // All constant in the SRG use the same the register id.
            bindingInfo.m_constantDataBindingInfo.m_registerId = layout->GetShaderInputListForConstants()[0].m_registerId;
        }

        AddShaderInputToBindingInfo(bindingInfo, layout->GetShaderInputListForBuffers());
        AddShaderInputToBindingInfo(bindingInfo, layout->GetShaderInputListForSamplers());
        AddShaderInputToBindingInfo(bindingInfo, layout->GetShaderInputListForImages());
        AddShaderInputToBindingInfo(bindingInfo, layout->GetStaticSamplers());

        return bindingInfo;
    }

    AZ::Data::Asset<AZ::RPI::ShaderAsset> CreateTestShaderAsset(
        const AZ::Data::AssetId& shaderAssetId,
        AZ::Data::Asset<AZ::RPI::ShaderResourceGroupAsset> optionalMaterialSrg,
        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> optionalShaderOptions)
    {
        using namespace AZ;
        using namespace RPI;
        using namespace RHI;

        Data::Asset<ShaderAsset> shaderAsset;

        RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor = RHI::PipelineLayoutDescriptor::Create();
        if (optionalMaterialSrg.IsReady())
        {
            const RHI::ShaderResourceGroupLayout* layout = optionalMaterialSrg->GetLayout();
            RHI::ShaderResourceGroupBindingInfo bindingInfo = CreateShaderResourceGroupBindingInfo(layout);
            pipelineLayoutDescriptor->AddShaderResourceGroupLayoutInfo(*layout, bindingInfo);
        }
        pipelineLayoutDescriptor->Finalize();

        if (!optionalShaderOptions)
        {
            optionalShaderOptions = RPI::ShaderOptionGroupLayout::Create();
            optionalShaderOptions->Finalize();
        }

        ShaderAssetCreator shaderAssetCreator;
        shaderAssetCreator.Begin(shaderAssetId);

        shaderAssetCreator.SetDrawListName(Name{"depth"});
        if (optionalMaterialSrg.IsReady())
        {
            shaderAssetCreator.AddShaderResourceGroupAsset(optionalMaterialSrg);
        }
        shaderAssetCreator.SetShaderOptionGroupLayout(optionalShaderOptions);

        shaderAssetCreator.BeginAPI(RHI::Factory::Get().GetType());

        RHI::ShaderStageAttributeMapList attributeMaps;
        attributeMaps.resize(RHI::ShaderStageCount);
        shaderAssetCreator.SetShaderStageAttributeMapList(attributeMaps);

        ShaderVariantAssetCreator shaderVariantAssetCreator;
        shaderVariantAssetCreator.Begin(Uuid::CreateRandom(), RPI::ShaderVariantId(), RootShaderVariantStableId, optionalShaderOptions.get());

        RHI::Ptr<RHI::ShaderStageFunction> shaderStageFunction = aznew StubRHI::ShaderStageFunction;
        shaderVariantAssetCreator.SetShaderFunction(RHI::ShaderStage::Vertex, shaderStageFunction);

        Data::Asset<ShaderVariantAsset> shaderVariantAsset;
        EXPECT_TRUE(shaderVariantAssetCreator.End(shaderVariantAsset));

        shaderAssetCreator.SetRootShaderVariantAsset(shaderVariantAsset);
        shaderAssetCreator.SetPipelineLayout(pipelineLayoutDescriptor);

        EXPECT_TRUE(shaderAssetCreator.EndAPI());
        EXPECT_TRUE(shaderAssetCreator.End(shaderAsset));

        return shaderAsset;
    }

}
