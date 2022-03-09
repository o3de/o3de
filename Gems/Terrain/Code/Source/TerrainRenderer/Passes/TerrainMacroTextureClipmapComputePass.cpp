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

    void ProcessPassAttachment(
        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool,
        AZ::RPI::Ptr<AZ::RPI::PassAttachment> passAttachment,
        AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage)
    {
        passAttachment->Update();

        passAttachment->m_lifetime = AZ::RHI::AttachmentLifetimeType::Imported;

        AZ::RHI::ImageDescriptor& imageDesc = passAttachment->m_descriptor.m_image;
        imageDesc.m_bindFlags |= AZ::RHI::ImageBindFlags::ShaderReadWrite;

        AZ::RHI::ClearValue clearValue = AZ::RHI::ClearValue::CreateVector4Float(0.0f, 0.0f, 0.0f, 1.0f);
        attachmentImage = AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(passAttachment->m_path.GetCStr()), &clearValue, nullptr);

        passAttachment->m_path = attachmentImage->GetAttachmentId();
        passAttachment->m_importedResource = attachmentImage;
    }

    void TerrainMacroTextureClipmapGenerationPass::BuildInternal()
    {
        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

        AZ_Assert(m_ownedAttachments.size() == ClipmapStackSize + 1, "The pass should own a stack of clipmaps and a mipmap for the bottom image.");

        for (uint32_t i = 0; i < ClipmapStackSize; ++i)
        {
            AZ::RPI::Ptr<AZ::RPI::PassAttachment> colorStack = m_ownedAttachments[i];
            AZ::RPI::Ptr<AZ::RPI::PassAttachment> normalStack = m_ownedAttachments[i + ClipmapStackSize + 1];
            ProcessPassAttachment(pool, colorStack, m_colorClipmapStacks[i]);
            ProcessPassAttachment(pool, normalStack, m_normalClipmapStacks[i]);
        }

        {
            AZ::RPI::Ptr<AZ::RPI::PassAttachment> colorPyramid = m_ownedAttachments[ClipmapStackSize];
            AZ::RPI::Ptr<AZ::RPI::PassAttachment> normalPyramid = m_ownedAttachments[ClipmapStackSize * 2 + 1];
            ProcessPassAttachment(pool, colorPyramid, m_colorClipmapPyramid);
            ProcessPassAttachment(pool, normalPyramid, m_normalClipmapPyramid);
        }
    }

    void TerrainMacroTextureClipmapGenerationPass::InitializeInternal()
    {
        for (uint32_t clipmapIndex = 0; clipmapIndex <= ClipmapStackSize; ++clipmapIndex)
        {
            m_clipmapData.SetPreviousClipmapCenter(AZ::Vector2(0.5f, 0.5f), clipmapIndex);
            m_clipmapData.SetCurrentClipmapCenter(AZ::Vector2(0.5f, 0.5f), clipmapIndex);

            m_clipmapData.m_fullUpdateFlag |= (1 << clipmapIndex);
        }

        m_clipmapData.m_previousViewPosition = AZ::Vector3::CreateZero();
        m_clipmapData.m_currentViewPosition = AZ::Vector3::CreateZero();

        m_colorClipmapStackIndex.Reset();
        m_colorClipmapPyramidIndex.Reset();

        ComputePass::InitializeInternal();
    }

    void TerrainMacroTextureClipmapGenerationPass::FrameBeginInternal(FramePrepareParams params)
    {
        UpdateClipmapData();

        [[maybe_unused]] size_t x = sizeof(ClipmapData);

        AZ::RPI::Scene* scene = m_pipeline->GetScene();
        TerrainFeatureProcessor* terrainFeatureProcessor = scene->GetFeatureProcessor<TerrainFeatureProcessor>();
        if (terrainFeatureProcessor)
        {
            const AZ::Aabb worldBounds = terrainFeatureProcessor->GetTerrainBounds();
            m_clipmapData.m_worldBoundsMin = worldBounds.GetMin();
            m_clipmapData.m_worldBoundsMax = worldBounds.GetMax();
            m_clipmapData.SetMaxRenderSize(worldBounds.GetMax() - worldBounds.GetMin());
        }
        else
        {
            const AZ::Vector3 Zero = AZ::Vector3::CreateZero();
            m_clipmapData.m_worldBoundsMin = Zero;
            m_clipmapData.m_worldBoundsMax = Zero;
            m_clipmapData.SetMaxRenderSize(Zero);
        }

        m_shaderResourceGroup->SetConstant(m_clipmapDataIndex, m_clipmapData);
        m_clipmapData.m_fullUpdateFlag = 0;

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

        memcpy(&m_clipmapData.m_previousViewPosition, &m_clipmapData.m_currentViewPosition, sizeof(m_clipmapData.m_previousViewPosition));
        m_clipmapData.m_currentViewPosition = view->GetViewToWorldMatrix().GetTranslation();

        const AZ::Vector2 maxRenderSize = AZ::Vector2(m_clipmapData.m_maxRenderSize[0], m_clipmapData.m_maxRenderSize[1]);
        const AZ::Vector2 viewTranslation = AZ::Vector2(
            m_clipmapData.m_currentViewPosition.GetX() - m_clipmapData.m_previousViewPosition.GetX(),
            m_clipmapData.m_currentViewPosition.GetY() - m_clipmapData.m_previousViewPosition.GetY());
        const AZ::Vector2 normalizedViewTranslation = viewTranslation / maxRenderSize;

        float clipmapScale = 1.0f;
        for (int32_t clipmapIndex = ClipmapStackSize; clipmapIndex >= 0; --clipmapIndex)
        {
            m_clipmapData.m_clipmapCenters[clipmapIndex].SetX(m_clipmapData.m_clipmapCenters[clipmapIndex].GetZ());
            m_clipmapData.m_clipmapCenters[clipmapIndex].SetY(m_clipmapData.m_clipmapCenters[clipmapIndex].GetW());

            const AZ::Vector2 scaledTranslation = normalizedViewTranslation * clipmapScale;
            // If normalized translation on a certain level of clipmap goes out of the current clipmap representation,
            // a full update will be triggered and the center will be reset to the center.
            if (AZStd::abs(scaledTranslation.GetX()) >= 1.0f || AZStd::abs(scaledTranslation.GetY()) >= 1.0f)
            {
                m_clipmapData.m_fullUpdateFlag |= (1 << clipmapIndex);
                m_clipmapData.SetCurrentClipmapCenter(AZ::Vector2(0.5f, 0.5f), clipmapIndex);
            }
            else
            {
                AZ::Vector2 clipmapCenter = AZ::Vector2(
                    m_clipmapData.m_clipmapCenters[clipmapIndex].GetZ(),
                    m_clipmapData.m_clipmapCenters[clipmapIndex].GetW());

                clipmapCenter = clipmapCenter + scaledTranslation;

                float centerX = clipmapCenter.GetX();
                float centerY = clipmapCenter.GetY();

                AZ_Assert((centerX > -1.0f) && (centerX < 2.0f) && (centerY > -1.0f) && (centerY < 2.0f),
                    "The new translated clipmap center should be within this range. Otherwise it should fall into the other if branch and reset the center to (0.5, 0.5).");

                // Equivalent to modulation.
                // It wraps around to use toroidal addressing.
                centerX -= centerX > 1.0f ? 1.0f : 0.0f;
                centerX += centerX < 0.0f ? 1.0f : 0.0f;

                centerY -= centerY > 1.0f ? 1.0f : 0.0f;
                centerY += centerY < 0.0f ? 1.0f : 0.0f;

                clipmapCenter = AZ::Vector2(centerX, centerY);

                m_clipmapData.SetCurrentClipmapCenter(clipmapCenter, clipmapIndex);
            }

            clipmapScale *= 2.0f;
        }
    }

    void TerrainMacroTextureClipmapGenerationPass::ClipmapData::SetPreviousClipmapCenter(const AZ::Vector2& clipmapCenter, uint32_t clipmapLevel)
    {
        m_clipmapCenters[clipmapLevel].SetX(clipmapCenter.GetX());
        m_clipmapCenters[clipmapLevel].SetY(clipmapCenter.GetY());
    }

    void TerrainMacroTextureClipmapGenerationPass::ClipmapData::SetCurrentClipmapCenter(const AZ::Vector2& clipmapCenter, uint32_t clipmapLevel)
    {
        m_clipmapCenters[clipmapLevel].SetZ(clipmapCenter.GetX());
        m_clipmapCenters[clipmapLevel].SetW(clipmapCenter.GetY());
    }

    void TerrainMacroTextureClipmapGenerationPass::ClipmapData::SetMaxRenderSize(const AZ::Vector3& maxRenderSize)
    {
        memcpy(&m_maxRenderSize, &maxRenderSize, sizeof(m_maxRenderSize));
    }
} // namespace Terrain
