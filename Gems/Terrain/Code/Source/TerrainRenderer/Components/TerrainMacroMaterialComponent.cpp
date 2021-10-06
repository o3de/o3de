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
    AZ::Data::AssetId TerrainMacroMaterialConfig::s_macroMaterialTypeAssetId{};

    void TerrainMacroMaterialConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainMacroMaterialConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("MacroMaterial", &TerrainMacroMaterialConfig::m_materialAsset)
            ;

            // The edit context for this appears in EditorTerrainMacroMaterialComponent.cpp.
        }
    }

    AZ::Data::AssetId TerrainMacroMaterialConfig::GetTerrainMacroMaterialTypeAssetId()
    {
        // Get the Asset ID for the TerrainMacroMaterial material type and store it in a class static so that we don't have to look it
        // up again.
        if (!s_macroMaterialTypeAssetId.IsValid())
        {
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                s_macroMaterialTypeAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, TerrainMacroMaterialTypeAsset,
                azrtti_typeid<AZ::RPI::MaterialTypeAsset>(), false);
            AZ_Assert(s_macroMaterialTypeAssetId.IsValid(), "The asset '%s' couldn't be found.", TerrainMacroMaterialTypeAsset);
        }

        return s_macroMaterialTypeAssetId;
    }

    bool TerrainMacroMaterialConfig::IsMaterialTypeCorrect(const AZ::Data::AssetId& assetId)
    {
        // We'll verify that whatever material we try to load has this material type as a dependency, as a way to implicitly detect
        // that we're only trying to use terrain macro materials even before we load the asset.
        auto macroMaterialTypeAssetId = GetTerrainMacroMaterialTypeAssetId();

        // Get the dependencies for the requested asset.
        AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            result, &AZ::Data::AssetCatalogRequestBus::Events::GetDirectProductDependencies, assetId);

        // If any of the dependencies match the TerrainMacroMaterial materialtype asset, then this should be the correct type of material.
        if (result)
        {
            for (auto& dependency : result.GetValue())
            {
                if (dependency.m_assetId == macroMaterialTypeAssetId)
                {
                    return true;
                }
            }
        }

        // Didn't have the expected dependency, so it must not be the right material type.
        return false;
    }

    AZ::Outcome<void, AZStd::string> TerrainMacroMaterialConfig::ValidateMaterialAsset(void* newValue, const AZ::Uuid& valueType)
    {
        if (azrtti_typeid<AZ::Data::Asset<AZ::RPI::MaterialAsset>>() != valueType)
        {
            AZ_Assert(false, "Unexpected value type");
            return AZ::Failure(AZStd::string("Unexpectedly received something other than a material asset for the MacroMaterial!"));
        }

        auto newMaterialAsset = *static_cast<AZ::Data::Asset<AZ::RPI::MaterialAsset>*>(newValue);

        if (!IsMaterialTypeCorrect(newMaterialAsset.GetId()))
        {
            return AZ::Failure(AZStd::string::format(
                "The selected MacroMaterial ('%s') needs to use the TerrainMacroMaterial material type.",
                newMaterialAsset.GetHint().c_str()));
        }

        return AZ::Success();
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
        m_configuration.m_materialAsset.QueueLoad();

        // Don't mark our material as active until it's finished loading and is valid.
        m_macroMaterialActive = false;

        // Listen for the material asset to complete loading.
        AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_materialAsset.GetId());
    }

    void TerrainMacroMaterialComponent::Deactivate()
    {
        TerrainMacroMaterialRequestBus::Handler::BusDisconnect();

        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_configuration.m_materialAsset.Release();

        m_macroMaterialInstance.reset();

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
        // We only want our component to appear active during the time that the macro material is loaded and valid.  The logic below
        // will handle all transition possibilities to notify if we've become active, inactive, or just changed.  We'll also only
        // keep a valid up-to-date copy of the shape bounds while the material is valid, since we don't need it any other time.

        bool wasPreviouslyActive = m_macroMaterialActive;
        bool isNowActive = (m_macroMaterialInstance != nullptr);

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
    }

    void TerrainMacroMaterialComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
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

        HandleMaterialStateChange();
    }

    void TerrainMacroMaterialComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void TerrainMacroMaterialComponent::GetTerrainMacroMaterialData(
        AZ::Data::Instance<AZ::RPI::Material>& macroMaterial, AZ::Aabb& macroMaterialRegion)
    {
        macroMaterial = m_macroMaterialInstance;
        macroMaterialRegion = m_cachedShapeBounds;
    }
}
