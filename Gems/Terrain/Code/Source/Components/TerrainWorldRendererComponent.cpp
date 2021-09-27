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

#include <SurfaceData/SurfaceDataSystemRequestBus.h>

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
            serialize->Class<TerrainWorldRendererConfig, AZ::ComponentConfig>()->Version(1);

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<TerrainWorldRendererConfig>("Terrain World Renderer Component", "Enables terrain rendering")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
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
        }

        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusConnect();
        m_terrainRendererActive = true;
    }

    void TerrainWorldRendererComponent::Deactivate()
    {
        // On component deactivation, unregister the feature processor and remove it from the default scene.

        m_terrainRendererActive = false;
        AzFramework::Terrain::TerrainDataNotificationBus::Handler::BusDisconnect();

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

    void TerrainWorldRendererComponent::OnTerrainDataDestroyBegin()
    {
        // If the terrain is being destroyed, remove all existing terrain data from the feature processor.

        if (m_terrainFeatureProcessor)
        {
            m_terrainFeatureProcessor->RemoveTerrainData();
        }
    }

    void TerrainWorldRendererComponent::OnTerrainDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion, [[maybe_unused]] TerrainDataChangedMask dataChangedMask)
    {
        // Block other threads from accessing the surface data bus while we are in GetValue (which may call into the SurfaceData bus).
        // We lock our surface data mutex *before* checking / setting "isRequestInProgress" so that we prevent race conditions
        // that create false detection of cyclic dependencies when multiple requests occur on different threads simultaneously.
        // (One case where this was previously able to occur was in rapid updating of the Preview widget on the
        // GradientSurfaceDataComponent in the Editor when moving the threshold sliders back and forth rapidly)
        auto& surfaceDataContext = SurfaceData::SurfaceDataSystemRequestBus::GetOrCreateContext(false);
        typename SurfaceData::SurfaceDataSystemRequestBus::Context::DispatchLockGuard scopeLock(surfaceDataContext.m_contextMutex);

        AZ::Vector2 queryResolution = AZ::Vector2(1.0f);
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            queryResolution, &AzFramework::Terrain::TerrainDataRequests::GetTerrainHeightQueryResolution);

        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
            worldBounds, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);


        AZ::Transform transform = AZ::Transform::CreateTranslation(worldBounds.GetCenter());

        uint32_t width = aznumeric_cast<uint32_t>(
            (float)worldBounds.GetXExtent() / queryResolution.GetX());
        uint32_t height = aznumeric_cast<uint32_t>(
            (float)worldBounds.GetYExtent() / queryResolution.GetY());
        AZStd::vector<float> pixels;
        pixels.resize_no_construct(width * height);
        const uint32_t pixelDataSize = width * height * sizeof(float);
        memset(pixels.data(), 0, pixelDataSize);

        for (uint32_t y = 0; y < height; y++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                bool terrainExists = true;
                float terrainHeight = 0.0f;
                AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                    terrainHeight, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats,
                    (x * queryResolution.GetX()) + worldBounds.GetMin().GetX(),
                    (y * queryResolution.GetY()) + worldBounds.GetMin().GetY(),
                    AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT,
                    &terrainExists);

                pixels[(y * width) + x] =
                    (terrainHeight - worldBounds.GetMin().GetZ()) / worldBounds.GetExtents().GetZ();
            }
        }

        if (m_terrainFeatureProcessor)
        {
            m_terrainFeatureProcessor->UpdateTerrainData(transform, worldBounds, queryResolution.GetX(), width, height, pixels);
        }
    }

}
