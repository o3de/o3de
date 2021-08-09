/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TerrainLayerSpawnerComponent.h"
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <TerrainProviderBus.h>

namespace Terrain
{
    void TerrainLayerSpawnerConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainLayerSpawnerConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("Layer", &TerrainLayerSpawnerConfig::m_layer)
                ->Field("Priority", &TerrainLayerSpawnerConfig::m_priority)
                ->Field("UseGroundPlane", &TerrainLayerSpawnerConfig::m_useGroundPlane)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainLayerSpawnerConfig>(
                    "Terrain Layer Spawner Component", "Provide terrain data for a region of the world")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TerrainLayerSpawnerConfig::m_layer, "Layer Priority", "Defines a high level order that terrain spawners are applied")
                    ->Attribute(AZ::Edit::Attributes::EnumValues, &TerrainLayerSpawnerConfig::GetSelectableLayers)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &TerrainLayerSpawnerConfig::m_priority, "Sub Priority", "Defines order terrain spawners are applied within a layer.  Larger numbers = higher priority")
                    ->Attribute(AZ::Edit::Attributes::Min, AreaConstants::s_priorityMin)
                    ->Attribute(AZ::Edit::Attributes::Max, AreaConstants::s_priorityMax)
                    ->Attribute(AZ::Edit::Attributes::SoftMin, AreaConstants::s_priorityMin)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, AreaConstants::s_prioritySoftMax)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainLayerSpawnerConfig::m_useGroundPlane, "Use Ground Plane", "Determines whether or not to provide a default ground plane")
                    ;
            }
        }
    }

    AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> TerrainLayerSpawnerConfig::GetSelectableLayers() const
    {
        AZStd::vector<AZStd::pair<AZ::u32, AZStd::string>> selectableLayers;
        selectableLayers.push_back({ AreaConstants::s_backgroundLayer, AZStd::string("Background") });
        selectableLayers.push_back({ AreaConstants::s_foregroundLayer, AZStd::string("Foreground") });
        return selectableLayers;
    }


    void TerrainLayerSpawnerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainAreaService", 0x98f9f606));
    }

    void TerrainLayerSpawnerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainAreaService", 0x98f9f606));
    }

    void TerrainLayerSpawnerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        //services.push_back(AZ_CRC("GradientService", 0x21c18d23));
        //services.push_back(AZ_CRC("SurfaceDataProviderService", 0xfe9fb95e));
        services.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void TerrainLayerSpawnerComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainLayerSpawnerConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainLayerSpawnerComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainLayerSpawnerComponent::m_configuration)
            ;
        }
    }

    TerrainLayerSpawnerComponent::TerrainLayerSpawnerComponent(const TerrainLayerSpawnerConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TerrainLayerSpawnerComponent::Activate()
    {
        //Terrain::HeightmapProviderInternalRequestBus::Handler::BusConnect();

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        TerrainAreaRequestBus::Handler::BusConnect(GetEntityId());

        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::RegisterArea, GetEntityId());
    }

    void TerrainLayerSpawnerComponent::Deactivate()
    {
        TerrainAreaRequestBus::Handler::BusDisconnect();
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::UnregisterArea, GetEntityId());

        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();

        //Terrain::HeightmapProviderInternalRequestBus::Handler::BusDisconnect();
    }

    bool TerrainLayerSpawnerComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainLayerSpawnerConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainLayerSpawnerComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainLayerSpawnerConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void TerrainLayerSpawnerComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        RefreshArea();
    }

    void TerrainLayerSpawnerComponent::OnShapeChanged([[maybe_unused]] ShapeChangeReasons changeReason)
    {
        RefreshArea();
    }

    void TerrainLayerSpawnerComponent::RegisterArea()
    {
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::RegisterArea, GetEntityId());
    }

    void TerrainLayerSpawnerComponent::RefreshArea()
    {
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::RefreshArea, GetEntityId());
    }
}
