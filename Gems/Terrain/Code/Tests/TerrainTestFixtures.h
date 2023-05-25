/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/GemTestEnvironment.h>
#include <TerrainSystem/TerrainSystem.h>
#include <Components/TerrainSurfaceGradientListComponent.h>

#include <Atom/RPI.Public/RPISystem.h>
#include <Atom/RPI.Public/Image/ImageSystem.h>
#include <Common/RHI/Factory.h>
#include <Common/RHI/Stubs.h>

#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <Tests/FileIOBaseTestTypes.h>

namespace UnitTest
{
    // The Terrain unit tests need to use the GemTestEnvironment to load LmbrCentral, SurfaceData, and GradientSignal Gems so that these
    // systems can be used in the unit tests.
    class TerrainTestEnvironment
        : public AZ::Test::GemTestEnvironment
    {
    public:
        void AddGemsAndComponents() override;
        void PostCreateApplication() override;
    };

#ifdef HAVE_BENCHMARK
    //! The Benchmark environment is used for one time setup and tear down of shared resources
    class TerrainBenchmarkEnvironment
        : public AZ::Test::BenchmarkEnvironmentBase
        , public TerrainTestEnvironment

    {
    protected:
        void SetUpBenchmark() override
        {
            SetupEnvironment();
        }

        void TearDownBenchmark() override
        {
            TeardownEnvironment();
        }
    };
#endif

    // Base test fixture used for Terrain unit tests and benchmark tests
    class TerrainBaseFixture
    {
    public:
        void SetupCoreSystems();
        void TearDownCoreSystems();

        AZStd::unique_ptr<AZ::Entity> CreateEntity() const
        {
            return AZStd::make_unique<AZ::Entity>();
        }

        void ActivateEntity(AZ::Entity* entity) const
        {
            entity->Init();
            entity->Activate();
        }

        // Create an entity with a box shape and a transform.
        AZStd::unique_ptr<AZ::Entity> CreateTestBoxEntity(float boxHalfBounds) const;

        // Create an entity with a box shape and a transform.
        AZStd::unique_ptr<AZ::Entity> CreateTestBoxEntity(const AZ::Aabb& box) const;

        // Create an entity with a sphere shape and a transform.
        AZStd::unique_ptr<AZ::Entity> CreateTestSphereEntity(float shapeRadius) const;
        AZStd::unique_ptr<AZ::Entity> CreateTestSphereEntity(float shapeRadius, const AZ::Vector3& center) const;

        // Create and activate an entity with a gradient component of the requested type, initialized with test data.
        AZStd::unique_ptr<AZ::Entity> CreateAndActivateTestRandomGradient(const AZ::Aabb& spawnerBox, uint32_t randomSeed) const;

        AZStd::unique_ptr<AZ::Entity> CreateTestLayerSpawnerEntity(
            const AZ::Aabb& spawnerBox,
            const AZ::EntityId& heightGradientEntityId,
            const Terrain::TerrainSurfaceGradientListConfig& surfaceConfig) const;

        // Create a terrain system with reasonable defaults for testing, but with the ability to override the defaults
        // on a test-by-test basis.
        AZStd::unique_ptr<Terrain::TerrainSystem> CreateAndActivateTerrainSystem(
            float queryResolution = 1.0f,
            AzFramework::Terrain::FloatRange heightBounds = {-128.0f, 128.0f}) const;
        AZStd::unique_ptr<Terrain::TerrainSystem> CreateAndActivateTerrainSystem(
            float heightQueryResolution, float surfaceQueryResolution, const AzFramework::Terrain::FloatRange& heightBounds) const;

        void CreateTestTerrainSystem(const AZ::Aabb& worldBounds, float queryResolution, uint32_t numSurfaces);
        void CreateTestTerrainSystemWithSurfaceGradients(const AZ::Aabb& worldBounds, float queryResolution);
        void DestroyTestTerrainSystem();

    private:
        // State data for a full test terrain system setup.
        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> m_heightGradientEntities;
        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> m_surfaceGradientEntities;
        AZStd::unique_ptr<AZ::Entity> m_terrainLayerSpawnerEntity;
        AZStd::unique_ptr<Terrain::TerrainSystem> m_terrainSystem;
    };

    class TerrainTestFixture
        : public TerrainBaseFixture
        , public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            SetupCoreSystems();
        }

        void TearDown() override
        {
            TearDownCoreSystems();
        }
    };

    // This test fixture initializes and destroys both the Atom RPI and the Terrain System Component as a part of setup and teardown.
    // It's useful for creating unit tests that use or test the terrain level components.
    class TerrainSystemTestFixture : public UnitTest::TerrainTestFixture
    {
    protected:
        TerrainSystemTestFixture();

        void SetUp() override;
        void TearDown() override;

    private:
        AZStd::unique_ptr<UnitTest::StubRHI::Factory> m_rhiFactory;
        AZStd::unique_ptr<AZ::RPI::RPISystem> m_rpiSystem;
        AZStd::unique_ptr<AZ::RPI::ImageSystem> m_imageSystem;

        UnitTest::SetRestoreFileIOBaseRAII m_restoreFileIO;
        ::testing::NiceMock<AZ::IO::MockFileIOBase> m_fileIOMock;

        AZStd::unique_ptr<AZ::Entity> m_systemEntity;
    };


#ifdef HAVE_BENCHMARK
    class TerrainBenchmarkFixture
        : public TerrainBaseFixture
        , public ::benchmark::Fixture
    {
    public:
        void internalSetUp()
        {
            SetupCoreSystems();
        }

        void internalTearDown()
        {
            TearDownCoreSystems();
        }

    protected:
        void SetUp([[maybe_unused]] const benchmark::State& state) override
        {
            internalSetUp();
        }
        void SetUp([[maybe_unused]] benchmark::State& state) override
        {
            internalSetUp();
        }

        void TearDown([[maybe_unused]] const benchmark::State& state) override
        {
            internalTearDown();
        }
        void TearDown([[maybe_unused]] benchmark::State& state) override
        {
            internalTearDown();
        }
    };
#endif
}
