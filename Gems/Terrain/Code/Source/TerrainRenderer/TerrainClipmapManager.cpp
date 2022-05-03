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

    //! Calculate how many layers of clipmap is needed.
    //! Final result must be less or equal than the MacroClipmapStackSizeMax/DetailClipmapStackSizeMax.
    uint32_t ClipmapConfiguration::CalculateMacroClipmapStackSize() const
    {
        float clipmapSize = aznumeric_cast<float>(m_clipmapSize);
        float minRenderDistance = clipmapSize / m_macroClipmapMaxResolution / 2.0f; // Render distance is half of the image.

        uint32_t stackSizeNeeded = 1u;
        // Add more layers until it meets the max resolution.
        for (float radius = m_macroClipmapMaxRenderRadius; radius > minRenderDistance; radius /= m_macroClipmapScaleBase)
        {
            ++stackSizeNeeded;
        }

        AZ_Assert(stackSizeNeeded <= MacroClipmapStackSizeMax, "Stack size needed is bigger than max. Consider increasing MacroClipmapStackSizeMax and the same name constant in TerrainSrg.azsli.");

        return stackSizeNeeded;
    }

    uint32_t ClipmapConfiguration::CalculateDetailClipmapStackSize() const
    {
        float clipmapSize = aznumeric_cast<float>(m_clipmapSize);
        float minRenderDistance = clipmapSize / m_detailClipmapMaxResolution / 2.0f; // Render distance is half of the image.

        uint32_t stackSizeNeeded = 1u;
        // Add more layers until it meets the max resolution.
        for (float radius = m_detailClipmapMaxRenderRadius; radius > minRenderDistance; radius /= m_detailClipmapScaleBase)
        {
            ++stackSizeNeeded;
        }

        AZ_Assert(stackSizeNeeded <= MacroClipmapStackSizeMax, "Stack size needed is bigger than max. Consider increasing DetailClipmapStackSizeMax and the same name constant in TerrainSrg.azsli.");

        return stackSizeNeeded;
    }

    TerrainClipmapManager::TerrainClipmapManager()
        : m_terrainSrgClipmapImageIndex{
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::MacroColor]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::MacroNormal]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailColor]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailNormal]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailHeight]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailRoughness]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailSpecularF0]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailMetalness]),
            AZ::RHI::ShaderInputNameIndex(ClipmapImageShaderInput[ClipmapName::DetailOcclusion])
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

        m_macroClipmapStackSize = m_config.CalculateMacroClipmapStackSize();
        m_detailClipmapStackSize = m_config.CalculateDetailClipmapStackSize();

        InitializeClipmapBounds();
        InitializeClipmapData();
        InitializeClipmapImages();

        m_isInitialized = UpdateSrgIndices(terrainSrg);
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

    void TerrainClipmapManager::InitializeClipmapBounds()
    {
        m_macroClipmapBounds.resize(m_macroClipmapStackSize);
        float clipToWorldScale = m_config.m_macroClipmapMaxRenderRadius * 2.0f / m_config.m_clipmapSize;
        for (int32_t clipmapIndex = m_macroClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            ClipmapBoundsDescriptor desc;
            desc.m_size = m_config.m_clipmapSize;
            desc.m_worldSpaceCenter = AZ::Vector2::CreateZero();
            desc.m_clipmapUpdateMultiple = 4;
            desc.m_clipToWorldScale = clipToWorldScale;

            m_macroClipmapBounds[clipmapIndex] = ClipmapBounds(desc);

            clipToWorldScale /= m_config.m_macroClipmapScaleBase;
        }

        m_detailClipmapBounds.resize(m_detailClipmapStackSize);
        clipToWorldScale = m_config.m_detailClipmapMaxRenderRadius * 2.0f / m_config.m_clipmapSize;
        for (int32_t clipmapIndex = m_detailClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            ClipmapBoundsDescriptor desc;
            desc.m_size = m_config.m_clipmapSize;
            desc.m_worldSpaceCenter = AZ::Vector2::CreateZero();
            desc.m_clipmapUpdateMultiple = 4;
            desc.m_clipToWorldScale = clipToWorldScale;

            m_detailClipmapBounds[clipmapIndex] = ClipmapBounds(desc);

            clipToWorldScale /= m_config.m_detailClipmapScaleBase;
        }
    }

    void TerrainClipmapManager::InitializeClipmapData()
    {
        AZ::Vector2::CreateZero().StoreToFloat2(m_clipmapData.m_previousViewPosition.data());
        AZ::Vector2::CreateZero().StoreToFloat2(m_clipmapData.m_currentViewPosition.data());

        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            worldBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

        m_clipmapData.m_worldBoundsMin[0] = worldBounds.GetMin().GetX();
        m_clipmapData.m_worldBoundsMin[1] = worldBounds.GetMin().GetY();
        m_clipmapData.m_worldBoundsMax[0] = worldBounds.GetMax().GetX();
        m_clipmapData.m_worldBoundsMax[1] = worldBounds.GetMax().GetY();

        m_clipmapData.m_macroClipmapMaxRenderRadius = m_config.m_macroClipmapMaxRenderRadius;
        m_clipmapData.m_detailClipmapMaxRenderRadius = m_config.m_detailClipmapMaxRenderRadius;

        m_clipmapData.m_macroClipmapScaleBase = m_config.m_macroClipmapScaleBase;
        m_clipmapData.m_detailClipmapScaleBase = m_config.m_detailClipmapScaleBase;

        m_clipmapData.m_macroClipmapStackSize = m_macroClipmapStackSize;
        m_clipmapData.m_detailClipmapStackSize = m_detailClipmapStackSize;

        m_clipmapData.m_clipmapSize = aznumeric_cast<float>(m_config.m_clipmapSize);

        float clipmapScaleInv = 1.0f;
        for (int32_t clipmapIndex = m_macroClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            AZ::Vector4(0.5f).StoreToFloat4(m_clipmapData.m_macroClipmapCenters[clipmapIndex].data());
            m_clipmapData.m_clipmapScaleInv[clipmapIndex][0] = clipmapScaleInv;
            clipmapScaleInv /= m_config.m_macroClipmapScaleBase;
        }
        clipmapScaleInv = 1.0f;
        for (int32_t clipmapIndex = m_detailClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            AZ::Vector4(0.5f).StoreToFloat4(m_clipmapData.m_detailClipmapCenters[clipmapIndex].data());
            m_clipmapData.m_clipmapScaleInv[clipmapIndex][1] = clipmapScaleInv;
            clipmapScaleInv /= m_config.m_detailClipmapScaleBase;
        }
    }

    void TerrainClipmapManager::InitializeClipmapImages()
    {
        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

        AZ::RHI::ImageDescriptor imageDesc;
        imageDesc.m_bindFlags = AZ::RHI::ImageBindFlags::ShaderReadWrite;
        imageDesc.m_size = AZ::RHI::Size(m_config.m_clipmapSize, m_config.m_clipmapSize, 1);

        // TODO: Test and find the most suitable precision for color map.
        imageDesc.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
        imageDesc.m_arraySize = aznumeric_cast<uint16_t>(m_macroClipmapStackSize);

        AZ::Name macroColorClipmapName = AZ::Name("MacroColorClipmaps");
        m_clipmaps[ClipmapName::MacroColor] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, macroColorClipmapName, nullptr, nullptr);

        imageDesc.m_arraySize = aznumeric_cast<uint16_t>(m_detailClipmapStackSize);

        AZ::Name detailColorClipmapName = AZ::Name("DetailColorClipmaps");
        m_clipmaps[ClipmapName::DetailColor] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailColorClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R16G16_SNORM;
        imageDesc.m_arraySize = aznumeric_cast<uint16_t>(m_macroClipmapStackSize);

        AZ::Name macroNormalClipmapName = AZ::Name("MacroNormalClipmaps");
        m_clipmaps[ClipmapName::MacroNormal] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, macroNormalClipmapName, nullptr, nullptr);

        imageDesc.m_arraySize = aznumeric_cast<uint16_t>(m_detailClipmapStackSize);

        AZ::Name detailNormalClipmapName = AZ::Name("DetailNormalClipmaps");
        m_clipmaps[ClipmapName::DetailNormal] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailNormalClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R32_FLOAT;

        AZ::Name detailHeightClipmapName = AZ::Name("DetailHeightClipmaps");
        m_clipmaps[ClipmapName::DetailHeight] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailHeightClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R8_UNORM;

        AZ::Name detailRoughnessClipmapName = AZ::Name("DetailRoughnessClipmaps");
        m_clipmaps[ClipmapName::DetailRoughness] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailRoughnessClipmapName, nullptr, nullptr);

        AZ::Name detailSpecularF0ClipmapName = AZ::Name("DetailSpecularF0Clipmaps");
        m_clipmaps[ClipmapName::DetailSpecularF0] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailSpecularF0ClipmapName, nullptr, nullptr);

        AZ::Name detailMetalnessClipmapName = AZ::Name("DetailMetalnessClipmaps");
        m_clipmaps[ClipmapName::DetailMetalness] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailMetalnessClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R16_FLOAT;

        AZ::Name detailOcclusionClipmapName = AZ::Name("DetailOcclusionClipmaps");
        m_clipmaps[ClipmapName::DetailOcclusion] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailOcclusionClipmapName, nullptr, nullptr);
    }

    void TerrainClipmapManager::UpdateClipmapData(const AZ::Vector3& cameraPosition)
    {
        // pass current to previous
        m_clipmapData.m_previousViewPosition[0] = m_clipmapData.m_currentViewPosition[0];
        m_clipmapData.m_previousViewPosition[1] = m_clipmapData.m_currentViewPosition[1];

        // set new current
        AZ::Vector2 currentViewPosition = AZ::Vector2(cameraPosition.GetX(), cameraPosition.GetY());
        currentViewPosition.StoreToFloat2(m_clipmapData.m_currentViewPosition.data());

        auto UpdateCenter = [&](ClipmapBounds& clipmapBounds, AZStd::array<float, 4>& shaderData) -> void
        {
            clipmapBounds.UpdateCenter(currentViewPosition);

            // copy current to previous slot
            shaderData[0] = shaderData[2];
            shaderData[1] = shaderData[3];

            // write current center
            clipmapBounds.GetNormalizedCenter().StoreToFloat2(&shaderData[2]);
        };

        // macro clipmap data:
        for (uint32_t clipmapIndex = 0; clipmapIndex < m_macroClipmapStackSize; ++clipmapIndex)
        {
            UpdateCenter(m_macroClipmapBounds[clipmapIndex], m_clipmapData.m_macroClipmapCenters[clipmapIndex]);
        }

        // detail clipmap data:
        for (int32_t clipmapIndex = m_detailClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            UpdateCenter(m_detailClipmapBounds[clipmapIndex], m_clipmapData.m_detailClipmapCenters[clipmapIndex]);
        }
    }

    AZ::Data::Instance<AZ::RPI::AttachmentImage> TerrainClipmapManager::GetClipmapImage(ClipmapName clipmapName) const
    {
        AZ_Assert(clipmapName < ClipmapName::Count, "Must be a valid ClipmapName enum.");

        return m_clipmaps[clipmapName];
    }
}
