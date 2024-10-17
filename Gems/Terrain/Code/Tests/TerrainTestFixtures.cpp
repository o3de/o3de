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
#include <Atom/RPI.Public/RPISystem.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Scene/SceneSystemComponent.h>
#include <GradientSignal/Components/GradientSurfaceDataComponent.h>
#include <GradientSignal/Components/GradientTransformComponent.h>
#include <GradientSignal/Components/RandomGradientComponent.h>
#include <GradientSignal/Components/SurfaceAltitudeGradientComponent.h>
#include <GradientSignal/Components/SurfaceMaskGradientComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <SurfaceData/Components/SurfaceDataShapeComponent.h>
#include <SurfaceData/Components/SurfaceDataSystemComponent.h>

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

extern "C" void CleanUpRpiPublicGenericClassInfo();

namespace UnitTest
{
    void TerrainTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({ "LmbrCentral", "SurfaceData", "GradientSignal" });

        AddComponentDescriptors({
            AzFramework::SceneSystemComponent::CreateDescriptor(),
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

        // Call the AZ::RPI::RPISystem reflection for use with the terrain rendering component unit tests.
        auto serializeContext = AZ::ReflectionEnvironment::GetReflectionManager()
            ? AZ::ReflectionEnvironment::GetReflectionManager()->GetReflectContext<AZ::SerializeContext>()
            : nullptr;
        AZ::RPI::RPISystem::Reflect(serializeContext);
    }

    void TerrainBaseFixture::SetupCoreSystems()
    {
    }

    void TerrainBaseFixture::TearDownCoreSystems()
    {
    }

    AZStd::unique_ptr<AZ::Entity> TerrainBaseFixture::CreateTestBoxEntity(float boxHalfBounds) const
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

    AZStd::unique_ptr<AZ::Entity> TerrainBaseFixture::CreateTestBoxEntity(const AZ::Aabb& box) const
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

    AZStd::unique_ptr<AZ::Entity> TerrainBaseFixture::CreateTestSphereEntity(float shapeRadius) const
    {
        return CreateTestSphereEntity(shapeRadius, AZ::Vector3(shapeRadius));
    }

    AZStd::unique_ptr<AZ::Entity> TerrainBaseFixture::CreateTestSphereEntity(float shapeRadius, const AZ::Vector3& center) const
    {
        // Create the base entity
        AZStd::unique_ptr<AZ::Entity> testEntity = CreateEntity();

        LmbrCentral::SphereShapeConfig sphereConfig(shapeRadius);
        auto sphereComponent = testEntity->CreateComponent(LmbrCentral::SphereShapeComponentTypeId);
        sphereComponent->SetConfiguration(sphereConfig);

        auto transform = testEntity->CreateComponent<AzFramework::TransformComponent>();
        transform->SetWorldTM(AZ::Transform::CreateTranslation(center));

        return testEntity;
    }

    AZStd::unique_ptr<AZ::Entity> TerrainBaseFixture::CreateAndActivateTestRandomGradient(
        const AZ::Aabb& spawnerBox, uint32_t randomSeed) const
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

    AZStd::unique_ptr<AZ::Entity> TerrainBaseFixture::CreateTestLayerSpawnerEntity(
        const AZ::Aabb& spawnerBox,
        const AZ::EntityId& heightGradientEntityId,
        const Terrain::TerrainSurfaceGradientListConfig& surfaceConfig) const
    {
        // Create the base entity
        AZStd::unique_ptr<AZ::Entity> testLayerSpawnerEntity = CreateTestBoxEntity(spawnerBox);

        // Add a Terrain Layer Spawner
        testLayerSpawnerEntity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>();

        // Add a Terrain Height Gradient List with one entry pointing to the given gradient entity
        Terrain::TerrainHeightGradientListConfig heightConfig;
        heightConfig.m_gradientEntities.emplace_back(heightGradientEntityId);
        testLayerSpawnerEntity->CreateComponent<Terrain::TerrainHeightGradientListComponent>(heightConfig);

        // Add a Terrain Surface Gradient List with however many entries we were given
        testLayerSpawnerEntity->CreateComponent<Terrain::TerrainSurfaceGradientListComponent>(surfaceConfig);

        return testLayerSpawnerEntity;
    }

    // Create a terrain system with reasonable defaults for testing, but with the ability to override the defaults
    // on a test-by-test basis.
    AZStd::unique_ptr<Terrain::TerrainSystem> TerrainBaseFixture::CreateAndActivateTerrainSystem(
        float queryResolution, AzFramework::Terrain::FloatRange heightBounds) const
    {
        const float defaultSurfaceQueryResolution = 1.0f;
        return CreateAndActivateTerrainSystem(queryResolution, defaultSurfaceQueryResolution, heightBounds);
    }

    // Create a terrain system with reasonable defaults for testing, but with the ability to override the defaults
    // on a test-by-test basis.
    AZStd::unique_ptr<Terrain::TerrainSystem> TerrainBaseFixture::CreateAndActivateTerrainSystem(
        float heightQueryResolution, float surfaceQueryResolution, const AzFramework::Terrain::FloatRange& heightBounds) const
    {
        // Create the terrain system and give it one tick to fully initialize itself.
        auto terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
        terrainSystem->SetTerrainHeightBounds(heightBounds);
        terrainSystem->SetTerrainHeightQueryResolution(heightQueryResolution);
        terrainSystem->SetTerrainSurfaceDataQueryResolution(surfaceQueryResolution);
        terrainSystem->Activate();
        AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.f, AZ::ScriptTimePoint{});
        return terrainSystem;
    }

    void TerrainBaseFixture::CreateTestTerrainSystem(const AZ::Aabb& worldBounds, float queryResolution, uint32_t numSurfaces)
    {
        // Create a Random Gradient to use as our height provider
        {
            const uint32_t heightRandomSeed = 12345;
            auto heightGradientEntity = CreateAndActivateTestRandomGradient(worldBounds, heightRandomSeed);
            m_heightGradientEntities.push_back(AZStd::move(heightGradientEntity));
        }

        // Create a set of Random Gradients to use as our surface providers
        Terrain::TerrainSurfaceGradientListConfig surfaceConfig;
        for (uint32_t surfaces = 0; surfaces < numSurfaces; surfaces++)
        {
            const uint32_t surfaceRandomSeed = 23456 + surfaces;
            auto surfaceGradientEntity = CreateAndActivateTestRandomGradient(worldBounds, surfaceRandomSeed);

            // Give each gradient a new surface tag
            surfaceConfig.m_gradientSurfaceMappings.emplace_back(
                surfaceGradientEntity->GetId(), SurfaceData::SurfaceTag(AZStd::string::format("test%u", surfaces)));

            m_surfaceGradientEntities.emplace_back(AZStd::move(surfaceGradientEntity));
        }

        // Create a single Terrain Layer Spawner that covers the entire terrain world bounds
        // (Do this *after* creating and activating the height and surface gradients)
        m_terrainLayerSpawnerEntity = CreateTestLayerSpawnerEntity(worldBounds, m_heightGradientEntities[0]->GetId(), surfaceConfig);
        ActivateEntity(m_terrainLayerSpawnerEntity.get());

        // Create the terrain system (do this after creating the terrain layer entity to ensure that we don't need any data refreshes)
        // Also ensure to do this after creating the global JobManager.
        AzFramework::Terrain::FloatRange heightBounds = { worldBounds.GetMin().GetZ(), worldBounds.GetMax().GetZ() };
        m_terrainSystem = CreateAndActivateTerrainSystem(queryResolution, heightBounds);
    }

    void TerrainBaseFixture::DestroyTestTerrainSystem()
    {
        m_terrainSystem.reset();
        m_terrainLayerSpawnerEntity.reset();
        m_heightGradientEntities.clear();
        m_surfaceGradientEntities.clear();
        m_heightGradientEntities.shrink_to_fit();
        m_surfaceGradientEntities.shrink_to_fit();
    }

    void TerrainBaseFixture::CreateTestTerrainSystemWithSurfaceGradients(const AZ::Aabb& worldBounds, float queryResolution)
    {
        // This will create a testing / benchmarking setup that uses surface-based gradients for terrain data so that we can
        // exercise the full pathway of "terrain -> gradient -> surface data" with both surface providers and surface modifiers.
        // From a benchmarking perspective, this will also let us verify that we can run multiple simultaneous queries that span
        // all three of those systems without hitting any locks.
        // 
        // The specific setup that we create here looks like the following:
        // - Height: This comes from an Altitude Gradient looking for an "altitude" tag, and a giant sphere that emits "altitude".
        // The Altitude Gradient is constrained to a box that only contains the top part of the sphere.
        // 
        // - Surfaces: This comes from a Surface Mask Gradient looking for a "surface" tag, and a combination of a Random Noise Gradient
        // for weight values, and a Gradient Surface Tag Emitter broadcasting "surface" with those weights for any surface points contained
        // in its bounds. It is bound to the same box as the Altitude Gradient, so only the top part of the sphere will get the "surface"
        // tag.
        //
        // The net result is a terrain that has a dome shape (from the sphere-based Altitude Gradient) and a surface with some randomly
        // distributed surface weights that come from the sphere + Random Noise + Gradient Surface Tag Emitter. With this setup, all
        // terrain queries will need to pass through Terrain -> Gradients -> Surface Data -> (Terrain, Shape, Gradient Surface Tag Emitter).
        // 
        // Note that there's a potential recursion loop with Surface Data getting surface points back from terrain.  We avoid this by
        // locating the sphere, the Altitude Gradient bounds, and the Gradient Surface Tag Emitter bounds, to all be below the terrain
        // surface, so that none of the queried terrain points will actually get reused or requeried. If our bounds overlapped the terrain
        // surface, then the Gradient Surface Tag Emitter would add its tags to the terrain points too, which would cause the recursion
        // loop.


        // This is the offset we'll use for locating our entities below the terrain world bounds.
        const float belowTerrainZ = worldBounds.GetMin().GetZ() - 100.0f;

        // Create our Sphere height surface. This is located in the center of the world bounds, but down below the terrain surface.
        {
            // We're intentionally making the *radius* (not the diameter) the size of the world bounds. This gives us a sphere large enough
            // to make a really nice dome for our heights.
            const float sphereRadius = worldBounds.GetXExtent();

            // The sphere is centered in the world bounds, but far enough below the terrain that we can modify its surface points without
            // also affecting the terrain surface points. We want the top of the sphere to be at our belowTerrainZ height.
            AZ::Vector3 sphereCenter = worldBounds.GetCenter();
            sphereCenter.SetZ(belowTerrainZ - sphereRadius);
            auto heightSurfaceEntity = CreateTestSphereEntity(sphereRadius, sphereCenter);

            SurfaceData::SurfaceDataShapeConfig heightSurfaceConfig;
            heightSurfaceConfig.m_providerTags.push_back(SurfaceData::SurfaceTag("altitude"));
            heightSurfaceEntity->CreateComponent<SurfaceData::SurfaceDataShapeComponent>(heightSurfaceConfig);

            ActivateEntity(heightSurfaceEntity.get());
            m_heightGradientEntities.push_back(AZStd::move(heightSurfaceEntity));
        }

        // Create our Altitude Gradient entity. This is located in the center of the world bounds, and contains the top 150 meters of
        // the sphere height surface created above.
        {
            // We'll use the top 150 meters of the sphere for our altitude gradient so that we get a nice dome.
            const float altitudeBoxHeight = 150.0f;
            AZ::Aabb altitudeBox = AZ::Aabb::CreateFromMinMaxValues(
                worldBounds.GetMin().GetX(), worldBounds.GetMin().GetY(), belowTerrainZ - altitudeBoxHeight,
                worldBounds.GetMax().GetX(), worldBounds.GetMax().GetY(), belowTerrainZ
            );
            auto heightGradientEntity = CreateTestBoxEntity(altitudeBox);

            GradientSignal::SurfaceAltitudeGradientConfig heightGradientConfig;
            heightGradientConfig.m_shapeEntityId = heightGradientEntity->GetId();
            heightGradientConfig.m_surfaceTagsToSample.push_back(SurfaceData::SurfaceTag("altitude"));
            heightGradientEntity->CreateComponent<GradientSignal::SurfaceAltitudeGradientComponent>(heightGradientConfig);

            ActivateEntity(heightGradientEntity.get());
            m_heightGradientEntities.push_back(AZStd::move(heightGradientEntity));
        }

        // Create a Surface Modifier entity so that we're testing both surface providers and surface modifiers.
        // This is a Gradient Surface Tag Emitter + Random Noise that will add the "surface" tag with random weights to the sphere
        // surface points.
        {
            // Create a box of arbitrary size centered in the terrain XY, but below the terrain.
            float halfBox = 0.5f;
            AZ::Aabb gradientBox = AZ::Aabb::CreateFromMinMaxValues(
                worldBounds.GetCenter().GetX() - halfBox, worldBounds.GetCenter().GetY() - halfBox, belowTerrainZ - halfBox,
                worldBounds.GetCenter().GetX() + halfBox, worldBounds.GetCenter().GetY() + halfBox, belowTerrainZ + halfBox);
            auto surfaceModifierEntity = CreateTestBoxEntity(gradientBox);

            // Create a Random Gradient Component with arbitrary parameters.
            GradientSignal::RandomGradientConfig config;
            config.m_randomSeed = 12345;
            surfaceModifierEntity->CreateComponent<GradientSignal::RandomGradientComponent>(config);

            // Create a Gradient Transform Component with arbitrary parameters.
            GradientSignal::GradientTransformConfig gradientTransformConfig;
            gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
            surfaceModifierEntity->CreateComponent<GradientSignal::GradientTransformComponent>(gradientTransformConfig);

            // Create a Gradient Surface Tag Emitter. Modify surface points to have "surface" with a random weight, but only when the
            // Random Gradient has values between 0.5 - 1.0, so that we aren't getting the modification on every point.
            GradientSignal::GradientSurfaceDataConfig gradientSurfaceConfig;
            gradientSurfaceConfig.m_shapeConstraintEntityId = m_heightGradientEntities[1]->GetId();
            gradientSurfaceConfig.m_thresholdMin = 0.5f;
            gradientSurfaceConfig.m_thresholdMax = 1.0f;
            gradientSurfaceConfig.m_modifierTags.push_back(SurfaceData::SurfaceTag("surface"));
            surfaceModifierEntity->CreateComponent<GradientSignal::GradientSurfaceDataComponent>(gradientSurfaceConfig);

            ActivateEntity(surfaceModifierEntity.get());
            m_surfaceGradientEntities.push_back(AZStd::move(surfaceModifierEntity));
        }

        // Create a Surface Gradient entity that turns surfaces with "surface" into a gradient.
        {
            // Create a box of arbitrary size centered in the terrain XY, but below the terrain.
            float halfBox = 0.5f;
            AZ::Aabb gradientBox = AZ::Aabb::CreateFromMinMaxValues(
                worldBounds.GetCenter().GetX() - halfBox, worldBounds.GetCenter().GetY() - halfBox, belowTerrainZ - halfBox,
                worldBounds.GetCenter().GetX() + halfBox, worldBounds.GetCenter().GetY() + halfBox, belowTerrainZ + halfBox);
            auto surfaceGradientEntity = CreateTestBoxEntity(gradientBox);

            GradientSignal::SurfaceMaskGradientConfig gradientSurfaceConfig;
            gradientSurfaceConfig.m_surfaceTagList.push_back(SurfaceData::SurfaceTag("surface"));
            surfaceGradientEntity->CreateComponent<GradientSignal::SurfaceMaskGradientComponent>(gradientSurfaceConfig);

            ActivateEntity(surfaceGradientEntity.get());
            m_surfaceGradientEntities.push_back(AZStd::move(surfaceGradientEntity));
        }

        Terrain::TerrainSurfaceGradientListConfig surfaceConfig;
        surfaceConfig.m_gradientSurfaceMappings.emplace_back(
                m_surfaceGradientEntities[1]->GetId(), SurfaceData::SurfaceTag("terrain_surface"));

        // Create a single Terrain Layer Spawner that covers the entire terrain world bounds
        // (Do this *after* creating and activating the height and surface gradients)
        m_terrainLayerSpawnerEntity = CreateTestLayerSpawnerEntity(worldBounds, m_heightGradientEntities[1]->GetId(), surfaceConfig);
        ActivateEntity(m_terrainLayerSpawnerEntity.get());

        // Create the terrain system (do this after creating the terrain layer entity to ensure that we don't need any data refreshes)
        // Also ensure to do this after creating the global JobManager.
        AzFramework::Terrain::FloatRange heightBounds = { worldBounds.GetMin().GetZ(), worldBounds.GetMax().GetZ() };
        m_terrainSystem = CreateAndActivateTerrainSystem(queryResolution, heightBounds);
    }

    TerrainSystemTestFixture::TerrainSystemTestFixture()
        : m_restoreFileIO(m_fileIOMock)
    {
        // Install Mock File IO, since the ShaderMetricsSystem inside of Atom's RPISystem will try to read/write a file.
        AZ::IO::MockFileIOBase::InstallDefaultReturns(m_fileIOMock);
    }

    void TerrainSystemTestFixture::SetUp()
    {
        UnitTest::TerrainTestFixture::SetUp();

        // Create a system entity with a SceneSystemComponent for Atom and a TerrainSystemComponent for the TerrainWorldComponent.
        // However, we don't initialize and activate it until *after* the RPI system is initialized, since the TerrainSystemComponent
        // relies on the RPI.
        m_systemEntity = CreateEntity();
        m_systemEntity->CreateComponent<AzFramework::SceneSystemComponent>();
        m_systemEntity->CreateComponent<Terrain::TerrainSystemComponent>();

        // Create a stub RHI for use by Atom
        m_rhiFactory.reset(aznew UnitTest::StubRHI::Factory());

        // Create the Atom RPISystem
        AZ::RPI::RPISystemDescriptor rpiSystemDescriptor;
        m_rpiSystem = AZStd::make_unique<AZ::RPI::RPISystem>();
        m_rpiSystem->Initialize(rpiSystemDescriptor);

        AZ::RPI::ImageSystemDescriptor imageSystemDescriptor;
        m_imageSystem = AZStd::make_unique<AZ::RPI::ImageSystem>();
        m_imageSystem->Init(imageSystemDescriptor);

        // Now that the RPISystem is activated, activate the system entity.
        m_systemEntity->Init();
        m_systemEntity->Activate();
    }

    void TerrainSystemTestFixture::TearDown()
    {
        m_imageSystem->Shutdown();
        m_rpiSystem->Shutdown();
        m_rpiSystem = nullptr;
        m_rhiFactory = nullptr;

        m_systemEntity.reset();

        CleanUpRpiPublicGenericClassInfo();

        UnitTest::TerrainTestFixture::TearDown();
    }
}

