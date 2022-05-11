/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <TerrainRenderer/Passes/TerrainClipmapComputePass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <TerrainRenderer/TerrainClipmapManager.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/CommandList.h>

namespace Terrain
{
    TerrainClipmapGenerationPass::TerrainClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::RenderPass(descriptor)
        , m_passDescriptor(descriptor)
    {
        LoadShader();
    }

    void TerrainClipmapGenerationPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        AZ::RPI::RenderPass::SetupFrameGraphDependencies(frameGraph);
        frameGraph.SetEstimatedItemCount(1);
    }

    void TerrainClipmapGenerationPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        if (m_shaderResourceGroup != nullptr)
        {
            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }
        if (m_drawSrg != nullptr)
        {
            BindSrg(m_drawSrg->GetRHIShaderResourceGroup());
            m_drawSrg->Compile();
        }
    }

    void TerrainClipmapGenerationPass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
    {
        //! Skip invoking the compute shader.
        if (m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX == 0 ||
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY == 0)
        {
            return;
        }

        AZ::RHI::CommandList* commandList = context.GetCommandList();

        SetSrgsForDispatch(commandList);

        commandList->Submit(m_dispatchItem);
    }

    void TerrainClipmapGenerationPass::OnShaderReinitialized([[maybe_unused]] const AZ::RPI::Shader& shader)
    {
        LoadShader();
    }

    void TerrainClipmapGenerationPass::OnShaderAssetReinitialized(
        [[maybe_unused]] const AZ::Data::Asset<AZ::RPI::ShaderAsset>& shaderAsset)
    {
        LoadShader();
    }

    void TerrainClipmapGenerationPass::OnShaderVariantReinitialized([[maybe_unused]] const AZ::RPI::ShaderVariant& shaderVariant)
    {
        LoadShader();
    }

    void TerrainClipmapGenerationPass::LoadShader()
    {
        // Load ComputePassData...
        const TerrainClipmapGenerationPassData* passData = AZ::RPI::PassUtils::GetPassData<TerrainClipmapGenerationPassData>(m_passDescriptor);
        if (passData == nullptr)
        {
            AZ_Error(
                "PassSystem", false, "[TerrainClipmapGenerationPass '%s']: Trying to construct without valid pass data!",
                GetPathName().GetCStr());
            return;
        }

        // Load Shader
        m_shader = AZ::RPI::LoadShader(passData->m_shaderReference.m_assetId, passData->m_shaderReference.m_filePath);
        if (m_shader == nullptr)
        {
            AZ_Error(
                "PassSystem", false, "[TerrainClipmapGenerationPass '%s']: Failed to load shader '%s'!", GetPathName().GetCStr(),
                passData->m_shaderReference.m_filePath.data());
            return;
        }

        // Load Pass SRG...
        const auto passSrgLayout = m_shader->FindShaderResourceGroupLayout(AZ::RPI::SrgBindingSlot::Pass);
        if (passSrgLayout)
        {
            m_shaderResourceGroup = AZ::RPI::ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), passSrgLayout->GetName());

            AZ_Assert(
                m_shaderResourceGroup, "[TerrainClipmapGenerationPass '%s']: Failed to create SRG from shader asset '%s'",
                GetPathName().GetCStr(), passData->m_shaderReference.m_filePath.data());

            AZ::RPI::PassUtils::BindDataMappingsToSrg(m_passDescriptor, m_shaderResourceGroup.get());
        }

        // Load Draw SRG...
        const auto drawSrgLayout = m_shader->FindShaderResourceGroupLayout(AZ::RPI::SrgBindingSlot::Draw);
        if (drawSrgLayout)
        {
            m_drawSrg = AZ::RPI::ShaderResourceGroup::Create(m_shader->GetAsset(), m_shader->GetSupervariantIndex(), drawSrgLayout->GetName());
        }

        AZ::RHI::DispatchDirect dispatchArgs;

        const auto outcome = AZ::RPI::GetComputeShaderNumThreads(m_shader->GetAsset(), dispatchArgs);
        if (!outcome.IsSuccess())
        {
            AZ_Error(
                "PassSystem", false, "[TerrainClipmapGenerationPass '%s']: Shader '%.*s' contains invalid numthreads arguments:\n%s",
                GetPathName().GetCStr(), passData->m_shaderReference.m_filePath.size(), passData->m_shaderReference.m_filePath.data(),
                outcome.GetError().c_str());
        }

        m_dispatchItem.m_arguments = dispatchArgs;

        // Setup pipeline state...
        AZ::RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
        const auto& shaderVariant = m_shader->GetVariant(AZ::RPI::ShaderAsset::RootShaderVariantStableId);
        shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

        m_dispatchItem.m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);

        AZ::RPI::ShaderReloadNotificationBus::Handler::BusDisconnect();
        AZ::RPI::ShaderReloadNotificationBus::Handler::BusConnect(passData->m_shaderReference.m_assetId);
    }

    AZ::RPI::Ptr<TerrainMacroClipmapGenerationPass> TerrainMacroClipmapGenerationPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainMacroClipmapGenerationPass> pass = aznew TerrainMacroClipmapGenerationPass(descriptor);
        return pass;
    }

    TerrainMacroClipmapGenerationPass::TerrainMacroClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor)
        : TerrainClipmapGenerationPass(descriptor)
    {
    }

    void TerrainMacroClipmapGenerationPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();
            AZ::RHI::FrameGraphAttachmentInterface attachmentDatabase = frameGraph.GetAttachmentDatabase();

            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::MacroColor, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::MacroNormal, attachmentDatabase);

            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::MacroColor, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::MacroNormal, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
        }
    }

    void TerrainMacroClipmapGenerationPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();

            clipmapManager.GetMacroDispatchThreadNum(
                m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX,
                m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY,
                m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ);

            auto terrainSrg = terrainFeatureProcessor->GetTerrainShaderResourceGroup();
            if (terrainSrg)
            {
                BindSrg(terrainFeatureProcessor->GetTerrainShaderResourceGroup()->GetRHIShaderResourceGroup());
            }

            auto material = terrainFeatureProcessor->GetMaterial();
            if (material)
            {
                BindSrg(material->GetRHIShaderResourceGroup());
            }

            if (m_needsUpdate)
            {
                m_shaderResourceGroup->SetImage(
                    m_macroColorClipmapsIndex,
                    clipmapManager.GetClipmapImage(TerrainClipmapManager::ClipmapName::MacroColor)
                );

                m_shaderResourceGroup->SetImage(
                    m_macroNormalClipmapsIndex,
                    clipmapManager.GetClipmapImage(TerrainClipmapManager::ClipmapName::MacroNormal)
                );

                m_needsUpdate = false;
            }
        }
        else
        {
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = 0;
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = 0;
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;
        }

        TerrainClipmapGenerationPass::CompileResources(context);
    }

    AZ::RPI::Ptr<TerrainDetailClipmapGenerationPass> TerrainDetailClipmapGenerationPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainDetailClipmapGenerationPass> pass = aznew TerrainDetailClipmapGenerationPass(descriptor);
        return pass;
    }

    TerrainDetailClipmapGenerationPass::TerrainDetailClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor)
        : TerrainClipmapGenerationPass(descriptor)
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

    void TerrainDetailClipmapGenerationPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();
            AZ::RHI::FrameGraphAttachmentInterface attachmentDatabase = frameGraph.GetAttachmentDatabase();

            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailColor, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailNormal, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailHeight, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailRoughness, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailSpecularF0, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailMetalness, attachmentDatabase);
            clipmapManager.ImportClipmap(TerrainClipmapManager::ClipmapName::DetailOcclusion, attachmentDatabase);

            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::MacroColor, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::MacroNormal, AZ::RHI::ScopeAttachmentAccess::Read, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailColor, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailNormal, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailHeight, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailRoughness, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailSpecularF0, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailMetalness, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
            clipmapManager.UseClipmap(TerrainClipmapManager::ClipmapName::DetailOcclusion, AZ::RHI::ScopeAttachmentAccess::ReadWrite, frameGraph);
        }

        TerrainClipmapGenerationPass::SetupFrameGraphDependencies(frameGraph);
    }

    void TerrainDetailClipmapGenerationPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();

            clipmapManager.GetDetailDispatchThreadNum(
                m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX,
                m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY,
                m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ);

            auto terrainSrg = terrainFeatureProcessor->GetTerrainShaderResourceGroup();
            if (terrainSrg)
            {
                BindSrg(terrainFeatureProcessor->GetTerrainShaderResourceGroup()->GetRHIShaderResourceGroup());
            }

            auto material = terrainFeatureProcessor->GetMaterial();
            if (material)
            {
                BindSrg(material->GetRHIShaderResourceGroup());
            }

            if (m_needsUpdate)
            {
                for (uint32_t i = 0; i < TerrainClipmapManager::ClipmapName::Count; ++i)
                {
                    m_shaderResourceGroup->SetImage(
                        m_clipmapImageIndex[i],
                        clipmapManager.GetClipmapImage(static_cast<TerrainClipmapManager::ClipmapName>(i))
                    );
                }

                m_needsUpdate = false;
            }
        }
        else
        {
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = 0;
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = 0;
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;
        }

        TerrainClipmapGenerationPass::CompileResources(context);
    }
} // namespace Terrain
