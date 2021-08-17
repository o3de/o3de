/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainWorldComponent.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Terrain
{
    void TerrainWorldConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainWorldConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("WorldMin", &TerrainWorldConfig::m_worldMin)
                ->Field("WorldMax", &TerrainWorldConfig::m_worldMax)
                ->Field("HeightQueryResolution", &TerrainWorldConfig::m_heightQueryResolution)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainWorldConfig>(
                    "Terrain World Component", "Data required for the terrain system to run")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC("Level") }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_worldMin, "World Bounds (Min)", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_worldMax, "World Bounds (Max)", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_heightQueryResolution, "Height Query Resolution (m)", "")
                ;
            }
        }
    }

    void TerrainWorldComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainService"));
    }

    void TerrainWorldComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainService"));
    }

    void TerrainWorldComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void TerrainWorldComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainWorldConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainWorldComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TerrainWorldComponent::m_configuration)
            ;
        }
    }

    TerrainWorldComponent::TerrainWorldComponent(const TerrainWorldConfig& configuration)
        : m_configuration(configuration)
    {
    }

    TerrainWorldComponent::~TerrainWorldComponent()
    {
    }

    void TerrainWorldComponent::Activate()
    {
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::SetWorldMin, m_configuration.m_worldMin);
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::SetWorldMax, m_configuration.m_worldMax);
        TerrainSystemServiceRequestBus::Broadcast(
            &TerrainSystemServiceRequestBus::Events::SetHeightQueryResolution, m_configuration.m_heightQueryResolution);
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::Activate);
    }

    void TerrainWorldComponent::Deactivate()
    {
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::Deactivate);
    }

    bool TerrainWorldComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainWorldConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainWorldComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainWorldConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }
}
