/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Components/TerrainMacroMaterialComponent.h>
#include <TerrainSystem/TerrainSystemBus.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace Terrain
{
    bool TerrainMacroMaterialConfig::NormalMapAttributesAreReadOnly() const
    {
        return !m_macroNormalAsset.GetId().IsValid();
    }

    void TerrainMacroMaterialConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context); serialize)
        {
            serialize->Class<TerrainMacroMaterialConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("MacroColor", &TerrainMacroMaterialConfig::m_macroColorAsset)
                ->Field("MacroNormal", &TerrainMacroMaterialConfig::m_macroNormalAsset)
                ->Field("NormalFlipX", &TerrainMacroMaterialConfig::m_normalFlipX)
                ->Field("NormalFlipY", &TerrainMacroMaterialConfig::m_normalFlipY)
                ->Field("NormalFactor", &TerrainMacroMaterialConfig::m_normalFactor)
                ->Field("Priority", &TerrainMacroMaterialConfig::m_priority)
                ;
        }
    }

    AZStd::string TerrainMacroMaterialConfig::GetMacroColorAssetPropertyName() const
    {
        return m_macroColorAssetPropertyLabel;
    }

    void TerrainMacroMaterialConfig::SetMacroColorAssetPropertyName(const AZStd::string& macroColorAssetPropertyName)
    {
        m_macroColorAssetPropertyLabel = macroColorAssetPropertyName;
    }


    void TerrainMacroMaterialComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainMacroMaterialProviderService"));
    }

    void TerrainMacroMaterialComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainMacroMaterialProviderService"));
    }

    void TerrainMacroMaterialComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void TerrainMacroMaterialComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainMacroMaterialConfig::Reflect(context);
        TerrainMacroMaterialRequests::Reflect(context);
        
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainMacroMaterialComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainMacroMaterialComponent::m_configuration)
                ;
        }
    }

    TerrainMacroMaterialComponent::TerrainMacroMaterialComponent(const TerrainMacroMaterialConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainMacroMaterialComponent::Activate()
    {
        // Clear out our shape bounds and make sure the texture assets are queued to load.
        m_cachedShapeBounds = AZ::Aabb::CreateNull();
        m_configuration.m_macroColorAsset.QueueLoad();
        m_configuration.m_macroNormalAsset.QueueLoad();

        // Don't mark our material as active until it's finished loading and is valid.
        m_macroMaterialActive = false;

        // Listen for the texture assets to complete loading.
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_macroColorAsset.GetId());
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_macroNormalAsset.GetId());
    }

    void TerrainMacroMaterialComponent::Deactivate()
    {
        TerrainMacroMaterialRequestBus::Handler::BusDisconnect();

        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        m_configuration.m_macroColorAsset.Release();
        m_configuration.m_macroNormalAsset.Release();

        m_colorImage.reset();
        m_normalImage.reset();

        DestroyMacroColorImageModificationBuffer();

        // Send out any notifications as appropriate based on the macro material destruction.
        HandleMaterialStateChange();
    }

    bool TerrainMacroMaterialComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainMacroMaterialConfig*>(baseConfig))
        {
            AZ_Assert(m_numImageModificationsActive == 0,
                "Changing the configuration during an image modification session might result in a broken modification state.")
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainMacroMaterialComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainMacroMaterialConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainMacroMaterialComponent::OnShapeChanged([[maybe_unused]] ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        // This should only get called while the macro material is active.  If it gets called while the macro material isn't active,
        // we've got a bug where we haven't managed the bus connections properly.
        AZ_Assert(m_macroMaterialActive, "The ShapeComponentNotificationBus connection is out of sync with the material load.");

        AZ::Aabb oldShapeBounds = m_cachedShapeBounds;

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            m_cachedShapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        TerrainMacroMaterialNotificationBus::Broadcast(
            &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialRegionChanged,
            GetEntityId(), oldShapeBounds, m_cachedShapeBounds);

        NotifyTerrainSystemOfColorChange(oldShapeBounds);
        NotifyTerrainSystemOfColorChange(m_cachedShapeBounds);
    }

    void TerrainMacroMaterialComponent::HandleMaterialStateChange()
    {
        // We only want our component to appear active during the time that the macro material is fully loaded and valid.  The logic below
        // will handle all transition possibilities to notify if we've become active, inactive, or just changed.  We'll also only
        // keep a valid up-to-date copy of the shape bounds while the material is valid, since we don't need it any other time.

        // Color and normal data is considered ready if it's finished loading or if we don't have a texture specified
        bool colorReady = m_colorImage || (!m_configuration.m_macroColorAsset.GetId().IsValid());
        bool normalReady = m_normalImage || (!m_configuration.m_macroNormalAsset.GetId().IsValid());
        // If we don't have color or normal data, then we don't have *any* useful data, so don't activate the macro material.
        bool hasAnyData = m_configuration.m_macroColorAsset.GetId().IsValid() || m_configuration.m_macroNormalAsset.GetId().IsValid();

        bool wasPreviouslyActive = m_macroMaterialActive;
        bool isNowActive = colorReady && normalReady && hasAnyData;

        // Set our state to active or inactive, based on whether or not the macro material instance is now valid.
        m_macroMaterialActive = isNowActive;

        // Handle the different inactive/active transition possibilities.

        if (!wasPreviouslyActive && !isNowActive)
        {
            // Do nothing, we haven't yet successfully loaded a valid material.
        }
        else if (!wasPreviouslyActive && isNowActive)
        {
            // We've transitioned from inactive to active, so send out a message saying that we've been created and start tracking the
            // overall shape bounds.

            // Get the current shape bounds.
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                m_cachedShapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

            // Start listening for terrain macro material requests.
            TerrainMacroMaterialRequestBus::Handler::BusConnect(GetEntityId());

            // Start listening for shape changes.
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());

            // Start listening for macro material modifications
            AzFramework::PaintBrushNotificationBus::Handler::BusConnect({ GetEntityId(), GetId() });
            TerrainMacroColorModificationBus::Handler::BusConnect(GetEntityId());

            MacroMaterialData material = GetTerrainMacroMaterialData();

            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialCreated, GetEntityId(), material);

            NotifyTerrainSystemOfColorChange(m_cachedShapeBounds);
        }
        else if (wasPreviouslyActive && !isNowActive)
        {
            // Stop listening to macro material requests or shape changes, and send out a notification that we no longer have a valid
            // macro material.

            NotifyTerrainSystemOfColorChange(m_cachedShapeBounds);

            DestroyMacroColorImageModificationBuffer();

            TerrainMacroColorModificationBus::Handler::BusDisconnect();
            AzFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
            TerrainMacroMaterialRequestBus::Handler::BusDisconnect();
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();

            m_cachedShapeBounds = AZ::Aabb::CreateNull();

            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialDestroyed, GetEntityId());
        }
        else
        {
            // We were active both before and after, so just send out a material changed event.
            MacroMaterialData material = GetTerrainMacroMaterialData();

            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialChanged, GetEntityId(), material);

            NotifyTerrainSystemOfColorChange(m_cachedShapeBounds);
        }
    }

    void TerrainMacroMaterialComponent::NotifyTerrainSystemOfColorChange(const AZ::Aabb& bounds)
    {
        // Given an AABB, notify the terrain system that the macro color data has changed within these bounds.
        // We also clamp the bounds to the existing Terrain AABB and set the min/max height range to the full Terrain AABB range,
        // since macro color materials don't really have a height bounds.

        AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);

        const AZ::Aabb clampedBounds = bounds.GetClamped(terrainAabb);
        const AZ::Aabb dirtyRegion = AZ::Aabb::CreateFromMinMaxValues(
            clampedBounds.GetMin().GetX(), clampedBounds.GetMin().GetY(), bounds.GetMin().GetZ(),
            clampedBounds.GetMax().GetX(), clampedBounds.GetMax().GetY(), bounds.GetMax().GetZ());

        TerrainSystemServiceRequestBus::Broadcast(
            &TerrainSystemServiceRequestBus::Events::RefreshRegion,
            dirtyRegion,
            AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask::ColorData);
    }

    void TerrainMacroMaterialComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset.GetId() == m_configuration.m_macroColorAsset.GetId())
        {
            m_configuration.m_macroColorAsset = asset;
            m_colorImage = AZ::RPI::StreamingImage::FindOrCreate(m_configuration.m_macroColorAsset);
            m_colorImage->GetRHIImage()->SetName(AZ::Name(m_configuration.m_macroColorAsset.GetHint()));

            // If we're changing the image data while we're in the middle of modifications,
            // refresh the modification buffer with the new data.
            if (MacroColorModificationBufferIsActive())
            {
                DestroyMacroColorImageModificationBuffer();
                CreateMacroColorImageModificationBuffer();
            }
        }
        else if (asset.GetId() == m_configuration.m_macroNormalAsset.GetId())
        {
            m_configuration.m_macroNormalAsset = asset;
            m_normalImage = AZ::RPI::StreamingImage::FindOrCreate(m_configuration.m_macroNormalAsset);
            m_normalImage->GetRHIImage()->SetName(AZ::Name(m_configuration.m_macroNormalAsset.GetHint()));
        }

        HandleMaterialStateChange();
    }

    void TerrainMacroMaterialComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Update our loaded asset state.
        OnAssetReady(asset);
    }

    MacroMaterialData TerrainMacroMaterialComponent::GetTerrainMacroMaterialData()
    {
        MacroMaterialData macroMaterial;

        macroMaterial.m_entityId = GetEntityId();
        macroMaterial.m_bounds = m_cachedShapeBounds;
        macroMaterial.m_colorImage = m_colorImage;
        macroMaterial.m_normalImage = m_normalImage;
        macroMaterial.m_normalFactor = m_configuration.m_normalFactor;
        macroMaterial.m_normalFlipX = m_configuration.m_normalFlipX;
        macroMaterial.m_normalFlipY = m_configuration.m_normalFlipY;
        macroMaterial.m_priority = m_configuration.m_priority;

        return macroMaterial;
    }

    AZ::RHI::Size TerrainMacroMaterialComponent::GetMacroColorImageSize() const
    {
        if (m_colorImage)
        {
            const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();
            return imageDescriptor.m_size;
        }

        return { 0, 0, 0 };
    }

    AZ::Vector2 TerrainMacroMaterialComponent::GetMacroColorImagePixelsPerMeter() const
    {
        if (m_colorImage)
        {
            const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();
            return AZ::Vector2(
                aznumeric_cast<float>(imageDescriptor.m_size.m_width) / m_cachedShapeBounds.GetXExtent(),
                aznumeric_cast<float>(imageDescriptor.m_size.m_height) / m_cachedShapeBounds.GetYExtent());
        }

        return AZ::Vector2::CreateZero();
    }

    AZ::Data::Asset<AZ::RPI::StreamingImageAsset> TerrainMacroMaterialComponent::GetMacroColorAsset() const
    {
        return m_configuration.m_macroColorAsset;
    }

    void TerrainMacroMaterialComponent::SetMacroColorAsset(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& asset)
    {
        // If we're setting the component to the same asset we're already using, then early-out.
        if (asset.GetId() == m_configuration.m_macroColorAsset.GetId())
        {
            return;
        }

        // Stop listening for the current image asset.
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(m_configuration.m_macroColorAsset.GetId());

        m_configuration.m_macroColorAsset = asset;

        // If we're using a modification buffer, we want to keep it active until the new image has finished loading in, so
        // we won't destroy or recreate it here.

        // If the new asset is valid, try to load it.
        if (m_configuration.m_macroColorAsset.GetId().IsValid())
        {
            // If we have a valid Asset ID, check to see if it also appears in the AssetCatalog. This might be an Asset ID for an asset
            // that doesn't exist yet if it was just created from the Editor component.
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_configuration.m_macroColorAsset.GetId());

            // Only queue the load if it appears in the Asset Catalog. If it doesn't, we'll get notified when it shows up.
            if (assetInfo.m_assetId.IsValid())
            {
                m_configuration.m_macroColorAsset.QueueLoad(
                    AZ::Data::AssetLoadParameters(nullptr, AZ::Data::AssetDependencyLoadRules::LoadAll));
            }

            // Start listening for all events for this asset.
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_macroColorAsset.GetId());
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void TerrainMacroMaterialComponent::OnPaintModeBegin()
    {
        StartMacroColorImageModification();
    }

    void TerrainMacroMaterialComponent::OnPaintModeEnd()
    {
        EndMacroColorImageModification();
    }

    AZ::Color TerrainMacroMaterialComponent::OnGetColor(const AZ::Vector3& brushCenter) const
    {
        AZ::Color color(0.0f, 0.0f, 0.0f, 1.0f);
        GetMacroColorPixelValuesByPosition(AZStd::span<const AZ::Vector3>(&brushCenter, 1), AZStd::span<AZ::Color>(&color, 1));
        return color;
    }

    void TerrainMacroMaterialComponent::StartMacroColorImageModification()
    {
        if (!m_macroColorImageModifier)
        {
            AZ_Assert(
                m_numImageModificationsActive == 0,
                "The imageModifier should exist since image modifications are already currently active.");
            m_macroColorImageModifier = AZStd::make_unique<MacroMaterialImageModifier>(AZ::EntityComponentIdPair(GetEntityId(), GetId()));
        }

        if (m_modifiedMacroColorImageData.empty())
        {
            CreateMacroColorImageModificationBuffer();
        }

        m_numImageModificationsActive++;
    }

    void TerrainMacroMaterialComponent::EndMacroColorImageModification()
    {
        AZ_Assert(m_numImageModificationsActive > 0, "Mismatched calls to StartMacroColorImageModification / EndMacroColorImageModification");

        m_numImageModificationsActive--;

        if (m_numImageModificationsActive == 0)
        {
            m_macroColorImageModifier = {};
        }
    }

    void TerrainMacroMaterialComponent::StartMacroColorPixelModifications()
    {
        AZ_Assert(!m_modifyingPixels, "Called StartMacroColorPixelModifications twice without calling EndMacroColorPixelModifications");
        m_modifyingPixels = true;

        // We'll use these to track the subregion of pixels that have been modified.
        m_leftTopPixel = PixelIndex(AZStd::numeric_limits<int16_t>::max(), AZStd::numeric_limits<int16_t>::max());
        m_rightBottomPixel = PixelIndex(aznumeric_cast<int16_t>(0), aznumeric_cast<int16_t>(0));
    }

    void TerrainMacroMaterialComponent::EndMacroColorPixelModifications()
    {
        if (m_modifyingPixels)
        {
            UpdateMacroMaterialTexture(m_leftTopPixel, m_rightBottomPixel);
        }

        m_modifyingPixels = false;
    }

    AZStd::span<const uint32_t> TerrainMacroMaterialComponent::GetMacroColorImageModificationBuffer() const
    {
        // This isn't generally safe to do, but this is a protected method only exposed outward to the editor component so that
        // we have a way to save the modification image buffer as a new source asset.
        // This method shouldn't get called by anything else.
        return AZStd::span<const uint32_t>(m_modifiedMacroColorImageData.data(), m_modifiedMacroColorImageData.size());
    }

    bool TerrainMacroMaterialComponent::MacroColorImageIsModified() const
    {
        return !m_modifiedMacroColorImageData.empty() && m_macroColorImageIsModified;
    }

    uint32_t TerrainMacroMaterialComponent::GetHighestLoadedMipLevel() const
    {
        if (m_configuration.m_macroColorAsset->IsReady())
        {
            // Walk through the mip chains to find the highest loaded mip level.
            const size_t numMipChains = m_configuration.m_macroColorAsset->GetMipChainCount();
            if (numMipChains > 0)
            {
                // Check the standard mip chain assets to look for the highest loaded mip.
                for (size_t mipChainIndex = 0; mipChainIndex < (numMipChains - 1); mipChainIndex++)
                {
                    auto mipChainAsset = m_configuration.m_macroColorAsset->GetMipChainAsset(mipChainIndex);
                    if (mipChainAsset->IsReady())
                    {
                        return aznumeric_cast<uint32_t>(m_configuration.m_macroColorAsset->GetMipLevel(mipChainIndex));
                    }
                }

                // The tail mip chain is always loaded but can't be queried the same way as the other mip chain assets,
                // so we'll just assume that it's loaded and return its highest mip level.
                // Note that this could still potentially be mip 0 if the image is extremely small.
                return aznumeric_cast<uint32_t>(m_configuration.m_macroColorAsset->GetMipLevel(numMipChains - 1));
            }
        }

        // No loaded mip level found.
        return InvalidMipLevel;
    }

    void TerrainMacroMaterialComponent::CreateMacroColorImageModificationBuffer()
    {
        // Get the highest loaded mip, which is hopefully mip 0.
        uint32_t mipLevel = GetHighestLoadedMipLevel();

        if (mipLevel == InvalidMipLevel)
        {
            AZ_Error("TerrainMacroMaterialComponent", false, "No mip levels are loaded, cannot create an image modification buffer.");
            return;
        }

        if (mipLevel > 0)
        {
            AZ_Warning("TerrainMacroMaterialComponent", false,
                "Highest mip level loaded is %zu, painting data will be of lesser quality.", mipLevel);
        }

        const AZ::RHI::ImageDescriptor& imageDescriptor = m_configuration.m_macroColorAsset->GetImageDescriptor();
        auto imageData = m_configuration.m_macroColorAsset->GetSubImageData(mipLevel, 0);

        const auto& width = imageDescriptor.m_size.m_width;
        const auto& height = imageDescriptor.m_size.m_height;

        // Track that the image hasn't been modified yet, even though we've created a modification buffer.
        m_macroColorImageIsModified = false;

        if (m_modifiedMacroColorImageData.empty())
        {
            // Create a memory buffer for holding all of our modified image information.
            // We use a buffer of uint32 colors (R8G8B8A8) so that it doesn't get overly large when modifying large textures.
            m_modifiedMacroColorImageData.reserve(width * height);

            // Fill the buffer with all of our existing pixel values.
            for (uint32_t y = 0; y < height; y++)
            {
                for (uint32_t x = 0; x < width; x++)
                {
                    AZ::Color pixel = AZ::RPI::GetImageDataPixelValue<AZ::Color>(imageData, imageDescriptor, x, y);
                    m_modifiedMacroColorImageData.emplace_back(pixel.ToU32());
                }
            }

            // Create an image descriptor describing our new buffer (correct width, height, and 8-bit RGB channels)
            auto modifiedImageDescriptor =
                AZ::RHI::ImageDescriptor::Create2D(AZ::RHI::ImageBindFlags::ShaderRead, width, height, AZ::RHI::Format::R8G8B8A8_UNORM);

            // Set our imageData pointer to point to our modified data buffer.
            auto modifiedImageData = AZStd::span<const uint8_t>(
                reinterpret_cast<uint8_t*>(m_modifiedMacroColorImageData.data()), m_modifiedMacroColorImageData.size() * sizeof(uint32_t));

            // Create the initial buffer for the downloaded color data
            const AZ::Data::Instance<AZ::RPI::AttachmentImagePool> imagePool =
                AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

            const AZ::Name ModificationImageName = AZ::Name("ModifiedMacroColorImage");
            m_colorImage =
                AZ::RPI::AttachmentImage::Create(*imagePool.get(), modifiedImageDescriptor, ModificationImageName, nullptr, nullptr);
            AZ_Error("Terrain", m_colorImage, "Failed to initialize the modification image buffer.");

            // Push our initialized pixel data up to the GPU.
            UpdateMacroMaterialTexture(
                PixelIndex(aznumeric_cast<int16_t>(0), aznumeric_cast<int16_t>(0)),
                PixelIndex(aznumeric_cast<int16_t>(width - 1), aznumeric_cast<int16_t>(height - 1)));

            // Notify that the material has changed.
            MacroMaterialData material = GetTerrainMacroMaterialData();
            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialChanged, GetEntityId(), material);
        }
        else
        {
            // If this triggers, we've somehow gotten our image modification buffer out of sync with the image descriptor information.
            AZ_Assert(m_modifiedMacroColorImageData.size() == (width * height), "Image modification buffer exists but is the wrong size.");
        }
    }

    void TerrainMacroMaterialComponent::DestroyMacroColorImageModificationBuffer()
    {
        m_modifiedMacroColorImageData.resize(0);
        m_macroColorImageIsModified = false;
    }

    bool TerrainMacroMaterialComponent::MacroColorModificationBufferIsActive() const
    {
        return (!m_modifiedMacroColorImageData.empty());
    }

    void TerrainMacroMaterialComponent::UpdateMacroMaterialTexture(
        PixelIndex leftTopPixel, PixelIndex rightBottomPixel)
    {
        // If these are still in their unitialized state, we've got nothing to update.
        if ((rightBottomPixel.first < leftTopPixel.first) || (rightBottomPixel.second < leftTopPixel.second))
        {
            return;
        }

        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();

        const auto& imageWidth = imageDescriptor.m_size.m_width;
        const auto& imageHeight = imageDescriptor.m_size.m_height;

        // When updating an image, the new data we provide needs to be a contiguous block of pixels, so we can't
        // just directly use our modification buffer unless we update the entire image. Instead, we'll need to create
        // a new temporary buffer containing just the subset of pixels that we want to update.
        // However, there's a performance tradeoff between creating a temporary buffer that updates less pixels and using
        // our full buffer as-is but updating all of the pixels.
        // To deal with this tradeoff, we'll use the heuristic that if less than 50% of the total pixels have changed, we'll
        // create the temporary buffer. If 50% or more have changed, we'll just upload the whole image.

        // By default, we'll update the entire image directly from our modification buffer.
        uint32_t updateLeftPixel = 0;
        uint32_t updateTopPixel = 0;
        uint32_t updateWidth = imageWidth;
        uint32_t updateHeight = imageHeight;
        void* sourceData = m_modifiedMacroColorImageData.data();

        uint32_t modifiedWidth = rightBottomPixel.first - leftTopPixel.first + 1;
        uint32_t modifiedHeight = rightBottomPixel.second - leftTopPixel.second + 1;

        AZStd::vector<uint32_t> tempBuffer;

        // If less than 50% of the total pixels have changed, create a new temporary buffer instead that only contains our
        // modified pixel region.
        constexpr float TotalPixelsChangedPercent = 0.50f;
        if (((modifiedWidth * modifiedHeight) / (imageWidth * imageHeight)) <= TotalPixelsChangedPercent)
        {
            updateLeftPixel = leftTopPixel.first;
            updateTopPixel = leftTopPixel.second;
            updateWidth = modifiedWidth;
            updateHeight = modifiedHeight;

            tempBuffer.resize_no_construct(updateWidth * updateHeight);
            for (uint32_t y = 0; y < updateHeight; y++)
            {
                memcpy(
                    &tempBuffer[(y * updateWidth)],
                    &m_modifiedMacroColorImageData[((y + leftTopPixel.second) * imageWidth) + leftTopPixel.first],
                    updateWidth * sizeof(uint32_t));
            }

            sourceData = tempBuffer.data();
        }

        // Upload the image changes to the GPU.
        const uint32_t BytesPerPixel = 4;
        AZ::RHI::ImageUpdateRequest imageUpdateRequest;
        imageUpdateRequest.m_imageSubresourcePixelOffset.m_left = updateLeftPixel;
        imageUpdateRequest.m_imageSubresourcePixelOffset.m_top = updateTopPixel;
        AZ::RHI::DeviceImageSubresourceLayout layout{{updateWidth, updateHeight, 1}, updateHeight, updateWidth * BytesPerPixel, updateWidth * updateHeight * BytesPerPixel, 1, 1};
        imageUpdateRequest.m_sourceSubresourceLayout.Init(m_colorImage->GetRHIImage()->GetDeviceMask(), layout);
        imageUpdateRequest.m_sourceData = sourceData;
        imageUpdateRequest.m_image = m_colorImage->GetRHIImage();
        m_colorImage->UpdateImageContents(imageUpdateRequest);
    }

    void TerrainMacroMaterialComponent::GetMacroColorPixelValuesByPosition(
        AZStd::span<const AZ::Vector3> positions, AZStd::span<AZ::Color> outValues) const
    {
        AZ_Assert(!m_modifiedMacroColorImageData.empty(), "Pixel values are only available during modifications.");

        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();

        const auto& width = imageDescriptor.m_size.m_width;
        const auto& height = imageDescriptor.m_size.m_height;

        for (size_t index = 0; index < positions.size(); index++)
        {
            auto pixelX = AZ::Lerp(
                0.0f,
                aznumeric_cast<float>(width),
                (positions[index].GetX() - m_cachedShapeBounds.GetMin().GetX()) / m_cachedShapeBounds.GetXExtent());

            auto pixelY = AZ::Lerp(
                0.0f,
                aznumeric_cast<float>(height),
                (positions[index].GetY() - m_cachedShapeBounds.GetMin().GetY()) / m_cachedShapeBounds.GetYExtent());

            if ((pixelX >= 0.0f) && (pixelY >= 0.0f) && (pixelX <= width) && (pixelY <= height))
            {
                auto x = AZStd::clamp(aznumeric_cast<AZ::u32>(pixelX), aznumeric_cast<AZ::u32>(0), width - 1);
                auto y = AZStd::clamp(aznumeric_cast<AZ::u32>(pixelY), aznumeric_cast<AZ::u32>(0), height - 1);

                // Flip the y because images are stored in reverse of our world axes
                y = (height - 1) - y;

                uint8_t r = (m_modifiedMacroColorImageData[(y * width) + x] >> 0) & 0xFF;
                uint8_t g = (m_modifiedMacroColorImageData[(y * width) + x] >> 8) & 0xFF;
                uint8_t b = (m_modifiedMacroColorImageData[(y * width) + x] >> 16) & 0xFF;
                uint8_t a = (m_modifiedMacroColorImageData[(y * width) + x] >> 24) & 0xFF;
                outValues[index] = AZ::Color(r, g, b, a);
            }
            else
            {
                outValues[index] = AZ::Color(0.0f, 0.0f, 0.0f, 1.0f);
            }
        }

    }

    void TerrainMacroMaterialComponent::GetMacroColorPixelIndicesForPositions(
        AZStd::span<const AZ::Vector3> positions, AZStd::span<PixelIndex> outIndices) const
    {
        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();

        const auto& width = imageDescriptor.m_size.m_width;
        const auto& height = imageDescriptor.m_size.m_height;

        for (size_t index = 0; index < positions.size(); index++)
        {
            auto pixelX = AZ::Lerp(
                0.0f,
                aznumeric_cast<float>(width),
                (positions[index].GetX() - m_cachedShapeBounds.GetMin().GetX()) / m_cachedShapeBounds.GetXExtent());

            auto pixelY = AZ::Lerp(
                0.0f,
                aznumeric_cast<float>(height),
                (positions[index].GetY() - m_cachedShapeBounds.GetMin().GetY()) / m_cachedShapeBounds.GetYExtent());

            if ((pixelX >= 0.0f) && (pixelY >= 0.0f) && (pixelX <= width) && (pixelY <= height))
            {
                auto x = AZStd::clamp(aznumeric_cast<AZ::u32>(pixelX), aznumeric_cast<AZ::u32>(0), width - 1);
                auto y = AZStd::clamp(aznumeric_cast<AZ::u32>(pixelY), aznumeric_cast<AZ::u32>(0), height - 1);

                // Flip the y because images are stored in reverse of our world axes
                y = (height - 1) - y;

                outIndices[index] = PixelIndex(aznumeric_cast<int16_t>(x), aznumeric_cast<int16_t>(y));
            }
            else
            {
                outIndices[index] = PixelIndex(aznumeric_cast<int16_t>(-1), aznumeric_cast<int16_t>(-1));
            }
        }
    }

    void TerrainMacroMaterialComponent::GetMacroColorPixelValuesByPixelIndex(
        AZStd::span<const PixelIndex> positions, AZStd::span<AZ::Color> outValues) const
    {
        AZ_Assert(!m_modifiedMacroColorImageData.empty(), "Pixel values are only available during modifications.");

        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();

        const auto& width = imageDescriptor.m_size.m_width;
        const auto& height = imageDescriptor.m_size.m_height;

        for (size_t index = 0; index < positions.size(); index++)
        {
            const auto& [x, y] = positions[index];

            if ((x >= 0) && (x < aznumeric_cast<int16_t>(width)) && (y >= 0) && (y < aznumeric_cast<int16_t>(height)))
            {
                uint8_t r = (m_modifiedMacroColorImageData[(y * width) + x] >> 0) & 0xFF;
                uint8_t g = (m_modifiedMacroColorImageData[(y * width) + x] >> 8) & 0xFF;
                uint8_t b = (m_modifiedMacroColorImageData[(y * width) + x] >> 16) & 0xFF;
                uint8_t a = (m_modifiedMacroColorImageData[(y * width) + x] >> 24) & 0xFF;
                outValues[index] = AZ::Color(r, g, b, a);
            }
        }
    }

    void TerrainMacroMaterialComponent::SetMacroColorPixelValuesByPixelIndex(
        AZStd::span<const PixelIndex> positions, AZStd::span<const AZ::Color> values)
    {
        if (m_modifiedMacroColorImageData.empty())
        {
            AZ_Error("TerrainMacroMaterialComponent", false,
                "Image modification mode needs to be started before the image values can be set.");
            return;
        }

        const AZ::RHI::ImageDescriptor& imageDescriptor = m_colorImage->GetDescriptor();

        const auto& width = imageDescriptor.m_size.m_width;
        const auto& height = imageDescriptor.m_size.m_height;

        // No pixels, so nothing to modify.
        if ((width == 0) || (height == 0))
        {
            return;
        }

        for (size_t index = 0; index < positions.size(); index++)
        {
            const auto& [x, y] = positions[index];

            if ((x >= 0) && (x < aznumeric_cast<int16_t>(width)) && (y >= 0) && (y < aznumeric_cast<int16_t>(height)))
            {
                // Modify the correct pixel in our modification buffer.
                m_modifiedMacroColorImageData[(y * width) + x] = values[index].ToU32();

                // Update the range of pixels that we've modified so that we know what subregion to upload to the GPU
                // once we're finished modifying the pixels.
                m_leftTopPixel.first = AZStd::min(m_leftTopPixel.first, x);
                m_rightBottomPixel.first = AZStd::max(m_rightBottomPixel.first, x);
                m_leftTopPixel.second = AZStd::min(m_leftTopPixel.second, y);
                m_rightBottomPixel.second = AZStd::max(m_rightBottomPixel.second, y);

                // Track that we've modified the image
                m_macroColorImageIsModified = true;
            }
        }
    }
} // namespace Terrain
