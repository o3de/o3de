/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Components/TerrainMacroMaterialComponent.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Terrain
{
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
                ;

            if (auto* editContext = serialize->GetEditContext(); editContext)
            {
                editContext
                    ->Class<TerrainMacroMaterialConfig>(
                        "Terrain Macro Material Component", "Provide a terrain macro material for a region of the world")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_macroColorAsset, "Color Texture",
                        "Terrain macro color texture for use by any terrain inside the bounding box on this entity.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_macroNormalAsset, "Normal Texture",
                        "Terrain macro normal texture for use by any terrain inside the bounding box on this entity.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_normalFlipX, "Normal Flip X",
                        "Flip the X values of the normal map.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_normalFlipY, "Normal Flip Y",
                        "Flip the Y values of the normal map.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainMacroMaterialConfig::m_normalFactor, "Normal Factor",
                        "Normal values scale factor.")
                    ;
            }
        }
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
        // Clear out our shape bounds and make sure the material is queued to load.
        m_cachedShapeBounds = AZ::Aabb::CreateNull();
        m_configuration.m_macroColorAsset.QueueLoad();
        m_configuration.m_macroNormalAsset.QueueLoad();

        // Don't mark our material as active until it's finished loading and is valid.
        m_macroMaterialActive = false;

        // Listen for the material asset to complete loading.
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_macroColorAsset.GetId());
        AZ::Data::AssetBus::MultiHandler::BusConnect(m_configuration.m_macroNormalAsset.GetId());
    }

    void TerrainMacroMaterialComponent::Deactivate()
    {
        TerrainMacroMaterialRequestBus::Handler::BusDisconnect();

        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        m_configuration.m_macroColorAsset.Release();
        m_configuration.m_macroNormalAsset.Release();

        // Send out any notifications as appropriate based on the macro material destruction.
        HandleMaterialStateChange();
    }

    bool TerrainMacroMaterialComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainMacroMaterialConfig*>(baseConfig))
        {
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
    }

    void TerrainMacroMaterialComponent::HandleMaterialStateChange()
    {
        /*
        // We only want our component to appear active during the time that the macro material is loaded and valid.  The logic below
        // will handle all transition possibilities to notify if we've become active, inactive, or just changed.  We'll also only
        // keep a valid up-to-date copy of the shape bounds while the material is valid, since we don't need it any other time.

        bool wasPreviouslyActive = m_macroMaterialActive;
        //bool isNowActive = (m_macroMaterialInstance != nullptr);
        bool isNowActive = false;

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

            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialCreated, GetEntityId(), m_macroMaterialInstance,
                m_cachedShapeBounds);
        }
        else if (wasPreviouslyActive && !isNowActive)
        {
            // Stop listening to macro material requests or shape changes, and send out a notification that we no longer have a valid
            // macro material.

            TerrainMacroMaterialRequestBus::Handler::BusDisconnect();
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();

            m_cachedShapeBounds = AZ::Aabb::CreateNull();

            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialDestroyed, GetEntityId());
        }
        else
        {
            // We were active both before and after, so just send out a material changed event.

            TerrainMacroMaterialNotificationBus::Broadcast(
                &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialChanged, GetEntityId(), m_macroMaterialInstance);
        }
        */
    }

    void TerrainMacroMaterialComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        /*
        m_configuration.m_materialAsset = asset;

        if (m_configuration.m_materialAsset.Get()->GetMaterialTypeAsset().GetId() ==
            TerrainMacroMaterialConfig::GetTerrainMacroMaterialTypeAssetId())
        {
            m_macroMaterialInstance = AZ::RPI::Material::FindOrCreate(m_configuration.m_materialAsset);
        }
        else
        {
            AZ_Error("Terrain", false, "Material '%s' has the wrong material type.", m_configuration.m_materialAsset.GetHint().c_str());
            m_macroMaterialInstance.reset();
        }

        // Clear the material asset reference to make sure we don't prevent hot-reloading.
        m_configuration.m_materialAsset.Release();
        */

        HandleMaterialStateChange();
    }

    void TerrainMacroMaterialComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void TerrainMacroMaterialComponent::GetTerrainMacroMaterialData(
        MacroMaterialData& macroMaterial, AZ::Aabb& macroMaterialRegion)
    {
        macroMaterial = m_materialData;
        macroMaterialRegion = m_cachedShapeBounds;
    }
}
