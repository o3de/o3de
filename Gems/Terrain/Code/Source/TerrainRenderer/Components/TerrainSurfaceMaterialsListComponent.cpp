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

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<TerrainSurfaceMaterialMapping>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Property("SurfaceTag", BehaviorValueProperty(&TerrainSurfaceMaterialMapping::m_surfaceTag))
                ->Property("MaterialAsset", BehaviorValueProperty(&TerrainSurfaceMaterialMapping::m_materialAsset))
                ;
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

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<TerrainSurfaceMaterialsListConfig>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Property("DefaultSurfaceMaterial", BehaviorValueProperty(&TerrainSurfaceMaterialsListConfig::m_defaultSurfaceMaterial))
                ->Property("SurfaceMaterials", BehaviorValueProperty(&TerrainSurfaceMaterialsListConfig::m_surfaceMaterials))
                ;
        }
    }

    TerrainSurfaceMaterialsListConfig::TerrainSurfaceMaterialsListConfig()
    {
       m_hideSurfaceTagData.m_attributes.push_back(
           {
               AZ::Edit::Attributes::Visibility,
               &m_hideAttribute
           }
       );
       m_hideSurfaceTagData.m_name = "hideSurfaceTagData";
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
        AZ::TickBus::Handler::BusConnect();

        // Start listening for data requests.
        TerrainAreaMaterialRequestBus::Handler::BusConnect(GetEntityId());

        // Start listening for shape changes.
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());

        // OnShapeChanged() will announce creation if the shape is valid
        OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

        // Set all the materials as inactive and start loading.
        auto handleQueueMaterial = [&](TerrainSurfaceMaterialMapping& mapping)
        {
            mapping.m_previousTag = {};
            mapping.m_materialAsset.Release();
            mapping.m_materialInstance.reset();

            if (mapping.m_materialAsset.GetId().IsValid())
            {
                mapping.m_materialAsset.QueueLoad();
                if (!AZ::Data::AssetBus::MultiHandler::BusIsConnectedId(mapping.m_materialAsset.GetId()))
                {
                    AZ::Data::AssetBus::MultiHandler::BusConnect(mapping.m_materialAsset.GetId());
                }
            }
        };

        handleQueueMaterial(m_configuration.m_defaultSurfaceMaterial);
        for (auto& mapping : m_configuration.m_surfaceMaterials)
        {
            handleQueueMaterial(mapping);
        }
    }

    void TerrainSurfaceMaterialsListComponent::Deactivate()
    {
        // disconnect from busses
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        TerrainAreaMaterialRequestBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

        auto handleResetMaterial = [&](TerrainSurfaceMaterialMapping& mapping)
        {
            if (mapping.m_materialInstance)
            {
                if (&mapping == &m_configuration.m_defaultSurfaceMaterial)
                {
                    TerrainAreaMaterialNotificationBus::Broadcast(
                        &TerrainAreaMaterialNotificationBus::Events::OnTerrainDefaultSurfaceMaterialDestroyed, GetEntityId());
                }
                else
                {
                    TerrainAreaMaterialNotificationBus::Broadcast(
                        &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingDestroyed,
                        GetEntityId(),
                        mapping.m_surfaceTag);
                }
            }

            mapping.m_previousTag = {};
            mapping.m_materialAsset.Release();
            mapping.m_materialInstance.reset();
        };

        handleResetMaterial(m_configuration.m_defaultSurfaceMaterial);
        for (auto& mapping : m_configuration.m_surfaceMaterials)
        {
            handleResetMaterial(mapping);
        }

        m_configuration.m_defaultSurfaceMaterial = {};
        m_configuration.m_surfaceMaterials.clear();

        if (m_cachedAabb.IsValid())
        {
            TerrainAreaMaterialNotificationBus::Broadcast(
                &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingRegionDestroyed, GetEntityId(), m_cachedAabb);
            m_cachedAabb = AZ::Aabb::CreateNull();
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

        if (m_cachedAabb.IsValid() && !oldAabb.IsValid())
        {
            TerrainAreaMaterialNotificationBus::Broadcast(
                &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingRegionCreated, GetEntityId(), m_cachedAabb);
        }
        else if (!m_cachedAabb.IsValid() && oldAabb.IsValid())
        {
            TerrainAreaMaterialNotificationBus::Broadcast(
                &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingRegionDestroyed, GetEntityId(), oldAabb);
        }
        else if (m_cachedAabb.IsValid() && oldAabb.IsValid())
        {
            TerrainAreaMaterialNotificationBus::Broadcast(
                &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingRegionChanged,
                GetEntityId(),
                oldAabb,
                m_cachedAabb);
        }
    }

    const AZ::Aabb& TerrainSurfaceMaterialsListComponent::GetTerrainSurfaceMaterialRegion() const
    {
        return m_cachedAabb;
    }

    const AZStd::vector<TerrainSurfaceMaterialMapping>& TerrainSurfaceMaterialsListComponent::GetSurfaceMaterialMappings() const
    {
        return m_configuration.m_surfaceMaterials;
    }

    const TerrainSurfaceMaterialMapping& TerrainSurfaceMaterialsListComponent::GetDefaultMaterial() const
    {
        return m_configuration.m_defaultSurfaceMaterial;
    }

    void TerrainSurfaceMaterialsListComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        auto handleTagChanges = [&](TerrainSurfaceMaterialMapping& mapping)
        {
            if (mapping.m_materialInstance && mapping.m_previousTag != mapping.m_surfaceTag)
            {
                TerrainAreaMaterialNotificationBus::Broadcast(
                    &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingTagChanged,
                    GetEntityId(),
                    mapping.m_previousTag,
                    mapping.m_surfaceTag);
                mapping.m_previousTag = mapping.m_surfaceTag;
            }
        };

        for (auto& mapping : m_configuration.m_surfaceMaterials)
        {
            handleTagChanges(mapping);
        }
    }

    void TerrainSurfaceMaterialsListComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Find the missing material instance with the correct id.
        auto handleCreateMaterial = [&](TerrainSurfaceMaterialMapping& mapping, const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            if (mapping.m_materialAsset.GetId() == asset.GetId())
            {
                mapping.m_materialAsset = asset;
                mapping.m_materialInstance = AZ::RPI::Material::FindOrCreate(mapping.m_materialAsset);

                if (&mapping == &m_configuration.m_defaultSurfaceMaterial)
                {
                    TerrainAreaMaterialNotificationBus::Broadcast(
                        &TerrainAreaMaterialNotificationBus::Events::OnTerrainDefaultSurfaceMaterialCreated,
                        GetEntityId(),
                        mapping.m_materialInstance);
                }
                else
                {
                    TerrainAreaMaterialNotificationBus::Broadcast(
                        &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingCreated,
                        GetEntityId(),
                        mapping.m_surfaceTag,
                        mapping.m_materialInstance);
                    mapping.m_previousTag = mapping.m_surfaceTag;
                }
            }
        };

        handleCreateMaterial(m_configuration.m_defaultSurfaceMaterial, asset);
        for (auto& mapping : m_configuration.m_surfaceMaterials)
        {
            handleCreateMaterial(mapping, asset);
        }
    }

    void TerrainSurfaceMaterialsListComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // Find the material instance with the correct id.
        auto handleUpdateMaterial = [&](TerrainSurfaceMaterialMapping& mapping, const AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            if (mapping.m_materialAsset.GetId() == asset.GetId())
            {
                mapping.m_materialAsset = asset;
                mapping.m_materialInstance = AZ::RPI::Material::FindOrCreate(mapping.m_materialAsset);

                if (&mapping == &m_configuration.m_defaultSurfaceMaterial)
                {
                    TerrainAreaMaterialNotificationBus::Broadcast(
                        &TerrainAreaMaterialNotificationBus::Events::OnTerrainDefaultSurfaceMaterialChanged,
                        GetEntityId(),
                        mapping.m_materialInstance);
                }
                else
                {
                    TerrainAreaMaterialNotificationBus::Broadcast(
                        &TerrainAreaMaterialNotificationBus::Events::OnTerrainSurfaceMaterialMappingMaterialChanged,
                        GetEntityId(),
                        mapping.m_surfaceTag,
                        mapping.m_materialInstance);
                    mapping.m_previousTag = mapping.m_surfaceTag;
                }
            }
        };

        handleUpdateMaterial(m_configuration.m_defaultSurfaceMaterial, asset);
        for (auto& mapping : m_configuration.m_surfaceMaterials)
        {
            handleUpdateMaterial(mapping, asset);
        }
    }
} // namespace Terrain
