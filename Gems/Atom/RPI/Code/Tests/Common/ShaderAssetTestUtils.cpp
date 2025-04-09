/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>

#include <Atom/RPI.Reflect/Shader/ShaderAssetCreator.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantAssetCreator.h>

#include <Common/RHI/Stubs.h>

#include "ShaderAssetTestUtils.h"

namespace UnitTest
{
    template<class T>
    void AddShaderInputToBindingInfo(AZ::RHI::ShaderResourceGroupBindingInfo& bindingInfo, const T& shaderInputs)
    {
        for (const auto& shaderInput : shaderInputs)
        {
            bindingInfo.m_resourcesRegisterMap.insert(
                { shaderInput.m_name.GetStringView(),
                  AZ::RHI::ResourceBindingInfo{ AZ::RHI::ShaderStageMask::Vertex, shaderInput.m_registerId, shaderInput.m_spaceId } });
        }
    }

    static AZ::RHI::ShaderResourceGroupBindingInfo CreateShaderResourceGroupBindingInfo(const AZ::RHI::ShaderResourceGroupLayout* layout)
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

    static AZ::Data::Asset<AZ::RPI::ShaderVariantAsset> CreateTestShaderVariantAsset(
        AZ::RPI::ShaderVariantId id,
        AZ::RPI::ShaderVariantStableId stableId,
        const AZStd::vector<AZ::RHI::ShaderStage>& stagesToActivate = { AZ::RHI::ShaderStage::Vertex, AZ::RHI::ShaderStage::Fragment })
    {
        using namespace AZ;
        RPI::ShaderVariantAssetCreator shaderVariantAssetCreator;
        shaderVariantAssetCreator.Begin(Uuid::CreateRandom(), id, stableId, false);

        for (RHI::ShaderStage rhiStage : stagesToActivate)
        {
            RHI::Ptr<RHI::ShaderStageFunction> shaderStageFunction = aznew StubRHI::ShaderStageFunction;
            shaderVariantAssetCreator.SetShaderFunction(rhiStage, shaderStageFunction);
        }

        Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset;
        shaderVariantAssetCreator.End(shaderVariantAsset);

        return shaderVariantAsset;
    }

    AZ::Data::Asset<AZ::RPI::ShaderAsset> CreateTestShaderAsset(
        const AZ::Data::AssetId& shaderAssetId,
        AZ::RHI::Ptr<AZ::RHI::ShaderResourceGroupLayout> optionalSrgLayout,
        AZ::RPI::Ptr<AZ::RPI::ShaderOptionGroupLayout> optionalShaderOptions,
        const AZ::Name& shaderName,
        const AZ::Name& drawListName)
    {
        using namespace AZ;
        using namespace RPI;
        using namespace RHI;

        Data::Asset<ShaderAsset> shaderAsset;

        RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor = RHI::PipelineLayoutDescriptor::Create();
        if (optionalSrgLayout)
        {
            const RHI::ShaderResourceGroupLayout* layout = optionalSrgLayout.get();
            RHI::ShaderResourceGroupBindingInfo bindingInfo = CreateShaderResourceGroupBindingInfo(layout);
            pipelineLayoutDescriptor->AddShaderResourceGroupLayoutInfo(*layout, bindingInfo);
        }
        pipelineLayoutDescriptor->Finalize();

        if (!optionalShaderOptions)
        {
            optionalShaderOptions = RPI::ShaderOptionGroupLayout::Create();
            optionalShaderOptions->Finalize();
        }

        ShaderAssetCreator creator;
        creator.Begin(shaderAssetId);
        creator.SetName(shaderName);
        creator.SetDrawListName(drawListName);
        creator.SetShaderOptionGroupLayout(optionalShaderOptions);

        creator.BeginAPI(RHI::Factory::Get().GetType());

        creator.BeginSupervariant(AZ::Name{}); // The default (first) supervariant MUST be nameless.

        if (optionalSrgLayout)
        {
            creator.SetSrgLayoutList({ optionalSrgLayout });
        }
        creator.SetPipelineLayout(pipelineLayoutDescriptor);

        //creator.SetRenderStates(m_renderStates);
        //creator.SetInputContract(CreateSimpleShaderInputContract());
        //creator.SetOutputContract(CreateSimpleShaderOutputContract());

        RHI::ShaderStageAttributeMapList attributeMaps;
        attributeMaps.resize(RHI::ShaderStageCount);
        creator.SetShaderStageAttributeMapList(attributeMaps);

        Data::Asset<RPI::ShaderVariantAsset> shaderVariantAsset =
            CreateTestShaderVariantAsset(RPI::ShaderVariantId{}, RPI::ShaderVariantStableId{ 0 });

        creator.SetRootShaderVariantAsset(shaderVariantAsset);

        creator.EndSupervariant();

        creator.EndAPI();
        creator.End(shaderAsset);

        return shaderAsset;
    }
}
