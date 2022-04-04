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
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Terrain/Passes/TerrainClipmapComputePass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphBuilder.h>

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] static const char* TerrainClipmapComputePassName = "TerrainClipmapComputePass";
        constexpr const char* MacroColorClipmapString = "MacroColorClipmaps";
        constexpr const char* MacroNormalClipmapString = "MacroNormalClipmaps";
        constexpr const char* DetailColorClipmapString = "DetailColorClipmaps";
        constexpr const char* DetailNormalClipmapString = "DetailNormalClipmaps";
        constexpr const char* DetailHeightClipmapString = "DetailHeightClipmaps";
        // Miscellany clipmap combining:
        // roughness, specularF0, metalness, occlusion
        constexpr const char* DetailMiscClipmapString = "DetailMiscClipmaps";
    } // namespace MacroClipmap

    namespace PassSrgInputs
    {
        static constexpr const char* ClipmapData = "m_clipmapData";
        static constexpr const char* MacroColorClipmaps = "m_macroColorClipmaps";
        static constexpr const char* MacroNormalClipmaps = "m_macroNormalClipmaps";
        static constexpr const char* DetailColorClipmaps = "m_detailColorClipmaps";
        static constexpr const char* DetailNormalClipmaps = "m_detailNormalClipmaps";
        static constexpr const char* DetailHeightClipmaps = "m_detailHeightClipmaps";
        static constexpr const char* DetailMiscClipmaps = "m_detailMiscClipmaps";
    } // namespace TerrainSrgInputs

    AZ::RPI::Ptr<TerrainClipmapGenerationPass> TerrainClipmapGenerationPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainClipmapGenerationPass> pass = aznew TerrainClipmapGenerationPass(descriptor);
        return pass;
    }

    TerrainClipmapGenerationPass::TerrainClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ComputePass(descriptor)
    {
    }

    void TerrainClipmapGenerationPass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();
            auto attachmentDatabase = frameGraph.GetAttachmentDatabase();

            auto macroColorClipmap = clipmapManager.GetClipmapImage(AZ::Name(MacroColorClipmapString));
            auto macroNormalClipmap = clipmapManager.GetClipmapImage(AZ::Name(MacroNormalClipmapString));
            auto detailColorClipmap = clipmapManager.GetClipmapImage(AZ::Name(DetailColorClipmapString));
            auto detailNormalClipmap = clipmapManager.GetClipmapImage(AZ::Name(DetailNormalClipmapString));
            auto detailHeightClipmap = clipmapManager.GetClipmapImage(AZ::Name(DetailHeightClipmapString));
            auto detailMiscClipmap = clipmapManager.GetClipmapImage(AZ::Name(DetailMiscClipmapString));

            attachmentDatabase.ImportImage(macroColorClipmap->GetAttachmentId(), macroColorClipmap->GetRHIImage());
            attachmentDatabase.ImportImage(macroNormalClipmap->GetAttachmentId(), macroNormalClipmap->GetRHIImage());
            attachmentDatabase.ImportImage(detailColorClipmap->GetAttachmentId(), detailColorClipmap->GetRHIImage());
            attachmentDatabase.ImportImage(detailNormalClipmap->GetAttachmentId(), detailNormalClipmap->GetRHIImage());
            attachmentDatabase.ImportImage(detailHeightClipmap->GetAttachmentId(), detailHeightClipmap->GetRHIImage());
            attachmentDatabase.ImportImage(detailMiscClipmap->GetAttachmentId(), detailMiscClipmap->GetRHIImage());

            AZ::RHI::ImageScopeAttachmentDescriptor desc;
            desc.m_imageViewDescriptor = macroColorClipmap->GetImageView()->GetDescriptor();
            desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

            desc.m_attachmentId = macroColorClipmap->GetAttachmentId();
            frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite);
            desc.m_attachmentId = macroNormalClipmap->GetAttachmentId();
            frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite);
            desc.m_attachmentId = detailColorClipmap->GetAttachmentId();
            frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite);
            desc.m_attachmentId = detailNormalClipmap->GetAttachmentId();
            frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite);
            desc.m_attachmentId = detailHeightClipmap->GetAttachmentId();
            frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite);
            desc.m_attachmentId = detailMiscClipmap->GetAttachmentId();
            frameGraph.UseShaderAttachment(desc, AZ::RHI::ScopeAttachmentAccess::ReadWrite);
        }

        ComputePass::SetupFrameGraphDependencies(frameGraph);
    }

    void TerrainClipmapGenerationPass::InitializeInternal()
    {
        ComputePass::InitializeInternal();

        m_macroColorClipmapsIndex = m_shaderResourceGroup->FindShaderInputImageIndex(AZ::Name(PassSrgInputs::MacroColorClipmaps));
        AZ_Error(TerrainClipmapComputePassName, m_macroColorClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", PassSrgInputs::MacroColorClipmaps);
        m_macroNormalClipmapsIndex = m_shaderResourceGroup->FindShaderInputImageIndex(AZ::Name(PassSrgInputs::MacroNormalClipmaps));
        AZ_Error(TerrainClipmapComputePassName, m_macroNormalClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", PassSrgInputs::MacroNormalClipmaps);
        m_detailColorClipmapsIndex = m_shaderResourceGroup->FindShaderInputImageIndex(AZ::Name(PassSrgInputs::DetailColorClipmaps));
        AZ_Error(TerrainClipmapComputePassName, m_detailColorClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", PassSrgInputs::DetailColorClipmaps);
        m_detailNormalClipmapsIndex = m_shaderResourceGroup->FindShaderInputImageIndex(AZ::Name(PassSrgInputs::DetailNormalClipmaps));
        AZ_Error(TerrainClipmapComputePassName, m_detailNormalClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", PassSrgInputs::DetailNormalClipmaps);
        m_detailHeightClipmapsIndex = m_shaderResourceGroup->FindShaderInputImageIndex(AZ::Name(PassSrgInputs::DetailHeightClipmaps));
        AZ_Error(TerrainClipmapComputePassName, m_detailHeightClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", PassSrgInputs::DetailHeightClipmaps);
        m_detailMiscClipmapsIndex = m_shaderResourceGroup->FindShaderInputImageIndex(AZ::Name(PassSrgInputs::DetailMiscClipmaps));
        AZ_Error(TerrainClipmapComputePassName, m_detailMiscClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", PassSrgInputs::DetailMiscClipmaps);
    }

    void TerrainClipmapGenerationPass::FrameBeginInternal(FramePrepareParams params)
    {
        ComputePass::FrameBeginInternal(params);
    }

    void TerrainClipmapGenerationPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
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
                const TerrainClipmapManager& clipmapManager = terrainFeatureProcessor->GetClipmapManager();

                auto macroColorClipmap = clipmapManager.GetClipmapImage(AZ::Name(MacroColorClipmapString));
                auto macroNormalClipmap = clipmapManager.GetClipmapImage(AZ::Name(MacroNormalClipmapString));
                auto detailColorClipmap = clipmapManager.GetClipmapImage(AZ::Name(DetailColorClipmapString));
                auto detailNormalClipmap = clipmapManager.GetClipmapImage(AZ::Name(DetailNormalClipmapString));
                auto detailHeightClipmap = clipmapManager.GetClipmapImage(AZ::Name(DetailHeightClipmapString));
                auto detailMiscClipmap = clipmapManager.GetClipmapImage(AZ::Name(DetailMiscClipmapString));

                m_shaderResourceGroup->SetImage(m_macroColorClipmapsIndex, macroColorClipmap);
                m_shaderResourceGroup->SetImage(m_macroNormalClipmapsIndex, macroNormalClipmap);
                m_shaderResourceGroup->SetImage(m_detailColorClipmapsIndex, detailColorClipmap);
                m_shaderResourceGroup->SetImage(m_detailNormalClipmapsIndex, detailNormalClipmap);
                m_shaderResourceGroup->SetImage(m_detailHeightClipmapsIndex, detailHeightClipmap);
                m_shaderResourceGroup->SetImage(m_detailMiscClipmapsIndex, detailMiscClipmap);

                m_needsUpdate = false;
            }
        }

        ComputePass::CompileResources(context);
    }
} // namespace Terrain
