/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <TerrainTestFixtures.h>

#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <AzFramework/Components/TransformComponent.h>
#include <GradientSignal/Components/GradientSurfaceDataComponent.h>
#include <GradientSignal/Components/GradientTransformComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <SurfaceData/Components/SurfaceDataShapeComponent.h>
#include <SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <GradientSignal/Components/RandomGradientComponent.h>

#include <Components/TerrainHeightGradientListComponent.h>
#include <Components/TerrainLayerSpawnerComponent.h>
#include <Components/TerrainPhysicsColliderComponent.h>
#include <Components/TerrainSurfaceDataSystemComponent.h>
#include <Components/TerrainSurfaceGradientListComponent.h>
#include <Components/TerrainSystemComponent.h>
#include <Components/TerrainWorldComponent.h>
#include <Components/TerrainWorldDebuggerComponent.h>
#include <Components/TerrainWorldRendererComponent.h>
#include <TerrainRenderer/Components/TerrainMacroMaterialComponent.h>
#include <TerrainRenderer/Components/TerrainSurfaceMaterialsListComponent.h>

#include <MockAxisAlignedBoxShapeComponent.h>
#include <Terrain/MockTerrainLayerSpawner.h>

namespace UnitTest
{
    void TerrainTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({ "LmbrCentral", "SurfaceData", "GradientSignal" });

        AddComponentDescriptors({
            AzFramework::TransformComponent::CreateDescriptor(),

            Terrain::TerrainHeightGradientListComponent::CreateDescriptor(),
            Terrain::TerrainLayerSpawnerComponent::CreateDescriptor(),
            Terrain::TerrainPhysicsColliderComponent::CreateDescriptor(),
            Terrain::TerrainSurfaceDataSystemComponent::CreateDescriptor(),
            Terrain::TerrainSurfaceGradientListComponent::CreateDescriptor(),
            Terrain::TerrainSystemComponent::CreateDescriptor(),
            Terrain::TerrainWorldComponent::CreateDescriptor(),
            Terrain::TerrainWorldDebuggerComponent::CreateDescriptor(),
            Terrain::TerrainWorldRendererComponent::CreateDescriptor(),
            Terrain::TerrainMacroMaterialComponent::CreateDescriptor(),
            Terrain::TerrainSurfaceMaterialsListComponent::CreateDescriptor(),

            UnitTest::MockAxisAlignedBoxShapeComponent::CreateDescriptor(),
            UnitTest::MockTerrainLayerSpawnerComponent::CreateDescriptor(),
        });
    }

    void TerrainTestEnvironment::PostCreateApplication()
    {
        // Ebus usage will allocate a global context on first usage. If that first usage occurs in a DLL, then the context will be
        // invalid on subsequent unit test runs if using gtest_repeat. However, if we force the ebus to create their global context in
        // the main test DLL (this one), the context will remain active throughout repeated runs. By creating them in
        // PostCreateApplication(), they will be created before the DLLs get loaded and any system components from those DLLs run, so we
        // can guarantee this will be the first usage.

        // These ebuses need their contexts created here before any of the dependent DLLs get loaded:
        AZ::AssetTypeInfoBus::GetOrCreateContext();
        GradientSignal::GradientRequestBus::GetOrCreateContext();
        SurfaceData::SurfaceDataSystemRequestBus::GetOrCreateContext();
        SurfaceData::SurfaceDataProviderRequestBus::GetOrCreateContext();
        SurfaceData::SurfaceDataModifierRequestBus::GetOrCreateContext();
        LmbrCentral::ShapeComponentRequestsBus::GetOrCreateContext();
    }

    void TerrainBaseFixture::SetupCoreSystems()
    {
    }

    void TerrainBaseFixture::TearDownCoreSystems()
    {
    }

    AZStd::unique_ptr<AZ::Entity> TerrainBaseFixture::CreateTestBoxEntity(float boxHalfBounds)
    {
        // Create the base entity
        AZStd::unique_ptr<AZ::Entity> testEntity = CreateEntity();

        LmbrCentral::BoxShapeConfig boxConfig(AZ::Vector3(boxHalfBounds * 2.0f));
        auto boxComponent = testEntity->CreateComponent(LmbrCentral::AxisAlignedBoxShapeComponentTypeId);
        boxComponent->SetConfiguration(boxConfig);

        // Create a transform that locates our gradient in the center of our desired Shape.
        auto transform = testEntity->CreateComponent<AzFramework::TransformComponent>();
        transform->SetWorldTM(AZ::Transform::CreateTranslation(AZ::Vector3(boxHalfBounds)));

        return testEntity;
    }

    AZStd::unique_ptr<AZ::Entity> TerrainBaseFixture::CreateTestBoxEntity(const AZ::Aabb& box)
    {
        // Create the base entity
        AZStd::unique_ptr<AZ::Entity> testEntity = CreateEntity();

        LmbrCentral::BoxShapeConfig boxConfig(box.GetExtents());
        auto boxComponent = testEntity->CreateComponent(LmbrCentral::AxisAlignedBoxShapeComponentTypeId);
        boxComponent->SetConfiguration(boxConfig);

        // Create a transform that locates our gradient in the center of our desired Shape.
        auto transform = testEntity->CreateComponent<AzFramework::TransformComponent>();
        transform->SetWorldTM(AZ::Transform::CreateTranslation(box.GetCenter()));

        return testEntity;
    }

    AZStd::unique_ptr<AZ::Entity> TerrainBaseFixture::CreateTestSphereEntity(float shapeRadius)
    {
        // Create the base entity
        AZStd::unique_ptr<AZ::Entity> testEntity = CreateEntity();

        LmbrCentral::SphereShapeConfig sphereConfig(shapeRadius);
        auto sphereComponent = testEntity->CreateComponent(LmbrCentral::SphereShapeComponentTypeId);
        sphereComponent->SetConfiguration(sphereConfig);

        // Create a transform that locates our gradient in the center of our desired Shape.
        auto transform = testEntity->CreateComponent<AzFramework::TransformComponent>();
        transform->SetWorldTM(AZ::Transform::CreateTranslation(AZ::Vector3(shapeRadius)));

        return testEntity;
    }

    AZStd::unique_ptr<AZ::Entity> TerrainBaseFixture::CreateAndActivateTestRandomGradient(const AZ::Aabb& spawnerBox, uint32_t randomSeed)
    {
        // Create a Random Gradient Component with arbitrary parameters.
        auto entity = CreateTestBoxEntity(spawnerBox);
        GradientSignal::RandomGradientConfig config;
        config.m_randomSeed = randomSeed;
        entity->CreateComponent<GradientSignal::RandomGradientComponent>(config);

        // Create a Gradient Transform Component with arbitrary parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        entity->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);

        ActivateEntity(entity.get());
        return entity;
    }

    // Create a terrain system with reasonable defaults for testing, but with the ability to override the defaults
    // on a test-by-test basis.
    AZStd::unique_ptr<Terrain::TerrainSystem> TerrainBaseFixture::CreateAndActivateTerrainSystem(
        float queryResolution, AZ::Aabb worldBounds)
    {
        // Create the terrain system and give it one tick to fully initialize itself.
        auto terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
        terrainSystem->SetTerrainAabb(worldBounds);
        terrainSystem->SetTerrainHeightQueryResolution(queryResolution);
        terrainSystem->Activate();
        AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.f, AZ::ScriptTimePoint{});
        return terrainSystem;
    }

}

