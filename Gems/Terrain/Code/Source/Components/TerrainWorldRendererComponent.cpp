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
            serialize->Class<MeshConfiguration>()
                ->Version(2)
                ->Field("RenderDistance", &MeshConfiguration::m_renderDistance)
                ->Field("FirstLodDistance", &MeshConfiguration::m_firstLodDistance)
                ->Field("ClodEnabled", &MeshConfiguration::m_clodEnabled)
                ->Field("ClodDistance", &MeshConfiguration::m_clodDistance)
                ;

            serialize->Class<DetailMaterialConfiguration>()
                ->Version(1)
                ->Field("UseHeightBasedBlending", &DetailMaterialConfiguration::m_useHeightBasedBlending)
                ->Field("RenderDistance", &DetailMaterialConfiguration::m_renderDistance)
                ->Field("FadeDistance", &DetailMaterialConfiguration::m_fadeDistance)
                ->Field("Scale", &DetailMaterialConfiguration::m_scale)
                ;

            serialize->Class<ClipmapConfiguration>()
                ->Version(2)
                ->Field("ClipmapEnabled", &ClipmapConfiguration::m_clipmapEnabled)
                ->Field("ClipmapSize", &ClipmapConfiguration::m_clipmapSize)
                ->Field("MacroClipmapMaxResolution", &ClipmapConfiguration::m_macroClipmapMaxResolution)
                ->Field("DetailClipmapMaxResolution", &ClipmapConfiguration::m_detailClipmapMaxResolution)
                ->Field("MacroClipmapScaleBase", &ClipmapConfiguration::m_macroClipmapScaleBase)
                ->Field("DetailClipmapScaleBase", &ClipmapConfiguration::m_detailClipmapScaleBase)
                ->Field("MacroClipmapMarginSize", &ClipmapConfiguration::m_macroClipmapMarginSize)
                ->Field("DetailClipmapMarginSize", &ClipmapConfiguration::m_detailClipmapMarginSize)
                ;

            serialize->Class<TerrainWorldRendererConfig, AZ::ComponentConfig>()
                ->Version(3)
                ->Field("MeshConfiguration", &TerrainWorldRendererConfig::m_meshConfig)
                ->Field("DetailMaterialConfiguration", &TerrainWorldRendererConfig::m_detailMaterialConfig)
                ->Field("ClipmapConfiguration", &TerrainWorldRendererConfig::m_clipmapConfig)
                ;

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
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

                editContext->Class<ClipmapConfiguration>("Clipmap", "Settings related to clipmap rendering")
                    ->GroupElementToggle("Clipmap Enabled", &ClipmapConfiguration::m_clipmapEnabled)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ClipmapConfiguration::m_clipmapSize,
                        "Clipmap image size",
                        "The size of the clipmap image in each layer.")
                        ->EnumAttribute(ClipmapConfiguration::ClipmapSize2048, "2048")
                        ->EnumAttribute(ClipmapConfiguration::ClipmapSize1024, "1024")
                        ->EnumAttribute(ClipmapConfiguration::ClipmapSize512, "512")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClipmapConfiguration::m_macroClipmapMaxResolution,
                        "Macro clipmap max resolution: texels/m",
                        "The resolution of the highest resolution clipmap in the stack.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 2.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClipmapConfiguration::m_detailClipmapMaxResolution,
                        "Detail clipmap max resolution: texels/m",
                        "The resolution of the highest resolution clipmap in the stack.")
                        ->Attribute(AZ::Edit::Attributes::Min, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 512.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 2048.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 4096.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClipmapConfiguration::m_macroClipmapScaleBase,
                        "Macro clipmap scale base",
                        "The scale base between two adjacent clipmap layers. \n"
                        "For example, 3 means the (n+1)th clipmap covers 3^2 = 9 times the area covered by the nth clipmap.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1.1f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 2.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 4.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClipmapConfiguration::m_detailClipmapScaleBase,
                        "Detail clipmap scale base",
                        "The scale base between two adjacent clipmap layers. \n"
                        "For example, 3 means the (n+1)th clipmap covers 3^2 = 9 times the area covered by the nth clipmap.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1.1f)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 2.0f)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 4.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClipmapConfiguration::m_macroClipmapMarginSize,
                        "Macro clipmap margin size: texels",
                        "The margin of the clipmap beyond the visible data. Increasing the margins results in less frequent clipmap updates "
                        "but also results in lower resolution clipmaps rendering closer to the camera.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1u)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 1u)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 8u)
                        ->Attribute(AZ::Edit::Attributes::Max, 16u)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &ClipmapConfiguration::m_detailClipmapMarginSize,
                        "Detail clipmap margin size: texels",
                        "The margin of the clipmap beyond the visible data. Increasing the margins results in less frequent clipmap updates "
                        "but also results in lower resolution clipmaps rendering closer to the camera.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1u)
                        ->Attribute(AZ::Edit::Attributes::SoftMin, 1u)
                        ->Attribute(AZ::Edit::Attributes::SoftMax, 8u)
                        ->Attribute(AZ::Edit::Attributes::Max, 16u)
                    // Note: m_extendedClipmapMarginSize, m_clipmapBlendSize won't be exposed because algorithm may change and we may not need them.
                    ;

                editContext->Class<TerrainWorldRendererConfig>("Terrain World Renderer Component", "Enables terrain rendering")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldRendererConfig::m_meshConfig, "Mesh configuration", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldRendererConfig::m_detailMaterialConfig, "Detail material configuration", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TerrainWorldRendererConfig::m_clipmapConfig, "Clipmap configuration", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
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

        if (AZ::RPI::Scene* scene = GetScene(); scene)
        {
            m_terrainFeatureProcessor = scene->EnableFeatureProcessor<Terrain::TerrainFeatureProcessor>();

            // Connect duplicate settings
            m_configuration.m_clipmapConfig.m_macroClipmapMaxRenderRadius = m_configuration.m_meshConfig.m_renderDistance;
            m_configuration.m_clipmapConfig.m_detailClipmapMaxRenderRadius = m_configuration.m_detailMaterialConfig.m_renderDistance;

            m_terrainFeatureProcessor->SetDetailMaterialConfiguration(m_configuration.m_detailMaterialConfig);
            m_terrainFeatureProcessor->SetMeshConfiguration(m_configuration.m_meshConfig);
            m_terrainFeatureProcessor->SetClipmapConfiguration(m_configuration.m_clipmapConfig);
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
