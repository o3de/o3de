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
        AddMaterialPropertyForSrg(materialTypeCreator, Name{ propertyGroupPrefix + "MyAttachmentImage" }, MaterialPropertyDataType::Image, Name{ "m_attachmentImage" });
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
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_unused" }, 0, 4, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_color" }, 4, 16, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float" }, 20, 4, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_int" }, 24, 4, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_uint" }, 28, 4, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float2" }, 32, 8, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float3" }, 40, 12, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float4" }, 52, 16, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_enum" }, 68, 4, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_bool" }, 72, 4, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float3x3" }, 76, 44, 0, 0}); // See ConstantsData::SetConstant<Matrix3x3> for packing rules
        srgLayout->AddShaderInput(RHI::ShaderInputConstantDescriptor{Name{ "m_float4x4" }, 120, 64, 0, 0});
        srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{Name{ "m_unusedImage" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 1, 1});
        srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{Name{ "m_image" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 2, 2});
        srgLayout->AddShaderInput(RHI::ShaderInputImageDescriptor{Name{ "m_attachmentImage" }, RHI::ShaderInputImageAccess::Read, RHI::ShaderInputImageType::Image2D, 1, 2, 2});

        EXPECT_TRUE(srgLayout->Finalize());

        return srgLayout;
    }

} // namespace UnitTest
