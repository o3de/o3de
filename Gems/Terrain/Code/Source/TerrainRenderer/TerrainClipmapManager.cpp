/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/TerrainClipmapManager.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

namespace Terrain
{
    namespace
    {
        [[maybe_unused]] static const char* TerrainClipmapManagerName = "TerrainClipmapManager";
        static constexpr const char* MacroColorClipmapString = "MacroColorClipmaps";
        static constexpr const char* MacroNormalClipmapString = "MacroNormalClipmaps";
        static constexpr const char* DetailColorClipmapString = "DetailColorClipmaps";
        static constexpr const char* DetailNormalClipmapString = "DetailNormalClipmaps";
        static constexpr const char* DetailHeightClipmapString = "DetailHeightClipmaps";
        static constexpr const char* DetailMiscClipmapString = "DetailMiscClipmaps";
    }

    namespace TerrainSrgInputs
    {
        static constexpr const char* ClipmapData = "m_clipmapData";
        static constexpr const char* MacroColorClipmaps = "m_macroColorClipmaps";
        static constexpr const char* MacroNormalClipmaps = "m_macroNormalClipmaps";
        static constexpr const char* DetailColorClipmaps = "m_detailColorClipmaps";
        static constexpr const char* DetailNormalClipmaps = "m_detailNormalClipmaps";
        static constexpr const char* DetailHeightClipmaps = "m_detailHeightClipmaps";
        static constexpr const char* DetailMiscClipmaps = "m_detailMiscClipmaps";
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
        const AZ::RHI::ShaderResourceGroupLayout* terrainSrgLayout = terrainSrg->GetLayout();

        m_macroColorClipmapsIndex = terrainSrgLayout->FindShaderInputImageIndex(AZ::Name(TerrainSrgInputs::MacroColorClipmaps));
        AZ_Error(TerrainClipmapManagerName, m_macroColorClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", TerrainSrgInputs::MacroColorClipmaps);
        m_macroNormalClipmapsIndex = terrainSrgLayout->FindShaderInputImageIndex(AZ::Name(TerrainSrgInputs::MacroNormalClipmaps));
        AZ_Error(TerrainClipmapManagerName, m_macroNormalClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", TerrainSrgInputs::MacroNormalClipmaps);
        m_detailColorClipmapsIndex = terrainSrgLayout->FindShaderInputImageIndex(AZ::Name(TerrainSrgInputs::DetailColorClipmaps));
        AZ_Error(TerrainClipmapManagerName, m_detailColorClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", TerrainSrgInputs::DetailColorClipmaps);
        m_detailNormalClipmapsIndex = terrainSrgLayout->FindShaderInputImageIndex(AZ::Name(TerrainSrgInputs::DetailNormalClipmaps));
        AZ_Error(TerrainClipmapManagerName, m_detailNormalClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", TerrainSrgInputs::DetailNormalClipmaps);
        m_detailHeightClipmapsIndex = terrainSrgLayout->FindShaderInputImageIndex(AZ::Name(TerrainSrgInputs::DetailHeightClipmaps));
        AZ_Error(TerrainClipmapManagerName, m_detailHeightClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", TerrainSrgInputs::DetailHeightClipmaps);
        m_detailMiscClipmapsIndex = terrainSrgLayout->FindShaderInputImageIndex(AZ::Name(TerrainSrgInputs::DetailMiscClipmaps));
        AZ_Error(TerrainClipmapManagerName, m_detailMiscClipmapsIndex.IsValid(), "Failed to find terrain srg input image %s.", TerrainSrgInputs::DetailMiscClipmaps);

        m_clipmapDataIndex = terrainSrgLayout->FindShaderInputConstantIndex(AZ::Name(TerrainSrgInputs::ClipmapData));
        AZ_Error(TerrainClipmapManagerName, m_clipmapDataIndex.IsValid(), "Failed to find terrain srg input constant %s.", TerrainSrgInputs::ClipmapData);


        bool IndicesValid = m_macroColorClipmapsIndex.IsValid() && m_macroNormalClipmapsIndex.IsValid() &&
            m_detailColorClipmapsIndex.IsValid() && m_detailNormalClipmapsIndex.IsValid() && m_detailHeightClipmapsIndex.IsValid() &&
            m_detailMiscClipmapsIndex.IsValid() && m_clipmapDataIndex.IsValid();

        m_clipmapsNeedUpdate = true;

        return IndicesValid;
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
        terrainSrg->SetConstant(m_clipmapDataIndex, m_clipmapData);

        if (m_clipmapsNeedUpdate)
        {
            terrainSrg->SetImage(m_macroColorClipmapsIndex, m_clipmaps.at(AZ::Name(MacroColorClipmapString)));
            terrainSrg->SetImage(m_macroNormalClipmapsIndex, m_clipmaps.at(AZ::Name(MacroNormalClipmapString)));
            terrainSrg->SetImage(m_detailColorClipmapsIndex, m_clipmaps.at(AZ::Name(DetailColorClipmapString)));
            terrainSrg->SetImage(m_detailNormalClipmapsIndex, m_clipmaps.at(AZ::Name(DetailNormalClipmapString)));
            terrainSrg->SetImage(m_detailHeightClipmapsIndex, m_clipmaps.at(AZ::Name(DetailHeightClipmapString)));
            terrainSrg->SetImage(m_detailMiscClipmapsIndex, m_clipmaps.at(AZ::Name(DetailMiscClipmapString)));

            m_clipmapsNeedUpdate = false;
        }
    }

    void TerrainClipmapManager::InitializeClipmapData()
    {
        float clipmapScaleInv = 1.0f;
        for (int32_t clipmapIndex = ClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            AZ::Vector4(0.5f).StoreToFloat4(m_clipmapData.m_clipmapCenters[clipmapIndex].data());
            AZ::Vector4(clipmapScaleInv).StoreToFloat4(m_clipmapData.m_clipmapScaleInv[clipmapIndex].data());
            clipmapScaleInv /= 2.0f;
        }

        AZ::Vector4::CreateZero().StoreToFloat4(m_clipmapData.m_viewPosition.data());

        m_clipmapData.m_clipmapSize[0] = ClipmapSizeWidth;
        m_clipmapData.m_clipmapSize[1] = ClipmapSizeHeight;

        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            worldBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

        m_clipmapData.m_worldBounds[0] = worldBounds.GetMin().GetX();
        m_clipmapData.m_worldBounds[1] = worldBounds.GetMin().GetY();
        m_clipmapData.m_worldBounds[2] = worldBounds.GetMax().GetX();
        m_clipmapData.m_worldBounds[3] = worldBounds.GetMax().GetY();

        // Use world size for now.
        const AZ::Vector3 worldSize = worldBounds.GetMax() - worldBounds.GetMin();
        AZ::Vector2(worldSize.GetX(), worldSize.GetY()).StoreToFloat2(m_clipmapData.m_maxRenderSize.data());
    }

    void TerrainClipmapManager::InitializeClipmapImages()
    {
        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

        AZ::RHI::ImageDescriptor imageDesc;
        imageDesc.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
        imageDesc.m_bindFlags = AZ::RHI::ImageBindFlags::ShaderReadWrite;
        imageDesc.m_size = AZ::RHI::Size(ClipmapSizeWidth, ClipmapSizeHeight, 1);
        imageDesc.m_arraySize = ClipmapStackSize;

        AZ::Name macroColorClipmapName = AZ::Name(MacroColorClipmapString);
        AZ::Data::Instance<AZ::RPI::AttachmentImage> macroColorClipmaps =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, macroColorClipmapName, nullptr, nullptr);
        AZ::Name detailColorClipmapName = AZ::Name(DetailColorClipmapString);
        AZ::Data::Instance<AZ::RPI::AttachmentImage> detailColorClipmaps =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailColorClipmapName, nullptr, nullptr);
        AZ::Name detailMiscClipmapName = AZ::Name(DetailMiscClipmapString);
        AZ::Data::Instance<AZ::RPI::AttachmentImage> detailMiscClipmaps =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailMiscClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R8G8B8A8_SNORM;

        AZ::Name macroNormalClipmapName = AZ::Name(MacroNormalClipmapString);
        AZ::Data::Instance<AZ::RPI::AttachmentImage> macroNormalClipmaps =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, macroNormalClipmapName, nullptr, nullptr);
        AZ::Name detailNormalClipmapName = AZ::Name(DetailNormalClipmapString);
        AZ::Data::Instance<AZ::RPI::AttachmentImage> detailNormalClipmaps =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailNormalClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R32_FLOAT;

        AZ::Name detailHeightClipmapName = AZ::Name(DetailHeightClipmapString);
        AZ::Data::Instance<AZ::RPI::AttachmentImage> detailHeightClipmaps =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailHeightClipmapName, nullptr, nullptr);

        m_clipmaps[macroColorClipmapName] = macroColorClipmaps;
        m_clipmaps[detailColorClipmapName] = detailColorClipmaps;
        m_clipmaps[detailMiscClipmapName] = detailMiscClipmaps;
        m_clipmaps[macroNormalClipmapName] = macroNormalClipmaps;
        m_clipmaps[detailNormalClipmapName] = detailNormalClipmaps;
        m_clipmaps[detailHeightClipmapName] = detailHeightClipmaps;
    }

    void TerrainClipmapManager::UpdateClipmapData(const AZ::Vector3& cameraPosition)
    {
        // pass current to previous
        m_clipmapData.m_viewPosition[0] = m_clipmapData.m_viewPosition[2];
        m_clipmapData.m_viewPosition[1] = m_clipmapData.m_viewPosition[3];

        // set new current
        m_clipmapData.m_viewPosition[2] = cameraPosition.GetX();
        m_clipmapData.m_viewPosition[3] = cameraPosition.GetY();

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

    AZ::Data::Instance<AZ::RPI::AttachmentImage> TerrainClipmapManager::GetClipmapImage(const AZ::Name& clipmapName) const
    {
        if (!m_clipmaps.contains(clipmapName))
        {
            AZ_Error("TerrainClipmapGenerationPass", false, "No clipmap %s is defined.", clipmapName.GetCStr());
            return nullptr;
        }

        return m_clipmaps.at(clipmapName);
    }
}
