/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainWorldRendererComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <TerrainRenderer/TerrainFeatureProcessor.h>

namespace Terrain
{
    void TerrainWorldRendererConfig::Reflect(AZ::ReflectContext* context)
    {
        Terrain::TerrainFeatureProcessor::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainWorldRendererConfig, AZ::ComponentConfig>()
                ->Version(1)
                ->Field("WorldSize", &TerrainWorldRendererConfig::m_worldSize)
                ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<TerrainWorldRendererConfig>("Terrain World Renderer Component", "Enables terrain rendering")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &TerrainWorldRendererConfig::m_worldSize, "Rendered world size", "The maximum amount of terrain that's rendered")
                        ->EnumAttribute(TerrainWorldRendererConfig::WorldSize::_512Meters, "512 Meters")
                        ->EnumAttribute(TerrainWorldRendererConfig::WorldSize::_1024Meters, "1 Kilometer")
                        ->EnumAttribute(TerrainWorldRendererConfig::WorldSize::_2048Meters, "2 Kilometers")
                        ->EnumAttribute(TerrainWorldRendererConfig::WorldSize::_4096Meters, "4 Kilometers")
                        ->EnumAttribute(TerrainWorldRendererConfig::WorldSize::_8192Meters, "8 Kilometers")
                        ->EnumAttribute(TerrainWorldRendererConfig::WorldSize::_16384Meters, "16 Kilometers")
                        ;
            }
        }
    }

    void TerrainWorldRendererComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainRendererService"));
    }

    void TerrainWorldRendererComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainRendererService"));
    }

    void TerrainWorldRendererComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("TerrainService"));
    }

    void TerrainWorldRendererComponent::Reflect(AZ::ReflectContext* context)
    {
        TerrainWorldRendererConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<TerrainWorldRendererComponent, AZ::Component>()->Version(0)->Field(
                "Configuration", &TerrainWorldRendererComponent::m_configuration);
        }
    }

    TerrainWorldRendererComponent::TerrainWorldRendererComponent(const TerrainWorldRendererConfig& configuration)
        : m_configuration(configuration)
    {
        switch (configuration.m_worldSize)
        {
        case TerrainWorldRendererConfig::WorldSize::_512Meters:
            m_terrainFeatureProcessor->SetWorldSize(AZ::Vector2(512.0f, 512.0f));
            break;
        case TerrainWorldRendererConfig::WorldSize::_1024Meters:
            m_terrainFeatureProcessor->SetWorldSize(AZ::Vector2(1024.0f, 1024.0f));
            break;
        case TerrainWorldRendererConfig::WorldSize::_2048Meters:
            m_terrainFeatureProcessor->SetWorldSize(AZ::Vector2(2048.0f, 2048.0f));
            break;
        case TerrainWorldRendererConfig::WorldSize::_4096Meters:
            m_terrainFeatureProcessor->SetWorldSize(AZ::Vector2(4096.0f, 4096.0f));
            break;
        case TerrainWorldRendererConfig::WorldSize::_8192Meters:
            m_terrainFeatureProcessor->SetWorldSize(AZ::Vector2(8192.0f, 8192.0f));
            break;
        case TerrainWorldRendererConfig::WorldSize::_16384Meters:
            m_terrainFeatureProcessor->SetWorldSize(AZ::Vector2(16384.0f, 16384.0f));
            break;

        }
    }

    TerrainWorldRendererComponent::~TerrainWorldRendererComponent()
    {
        if (m_terrainRendererActive)
        {
            Deactivate();
        }
    }

    AZ::RPI::Scene* TerrainWorldRendererComponent::GetScene() const
    {
        // Find the entity context for the entity ID.
        AzFramework::EntityContextId entityContextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityIdContextQueryBus::EventResult(
            entityContextId, GetEntityId(), &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);

        return AZ::RPI::Scene::GetSceneForEntityContextId(entityContextId);
    }

    void TerrainWorldRendererComponent::Activate()
    {
        // On component activation, register the terrain feature processor with Atom and the scene related to this entity context.

        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<Terrain::TerrainFeatureProcessor>();

        if (AZ::RPI::Scene* scene = GetScene(); scene)
        {
            m_terrainFeatureProcessor = scene->EnableFeatureProcessor<Terrain::TerrainFeatureProcessor>();
        }
        m_terrainRendererActive = true;
    }

    void TerrainWorldRendererComponent::Deactivate()
    {
        // On component deactivation, unregister the feature processor and remove it from the default scene.
        m_terrainRendererActive = false;

        if (AZ::RPI::Scene* scene = GetScene(); scene)
        {
            if (scene->GetFeatureProcessor<Terrain::TerrainFeatureProcessor>())
            {
                scene->DisableFeatureProcessor<Terrain::TerrainFeatureProcessor>();
            }
        }
        m_terrainFeatureProcessor = nullptr;

        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<Terrain::TerrainFeatureProcessor>();
    }

    bool TerrainWorldRendererComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TerrainWorldRendererConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TerrainWorldRendererComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TerrainWorldRendererConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }
}
