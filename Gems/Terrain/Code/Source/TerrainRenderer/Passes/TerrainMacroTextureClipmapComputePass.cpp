/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Terrain/Passes/TerrainMacroTextureClipmapComputePass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

namespace Terrain
{
    AZ::RPI::Ptr<TerrainMacroTextureClipmapComputePass> TerrainMacroTextureClipmapComputePass::Create(
        const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainMacroTextureClipmapComputePass> pass = aznew TerrainMacroTextureClipmapComputePass(descriptor);
        return pass;
    }

    TerrainMacroTextureClipmapComputePass::TerrainMacroTextureClipmapComputePass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ParentPass(descriptor)
    {
    }

    AZ::RPI::Ptr<TerrainMacroTextureClipmapGenerationPass> TerrainMacroTextureClipmapGenerationPass::Create(
        const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<TerrainMacroTextureClipmapGenerationPass> pass = aznew TerrainMacroTextureClipmapGenerationPass(descriptor);
        return pass;
    }

    TerrainMacroTextureClipmapGenerationPass::TerrainMacroTextureClipmapGenerationPass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ComputePass(descriptor)
    {
    }

    void TerrainMacroTextureClipmapGenerationPass::BuildInternal()
    {
        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

        AZ_Assert(m_ownedAttachments.size() == ClipmapStackSize + 1, "The pass should own a stack of clipmaps and a mipmap for the bottom image.");

        for (uint32_t i = 0; i < ClipmapStackSize; ++i)
        {
            AZ::RPI::Ptr<AZ::RPI::PassAttachment> clipmapStack = m_ownedAttachments[i];

            clipmapStack->Update();

            clipmapStack->m_lifetime = AZ::RHI::AttachmentLifetimeType::Imported;

            AZ::RHI::ImageDescriptor& imageDesc = clipmapStack->m_descriptor.m_image;
            imageDesc.m_bindFlags |= AZ::RHI::ImageBindFlags::ShaderReadWrite;

            AZ::RHI::ClearValue clearValue = AZ::RHI::ClearValue::CreateVector4Float(1.0f, 1.0f, 1.0f, 1.0f);
            m_clipmapStacks[i] = AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(clipmapStack->m_path.GetCStr()), &clearValue, nullptr);

            clipmapStack->m_path = clipmapStack->GetAttachmentId();
            clipmapStack->m_importedResource = m_clipmapStacks[i];
        }

        {
            AZ::RPI::Ptr<AZ::RPI::PassAttachment> clipmapPyramid = m_ownedAttachments[ClipmapStackSize];

            clipmapPyramid->Update();

            clipmapPyramid->m_lifetime = AZ::RHI::AttachmentLifetimeType::Imported;

            AZ::RHI::ImageDescriptor& imageDesc = clipmapPyramid->m_descriptor.m_image;
            imageDesc.m_bindFlags |= AZ::RHI::ImageBindFlags::ShaderReadWrite;

            AZ::RHI::ClearValue clearValue = AZ::RHI::ClearValue::CreateVector4Float(1.0f, 1.0f, 1.0f, 1.0f);
            m_clipmapPyramid = AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(clipmapPyramid->m_path.GetCStr()), &clearValue, nullptr);

            clipmapPyramid->m_path = m_clipmapPyramid->GetAttachmentId();
            clipmapPyramid->m_importedResource = m_clipmapPyramid;
        }
    }

    void TerrainMacroTextureClipmapGenerationPass::InitializeInternal()
    {
        for (uint32_t clipmapIndex = 0; clipmapIndex <= ClipmapStackSize; ++clipmapIndex)
        {
            m_previousClipmapCenters[clipmapIndex] = AZ::Vector4(0.5f, 0.5f, 0.0f, 0.0f);
            m_currentClipmapCenters[clipmapIndex] = AZ::Vector4(0.5f, 0.5f, 0.0f, 0.0f);

            m_fullUpdateFlag = m_fullUpdateFlag | (1 << clipmapIndex);
        }

        m_previousViewPosition = AZ::Vector3(0.0, 0.0, 0.0);
        m_currentViewPosition = AZ::Vector3(0.0, 0.0, 0.0);

        m_colorClipmapStackIndex.Reset();
        m_colorClipmapPyramidIndex.Reset();
        m_worldBoundsMinIndex.Reset();
        m_worldBoundsMaxIndex.Reset();
        m_previousViewPositionIndex.Reset();
        m_currentViewPositionIndex.Reset();
        m_currentClipmapCentersIndex.Reset();
        m_previousClipmapCentersIndex.Reset();

        ComputePass::InitializeInternal();
    }

    void TerrainMacroTextureClipmapGenerationPass::FrameBeginInternal(FramePrepareParams params)
    {
        UpdateClipmapData();

        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const AZ::Aabb worldBounds = terrainFeatureProcessor->GetTerrainBounds();
            m_shaderResourceGroup->SetConstant(m_worldBoundsMinIndex, worldBounds.GetMin());
            m_shaderResourceGroup->SetConstant(m_worldBoundsMaxIndex, worldBounds.GetMax());
        }
        else
        {
            m_shaderResourceGroup->SetConstant(m_worldBoundsMinIndex, AZ::Vector3(0.0, 0.0, 0.0));
            m_shaderResourceGroup->SetConstant(m_worldBoundsMaxIndex, AZ::Vector3(0.0, 0.0, 0.0));
        }

        m_shaderResourceGroup->SetConstant(m_currentViewPositionIndex, m_currentViewPosition);
        m_shaderResourceGroup->SetConstant(m_previousViewPositionIndex, m_previousViewPosition);

        m_shaderResourceGroup->SetConstantArray(m_previousClipmapCentersIndex, m_previousClipmapCenters);
        m_shaderResourceGroup->SetConstantArray(m_currentClipmapCentersIndex, m_currentClipmapCenters);

        m_shaderResourceGroup->SetConstant(m_fullUpdateFlagIndex, m_fullUpdateFlag);
        m_fullUpdateFlag = 0;

        ComputePass::FrameBeginInternal(params);
    }

    void TerrainMacroTextureClipmapGenerationPass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            auto terrainSrg = terrainFeatureProcessor->GetTerrainShaderResourceGroup();
            if (terrainSrg)
            {
                if (!terrainSrg->IsQueuedForCompile())
                {
                    terrainSrg->Compile();
                }
                BindSrg(terrainFeatureProcessor->GetTerrainShaderResourceGroup()->GetRHIShaderResourceGroup());
            }

            auto material = terrainFeatureProcessor->GetMaterial();
            if (material)
            {
                BindSrg(material->GetRHIShaderResourceGroup());
            }
        }

        ComputePass::CompileResources(context);
    }

    void TerrainMacroTextureClipmapGenerationPass::UpdateClipmapData()
    {
        AZ::RPI::ViewPtr view = GetView();
        AZ_Assert(view, "TerrainMacroTextureClipmapGenerationPass should have the MainCamera as the view.");

        m_previousViewPosition = m_currentViewPosition;
        m_currentViewPosition = view->GetViewToWorldMatrix().GetTranslation();

        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (!terrainFeatureProcessor)
        {
            return;
        }

        const AZ::Aabb worldBounds = terrainFeatureProcessor->GetTerrainBounds();
        const AZ::Vector2 worldSize = AZ::Vector2(
            worldBounds.GetMax().GetX() - worldBounds.GetMin().GetX(), worldBounds.GetMax().GetY() - worldBounds.GetMin().GetY());
        const AZ::Vector2 viewTranslation = AZ::Vector2(
            m_currentViewPosition.GetX() - m_previousViewPosition.GetX(), m_currentViewPosition.GetY() - m_previousViewPosition.GetY());
        const AZ::Vector2 normalizedViewTranslation = viewTranslation / worldSize;

        float clipmapScale = 1.0f;
        for (int32_t clipmapIndex = ClipmapStackSize; clipmapIndex >= 0; --clipmapIndex)
        {
            AZ::Vector4& currentClipmapCenter = m_currentClipmapCenters[clipmapIndex];
            AZ::Vector4& previousClipmapCenter = m_previousClipmapCenters[clipmapIndex];

            previousClipmapCenter = currentClipmapCenter;

            const AZ::Vector2 scaledTranslation = normalizedViewTranslation * clipmapScale;
            // If normalized translation on a certain level of clipmap goes out of the current clipmap representation,
            // a full update will be triggered and the center will be reset to the center.
            if (AZStd::abs(scaledTranslation.GetX()) >= 1.0f || AZStd::abs(scaledTranslation.GetY()) >= 1.0f)
            {
                m_fullUpdateFlag = m_fullUpdateFlag | (1 << clipmapIndex);
                currentClipmapCenter = AZ::Vector4(0.5f, 0.5f, 0.0f, 0.0f);
            }
            else
            {
                currentClipmapCenter = currentClipmapCenter + AZ::Vector4(scaledTranslation.GetX(), scaledTranslation.GetY(), 0.0f, 0.0f);

                float centerX = currentClipmapCenter.GetX();
                float centerY = currentClipmapCenter.GetY();

                AZ_Assert((centerX > -1.0f) && (centerX < 2.0f) && (centerY > -1.0f) && (centerY < 2.0f),
                    "The new translated clipmap center should be within this range. Otherwise it should fall into the other if branch and reset the center to (0.5, 0.5).");

                // Equivalent to modulation.
                // It wraps around to use toroidal addressing.
                centerX -= centerX > 1.0f ? 1.0f : 0.0f;
                centerX += centerX < 0.0f ? 1.0f : 0.0f;

                centerY -= centerY > 1.0f ? 1.0f : 0.0f;
                centerY += centerY < 0.0f ? 1.0f : 0.0f;

                currentClipmapCenter = AZ::Vector4(centerX, centerY, 0.0f, 0.0f);
            }

            clipmapScale *= 2.0f;
        }
    }

} // namespace Terrain
