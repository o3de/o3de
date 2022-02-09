/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TerrainRenderer/Components/TerrainSurfaceMaterialsListComponent.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
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
                edit->Class<TerrainSurfaceMaterialMapping>("Terrain surface gradient mapping", "Mapping between a surface and a material.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TerrainSurfaceMaterialMapping::m_surfaceTag, "Surface tag", "Surface type to map to a material.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainSurfaceMaterialMapping::m_materialAsset, "Material asset", "")
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
                ->Version(2)
                ->Field("DefaultMaterial", &TerrainSurfaceMaterialsListConfig::m_defaultSurfaceMaterial)
                ->Field("Mappings", &TerrainSurfaceMaterialsListConfig::m_surfaceMaterials);

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainSurfaceMaterialsListConfig>(
                        "Terrain Surface Material List Component", "Provide mapping between surfaces and render materials.")
                    ->SetDynamicEditDataProvider(&TerrainSurfaceMaterialsListConfig::GetDynamicData)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainSurfaceMaterialsListConfig::m_defaultSurfaceMaterial,
                        "Default Material", "The default material to fall back to where no other material surface mappings exist.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainSurfaceMaterialsListConfig::m_surfaceMaterials,
                        "Material Mappings", "Maps surfaces to materials.");
            }
        }
    }

    TerrainSurfaceMaterialsListConfig::TerrainSurfaceMaterialsListConfig()
    {
       m_hideSurfaceTagData.m_attributes.push_back(
           {
               AZ::Edit::Attributes::Visibility,
               aznew AZ::Edit::AttributeData<AZ::Crc32>(AZ::Edit::PropertyVisibility::Hide)
           }
       );
    }

    const AZ::Edit::ElementData* TerrainSurfaceMaterialsListConfig::GetDynamicData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& )
    {
        const TerrainSurfaceMaterialsListConfig* owner = reinterpret_cast<const TerrainSurfaceMaterialsListConfig*>(handlerPtr);
        if (elementPtr == &owner->m_defaultSurfaceMaterial.m_surfaceTag)
        {
            return &owner->m_hideSurfaceTagData;
        }
        return nullptr;
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

        auto checkLoadMaterial = [&](TerrainSurfaceMaterialMapping& material)
        {
            if (material.m_materialAsset.GetId().IsValid())
            {
                material.m_active = false;
                material.m_materialAsset.QueueLoad();
                AZ::Data::AssetBus::MultiHandler::BusConnect(material.m_materialAsset.GetId());
            }
        };

        // Set all the materials as inactive and start loading.
        checkLoadMaterial(m_configuration.m_defaultSurfaceMaterial);
        for (auto& surfaceMaterialMapping : m_configuration.m_surfaceMaterials)
        {
            checkLoadMaterial(surfaceMaterialMapping);
        }

        // Announce initial shape using OnShapeChanged
        OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void TerrainSurfaceMaterialsListComponent::Deactivate()
    {
        TerrainAreaMaterialRequestBus::Handler::BusDisconnect();

        auto checkResetMaterial = [&](TerrainSurfaceMaterialMapping& material)
        {
            if (material.m_materialAsset.GetId().IsValid())
            {
                AZ::Data::AssetBus::MultiHandler::BusDisconnect(material.m_materialAsset.GetId());
                material.m_materialAsset.Release();
                material.m_materialInstance.reset();
                material.m_activeMaterialAssetId = AZ::Data::AssetId();
            }
        };
        
        checkResetMaterial(m_configuration.m_defaultSurfaceMaterial);
        for (auto& surfaceMaterialMapping : m_configuration.m_surfaceMaterials)
        {
            checkResetMaterial(surfaceMaterialMapping);
        }

        HandleMaterialStateChanges();
    }

    int TerrainSurfaceMaterialsListComponent::CountMaterialIdInstances(AZ::Data::AssetId id) const
    {
        int count = 0;

        if (m_configuration.m_defaultSurfaceMaterial.m_activeMaterialAssetId == id)
        {
            count++;
        }

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

        {
            // Handle default material first
            auto& defaultMaterial = m_configuration.m_defaultSurfaceMaterial;

            const bool wasPreviouslyActive = defaultMaterial.m_active;
            defaultMaterial.m_active = (defaultMaterial.m_materialInstance != nullptr);

            anyMaterialWasAlreadyActive = wasPreviouslyActive;
            anyMaterialIsActive = defaultMaterial.m_active;
            
            if (!wasPreviouslyActive && !defaultMaterial.m_active)
            {
                // A material has been assigned but has not yet completed loading.
            }
            else if (!wasPreviouslyActive && defaultMaterial.m_active)
            {
                TerrainAreaMaterialNotificationBus::Broadcast(
                    &TerrainAreaMaterialNotificationBus::Events::OnTerrainDefaultSurfaceMaterialCreated, GetEntityId(),
                    defaultMaterial.m_materialInstance);
                defaultMaterial.m_previousChangeId = defaultMaterial.m_materialInstance->GetCurrentChangeId();
            }
            else if (wasPreviouslyActive && !defaultMaterial.m_active)
            {
                // Don't disconnect from the AssetBus if this material is mapped more than once.
                if (CountMaterialIdInstances(defaultMaterial.m_activeMaterialAssetId) == 1)
                {
                    AZ::Data::AssetBus::MultiHandler::BusDisconnect(defaultMaterial.m_activeMaterialAssetId);
                }
                defaultMaterial = {};

                TerrainAreaMaterialNotificationBus::Broadcast(
                    &TerrainAreaMaterialNotificationBus::Events::OnTerrainDefaultSurfaceMaterialDestroyed, GetEntityId());
            }
            else if (defaultMaterial.m_materialInstance->GetAssetId() != defaultMaterial.m_activeMaterialAssetId ||
                defaultMaterial.m_materialInstance->GetCurrentChangeId() != defaultMaterial.m_previousChangeId)
            {
                defaultMaterial.m_previousChangeId = defaultMaterial.m_materialInstance->GetCurrentChangeId();
                defaultMaterial.m_activeMaterialAssetId = defaultMaterial.m_materialInstance->GetAssetId();

                TerrainAreaMaterialNotificationBus::Broadcast(
                    &TerrainAreaMaterialNotificationBus::Events::OnTerrainDefaultSurfaceMaterialChanged, GetEntityId(), defaultMaterial.m_materialInstance);
            }
        }

        for (auto& surfaceMaterialMapping : m_configuration.m_surfaceMaterials)
        {
            const bool wasPreviouslyActive = surfaceMaterialMapping.m_active;
            surfaceMaterialMapping.m_active = surfaceMaterialMapping.m_materialInstance != nullptr;

            anyMaterialWasAlreadyActive = anyMaterialWasAlreadyActive || wasPreviouslyActive;
            anyMaterialIsActive = anyMaterialIsActive || surfaceMaterialMapping.m_active;

            if (!wasPreviouslyActive && !surfaceMaterialMapping.m_active)
            {
                // A material has been assigned but has not yet completed loading.
            }
            else if (!wasPreviouslyActive && surfaceMaterialMapping.m_active)
            {
                // Remember the asset id so we can disconnect from the AssetBus if the material asset is removed.
                surfaceMaterialMapping.m_activeMaterialAssetId = surfaceMaterialMapping.m_materialAsset.GetId();

                TerrainAreaMaterialNotificationBus::Broadcast(
                    &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingCreated, GetEntityId(),
                    surfaceMaterialMapping.m_surfaceTag,
                    surfaceMaterialMapping.m_materialInstance);
                
                surfaceMaterialMapping.m_previousChangeId = surfaceMaterialMapping.m_materialInstance->GetCurrentChangeId();
                surfaceMaterialMapping.m_previousTag = surfaceMaterialMapping.m_surfaceTag;
            }
            else if (wasPreviouslyActive && !surfaceMaterialMapping.m_active)
            {
                // Don't disconnect from the AssetBus if this material is mapped more than once.
                if (CountMaterialIdInstances(surfaceMaterialMapping.m_activeMaterialAssetId) == 1)
                {
                    AZ::Data::AssetBus::MultiHandler::BusDisconnect(surfaceMaterialMapping.m_activeMaterialAssetId);
                }

                surfaceMaterialMapping.m_activeMaterialAssetId = {};
                surfaceMaterialMapping.m_previousChangeId = AZ::RPI::Material::DEFAULT_CHANGE_ID;
                surfaceMaterialMapping.m_previousTag = {};

                TerrainAreaMaterialNotificationBus::Broadcast(
                    &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingDestroyed, GetEntityId(),
                    surfaceMaterialMapping.m_surfaceTag);
            }
            else 
            {
                if (surfaceMaterialMapping.m_previousTag != surfaceMaterialMapping.m_surfaceTag)
                {
                    TerrainAreaMaterialNotificationBus::Broadcast(
                        &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingTagChanged, GetEntityId(),
                        surfaceMaterialMapping.m_previousTag,
                        surfaceMaterialMapping.m_surfaceTag);
                    surfaceMaterialMapping.m_previousTag = surfaceMaterialMapping.m_surfaceTag;
                }
                if (surfaceMaterialMapping.m_materialInstance->GetAssetId() != surfaceMaterialMapping.m_activeMaterialAssetId ||
                    surfaceMaterialMapping.m_materialInstance->GetCurrentChangeId() != surfaceMaterialMapping.m_previousChangeId)
                {
                    surfaceMaterialMapping.m_previousChangeId = surfaceMaterialMapping.m_materialInstance->GetCurrentChangeId();
                    surfaceMaterialMapping.m_activeMaterialAssetId = surfaceMaterialMapping.m_materialInstance->GetAssetId();

                    TerrainAreaMaterialNotificationBus::Broadcast(
                        &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingMaterialChanged, GetEntityId(),
                        surfaceMaterialMapping.m_surfaceTag,
                        surfaceMaterialMapping.m_materialInstance);
                }
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
            TerrainAreaMaterialRequestBus::Handler::BusDisconnect();
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
        auto checkUpdateMaterialAsset = [](TerrainSurfaceMaterialMapping& mapping, const AZ::Data::Asset<AZ::Data::AssetData>& asset) -> bool
        {
            if (mapping.m_materialAsset.GetId() == asset.GetId() &&
                (!mapping.m_materialInstance || mapping.m_materialInstance->GetAssetId() != mapping.m_materialAsset.GetId()))
            {
                mapping.m_materialInstance = AZ::RPI::Material::FindOrCreate(mapping.m_materialAsset);
                mapping.m_materialAsset.Release();
                return true;
            }
            return false;
        };

        // First check the default material
        if (!checkUpdateMaterialAsset(m_configuration.m_defaultSurfaceMaterial, asset))
        {
            // If the default materail wasn't updated, then check all the surface material mappings.
            for (auto& surfaceMaterialMapping : m_configuration.m_surfaceMaterials)
            {
                if (checkUpdateMaterialAsset(surfaceMaterialMapping, asset))
                {
                    break;
                }
            }
        }
        HandleMaterialStateChanges();
    }

    void TerrainSurfaceMaterialsListComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }
} // namespace Terrain
