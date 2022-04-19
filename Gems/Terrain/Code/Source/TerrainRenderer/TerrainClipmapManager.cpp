/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainClipmapManager.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] static const char* TerrainClipmapManagerName = "TerrainClipmapManager";
    }

    TerrainClipmapManager::TerrainClipmapManager()
        : m_terrainSrgClipmapImageIndex{
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::MacroColor]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::MacroNormal]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailColor]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailNormal]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailHeight]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailMisc])
        }
    {
    }

    void TerrainClipmapManager::Initialize(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        AZ_Error(TerrainClipmapManagerName, terrainSrg, "terrainSrg must not be null.");
        AZ_Error(TerrainClipmapManagerName, !m_isInitialized, "Already initialized.");

        if (!terrainSrg || m_isInitialized)
        {
            return;
        }

        m_isInitialized = UpdateSrgIndices(terrainSrg);

        InitializeClipmapData();
        InitializeClipmapImages();
    }
    
    bool TerrainClipmapManager::UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        for (uint32_t i = 0; i < ClipmapName::Count; ++i)
        {
            terrainSrg->SetImage(m_terrainSrgClipmapImageIndex[i], m_clipmaps[i]);
        }

        return true;
    }

    bool TerrainClipmapManager::IsInitialized() const
    {
        return m_isInitialized;
    }

    void TerrainClipmapManager::Reset()
    {
        m_isInitialized = false;
    }

    void TerrainClipmapManager::Update(const AZ::Vector3& cameraPosition, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        UpdateClipmapData(cameraPosition);
        terrainSrg->SetConstant(m_terrainSrgClipmapDataIndex, m_clipmapData);
    }

    void TerrainClipmapManager::ImportClipmap(ClipmapName clipmapName, AZ::RHI::FrameGraphAttachmentInterface attachmentDatabase) const
    {
        auto clipmap = m_clipmaps[clipmapName];
        attachmentDatabase.ImportImage(clipmap->GetAttachmentId(), clipmap->GetRHIImage());
    }

    void TerrainClipmapManager::UseClipmap(ClipmapName clipmapName, AZ::RHI::ScopeAttachmentAccess access, AZ::RHI::FrameGraphInterface frameGraph) const
    {
        auto clipmap = m_clipmaps[clipmapName];

        AZ::RHI::ImageScopeAttachmentDescriptor desc;
        desc.m_imageViewDescriptor = clipmap->GetImageView()->GetDescriptor();
        desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;
        desc.m_attachmentId = clipmap->GetAttachmentId();
        frameGraph.UseShaderAttachment(desc, access);
    }


    void TerrainClipmapManager::InitializeClipmapData()
    {
        float clipmapScaleInv = 1.0f;
        for (int32_t clipmapIndex = DetailClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            AZ::Vector4(0.5f).StoreToFloat4(m_clipmapData.m_clipmapCenters[clipmapIndex].data());
            AZ::Vector4(clipmapScaleInv).StoreToFloat4(m_clipmapData.m_clipmapScaleInv[clipmapIndex].data());
            clipmapScaleInv /= 2.0f;
        }

        AZ::Vector2::CreateZero().StoreToFloat2(m_clipmapData.m_previousViewPosition.data());
        AZ::Vector2::CreateZero().StoreToFloat2(m_clipmapData.m_currentViewPosition.data());

        m_clipmapData.m_clipmapSize[0] = ClipmapSizeWidth;
        m_clipmapData.m_clipmapSize[1] = ClipmapSizeHeight;

        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            worldBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

        m_clipmapData.m_worldBoundsMin[0] = worldBounds.GetMin().GetX();
        m_clipmapData.m_worldBoundsMin[1] = worldBounds.GetMin().GetY();
        m_clipmapData.m_worldBoundsMax[0] = worldBounds.GetMax().GetX();
        m_clipmapData.m_worldBoundsMax[1] = worldBounds.GetMax().GetY();

        // Use world size for now.
        const AZ::Vector3 worldSize = worldBounds.GetMax() - worldBounds.GetMin();
        AZ::Vector2(worldSize.GetX(), worldSize.GetY()).StoreToFloat2(m_clipmapData.m_maxRenderSize.data());
    }

    void TerrainClipmapManager::InitializeClipmapImages()
    {
        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

        AZ::RHI::ImageDescriptor imageDesc;
        imageDesc.m_bindFlags = AZ::RHI::ImageBindFlags::ShaderReadWrite;
        imageDesc.m_size = AZ::RHI::Size(ClipmapSizeWidth, ClipmapSizeHeight, 1);

        // TODO: Test and find the most suitable precision for color map.
        imageDesc.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
        imageDesc.m_arraySize = MacroClipmapStackSize;

        AZ::Name macroColorClipmapName = AZ::Name("MacroColorClipmaps");
        m_clipmaps[ClipmapName::MacroColor] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, macroColorClipmapName, nullptr, nullptr);

        imageDesc.m_arraySize = DetailClipmapStackSize;

        AZ::Name detailColorClipmapName = AZ::Name("DetailColorClipmaps");
        m_clipmaps[ClipmapName::DetailColor] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailColorClipmapName, nullptr, nullptr);

        AZ::Name detailMiscClipmapName = AZ::Name("DetailMiscClipmaps");
        m_clipmaps[ClipmapName::DetailMisc] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailMiscClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R16G16_SNORM;
        imageDesc.m_arraySize = MacroClipmapStackSize;

        AZ::Name macroNormalClipmapName = AZ::Name("MacroNormalClipmaps");
        m_clipmaps[ClipmapName::MacroNormal] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, macroNormalClipmapName, nullptr, nullptr);

        imageDesc.m_arraySize = DetailClipmapStackSize;

        AZ::Name detailNormalClipmapName = AZ::Name("DetailNormalClipmaps");
        m_clipmaps[ClipmapName::DetailNormal] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailNormalClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R32_FLOAT;

        AZ::Name detailHeightClipmapName = AZ::Name("DetailHeightClipmaps");
        m_clipmaps[ClipmapName::DetailHeight] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailHeightClipmapName, nullptr, nullptr);
    }

    void TerrainClipmapManager::UpdateClipmapData(const AZ::Vector3& cameraPosition)
    {
        // pass current to previous
        m_clipmapData.m_previousViewPosition[0] = m_clipmapData.m_currentViewPosition[0];
        m_clipmapData.m_previousViewPosition[1] = m_clipmapData.m_currentViewPosition[1];

        // set new current
        m_clipmapData.m_currentViewPosition[0] = cameraPosition.GetX();
        m_clipmapData.m_currentViewPosition[1] = cameraPosition.GetY();

        const AZ::Vector2 maxRenderSize = AZ::Vector2(m_clipmapData.m_maxRenderSize[0], m_clipmapData.m_maxRenderSize[1]);
        const AZ::Vector2 viewTranslation = AZ::Vector2(
            m_clipmapData.m_currentViewPosition[0] - m_clipmapData.m_previousViewPosition[0],
            m_clipmapData.m_currentViewPosition[1] - m_clipmapData.m_previousViewPosition[1]);
        const AZ::Vector2 normalizedViewTranslation = viewTranslation / maxRenderSize;

        float clipmapScale = 1.0f;
        for (int32_t clipmapIndex = DetailClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
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

    AZ::Data::Instance<AZ::RPI::AttachmentImage> TerrainClipmapManager::GetClipmapImage(ClipmapName clipmapName) const
    {
        AZ_Assert(clipmapName < ClipmapName::Count, "Must be a valid ClipmapName enum.");

        return m_clipmaps[clipmapName];
    }
}
