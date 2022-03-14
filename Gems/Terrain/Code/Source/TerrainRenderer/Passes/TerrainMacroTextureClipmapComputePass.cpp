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
    namespace MacroClipmap
    {
        constexpr const char* colorClipmapName = "MacroColorClipmap";
        constexpr const char* normalClipmapName = "MacroNormalClipmap";
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

        // Color stack
        {
            AZ::RHI::ImageDescriptor imageDesc;
            imageDesc.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
            imageDesc.m_bindFlags = AZ::RHI::ImageBindFlags::ShaderReadWrite;
            imageDesc.m_size = AZ::RHI::Size(ClipmapSizeWidth, ClipmapSizeHeight, 1);
            imageDesc.m_arraySize = ClipmapStackSize;

            m_macroColorClipmaps = AZ::RPI::AttachmentImage::Create(
                *pool.get(), imageDesc, AZ::Name(MacroClipmap::colorClipmapName), nullptr, nullptr);

            AttachImageToSlot(AZ::Name(MacroClipmap::colorClipmapName), m_macroColorClipmaps);
        }

        // Normal stack
        {
            AZ::RHI::ImageDescriptor imageDesc;
            imageDesc.m_format = AZ::RHI::Format::R8G8_UNORM;
            imageDesc.m_bindFlags = AZ::RHI::ImageBindFlags::ShaderReadWrite;
            imageDesc.m_size = AZ::RHI::Size(ClipmapSizeWidth, ClipmapSizeHeight, 1);
            imageDesc.m_arraySize = ClipmapStackSize;

            m_macroColorClipmaps = AZ::RPI::AttachmentImage::Create(
                *pool.get(), imageDesc, AZ::Name(MacroClipmap::normalClipmapName), nullptr, nullptr);

            AttachImageToSlot(AZ::Name(MacroClipmap::normalClipmapName), m_macroColorClipmaps);
        }

        m_clipmapData.m_clipmapSize[0] = ClipmapSizeWidth;
        m_clipmapData.m_clipmapSize[1] = ClipmapSizeHeight;
    }

    void TerrainMacroTextureClipmapGenerationPass::InitializeInternal()
    {
        for (uint32_t clipmapIndex = 0; clipmapIndex <= ClipmapStackSize; ++clipmapIndex)
        {
            m_clipmapData.SetPreviousClipmapCenter(AZ::Vector2(0.5f, 0.5f), clipmapIndex);
            m_clipmapData.SetCurrentClipmapCenter(AZ::Vector2(0.5f, 0.5f), clipmapIndex);
        }

        m_clipmapData.m_viewPosition = AZ::Vector4::CreateZero();

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
            m_clipmapData.m_worldBounds = AZ::Vector4(
                worldBounds.GetMin().GetX(), worldBounds.GetMin().GetY(),
                worldBounds.GetMax().GetX(), worldBounds.GetMax().GetY());

            // Use world size for now.
            const AZ::Vector3 worldSize = worldBounds.GetMax() - worldBounds.GetMin();
            m_clipmapData.SetMaxRenderSize(AZ::Vector2(worldSize.GetX(), worldSize.GetY()));
        }
        else
        {
            m_clipmapData.m_worldBounds = AZ::Vector4::CreateZero();
            m_clipmapData.SetMaxRenderSize(AZ::Vector2::CreateZero());
        }

        m_shaderResourceGroup->SetConstant(m_clipmapDataIndex, m_clipmapData);

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

        m_clipmapData.m_viewPosition.SetX(m_clipmapData.m_viewPosition.GetZ());
        m_clipmapData.m_viewPosition.SetY(m_clipmapData.m_viewPosition.GetW());

        const AZ::Vector3 currentViewPosition = view->GetViewToWorldMatrix().GetTranslation();
        m_clipmapData.m_viewPosition.SetZ(currentViewPosition.GetX());
        m_clipmapData.m_viewPosition.SetW(currentViewPosition.GetY());

        const AZ::Vector2 maxRenderSize = AZ::Vector2(m_clipmapData.m_maxRenderSize[0], m_clipmapData.m_maxRenderSize[1]);
        const AZ::Vector2 viewTranslation = AZ::Vector2(
            m_clipmapData.m_viewPosition.GetZ() - m_clipmapData.m_viewPosition.GetX(),
            m_clipmapData.m_viewPosition.GetW() - m_clipmapData.m_viewPosition.GetY());
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

    void TerrainMacroTextureClipmapGenerationPass::ClipmapData::SetMaxRenderSize(const AZ::Vector2& maxRenderSize)
    {
        m_maxRenderSize[0] = maxRenderSize.GetX();
        m_maxRenderSize[1] = maxRenderSize.GetY();
    }
} // namespace Terrain
