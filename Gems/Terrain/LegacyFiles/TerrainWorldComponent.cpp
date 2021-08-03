/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "TerrainWorldComponent.h"
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <TerrainRenderNode.h>

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
                ->Field("RegionBounds", &TerrainWorldConfig::m_regionBounds)
                ->Field("HeightmapCellSize", &TerrainWorldConfig::m_heightmapCellSize)
                ->Field("WorldMaterial", &TerrainWorldConfig::m_worldMaterialAssetName)
            ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainWorldConfig>(
                    "Terrain World Component", "Data required for the terrain system to run")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC("Level", 0x9aeacc13) }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_worldMin, "World Min", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_worldMax, "World Max", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_regionBounds, "Region Bounds", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_heightmapCellSize, "Heightmap Cell Size", "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldConfig::m_worldMaterialAssetName, "World Material", "")
                ;
            }
        }
    }

    void TerrainWorldComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainService", 0x28ee7719));
    }

    void TerrainWorldComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("TerrainService", 0x28ee7719));
    }

    void TerrainWorldComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
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

    void TerrainWorldComponent::Activate()
    {
        m_terrainRenderNode = new TerrainRenderNode("Materials/Terrain/TerrainSystem.mtl");
        m_terrainProvider = new TerrainProvider();

        m_terrainProvider->SetWorldMin(m_configuration.m_worldMin);
        m_terrainProvider->SetWorldMax(m_configuration.m_worldMax);
        m_terrainProvider->SetRegionBounds(m_configuration.m_regionBounds);
        m_terrainProvider->SetHeightmapCellSize(m_configuration.m_heightmapCellSize);
        m_terrainProvider->SetMaterialName(m_configuration.m_worldMaterialAssetName);
    }

    void TerrainWorldComponent::Deactivate()
    {
        delete m_terrainProvider;
        delete m_terrainRenderNode;
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

    void TerrainWorldComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
    }

    void TerrainWorldComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void TerrainWorldComponent::LoadAssets()
    {
    }


    bool TerrainWorldComponent::IsFullyLoaded() const
    {
        return true;
    }
}
