/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Components/TerrainSurfaceMaterialsListComponent.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>

#include <TerrainSystem/TerrainSystemBus.h>

namespace Terrain
{
    void TerrainSurfaceMaterialMapping::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<TerrainSurfaceMaterialMapping>()
                ->Version(1)
                ->Field("Surface", &TerrainSurfaceMaterialMapping::m_surfaceTag)
                ->Field("MaterialAsset", &TerrainSurfaceMaterialMapping::m_materialAsset);

            if (auto edit = serialize->GetEditContext())
            {
                edit->Class<TerrainSurfaceMaterialMapping>("Terrain Surface Gradient Mapping", "Mapping between a surface and a material.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TerrainSurfaceMaterialMapping::m_surfaceTag, "Surface Tag", "Surface type to map to a material.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainSurfaceMaterialMapping::m_materialAsset, "Material Asset", "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, true)
                    ;
            }
        }
    }

    void TerrainSurfaceMaterialsListConfig::Reflect(AZ::ReflectContext* context)
    {
        TerrainSurfaceMaterialMapping::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainSurfaceMaterialsListConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("Mappings", &TerrainSurfaceMaterialsListConfig::m_surfaceMaterials);

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainSurfaceMaterialsListConfig>(
                        "Terrain Surface Material List Component", "Provide mapping between surfaces and render materials.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceMaterialsListConfig::m_surfaceMaterials,
                        "Gradient to Material Mappings", "Maps surfaces to materials.");
            }
        }
    }

    void TerrainSurfaceMaterialsListComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainMaterialProviderService"));
    }

    void TerrainSurfaceMaterialsListComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainMaterialProviderService"));
    }

    void TerrainSurfaceMaterialsListComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
    }

    void TerrainSurfaceMaterialsListComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainSurfaceMaterialsListConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainSurfaceMaterialsListComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainSurfaceMaterialsListComponent::m_configuration);
        }
    }

    TerrainSurfaceMaterialsListComponent::TerrainSurfaceMaterialsListComponent(const TerrainSurfaceMaterialsListConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainSurfaceMaterialsListComponent::Activate()
    {
        m_cachedAabb = AZ::Aabb::CreateNull();

        // Set all the materials as inactive and start loading.
        for (auto& surfaceMaterialMapping : m_configuration.m_surfaceMaterials)
        {
            if (surfaceMaterialMapping.m_materialAsset.GetId().IsValid())
            {
                surfaceMaterialMapping.m_active = false;
                surfaceMaterialMapping.m_materialAsset.QueueLoad();
                AZ::Data::AssetBus::MultiHandler::BusConnect(surfaceMaterialMapping.m_materialAsset.GetId());
            }
        }

        // Announce initial shape using OnShapeChanged
        OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void TerrainSurfaceMaterialsListComponent::Deactivate()
    {
        TerrainAreaMaterialRequestBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();

        for (auto& surfaceMaterialMapping : m_configuration.m_surfaceMaterials)
        {
            if (surfaceMaterialMapping.m_materialAsset.GetId().IsValid())
            {
                surfaceMaterialMapping.m_materialAsset.Release();
                surfaceMaterialMapping.m_materialInstance.reset();
                surfaceMaterialMapping.m_activeMaterialAssetId = AZ::Data::AssetId();
            }
        }

        HandleMaterialStateChanges();
    }

    int TerrainSurfaceMaterialsListComponent::CountMaterialIDInstances(AZ::Data::AssetId id) const
    {
        int count = 0;

        for (const auto& surfaceMaterialMapping : m_configuration.m_surfaceMaterials)
        {
            if (surfaceMaterialMapping.m_activeMaterialAssetId == id)
            {
                count++;
            }
        }

        return count;
    }

    void TerrainSurfaceMaterialsListComponent::HandleMaterialStateChanges()
    {
        bool anyMaterialIsActive = false;
        bool anyMaterialWasAlreadyActive = false;

        for (auto& surfaceMaterialMapping : m_configuration.m_surfaceMaterials)
        {
            const bool wasPreviouslyActive = surfaceMaterialMapping.m_active;
            const bool isNowActive = (surfaceMaterialMapping.m_materialInstance != nullptr);

            if (wasPreviouslyActive)
            {
                anyMaterialWasAlreadyActive = true;
            }

            if (isNowActive)
            {
                anyMaterialIsActive = true;
            }

            surfaceMaterialMapping.m_active = isNowActive;

            if (!wasPreviouslyActive && !isNowActive)
            {
                // A material has been assigned but has not yet completed loading.
            }
            else if (!wasPreviouslyActive && isNowActive)
            {
                // Remember the asset id so we can disconnect from the AssetBus if the material asset is removed.
                surfaceMaterialMapping.m_activeMaterialAssetId = surfaceMaterialMapping.m_materialAsset.GetId();

                TerrainAreaMaterialNotificationBus::Broadcast(
                    &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingCreated, GetEntityId(),
                    surfaceMaterialMapping.m_surfaceTag,
                    surfaceMaterialMapping.m_materialInstance);
            }
            else if (wasPreviouslyActive && !isNowActive)
            {
                // Don't disconnect from the AssetBus if this material is mapped more than once.
                if (CountMaterialIDInstances(surfaceMaterialMapping.m_activeMaterialAssetId) == 1)
                {
                    AZ::Data::AssetBus::MultiHandler::BusDisconnect(surfaceMaterialMapping.m_activeMaterialAssetId);
                }

                surfaceMaterialMapping.m_activeMaterialAssetId = AZ::Data::AssetId();

                TerrainAreaMaterialNotificationBus::Broadcast(
                    &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingDestroyed, GetEntityId(),
                    surfaceMaterialMapping.m_surfaceTag);
            }
            else
            {
                TerrainAreaMaterialNotificationBus::Broadcast(
                    &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingChanged, GetEntityId(),
                    surfaceMaterialMapping.m_surfaceTag,
                    surfaceMaterialMapping.m_materialInstance);
            }
        }

        if (!anyMaterialWasAlreadyActive && anyMaterialIsActive)
        {
            // Cache the current shape bounds.
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                m_cachedAabb, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

            // Start listening for data requests.
            TerrainAreaMaterialRequestBus::Handler::BusConnect(GetEntityId());

            // Start listening for shape changes.
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());

        }
        else if (anyMaterialWasAlreadyActive && !anyMaterialIsActive)
        {
            // All materials have been deactivated, stop listening for requests and notifications.
            m_cachedAabb = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
            TerrainAreaMaterialRequestBus::Handler::BusConnect(GetEntityId());
        }
    }

    bool TerrainSurfaceMaterialsListComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainSurfaceMaterialsListConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainSurfaceMaterialsListComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainSurfaceMaterialsListConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainSurfaceMaterialsListComponent::OnShapeChanged([[maybe_unused]] ShapeComponentNotifications::ShapeChangeReasons reasons)
     {
         AZ::Aabb oldAabb = m_cachedAabb;

         LmbrCentral::ShapeComponentRequestsBus::EventResult(
             m_cachedAabb, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

         TerrainAreaMaterialNotificationBus::Broadcast(
             &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingRegionChanged, GetEntityId(), oldAabb,
             m_cachedAabb);
     }
    
    const AZ::Aabb& TerrainSurfaceMaterialsListComponent::GetTerrainSurfaceMaterialRegion() const
    {
        return m_cachedAabb;
    }

    const AZStd::vector<TerrainSurfaceMaterialMapping>& TerrainSurfaceMaterialsListComponent::GetSurfaceMaterialMappings() const
    {
        return m_configuration.m_surfaceMaterials;
    }

    void TerrainSurfaceMaterialsListComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Find the missing material instance with the correct id.
        for (auto& surfaceMaterialMapping : m_configuration.m_surfaceMaterials)
        {
            if (surfaceMaterialMapping.m_materialAsset.GetId() == asset.GetId() &&
                (!surfaceMaterialMapping.m_materialInstance ||
                    surfaceMaterialMapping.m_materialInstance->GetAssetId() != surfaceMaterialMapping.m_materialAsset.GetId()))
            {
                surfaceMaterialMapping.m_materialInstance = AZ::RPI::Material::FindOrCreate(surfaceMaterialMapping.m_materialAsset);
                surfaceMaterialMapping.m_materialAsset.Release();
            }
        }
        HandleMaterialStateChanges();
    }

    void TerrainSurfaceMaterialsListComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }
} // namespace Terrain
