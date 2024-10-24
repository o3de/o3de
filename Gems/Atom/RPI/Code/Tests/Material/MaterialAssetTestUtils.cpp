/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/MaterialAssetTestUtils.h>
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
        materialTypeCreator.ConnectMaterialPropertyToShaderParameter(shaderInputName);
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

        // clang-format off
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
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyAttachmentImage" }, MaterialPropertyDataType::Image, Name{ "m_attachmentImage" });
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MySamplerState" }, MaterialPropertyDataType::SamplerState, Name{ "m_samplerIndex" });
        // clang-format on
    }

    AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> CreateCommonTestMaterialSrgLayout()
    {
        using namespace AZ;
        using namespace RPI;
        using namespace RHI;

        // Note we specify the shader inputs and material properties in a different order so the indexes don't align
        // We also include a couple unused inputs to further make sure shader and material indexes don't align
        AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> srgLayout = RHI::ShaderResourceGroupLayout::Create();
        srgLayout->SetName(Name("MaterialSrg"));
        srgLayout->SetUniqueId(Uuid::CreateRandom().ToString<AZStd::string>()); // Any random string will suffice.
        srgLayout->SetBindingSlot(SrgBindingSlot::Material);

        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_unused" }, 0, 4, 0, 0 });
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_float" }, 4, 4, 0, 0 });
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_int" }, 8, 4, 0, 0 });
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_uint" }, 12, 4, 0, 0 });

        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_color" }, 16, 16, 0, 0 });
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float2" }, 32, 8, 0, 0});
        // padding, 8 byte

        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_float3" }, 48, 12, 0, 0 });
        // padding, 4 byte

        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_float4" }, 64, 16, 0, 0 });

        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_enum" }, 80, 4, 0, 0 });
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_bool" }, 84, 4, 0, 0 });
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_samplerIndex" }, 88, 4, 0, 0 });
        // padding, 4 byte

        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{
            Name{ "m_float3x3" }, 96, 44, 0, 0 }); // See ConstantsData::SetConstant<Matrix3x3> for packing rules
        // padding, 4 byte

        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{ Name{ "m_float4x4" }, 144, 64, 0, 0 });

        srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{Name{ "m_unusedImage" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1, 1});
        srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{Name{ "m_image" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 2, 2});
        srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{Name{ "m_attachmentImage" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 2, 2});

        EXPECT_TRUE(srgLayout->Finalize());

        return srgLayout;
    }

    AZ::RPI::MaterialShaderParameterLayout CreateCommonTestMaterialShaderParameterLayout()
    {
        using namespace AZ;
        using namespace RPI;

        // Note we specify the shader parameters and material properties in a different order so the indexes don't align
        // We also include a couple unused inputs to further make sure shader and material indexes don't align

        MaterialShaderParameterLayout shaderParamsLayout{};

        // the first entry is always the material type
        shaderParamsLayout.AddMaterialParameter<uint32_t>("m_materialType", true);
        shaderParamsLayout.AddMaterialParameter<uint32_t>("m_materialInstance", true);

        shaderParamsLayout.AddMaterialParameter<uint32_t>("m_unused");
        shaderParamsLayout.AddMaterialParameter<Color>("m_color");
        shaderParamsLayout.AddMaterialParameter<float>("m_float");
        shaderParamsLayout.AddMaterialParameter<RHI::SamplerState>("m_samplerIndex");
        shaderParamsLayout.AddMaterialParameter<Vector2>("m_float2");
        shaderParamsLayout.AddMaterialParameter<Vector3>("m_float3");
        shaderParamsLayout.AddMaterialParameter<Vector4>("m_float4");
        shaderParamsLayout.AddMaterialParameter<Data::Asset<Image>>("m_unusedImage");
        shaderParamsLayout.AddMaterialParameter<Data::Asset<Image>>("m_image");
        shaderParamsLayout.AddMaterialParameter<Data::Asset<Image>>("m_attachmentImage");
        shaderParamsLayout.AddMaterialParameter<int>("m_int");
        shaderParamsLayout.AddMaterialParameter<uint32_t>("m_uint");
        shaderParamsLayout.AddMaterialParameter<uint32_t>("m_enum");
        shaderParamsLayout.AddMaterialParameter<bool>("m_bool");
        shaderParamsLayout.AddMaterialParameter<Matrix3x3>("m_float3x3");
        shaderParamsLayout.AddMaterialParameter<Matrix4x4>("m_float4x4");

        shaderParamsLayout.FinalizeLayout();

        return shaderParamsLayout;
    }

    AZ::RHI::SamplerState GetDefaultSamplerState()
    {
        return AZ::RHI::SamplerState::Create(AZ::RHI::FilterMode::Linear, AZ::RHI::FilterMode::Linear, AZ::RHI::AddressMode::Wrap);
    }

    AZ::RHI::SamplerState GetClampSamplerState()
    {
        return AZ::RHI::SamplerState::Create(AZ::RHI::FilterMode::Linear, AZ::RHI::FilterMode::Linear, AZ::RHI::AddressMode::Clamp);
    }

} // namespace UnitTest
