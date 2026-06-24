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

        if (!m_material)
        {
            AZ_Error("ParticleSystem", false,
                "EmitterInstance::Setup - Material::Create() returned null. "
                "The MaterialAsset or MaterialTypeAsset may not be fully loaded.");
            m_needsPipelineRebuild = true;
            return;
        }

        ReBuildPipeline();

        // If no shaders were built, mark for deferred rebuild
        m_needsPipelineRebuild = m_shaders.empty();
        if (m_needsPipelineRebuild)
        {
            AZ_Warning("ParticleSystem", false,
                "EmitterInstance::Setup - no shaders built after ReBuildPipeline. "
                "Pipeline will be rebuilt when shaders become available.");
        }
    }

    void EmitterInstance::Reset()
    {
        m_perDrawSrgs.clear();
        m_shaders.clear();
        m_emitterForDrawPair.clear();
    }

    bool EmitterInstance::TryRebuildPipeline()
    {
        if (!m_needsPipelineRebuild)
        {
            return false;
        }

        if (!m_material)
        {
            // Material creation previously failed - try again
            if (m_materialAsset && m_materialAsset->IsReady())
            {
                m_material = AZ::RPI::Material::Create(m_materialAsset);
                if (!m_material)
                {
                    AZ_Warning("ParticleSystem", false,
                        "TryRebuildPipeline - Material::Create() still returning null, will retry later");
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        // Check if the material's change ID has changed (indicates shader reload or recompilation)
        auto currentChangeId = m_material->GetCurrentChangeId();
        if (currentChangeId == m_materialChangeId && !m_shaders.empty())
        {
            return false;
        }

        ReBuildPipeline();
        m_needsPipelineRebuild = m_shaders.empty();
        return !m_needsPipelineRebuild;
    }

    void EmitterInstance::ReBuildPipeline()
    {
        Reset();
        if (!m_material)
        {
            return;
        }

        auto& objectSrgLayout = m_material->GetAsset()->GetObjectSrgLayout();
        if (objectSrgLayout != nullptr)
        {
            auto& objSrgAsset = m_material->GetAsset()->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
            m_objSrg = AZ::RPI::ShaderResourceGroup::Create(objSrgAsset, objectSrgLayout->GetName());
        }

        m_material->ForAllShaderItems(
            [this](const AZ::Name& materialPipelineName, const AZ::RPI::ShaderCollection::Item& shaderItem)
            {
                if (!shaderItem.IsEnabled())
                {
                    return true;
                }

                auto drawListTag = shaderItem.GetDrawListTagOverride();
                if (drawListTag.IsNull())
                {
                    auto shaderAsset = shaderItem.GetShaderAsset();
                    if (!shaderAsset.IsReady())
                    {
                        shaderAsset.QueueLoad();
                        shaderAsset.BlockUntilLoadComplete();
                    }
                    drawListTag = AZ::RHI::RHISystemInterface::Get()->GetDrawListTagRegistry()->FindTag(shaderAsset->GetDrawListName());
                }

                if (!m_scene->HasOutputForPipelineState(drawListTag))
                {
                    return true;
                }

                auto shader = AZ::RPI::Shader::FindOrCreate(shaderItem.GetShaderAsset());
                if (!shader)
                {
                    return true;
                }

                EmitterDrawKey drawKey;
                drawKey.m_shader = shader.get();
                drawKey.m_drawListTag = drawListTag;
                drawKey.m_materialPipelineName = materialPipelineName;
                drawKey.m_shaderOptionHash = 0;

                m_shaders.emplace_back(ShaderDrawRecord{ drawListTag, materialPipelineName, shader, drawKey });
                auto drawSrgLayout = shader->GetAsset()->GetDrawSrgLayout();
                if (drawSrgLayout)
                {
                    auto drawSrg =
                        AZ::RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), drawSrgLayout->GetName());
                    m_perDrawSrgs.emplace_back(drawSrg);
                    m_emitterForDrawPair.emplace(
                        drawKey,
                        EmitterForDraw{ drawSrg.get(), { UINT64_MAX }, *shaderItem.GetRenderStatesOverlay(), shaderItem.GetShaderOptionGroup(), AZ::RHI::GeometryView(AZ::RHI::MultiDevice::AllDevices) });
                }
                else
                {
                    m_emitterForDrawPair.emplace(drawKey,
                        EmitterForDraw{ nullptr, { UINT64_MAX }, *shaderItem.GetRenderStatesOverlay(), shaderItem.GetShaderOptionGroup(), AZ::RHI::GeometryView(AZ::RHI::MultiDevice::AllDevices) });
                }

                return true;
            });

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
