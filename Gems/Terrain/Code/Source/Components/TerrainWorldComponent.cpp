/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <AzFramework/Terrain/TerrainDataRequestBus.h>

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
                edit->Class<TerrainWorldConfig>("Terrain World Component", "Data required for the terrain system to run")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_worldMin, "World Bounds (Min)", "")
                    // Temporary constraint until the rest of the Terrain system is updated to support larger worlds.
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &TerrainWorldConfig::ValidateWorldMin)
                    ->Attribute(AZ::Edit::Attributes::Min, -2048.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 2048.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_worldMax, "World Bounds (Max)", "")
                    // Temporary constraint until the rest of the Terrain system is updated to support larger worlds.
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &TerrainWorldConfig::ValidateWorldMax)
                    ->Attribute(AZ::Edit::Attributes::Min, -2048.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 2048.0f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_heightQueryResolution, "Height Query Resolution (m)", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &TerrainWorldConfig::ValidateWorldHeight);
            }
        }
    }

    void TerrainWorldComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainService"));
    }

    void TerrainWorldComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainService"));
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
        // Currently, the Terrain System Component owns the Terrain System instance because the Terrain World component gets recreated
        // every time an entity is added or removed to a level.  If this ever changes, the Terrain System ownership could move into
        // the level component.
        TerrainSystemServiceRequestBus::Broadcast(&TerrainSystemServiceRequestBus::Events::Activate);

        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequestBus::Events::SetTerrainAabb,
            AZ::Aabb::CreateFromMinMax(m_configuration.m_worldMin, m_configuration.m_worldMax));
        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
            &AzFramework::Terrain::TerrainDataRequestBus::Events::SetTerrainHeightQueryResolution, m_configuration.m_heightQueryResolution);
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

    float TerrainWorldConfig::NumberOfSamples(AZ::Vector3* min, AZ::Vector3* max, AZ::Vector2* heightQuery)
    {
        float numberOfSamples = ((max->GetX() - min->GetX()) / heightQuery->GetX()) * ((max->GetY() - min->GetY()) / heightQuery->GetY());
        return numberOfSamples;
    }

    AZ::Outcome<void, AZStd::string> TerrainWorldConfig::DetermineMessage(float numSamples)
    {
        const float maximumSamplesAllowed = 8.0f * 1024.0f * 1024.0f;
        if (numSamples < maximumSamplesAllowed)
        {
            return AZ::Success();
        }
        return AZ::Failure(AZStd::string("The number of samples exceeds the maximum allowed."));
    }

    AZ::Outcome<void, AZStd::string> TerrainWorldConfig::ValidateWorldMin(void* newValue, [[maybe_unused]]const AZ::Uuid& valueType)
    {
        AZ::Vector3 minValue = *static_cast<AZ::Vector3*>(newValue);

        return DetermineMessage(NumberOfSamples(&minValue, &m_worldMax, &m_heightQueryResolution));
    }

    AZ::Outcome<void, AZStd::string> TerrainWorldConfig::ValidateWorldMax(void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
    {
        AZ::Vector3 maxValue = *static_cast<AZ::Vector3*>(newValue);

        return DetermineMessage(NumberOfSamples(&m_worldMin, &maxValue, &m_heightQueryResolution));
    }

    AZ::Outcome<void, AZStd::string> TerrainWorldConfig::ValidateWorldHeight(void* newValue, [[maybe_unused]] const AZ::Uuid& valueType)
    {
        AZ::Vector2 heightValue = *static_cast<AZ::Vector2*>(newValue);

        return DetermineMessage(NumberOfSamples(&m_worldMin, &m_worldMax, &heightValue));
    }

} // namespace Terrain
