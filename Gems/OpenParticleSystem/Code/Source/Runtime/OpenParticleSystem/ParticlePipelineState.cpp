/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <OpenParticleSystem/ParticlePipelineState.h>

namespace OpenParticle
{
    void EmitterInstance::Setup(AZ::Data::Asset<AZ::RPI::MaterialAsset>& mat)
    {
        m_materialAsset = mat;
        m_material = AZ::RPI::Material::Create(m_materialAsset);
        ReBuildPipeline();
    }

    void EmitterInstance::Reset()
    {
        m_perDrawSrgs.clear();
        m_shaders.clear();
    }

    void EmitterInstance::ReBuildPipeline()
    {
        Reset();
        auto& objectSrgLayout = m_material->GetAsset()->GetObjectSrgLayout();
        if (objectSrgLayout != nullptr)
        {
            auto& objSrgAsset = m_material->GetAsset()->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
            m_objSrg = AZ::RPI::ShaderResourceGroup::Create(objSrgAsset, objectSrgLayout->GetName());
        }

        auto& shaderCollection = m_material->GetGeneralShaderCollection();
        for (auto& shaderItem : shaderCollection)
        {
            auto drawListTag = shaderItem.GetDrawListTagOverride();
            if (drawListTag.IsNull())
            {
                auto shaderAsset = shaderItem.GetShaderAsset();
                shaderAsset.QueueLoad();
                if (shaderAsset.IsLoading())
                {
                    shaderAsset.BlockUntilLoadComplete();
                }
                drawListTag = AZ::RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->FindTag(shaderAsset->GetDrawListName());
            }
            if (!m_scene->HasOutputForPipelineState(drawListTag))
            {
                continue;
            }

            if (!shaderItem.IsEnabled())
            {
                continue;
            }

            auto shader = AZ::RPI::Shader::FindOrCreate(shaderItem.GetShaderAsset());
            if (!shader)
            {
                continue;
            }

            m_shaders.emplace_back(AZStd::pair{ drawListTag, shader });
            auto drawSrgLayout = shader->GetAsset()->GetDrawSrgLayout();
            if (drawSrgLayout)
            {
                auto drawSrg =
                    AZ::RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), drawSrgLayout->GetName());
                m_perDrawSrgs.emplace_back(drawSrg);
                m_emitterForDrawPair.emplace(
                    shader.get(),
                    EmitterForDraw{ drawSrg.get(), { UINT64_MAX }, *shaderItem.GetRenderStatesOverlay(), shaderItem.GetShaderOptionGroup(), AZ::RHI::GeometryView(AZ::RHI::MultiDevice::AllDevices) });
            }
            else
            {
                m_emitterForDrawPair.emplace(shader.get(),
                    EmitterForDraw{ nullptr, { UINT64_MAX }, *shaderItem.GetRenderStatesOverlay(), shaderItem.GetShaderOptionGroup(), AZ::RHI::GeometryView(AZ::RHI::MultiDevice::AllDevices) });
            }
        }
        m_materialChangeId = m_material->GetCurrentChangeId();
    }

    bool ParticlePipelineState::Setup(AZ::u32 key)
    {
        SimuCore::ParticleCore::RenderType type = static_cast<SimuCore::ParticleCore::RenderType>(key);
        switch (type)
        {
        case SimuCore::ParticleCore::RenderType::SPRITE:
            SetupSprite();
            break;
        case SimuCore::ParticleCore::RenderType::MESH:
            SetupMesh();
            break;
        case SimuCore::ParticleCore::RenderType::RIBBON:
            SetupRibbon();
            break;
        default:
            return false;
        }
        return true;
    }

    void ParticlePipelineState::SetupSprite()
    {
        AZ::RHI::InputStreamLayoutBuilder layoutBuilder;
        layoutBuilder.Begin();
        layoutBuilder.SetTopology(AZ::RHI::PrimitiveTopology::TriangleStrip);
        layoutBuilder.AddBuffer(AZ::RHI::StreamStepFunction::PerInstance)
            ->Channel("POSITION", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("COLOR", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("INITROTATION", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("ROTATEVECTOR", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("SCALE", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("UP", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("VELOCITY", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("SUBUV", AZ::RHI::Format::R32G32B32A32_FLOAT);
        m_streamLayout = layoutBuilder.End();
    }

    void ParticlePipelineState::SetupMesh()
    {
        AZ::RHI::InputStreamLayoutBuilder layoutBuilder;
        layoutBuilder.Begin();
        layoutBuilder.SetTopology(AZ::RHI::PrimitiveTopology::TriangleStrip);
        layoutBuilder.AddBuffer()->Channel("POSITION", AZ::RHI::Format::R32G32B32_FLOAT);
        layoutBuilder.AddBuffer()->Channel("NORMAL", AZ::RHI::Format::R32G32B32_FLOAT);
        layoutBuilder.AddBuffer()->Channel("TANGENT", AZ::RHI::Format::R32G32B32A32_FLOAT);
        layoutBuilder.AddBuffer()->Channel("BITANGENT", AZ::RHI::Format::R32G32B32_FLOAT);
        layoutBuilder.AddBuffer()->Channel("UV", AZ::RHI::Format::R32G32_FLOAT);
        layoutBuilder.AddBuffer()->Channel("UV", AZ::RHI::Format::R32G32_FLOAT);
        layoutBuilder.AddBuffer(AZ::RHI::StreamStepFunction::PerInstance)
            ->Channel("OFFSET", AZ::RHI::Format::R32G32B32_FLOAT)
            ->Channel("COLOR", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("INITROTATION", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("ROTATEVECTOR", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("SCALE", AZ::RHI::Format::R32G32B32_FLOAT);
        m_streamLayout = layoutBuilder.End();
    }

    void ParticlePipelineState::SetupRibbon()
    {
        AZ::RHI::InputStreamLayoutBuilder layoutBuilder;
        layoutBuilder.Begin();
        layoutBuilder.SetTopology(AZ::RHI::PrimitiveTopology::TriangleList);
        layoutBuilder.AddBuffer()
            ->Channel("POSITION", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("COLOR", AZ::RHI::Format::R32G32B32A32_FLOAT)
            ->Channel("UV", AZ::RHI::Format::R32G32B32A32_FLOAT);
        m_streamLayout = layoutBuilder.End();
    }
} // namespace OpenParticle
