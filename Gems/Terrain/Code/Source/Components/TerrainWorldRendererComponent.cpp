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
            serialize->Class<DetailMaterialConfiguration>()
                ->Version(1)
                ->Field("UseHeightBasedBlending", &DetailMaterialConfiguration::m_useHeightBasedBlending)
                ->Field("RenderDistance", &DetailMaterialConfiguration::m_renderDistance)
                ->Field("FadeDistance", &DetailMaterialConfiguration::m_fadeDistance)
                ->Field("Scale", &DetailMaterialConfiguration::m_scale)
                ;

            serialize->Class<MeshConfiguration>()
                ->Version(2)
                ->Field("RenderDistance", &MeshConfiguration::m_renderDistance)
                ->Field("FirstLodDistance", &MeshConfiguration::m_firstLodDistance)
                ->Field("ClodEnabled", &MeshConfiguration::m_clodEnabled)
                ->Field("ClodDistance", &MeshConfiguration::m_clodDistance)
                ;

            serialize->Class<TerrainWorldRendererConfig, AZ::ComponentConfig>()
                ->Version(2)
                ->Field("DetailMaterialConfiguration", &TerrainWorldRendererConfig::m_detailMaterialConfig)
                ->Field("MeshConfiguration", &TerrainWorldRendererConfig::m_meshConfig)
                ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<DetailMaterialConfiguration>("Detail material", "Settings related to rendering detail surface materials.")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &DetailMaterialConfiguration::m_useHeightBasedBlending, "Height based texture blending", "When turned on, detail materials will use the height texture to aid with blending.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &DetailMaterialConfiguration::m_renderDistance, "Detail material render distance", "The distance from the camera that the detail material will render.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 2048.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &DetailMaterialConfiguration::m_fadeDistance, "Detail material fade distance", "The distance over which the detail material will fade out into the macro material.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 2048.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &DetailMaterialConfiguration::m_scale, "Detail material scale", "The scale at which all detail materials are rendered at.")
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0001f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                    ;

                editContext->Class<MeshConfiguration>("Mesh", "Settings related to rendering terrain meshes")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshConfiguration::m_renderDistance, "Mesh render distance", "The distance from the camera that terrain meshes will render.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100000.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10000.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshConfiguration::m_firstLodDistance, "First LOD distance", "The distance from the camera that the first Lod renders to. Subsequent LODs will be at double the distance from the previous LOD.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10000.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 1000.0f)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MeshConfiguration::m_clodEnabled, "Continuous LOD (CLOD)", "Enables the use of continuous level of detail, which smoothly blends geometry between terrain lods.")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshConfiguration::m_clodDistance, "CLOD Distance", "Distance in meters over which the first lod will blend into the next lod. Subsequent lod blend distances will double with each lod for a consistent visual appearance.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &MeshConfiguration::IsClodDisabled)
                    ;

                editContext->Class<TerrainWorldRendererConfig>("Terrain World Renderer Component", "Enables terrain rendering")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldRendererConfig::m_meshConfig, "Mesh configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldRendererConfig::m_detailMaterialConfig, "Detail material configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
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
            
            m_terrainFeatureProcessor->SetDetailMaterialConfiguration(m_configuration.m_detailMaterialConfig);
            m_terrainFeatureProcessor->SetMeshConfiguration(m_configuration.m_meshConfig);
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
