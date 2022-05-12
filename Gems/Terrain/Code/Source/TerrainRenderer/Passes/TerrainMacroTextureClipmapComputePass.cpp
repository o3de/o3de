/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Terrain/Passes/TerrainMacroTextureClipmapComputePass.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>

namespace Terrain
{
    namespace MacroClipmap
    {
        constexpr const char* colorClipmapName = "MacroColorClipmap";
        constexpr const char* normalClipmapName = "MacroNormalClipmap";
    } // namespace MacroClipmap

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

        // Color stack
        {
            AZ::RHI::ImageDescriptor imageDesc;
            imageDesc.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
            imageDesc.m_bindFlags = AZ::RHI::ImageBindFlags::ShaderReadWrite;
            imageDesc.m_size = AZ::RHI::Size(ClipmapSizeWidth, ClipmapSizeHeight, 1);
            imageDesc.m_arraySize = ClipmapStackSize;

            m_macroColorClipmaps =
                AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(MacroClipmap::colorClipmapName), nullptr, nullptr);

            AttachImageToSlot(AZ::Name(MacroClipmap::colorClipmapName), m_macroColorClipmaps);
        }

        // Normal stack
        {
            AZ::RHI::ImageDescriptor imageDesc;
            imageDesc.m_format = AZ::RHI::Format::R8G8_UNORM;
            imageDesc.m_bindFlags = AZ::RHI::ImageBindFlags::ShaderReadWrite;
            imageDesc.m_size = AZ::RHI::Size(ClipmapSizeWidth, ClipmapSizeHeight, 1);
            imageDesc.m_arraySize = ClipmapStackSize;

            m_macroColorClipmaps =
                AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, AZ::Name(MacroClipmap::normalClipmapName), nullptr, nullptr);

            AttachImageToSlot(AZ::Name(MacroClipmap::normalClipmapName), m_macroColorClipmaps);
        }

        m_clipmapData.m_clipmapSize[0] = ClipmapSizeWidth;
        m_clipmapData.m_clipmapSize[1] = ClipmapSizeHeight;
    }

    void TerrainMacroTextureClipmapGenerationPass::InitializeInternal()
    {
        float clipmapScaleInv = 1.0f;
        for (int32_t clipmapIndex = ClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            AZ::Vector4(0.5f).StoreToFloat4(m_clipmapData.m_clipmapCenters[clipmapIndex].data());
            AZ::Vector4(clipmapScaleInv).StoreToFloat4(m_clipmapData.m_clipmapScaleInv[clipmapIndex].data());
            clipmapScaleInv /= 2.0f;
        }

        AZ::Vector4::CreateZero().StoreToFloat4(m_clipmapData.m_viewPosition.data());

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
            m_clipmapData.m_worldBounds[0] = worldBounds.GetMin().GetX();
            m_clipmapData.m_worldBounds[1] = worldBounds.GetMin().GetY();
            m_clipmapData.m_worldBounds[2] = worldBounds.GetMax().GetX();
            m_clipmapData.m_worldBounds[3] = worldBounds.GetMax().GetY();

            // Use world size for now.
            const AZ::Vector3 worldSize = worldBounds.GetMax() - worldBounds.GetMin();
            AZ::Vector2(worldSize.GetX(), worldSize.GetY()).StoreToFloat2(m_clipmapData.m_maxRenderSize.data());
        }
        else
        {
            AZ::Vector4::CreateZero().StoreToFloat4(m_clipmapData.m_worldBounds.data());
            AZ::Vector2::CreateZero().StoreToFloat2(m_clipmapData.m_maxRenderSize.data());
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

        // pass current to previous
        m_clipmapData.m_viewPosition[0] = m_clipmapData.m_viewPosition[2];
        m_clipmapData.m_viewPosition[1] = m_clipmapData.m_viewPosition[3];

        // set new current
        const AZ::Vector3 currentViewPosition = view->GetViewToWorldMatrix().GetTranslation();
        m_clipmapData.m_viewPosition[2] = currentViewPosition.GetX();
        m_clipmapData.m_viewPosition[3] = currentViewPosition.GetY();

        const AZ::Vector2 maxRenderSize = AZ::Vector2(m_clipmapData.m_maxRenderSize[0], m_clipmapData.m_maxRenderSize[1]);
        const AZ::Vector2 viewTranslation = AZ::Vector2(
            m_clipmapData.m_viewPosition[2] - m_clipmapData.m_viewPosition[0],
            m_clipmapData.m_viewPosition[3] - m_clipmapData.m_viewPosition[1]);
        const AZ::Vector2 normalizedViewTranslation = viewTranslation / maxRenderSize;

        float clipmapScale = 1.0f;
        for (int32_t clipmapIndex = ClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            m_clipmapData.m_clipmapCenters[clipmapIndex][0] = m_clipmapData.m_clipmapCenters[clipmapIndex][2];
            m_clipmapData.m_clipmapCenters[clipmapIndex][1] = m_clipmapData.m_clipmapCenters[clipmapIndex][3];

            const AZ::Vector2 scaledTranslation = normalizedViewTranslation * clipmapScale;
            // If normalized translation on a certain level of clipmap goes out of the current clipmap representation,
            // a full update will be triggered and the center will be reset to the center.
            if (AZStd::abs(scaledTranslation.GetX()) >= 1.0f || AZStd::abs(scaledTranslation.GetY()) >= 1.0f)
            {
                m_clipmapData.m_clipmapCenters[clipmapIndex][2] = 0.5f;
                m_clipmapData.m_clipmapCenters[clipmapIndex][3] = 0.5f;
            }
            else
            {
                float centerX = m_clipmapData.m_clipmapCenters[clipmapIndex][2] + scaledTranslation.GetX();
                float centerY = m_clipmapData.m_clipmapCenters[clipmapIndex][3] + scaledTranslation.GetY();

                AZ_Assert(
                    (centerX > -1.0f) && (centerX < 2.0f) && (centerY > -1.0f) && (centerY < 2.0f),
                    "The new translated clipmap center should be within this range. Otherwise it should fall into the other if branch and "
                    "reset the center to (0.5, 0.5).");

                // Equivalent to modulation.
                // It wraps around to use toroidal addressing.
                centerX -= centerX > 1.0f ? 1.0f : 0.0f;
                centerX += centerX < 0.0f ? 1.0f : 0.0f;

                centerY -= centerY > 1.0f ? 1.0f : 0.0f;
                centerY += centerY < 0.0f ? 1.0f : 0.0f;

                m_clipmapData.m_clipmapCenters[clipmapIndex][2] = centerX;
                m_clipmapData.m_clipmapCenters[clipmapIndex][3] = centerY;
            }

            clipmapScale *= 2.0f;
        }
    }
} // namespace Terrain
