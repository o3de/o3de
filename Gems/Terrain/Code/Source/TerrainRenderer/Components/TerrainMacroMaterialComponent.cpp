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
        // Start listening for terrain macro material requests.  We connect to this first, since some of the notifications below
        // might result in calls to this bus.
        TerrainMacroMaterialRequestBus::Handler::BusConnect(GetEntityId());

        // Start listening for shape changes.
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());

        // Refresh our cached bounds and notify any listeners that the shape has changed.
        m_cachedShapeBounds = AZ::Aabb::CreateNull();
        OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

        // Start loading the material and listen for its completion.
        m_configuration.m_materialAsset.QueueLoad();
        AZ::Data::AssetBus::Handler::BusConnect(m_configuration.m_materialAsset.GetId());
    }

    void TerrainMacroMaterialComponent::Deactivate()
    {
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();

        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_configuration.m_materialAsset.Release();

        TerrainMacroMaterialNotificationBus::Broadcast(
            &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialChanged, m_macroMaterialInstance);

        m_cachedShapeBounds = AZ::Aabb::CreateNull();

        TerrainMacroMaterialRequestBus::Handler::BusDisconnect();
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
        AZ::Aabb oldShapeBounds = m_cachedShapeBounds;

        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            m_cachedShapeBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        TerrainMacroMaterialNotificationBus::Broadcast(
            &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialRegionChanged,
            oldShapeBounds, m_cachedShapeBounds);
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

        m_configuration.m_materialAsset.Release();

        TerrainMacroMaterialNotificationBus::Broadcast(
            &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialChanged, m_macroMaterialInstance);
    }

    void TerrainMacroMaterialComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
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

        m_configuration.m_materialAsset.Release();

        TerrainMacroMaterialNotificationBus::Broadcast(
            &TerrainMacroMaterialNotificationBus::Events::OnTerrainMacroMaterialChanged, m_macroMaterialInstance);
    }

    void TerrainMacroMaterialComponent::GetTerrainMacroMaterialData(
        AZ::Data::Instance<AZ::RPI::Material>& macroMaterial, AZ::Aabb& macroMaterialRegion)
    {
        macroMaterial = m_macroMaterialInstance;
        macroMaterialRegion = m_cachedShapeBounds;
    }
}
