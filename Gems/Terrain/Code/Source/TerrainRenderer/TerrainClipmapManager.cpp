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
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzCore/Console/Console.h>

namespace Terrain
{
    AZ_CVAR(
        uint32_t,
        r_terrainClipmapDebugOverlay,
        0,
        nullptr,
        AZ::ConsoleFunctorFlags::Null,
        "The clipmap index to be rendered on the screen.\n"
        "0: off\n"
        "1: macro clipmap overlay\n"
        "2: detail clipmap overlay");

    AZ_CVAR(
        uint32_t,
        r_terrainClipmapDebugClipmapId,
        2,
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
        "8: detail occlusion clipmap");

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

    void TerrainClipmapManager::SetConfiguration(const ClipmapConfiguration& config)
    {
        AZ::RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(AZ::Name{ "o_useClipmap" }, AZ::RPI::ShaderOptionValue{ config.m_clipmapEnabled });

        if (!m_isInitialized)
        {
            m_config = config;
            return;
        }

        if (!config.m_clipmapEnabled)
        {
            ClearMacroClipmapImages();
            ClearMacroClipmapGpuBuffer();
            m_config = config;

            return;
        }

        bool macroRecreation = false;
        bool detailRecreation = false;
        bool macroRefresh = false;
        bool detailRefresh = false;

        // If clipmap size is changed, everything will be recreated.
        if (config.m_clipmapSize != m_config.m_clipmapSize)
        {
            macroRecreation = true;
            detailRecreation = true;
        }

        // Check if macro or detail stack size has a change, it will need recreation.
        if (config.CalculateMacroClipmapStackSize() != m_config.CalculateMacroClipmapStackSize())
        {
            macroRecreation = true;
        }

        if (config.CalculateDetailClipmapStackSize() != m_config.CalculateDetailClipmapStackSize())
        {
            detailRecreation = true;
        }

        //! Minor changes can be resolved by updating the full clipmap.
        if (config.m_macroClipmapMaxRenderRadius != m_config.m_macroClipmapMaxRenderRadius ||
            config.m_macroClipmapMaxResolution != m_config.m_macroClipmapMaxResolution ||
            config.m_macroClipmapScaleBase != m_config.m_macroClipmapScaleBase ||
            config.m_macroClipmapMarginSize != m_config.m_macroClipmapMarginSize)
        {
            macroRefresh = true;
        }

        if (config.m_detailClipmapMaxRenderRadius != m_config.m_detailClipmapMaxRenderRadius ||
            config.m_detailClipmapMaxResolution != m_config.m_detailClipmapMaxResolution ||
            config.m_detailClipmapScaleBase != m_config.m_detailClipmapScaleBase ||
            config.m_detailClipmapMarginSize != m_config.m_detailClipmapMarginSize)
        {
            detailRefresh = true;
        }

        if (macroRecreation || detailRecreation || macroRefresh || detailRefresh)
        {
            m_config = config;

            TriggerFullRefresh();

            if (macroRecreation)
            {
                ClearMacroClipmapImages();
                ClearMacroClipmapGpuBuffer();

                QueryMacroClipmapStackSize();

                InitializeMacroClipmapData();
                InitializeMacroClipmapImages();
                InitializeMacroClipmapGpuBuffer();
            }
            else if (macroRefresh)
            {
                InitializeMacroClipmapData();
            }

            if (detailRecreation)
            {
                ClearDetailClipmapImages();
                ClearDetailClipmapGpuBuffer();

                QueryDetailClipmapStackSize();

                InitializeDetailClipmapData();
                InitializeDetailClipmapImages();
                InitializeDetailClipmapGpuBuffer();
            }
            else if (detailRefresh)
            {
                InitializeDetailClipmapData();
            }

            return;
        }

        if (config.m_extendedClipmapMarginSize != m_config.m_extendedClipmapMarginSize ||
            config.m_clipmapBlendSize != m_config.m_clipmapBlendSize)
        {
            m_config = config;

            m_clipmapData.m_extendedClipmapMarginSize = aznumeric_cast<float>(m_config.m_extendedClipmapMarginSize);
            m_clipmapData.m_clipmapBlendSize = aznumeric_cast<float>(m_config.m_clipmapBlendSize);
        }
    }

    bool TerrainClipmapManager::IsClipmapEnabled() const
    {
        return m_config.m_clipmapEnabled;
    }

    void TerrainClipmapManager::Initialize(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        AZ_Error(TerrainClipmapManagerName, terrainSrg, "terrainSrg must not be null.");
        AZ_Error(TerrainClipmapManagerName, !m_isInitialized, "Already initialized.");

        if (!terrainSrg || m_isInitialized)
        {
            return;
        }

        m_terrainSrg = terrainSrg;

        QueryMacroClipmapStackSize();
        QueryDetailClipmapStackSize();

        InitializeMacroClipmapData();
        InitializeDetailClipmapData();
        InitializeMacroClipmapImages();
        InitializeDetailClipmapImages();
        InitializeMacroClipmapGpuBuffer();
        InitializeDetailClipmapGpuBuffer();

        UpdateSrgIndices(terrainSrg);

        TerrainAreaMaterialNotificationBus::Handler::BusConnect();
        TerrainMacroMaterialNotificationBus::Handler::BusConnect();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();

        m_isInitialized = true;
    }
    
    void TerrainClipmapManager::UpdateSrgIndices(AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        for (uint32_t i = 0; i < ClipmapName::Count; ++i)
        {
            terrainSrg->SetImage(m_terrainSrgClipmapImageIndex[i], m_clipmaps[i]);
        }
    }

    bool TerrainClipmapManager::IsInitialized() const
    {
        return m_isInitialized;
    }

    void TerrainClipmapManager::Reset()
    {
        m_isInitialized = false;
        m_fullRefreshClipmaps = true;

        m_terrainSrg = nullptr;

        ClearMacroClipmapImages();
        ClearMacroClipmapGpuBuffer();

        ClearDetailClipmapImages();
        ClearDetailClipmapGpuBuffer();

        TerrainAreaMaterialNotificationBus::Handler::BusDisconnect();
        TerrainMacroMaterialNotificationBus::Handler::BusDisconnect();
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();
    }

    void TerrainClipmapManager::TriggerFullRefresh()
    {
        m_fullRefreshClipmaps = true;
    }

    void TerrainClipmapManager::Update(const AZ::Vector3& cameraPosition, const AZ::RPI::Scene* scene, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        UpdateClipmapData(cameraPosition, scene, terrainSrg);
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
        frameGraph.UseShaderAttachment(desc, access, AZ::RHI::ScopeAttachmentStage::ComputeShader);
    }

    void TerrainClipmapManager::QueryMacroClipmapStackSize()
    {
        m_macroClipmapStackSize = m_config.CalculateMacroClipmapStackSize();
    }

    void TerrainClipmapManager::QueryDetailClipmapStackSize()
    {
        m_detailClipmapStackSize = m_config.CalculateDetailClipmapStackSize();
    }

    void TerrainClipmapManager::InitializeMacroClipmapGpuBuffer()
    {
        AZ::Render::GpuBufferHandler::Descriptor desc;
        desc.m_bufferName = "Macro Clipmap Update Regions";
        desc.m_bufferSrgName = "m_macroClipmapUpdateRegions";
        desc.m_elementSize = sizeof(ClipmapUpdateRegion);
        desc.m_srgLayout = m_terrainSrg->GetLayout();
        m_macroClipmapUpdateRegionsBuffer = AZ::Render::GpuBufferHandler(desc);

        // Reserve the max possible size.
        m_macroClipmapUpdateRegions.reserve(ClipmapBounds::MaxUpdateRegions * m_macroClipmapStackSize);
    }

    void TerrainClipmapManager::InitializeDetailClipmapGpuBuffer()
    {
        AZ::Render::GpuBufferHandler::Descriptor desc;
        desc.m_bufferName = "Detail Clipmap Update Regions";
        desc.m_bufferSrgName = "m_detailClipmapUpdateRegions";
        desc.m_elementSize = sizeof(ClipmapUpdateRegion);
        desc.m_srgLayout = m_terrainSrg->GetLayout();
        m_detailClipmapUpdateRegionsBuffer = AZ::Render::GpuBufferHandler(desc);

        // Reserve the max possible size.
        m_detailClipmapUpdateRegions.reserve(ClipmapBounds::MaxUpdateRegions * m_detailClipmapStackSize);
    }

    void TerrainClipmapManager::ClearMacroClipmapGpuBuffer()
    {
        m_macroClipmapUpdateRegionsBuffer.Release();
    }

    void TerrainClipmapManager::ClearDetailClipmapGpuBuffer()
    {
        m_detailClipmapUpdateRegionsBuffer.Release();
    }

    void TerrainClipmapManager::InitializeMacroClipmapBounds(const AZ::Vector2& center)
    {
        m_macroClipmapBounds.resize(m_macroClipmapStackSize);
        float clipmapToWorldScale = m_config.m_macroClipmapMaxRenderRadius * 2.0f / m_config.m_clipmapSize;
        for (int32_t clipmapIndex = m_macroClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            ClipmapBoundsDescriptor desc;
            desc.m_size = m_config.m_clipmapSize;
            desc.m_worldSpaceCenter = center;
            desc.m_clipmapUpdateMultiple = m_config.m_macroClipmapMarginSize;
            desc.m_clipmapToWorldScale = clipmapToWorldScale;

            m_macroClipmapBounds[clipmapIndex] = ClipmapBounds(desc);

            clipmapToWorldScale /= m_config.m_macroClipmapScaleBase;
        }
    }

    void TerrainClipmapManager::InitializeDetailClipmapBounds(const AZ::Vector2& center)
    {
        m_detailClipmapBounds.resize(m_detailClipmapStackSize);
        float clipmapToWorldScale = m_config.m_detailClipmapMaxRenderRadius * 2.0f / m_config.m_clipmapSize;
        for (int32_t clipmapIndex = m_detailClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            ClipmapBoundsDescriptor desc;
            desc.m_size = m_config.m_clipmapSize;
            desc.m_worldSpaceCenter = center;
            desc.m_clipmapUpdateMultiple = m_config.m_detailClipmapMarginSize;
            desc.m_clipmapToWorldScale = clipmapToWorldScale;

            m_detailClipmapBounds[clipmapIndex] = ClipmapBounds(desc);

            clipmapToWorldScale /= m_config.m_detailClipmapScaleBase;
        }
    }

    void TerrainClipmapManager::InitializeMacroClipmapData()
    {
        m_clipmapData.m_extendedClipmapMarginSize = aznumeric_cast<float>(m_config.m_extendedClipmapMarginSize);
        m_clipmapData.m_clipmapSizeFloat = aznumeric_cast<float>(m_config.m_clipmapSize);
        m_clipmapData.m_clipmapSizeUint = m_config.m_clipmapSize;
        m_clipmapData.m_clipmapBlendSize = aznumeric_cast<float>(m_config.m_clipmapBlendSize);

        m_clipmapData.m_macroClipmapMaxRenderRadius = m_config.m_macroClipmapMaxRenderRadius;
        m_clipmapData.m_macroClipmapScaleBase = m_config.m_macroClipmapScaleBase;
        m_clipmapData.m_macroClipmapStackSize = m_macroClipmapStackSize;
        m_clipmapData.m_macroClipmapMarginSize = aznumeric_cast<float>(m_config.m_macroClipmapMarginSize);
        m_clipmapData.m_validMacroClipmapRadius =
            m_clipmapData.m_clipmapSizeFloat / 2.0f - m_clipmapData.m_macroClipmapMarginSize - m_clipmapData.m_extendedClipmapMarginSize;

        float clipmapToWorldScale = m_config.m_macroClipmapMaxRenderRadius * 2.0f / m_config.m_clipmapSize;
        for (int32_t clipmapIndex = m_macroClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            m_clipmapData.m_clipmapToWorldScale[clipmapIndex].m_macro = clipmapToWorldScale;
            clipmapToWorldScale /= m_config.m_macroClipmapScaleBase;

            m_clipmapData.m_clipmapCenters[clipmapIndex].m_macro = { 0u, 0u };
            m_clipmapData.m_clipmapWorldCenters[clipmapIndex].m_macro = { 0.0f, 0.0f };
        }
    }

    void TerrainClipmapManager::InitializeDetailClipmapData()
    {
        m_clipmapData.m_extendedClipmapMarginSize = aznumeric_cast<float>(m_config.m_extendedClipmapMarginSize);
        m_clipmapData.m_clipmapSizeFloat = aznumeric_cast<float>(m_config.m_clipmapSize);
        m_clipmapData.m_clipmapSizeUint = m_config.m_clipmapSize;
        m_clipmapData.m_clipmapBlendSize = aznumeric_cast<float>(m_config.m_clipmapBlendSize);

        m_clipmapData.m_detailClipmapMaxRenderRadius = m_config.m_detailClipmapMaxRenderRadius;
        m_clipmapData.m_detailClipmapScaleBase = m_config.m_detailClipmapScaleBase;
        m_clipmapData.m_detailClipmapStackSize = m_detailClipmapStackSize;
        m_clipmapData.m_detailClipmapMarginSize = aznumeric_cast<float>(m_config.m_detailClipmapMarginSize);
        m_clipmapData.m_validDetailClipmapRadius =
            m_clipmapData.m_clipmapSizeFloat / 2.0f - m_clipmapData.m_detailClipmapMarginSize - m_clipmapData.m_extendedClipmapMarginSize;

        float clipmapToWorldScale = m_config.m_detailClipmapMaxRenderRadius * 2.0f / m_config.m_clipmapSize;
        for (int32_t clipmapIndex = m_detailClipmapStackSize - 1; clipmapIndex >= 0; --clipmapIndex)
        {
            m_clipmapData.m_clipmapToWorldScale[clipmapIndex].m_detail = clipmapToWorldScale;
            clipmapToWorldScale /= m_config.m_detailClipmapScaleBase;

            m_clipmapData.m_clipmapCenters[clipmapIndex].m_detail = { 0u, 0u };
            m_clipmapData.m_clipmapWorldCenters[clipmapIndex].m_detail = { 0.0f, 0.0f };
        }
    }

    void TerrainClipmapManager::InitializeMacroClipmapImages()
    {
        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

        AZ::RHI::ImageDescriptor imageDesc;
        imageDesc.m_bindFlags = AZ::RHI::ImageBindFlags::ShaderReadWrite;
        imageDesc.m_size = AZ::RHI::Size(m_config.m_clipmapSize, m_config.m_clipmapSize, 1);

        imageDesc.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
        imageDesc.m_arraySize = aznumeric_cast<uint16_t>(m_macroClipmapStackSize);

        AZ::Name macroColorClipmapName = AZ::Name("MacroColorClipmaps");
        m_clipmaps[ClipmapName::MacroColor] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, macroColorClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R16G16_SNORM;
        imageDesc.m_arraySize = aznumeric_cast<uint16_t>(m_macroClipmapStackSize);

        AZ::Name macroNormalClipmapName = AZ::Name("MacroNormalClipmaps");
        m_clipmaps[ClipmapName::MacroNormal] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, macroNormalClipmapName, nullptr, nullptr);
    }

    void TerrainClipmapManager::InitializeDetailClipmapImages()
    {
        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

        AZ::RHI::ImageDescriptor imageDesc;
        imageDesc.m_bindFlags = AZ::RHI::ImageBindFlags::ShaderReadWrite;
        imageDesc.m_size = AZ::RHI::Size(m_config.m_clipmapSize, m_config.m_clipmapSize, 1);

        imageDesc.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;
        imageDesc.m_arraySize = aznumeric_cast<uint16_t>(m_detailClipmapStackSize);

        AZ::Name detailColorClipmapName = AZ::Name("DetailColorClipmaps");
        m_clipmaps[ClipmapName::DetailColor] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailColorClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R16G16_SNORM;
        imageDesc.m_arraySize = aznumeric_cast<uint16_t>(m_detailClipmapStackSize);

        AZ::Name detailNormalClipmapName = AZ::Name("DetailNormalClipmaps");
        m_clipmaps[ClipmapName::DetailNormal] =
            AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc, detailNormalClipmapName, nullptr, nullptr);

        imageDesc.m_format = AZ::RHI::Format::R16_FLOAT;

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

    void TerrainClipmapManager::ClearMacroClipmapImages()
    {
        m_clipmaps[ClipmapName::MacroColor] = nullptr;
        m_clipmaps[ClipmapName::MacroNormal] = nullptr;
    }

    void TerrainClipmapManager::ClearDetailClipmapImages()
    {
        m_clipmaps[ClipmapName::DetailColor] = nullptr;
        m_clipmaps[ClipmapName::DetailNormal] = nullptr;
        m_clipmaps[ClipmapName::DetailHeight] = nullptr;
        m_clipmaps[ClipmapName::DetailRoughness] = nullptr;
        m_clipmaps[ClipmapName::DetailSpecularF0] = nullptr;
        m_clipmaps[ClipmapName::DetailMetalness] = nullptr;
        m_clipmaps[ClipmapName::DetailOcclusion] = nullptr;
    }

    void TerrainClipmapManager::UpdateClipmapData(const AZ::Vector3& cameraPosition, const AZ::RPI::Scene* scene, AZ::Data::Instance<AZ::RPI::ShaderResourceGroup>& terrainSrg)
    {
        // set new view position
        AZ::Vector2 currentViewPosition = AZ::Vector2(cameraPosition.GetX(), cameraPosition.GetY());

        // Update debug data
        auto viewportContextInterface = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        auto viewportContext = viewportContextInterface->GetViewportContextByScene(scene);
        auto viewportWindowSize = viewportContext->GetViewportSize();
        m_clipmapData.m_viewportSize[0] = aznumeric_cast<float>(viewportWindowSize.m_width);
        m_clipmapData.m_viewportSize[1] = aznumeric_cast<float>(viewportWindowSize.m_height);

        m_clipmapData.m_debugClipmapId = uint32_t(r_terrainClipmapDebugClipmapId);
        m_clipmapData.m_debugClipmapLevel = aznumeric_cast<float>(uint32_t(r_terrainClipmapDebugClipmapLevel));
        m_clipmapData.m_debugScale = float(r_terrainClipmapDebugScale);
        m_clipmapData.m_debugBrightness = float(r_terrainClipmapDebugBrightness);

        if (r_terrainClipmapDebugOverlay == 0u)
        {
            m_clipmapData.m_macroClipmapOverlayFactor = 0.0f;
            m_clipmapData.m_detailClipmapOverlayFactor = 0.0f;
        }
        else if (r_terrainClipmapDebugOverlay == 1u)
        {
            m_clipmapData.m_macroClipmapOverlayFactor = 1.0f;
            m_clipmapData.m_detailClipmapOverlayFactor = 0.0f;
        }
        else
        {
            m_clipmapData.m_macroClipmapOverlayFactor = 0.0f;
            m_clipmapData.m_detailClipmapOverlayFactor = 1.0f;
        }

        // Update clipmap center
        m_macroClipmapUpdateRegions.clear();
        m_detailClipmapUpdateRegions.clear();

        // First time update will run through the whole clipmap
        if (m_fullRefreshClipmaps)
        {
            m_fullRefreshClipmaps = false;

            InitializeMacroClipmapBounds(currentViewPosition);
            InitializeDetailClipmapBounds(currentViewPosition);

            AZStd::array<uint32_t, 4> aabb = { 0, 0, m_config.m_clipmapSize, m_config.m_clipmapSize };

            for (uint32_t clipmapIndex = 0; clipmapIndex < m_macroClipmapStackSize; ++clipmapIndex)
            {
                m_macroClipmapUpdateRegions.push_back(ClipmapUpdateRegion(clipmapIndex, aabb));

                Vector2i center = m_macroClipmapBounds[clipmapIndex].GetModCenter();
                AZ::Vector2 centerWorld = m_macroClipmapBounds[clipmapIndex].GetCenterInWorldSpace();
                m_clipmapData.m_clipmapCenters[clipmapIndex].m_macro[0] = center.m_x;
                m_clipmapData.m_clipmapCenters[clipmapIndex].m_macro[1] = center.m_y;
                m_clipmapData.m_clipmapWorldCenters[clipmapIndex].m_macro[0] = centerWorld.GetX();
                m_clipmapData.m_clipmapWorldCenters[clipmapIndex].m_macro[1] = centerWorld.GetY();
            }

            for (uint32_t clipmapIndex = 0; clipmapIndex < m_detailClipmapStackSize; ++clipmapIndex)
            {
                m_detailClipmapUpdateRegions.push_back(ClipmapUpdateRegion(clipmapIndex, aabb));

                Vector2i center = m_detailClipmapBounds[clipmapIndex].GetModCenter();
                AZ::Vector2 centerWorld = m_detailClipmapBounds[clipmapIndex].GetCenterInWorldSpace();
                m_clipmapData.m_clipmapCenters[clipmapIndex].m_detail[0] = center.m_x;
                m_clipmapData.m_clipmapCenters[clipmapIndex].m_detail[1] = center.m_y;
                m_clipmapData.m_clipmapWorldCenters[clipmapIndex].m_detail[0] = centerWorld.GetX();
                m_clipmapData.m_clipmapWorldCenters[clipmapIndex].m_detail[1] = centerWorld.GetY();
            }

            m_clipmapData.m_macroClipmapUpdateRegionCount = aznumeric_cast<uint32_t>(m_macroClipmapUpdateRegions.size());
            m_clipmapData.m_detailClipmapUpdateRegionCount = aznumeric_cast<uint32_t>(m_detailClipmapUpdateRegions.size());
            m_macroClipmapUpdateRegionsBuffer.UpdateBuffer(m_macroClipmapUpdateRegions.data(), m_clipmapData.m_macroClipmapUpdateRegionCount);
            m_detailClipmapUpdateRegionsBuffer.UpdateBuffer(m_detailClipmapUpdateRegions.data(), m_clipmapData.m_detailClipmapUpdateRegionCount);
            m_macroClipmapUpdateRegionsBuffer.UpdateSrg(terrainSrg.get());
            m_detailClipmapUpdateRegionsBuffer.UpdateSrg(terrainSrg.get());

            m_macroTotalDispatchThreadX = 1024;
            m_macroTotalDispatchThreadY = 1024;
            m_detailTotalDispatchThreadX = 1024;
            m_detailTotalDispatchThreadY = 1024;

            m_clipmapData.m_macroDispatchGroupCountX = m_macroTotalDispatchThreadX / MacroGroupThreadX;
            m_clipmapData.m_macroDispatchGroupCountY = m_macroTotalDispatchThreadY / MacroGroupThreadY;

            m_clipmapData.m_detailDispatchGroupCountX = m_detailTotalDispatchThreadX / DetailGroupThreadX;
            m_clipmapData.m_detailDispatchGroupCountY = m_detailTotalDispatchThreadY / DetailGroupThreadY;

            return;
        }

        // macro clipmap data:
        for (uint32_t clipmapIndex = 0; clipmapIndex < m_macroClipmapStackSize; ++clipmapIndex)
        {
            ClipmapBounds& clipmapBounds = m_macroClipmapBounds[clipmapIndex];

            ClipmapBoundsRegionList updateRegionList = clipmapBounds.UpdateCenter(currentViewPosition);

            // write updated center
            Vector2i center = clipmapBounds.GetModCenter();
            m_clipmapData.m_clipmapCenters[clipmapIndex].m_macro[0] = aznumeric_cast<uint32_t>(center.m_x);
            m_clipmapData.m_clipmapCenters[clipmapIndex].m_macro[1] = aznumeric_cast<uint32_t>(center.m_y);

            AZ::Vector2 centerWorld = clipmapBounds.GetCenterInWorldSpace();
            m_clipmapData.m_clipmapWorldCenters[clipmapIndex].m_macro[0] = centerWorld.GetX();
            m_clipmapData.m_clipmapWorldCenters[clipmapIndex].m_macro[1] = centerWorld.GetY();

            for (uint32_t i = 0; i < updateRegionList.size(); ++i)
            {
                AZStd::array<uint32_t, 4> aabb = { aznumeric_cast<uint32_t>(updateRegionList[i].m_localAabb.m_min.m_x),
                                                   aznumeric_cast<uint32_t>(updateRegionList[i].m_localAabb.m_min.m_y),
                                                   aznumeric_cast<uint32_t>(updateRegionList[i].m_localAabb.m_max.m_x),
                                                   aznumeric_cast<uint32_t>(updateRegionList[i].m_localAabb.m_max.m_y) };
                m_macroClipmapUpdateRegions.push_back(ClipmapUpdateRegion(clipmapIndex, aabb));
            }
        }

        uint32_t updateRegionCount = aznumeric_cast<uint32_t>(m_macroClipmapUpdateRegions.size());
        if (updateRegionCount)
        {
            m_macroTotalDispatchThreadX = 64;
            m_macroTotalDispatchThreadY = 64;
            m_clipmapData.m_macroDispatchGroupCountX = m_macroTotalDispatchThreadX / MacroGroupThreadX;
            m_clipmapData.m_macroDispatchGroupCountY = m_macroTotalDispatchThreadY / MacroGroupThreadY;

            m_clipmapData.m_macroClipmapUpdateRegionCount = updateRegionCount;
            m_macroClipmapUpdateRegionsBuffer.UpdateBuffer(m_macroClipmapUpdateRegions.data(), updateRegionCount);
            m_macroClipmapUpdateRegionsBuffer.UpdateSrg(terrainSrg.get());
        }
        else
        {
            m_macroTotalDispatchThreadX = 0;
            m_macroTotalDispatchThreadY = 0;
            m_clipmapData.m_macroDispatchGroupCountX = 1;
            m_clipmapData.m_macroDispatchGroupCountY = 1;
        }

        // detail clipmap data:
        for (uint32_t clipmapIndex = 0; clipmapIndex < m_detailClipmapStackSize; ++clipmapIndex)
        {
            ClipmapBounds& clipmapBounds = m_detailClipmapBounds[clipmapIndex];

            ClipmapBoundsRegionList updateRegionList = clipmapBounds.UpdateCenter(currentViewPosition);

            // write updated center
            Vector2i center = clipmapBounds.GetModCenter();
            m_clipmapData.m_clipmapCenters[clipmapIndex].m_detail[0] = aznumeric_cast<uint32_t>(center.m_x);
            m_clipmapData.m_clipmapCenters[clipmapIndex].m_detail[1] = aznumeric_cast<uint32_t>(center.m_y);

            AZ::Vector2 centerWorld = clipmapBounds.GetCenterInWorldSpace();
            m_clipmapData.m_clipmapWorldCenters[clipmapIndex].m_detail[0] = centerWorld.GetX();
            m_clipmapData.m_clipmapWorldCenters[clipmapIndex].m_detail[1] = centerWorld.GetY();

            for (uint32_t i = 0; i < updateRegionList.size(); ++i)
            {
                AZStd::array<uint32_t, 4> aabb = { aznumeric_cast<uint32_t>(updateRegionList[i].m_localAabb.m_min.m_x),
                                                   aznumeric_cast<uint32_t>(updateRegionList[i].m_localAabb.m_min.m_y),
                                                   aznumeric_cast<uint32_t>(updateRegionList[i].m_localAabb.m_max.m_x),
                                                   aznumeric_cast<uint32_t>(updateRegionList[i].m_localAabb.m_max.m_y) };
                m_detailClipmapUpdateRegions.push_back(ClipmapUpdateRegion(clipmapIndex, aabb));
            }
        }

        updateRegionCount = aznumeric_cast<uint32_t>(m_detailClipmapUpdateRegions.size());
        if (updateRegionCount)
        {
            m_detailTotalDispatchThreadX = 64;
            m_detailTotalDispatchThreadY = 64;
            m_clipmapData.m_detailDispatchGroupCountX = m_detailTotalDispatchThreadX / DetailGroupThreadX;
            m_clipmapData.m_detailDispatchGroupCountY = m_detailTotalDispatchThreadY / DetailGroupThreadY;

            m_clipmapData.m_detailClipmapUpdateRegionCount = updateRegionCount;
            m_detailClipmapUpdateRegionsBuffer.UpdateBuffer(m_detailClipmapUpdateRegions.data(), updateRegionCount);
            m_detailClipmapUpdateRegionsBuffer.UpdateSrg(terrainSrg.get());
        }
        else
        {
            m_detailTotalDispatchThreadX = 0;
            m_detailTotalDispatchThreadY = 0;
            m_clipmapData.m_detailDispatchGroupCountX = 1;
            m_clipmapData.m_detailDispatchGroupCountY = 1;
        }
    }

    AZ::Data::Instance<AZ::RPI::AttachmentImage> TerrainClipmapManager::GetClipmapImage(ClipmapName clipmapName) const
    {
        AZ_Assert(clipmapName < ClipmapName::Count, "Must be a valid ClipmapName enum.");

        return m_clipmaps[clipmapName];
    }

    void TerrainClipmapManager::GetMacroDispatchThreadNum(uint32_t& outThreadX, uint32_t& outThreadY, uint32_t& outThreadZ) const
    {
        outThreadX = m_macroTotalDispatchThreadX;
        outThreadY = m_macroTotalDispatchThreadY;
        outThreadZ = 1;
    }

    void TerrainClipmapManager::GetDetailDispatchThreadNum(uint32_t& outThreadX, uint32_t& outThreadY, uint32_t& outThreadZ) const
    {
        outThreadX = m_detailTotalDispatchThreadX;
        outThreadY = m_detailTotalDispatchThreadY;
        outThreadZ = 1;
    }

    uint32_t TerrainClipmapManager::GetClipmapSize() const
    {
        return m_config.m_clipmapSize;
    }

    bool TerrainClipmapManager::HasMacroClipmapUpdate() const
    {
        return m_macroTotalDispatchThreadX != 0 && m_macroTotalDispatchThreadY != 0;
    }

    bool TerrainClipmapManager::HasDetailClipmapUpdate() const
    {
        return m_detailTotalDispatchThreadX != 0 && m_detailTotalDispatchThreadY != 0;
    }

    // AzFramework::Terrain::TerrainDataNotificationBus overrides...
    void TerrainClipmapManager::OnTerrainDataChanged(
        [[maybe_unused]] const AZ::Aabb& dirtyRegion, [[maybe_unused]] TerrainDataChangedMask dataChangedMask)
    {
        TriggerFullRefresh();
    }

    // TerrainMacroMaterialNotificationBus overrides...
    void TerrainClipmapManager::OnTerrainMacroMaterialCreated(
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const MacroMaterialData& material)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainMacroMaterialChanged(
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const MacroMaterialData& material)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainMacroMaterialRegionChanged(
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const AZ::Aabb& oldRegion, [[maybe_unused]] const AZ::Aabb& newRegion)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainMacroMaterialDestroyed([[maybe_unused]] AZ::EntityId entityId)
    {
        TriggerFullRefresh();
    }

    // TerrainAreaMaterialNotificationBus overrides...
    void TerrainClipmapManager::OnTerrainDefaultSurfaceMaterialCreated(
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] AZ::Data::Instance<AZ::RPI::Material> material)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainDefaultSurfaceMaterialDestroyed([[maybe_unused]] AZ::EntityId entityId)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainDefaultSurfaceMaterialChanged(
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] AZ::Data::Instance<AZ::RPI::Material> newMaterial)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainSurfaceMaterialMappingCreated(
        [[maybe_unused]] AZ::EntityId entityId,
        [[maybe_unused]] SurfaceData::SurfaceTag surfaceTag,
        [[maybe_unused]] AZ::Data::Instance<AZ::RPI::Material> material)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainSurfaceMaterialMappingDestroyed(
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] SurfaceData::SurfaceTag surfaceTag)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainSurfaceMaterialMappingMaterialChanged(
        [[maybe_unused]] AZ::EntityId entityId,
        [[maybe_unused]] SurfaceData::SurfaceTag surfaceTag,
        [[maybe_unused]] AZ::Data::Instance<AZ::RPI::Material> material)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainSurfaceMaterialMappingTagChanged(
        [[maybe_unused]] AZ::EntityId entityId,
        [[maybe_unused]] SurfaceData::SurfaceTag oldSurfaceTag,
        [[maybe_unused]] SurfaceData::SurfaceTag newSurfaceTag)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainSurfaceMaterialMappingRegionCreated(
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const AZ::Aabb& region)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainSurfaceMaterialMappingRegionDestroyed(
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const AZ::Aabb& oldRegion)
    {
        TriggerFullRefresh();
    }

    void TerrainClipmapManager::OnTerrainSurfaceMaterialMappingRegionChanged(
        [[maybe_unused]] AZ::EntityId entityId, [[maybe_unused]] const AZ::Aabb& oldRegion, [[maybe_unused]] const AZ::Aabb& newRegion)
    {
        TriggerFullRefresh();
    }
}
