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

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

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
                edit->Class<TerrainSurfaceMaterialMapping>("Terrain Surface Gradient Mapping", "Mapping between a gradient and a surface.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceMaterialMapping::m_surfaceTag, "Surface Tag",
                        "Surface type to map to a material.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainSurfaceMaterialMapping::m_materialAsset, "Material Asset", "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, true)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &TerrainSurfaceMaterialMapping::MaterialChanged)
                    ;
            }
        }
    }

    void TerrainSurfaceMaterialMapping::MaterialChanged()
    {
        m_dirty = true;
    }

    void TerrainSurfaceMaterialsListConfig::Reflect(AZ::ReflectContext* context)
    {
        TerrainSurfaceMaterialMapping::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainSurfaceMaterialsListConfig, AZ::ComponentConfig>()->Version(1)->Field(
                "Mappings", &TerrainSurfaceMaterialsListConfig::m_surfaceMaterials);

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainSurfaceMaterialsListConfig>(
                        "Terrain Surface Material List Component", "Provide mapping between gradients and render materials.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceMaterialsListConfig::m_surfaceMaterials,
                        "Gradient to Material Mappings", "Maps Gradient Entities to Materials.");
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
        services.push_back(AZ_CRC_CE("TerrainAreaService"));
        services.push_back(AZ_CRC_CE("BoxShapeService"));
    }

    void TerrainSurfaceMaterialsListComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainSurfaceMaterialsListConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainSurfaceMaterialsListComponent, AZ::Component>()->Version(0)->Field(
                "Configuration", &TerrainSurfaceMaterialsListComponent::m_configuration);
        }
    }

    TerrainSurfaceMaterialsListComponent::TerrainSurfaceMaterialsListComponent(const TerrainSurfaceMaterialsListConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainSurfaceMaterialsListComponent::Activate()
    {
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        TerrainAreaMaterialRequestBus::Handler::BusConnect(GetEntityId());

        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(GetEntityId());

        OnCompositionChanged();
    }

    void TerrainSurfaceMaterialsListComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();

        TerrainAreaMaterialRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        OnCompositionChanged();
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

    void TerrainSurfaceMaterialsListComponent::OnCompositionChanged()
    {
        // Check whether any materials assignments have changed.
        for (auto surfaceMaterialMapping : m_configuration.m_surfaceMaterials)
        {
            if (!surfaceMaterialMapping.m_dirty)
            {
                continue;
            }

            if (surfaceMaterialMapping.m_materialAsset.GetId().IsValid())
            {
                if (!surfaceMaterialMapping.m_materialInstance ||
                    surfaceMaterialMapping.m_materialInstance->GetAssetId() != surfaceMaterialMapping.m_materialAsset.GetId())
                {
                    // A material asset has been assigned: start loading an instance.
                    AZ::Data::AssetBus::Handler::BusConnect(surfaceMaterialMapping.m_materialAsset.GetId());
                    surfaceMaterialMapping.m_materialInstance = AZ::RPI::Material::FindOrCreate(surfaceMaterialMapping.m_materialAsset);
                }
            }
            else
            {
                if (surfaceMaterialMapping.m_materialInstance)
                {
                    // A material asset has been removed: reset the instance.
                    surfaceMaterialMapping.m_materialInstance.reset();
                    OnMaterialInstanceChanged();
                }
            }
            surfaceMaterialMapping.m_dirty = false;
        }
     }

    AZStd::vector<SurfaceMaterialMapping> TerrainSurfaceMaterialsListComponent::GetSurfaceMaterialMappings() const
    {
         AZStd::vector<SurfaceMaterialMapping> mappings;

         for (auto inMapping : m_configuration.m_surfaceMaterials)
         {
             SurfaceMaterialMapping outMapping;
             outMapping.m_surfaceTag = inMapping.m_surfaceTag;
             outMapping.m_materialInstance = inMapping.m_materialInstance;
             mappings.emplace_back(outMapping);
         }

         return mappings;
    }

    void TerrainSurfaceMaterialsListComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect(asset.GetId());

        OnMaterialInstanceChanged();
    }

    void TerrainSurfaceMaterialsListComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::AssetBus::Handler::BusDisconnect(asset.GetId());

        OnMaterialInstanceChanged();
    }

    void TerrainSurfaceMaterialsListComponent::OnMaterialInstanceChanged()
    {
        TerrainAreaMaterialNotificationBus::Broadcast(&TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingChanged, GetEntityId());
    }
} // namespace Terrain
