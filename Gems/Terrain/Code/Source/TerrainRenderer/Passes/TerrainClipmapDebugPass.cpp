/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Passes/TerrainClipmapDebugPass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzCore/Console/Console.h>

namespace Terrain
{
    AZ_CVAR(
        bool,
        r_terrainClipmapDebugEnable,
        false,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "Turn on clipmap debug rendering on the screen.");

    AZ_CVAR(
        uint32_t,
        r_terrainClipmapDebugClipmapId,
        0,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The clipmap index to be rendered on the screen.\n"
        "0: macro color clipmap\n"
        "1: macro normal clipmap\n"
        "2: detail color clipmap\n"
        "3: detail normal clipmap\n"
        "4: detail height clipmap\n"
        "5: detail roughness clipmap\n"
        "6: detail specularF0 clipmap\n"
        "7: detail metalness clipmap\n"
        "8: detail occlusion clipmap\n");

    AZ_CVAR(
        uint32_t,
        r_terrainClipmapDebugClipmapLevel,
        0,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The clipmap level to be rendered on the screen.");

    AZ_CVAR(
        float,
        r_terrainClipmapDebugScale,
        0.5f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The size multiplier of the clipmap texture's debug display.");

    AZ_CVAR(
        float,
        r_terrainClipmapDebugBrightness,
        1.0f,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "A multiplier to the final output of the clipmap texture's debug display.");

    AZ::RPI::Ptr<TerrainClipmapDebugPass> TerrainClipmapDebugPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainClipmapDebugPass> pass = aznew TerrainClipmapDebugPass(descriptor);
        return pass;
    }

    TerrainClipmapDebugPass::TerrainClipmapDebugPass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::FullscreenTrianglePass(descriptor)
        , m_clipmapImageIndex{
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::MacroColor]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::MacroNormal]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailColor]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailNormal]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailHeight]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailRoughness]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailSpecularF0]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailMetalness]),
            AZ::RHI::ShaderInputNameIndex(TerrainClipmapManager::ClipmapImageShaderInput[TerrainClipmapManager::ClipmapName::DetailOcclusion])
        }
    {
    }

    void TerrainClipmapDebugPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();
            AZ::RHI::FrameGraphAttachmentInterface attachmentDatabase = frameGraph.GetAttachmentDatabase();

            //! If this frame, macro clipmap update is skipped but detail is not,
            //! then the detail pass will be responsible for importing the clipmaps.
            if (!clipmapManager.HasMacroClipmapUpdate() && !clipmapManager.HasDetailClipmapUpdate())
            {
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::MacroColor, attachmentDatabase);
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::MacroNormal, attachmentDatabase);
            }

            if (!clipmapManager.HasDetailClipmapUpdate())
            {
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailColor, attachmentDatabase);
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailNormal, attachmentDatabase);
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailHeight, attachmentDatabase);
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailRoughness, attachmentDatabase);
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailSpecularF0, attachmentDatabase);
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailMetalness, attachmentDatabase);
                clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailOcclusion, attachmentDatabase);
            }

            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::MacroColor, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::MacroNormal, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailColor, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailNormal, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailHeight, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailRoughness, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailSpecularF0, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailMetalness, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailOcclusion, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
        }

        AZ::RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);
    }

    void TerrainClipmapDebugPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();

            if (m_needsUpdate)
            {
                for (uint32_t i = 0; i < TerrainClipmapManager::ClipmapName::Count; ++i)
                {
                    m_shaderResourceGroup->SetImage(
                        m_clipmapImageIndex[i], clipmapManager.GetClipmapImage(static_cast<TerrainClipmapManager::ClipmapName>(i)));
                }

                m_needsUpdate = false;
            }

            m_shaderResourceGroup->SetConstant(m_clipmapSize, aznumeric_cast<float>(clipmapManager.GetClipmapSize()));
        }

        auto viewportContextInterface = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        auto viewportContext = viewportContextInterface->GetViewportContextByScene(scene);
        auto viewportWindowSize = viewportContext->GetViewportSize();

        AZStd::array<float, 2> viewportSize;
        viewportSize[0] = aznumeric_cast<float>(viewportWindowSize.m_width);
        viewportSize[1] = aznumeric_cast<float>(viewportWindowSize.m_height);
        m_shaderResourceGroup->SetConstant(m_viewportSize, viewportSize);

        m_shaderResourceGroup->SetConstant(m_clipmapId, uint32_t(r_terrainClipmapDebugClipmapId));
        m_shaderResourceGroup->SetConstant(m_clipmapLevel, aznumeric_cast<float>(uint32_t(r_terrainClipmapDebugClipmapLevel)));

        m_shaderResourceGroup->SetConstant(m_scale, float(r_terrainClipmapDebugScale));
        m_shaderResourceGroup->SetConstant(m_brightness, float(r_terrainClipmapDebugBrightness));

        AZ::RPI::FullscreenTrianglePass::CompileResources(context);
    }

    bool TerrainClipmapDebugPass::IsEnabled() const
    {
        return AZ::RPI::Pass::IsEnabled() && r_terrainClipmapDebugEnable;
    }
}
