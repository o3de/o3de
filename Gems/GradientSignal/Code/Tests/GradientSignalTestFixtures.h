/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Tests/GradientSignalTestMocks.h>
#include <LmbrCentral/Shape/MockShapes.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <AzTest/GemTestEnvironment.h>

namespace UnitTest::StubRHI
{
    class Factory;
}// namespace UnitTest::StubRHI

namespace AZ::RPI
{
    class RPISystem;
    class ImageSystem;
} // namespace AZ::RPI

namespace UnitTest
{
    // The GradientSignal unit tests need to use the GemTestEnvironment to load the LmbrCentral Gem so that Shape components can be used
    // in the unit tests and benchmarks.
    class GradientSignalTestEnvironment
        : public AZ::Test::GemTestEnvironment
    {
    public:
        void AddGemsAndComponents() override;
        void PostCreateApplication() override;
    };

#ifdef HAVE_BENCHMARK
    //! The Benchmark environment is used for one time setup and tear down of shared resources
    class GradientSignalBenchmarkEnvironment
        : public AZ::Test::BenchmarkEnvironmentBase
        , public GradientSignalTestEnvironment

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

    // Base test fixture used for GradientSignal unit tests and benchmark tests
    class GradientSignalBaseFixture
    {
    public:
        GradientSignalBaseFixture();
        virtual ~GradientSignalBaseFixture();

        void SetupCoreSystems();
        void TearDownCoreSystems();

        AZStd::unique_ptr<AZ::Entity> CreateEntity()
        {
            return AZStd::make_unique<AZ::Entity>();
        }

        void ActivateEntity(AZ::Entity* entity)
        {
            entity->Init();
            entity->Activate();
        }

        // Create an entity with a box shape and a transform. It won't be activated yet though, because we expect a gradient component
        // to also get added to it first before activation.
        AZStd::unique_ptr<AZ::Entity> CreateTestEntity(float shapeHalfBounds);

        // Create an entity with a sphere shape and a transform. It won't be activated yet though, because we expect a gradient component
        // to also get added to it first before activation.
        AZStd::unique_ptr<AZ::Entity> CreateTestSphereEntity(float shapeRadius);

        // Create and activate an entity with a gradient component of the requested type, initialized with test data.
        AZStd::unique_ptr<AZ::Entity> BuildTestConstantGradient(float shapeHalfBounds, float value = 0.75f);
        AZStd::unique_ptr<AZ::Entity> BuildTestImageGradient(float shapeHalfBounds);
        AZStd::unique_ptr<AZ::Entity> BuildTestPerlinGradient(float shapeHalfBounds);
        AZStd::unique_ptr<AZ::Entity> BuildTestRandomGradient(float shapeHalfBounds);
        AZStd::unique_ptr<AZ::Entity> BuildTestShapeAreaFalloffGradient(float shapeHalfBounds);

        AZStd::unique_ptr<AZ::Entity> BuildTestDitherGradient(float shapeHalfBounds, const AZ::EntityId& inputGradientId);
        AZStd::unique_ptr<AZ::Entity> BuildTestInvertGradient(float shapeHalfBounds, const AZ::EntityId& inputGradientId);
        AZStd::unique_ptr<AZ::Entity> BuildTestLevelsGradient(float shapeHalfBounds, const AZ::EntityId& inputGradientId);
        AZStd::unique_ptr<AZ::Entity> BuildTestMixedGradient(
            float shapeHalfBounds, const AZ::EntityId& baseGradientId, const AZ::EntityId& mixedGradientId);
        AZStd::unique_ptr<AZ::Entity> BuildTestPosterizeGradient(float shapeHalfBounds, const AZ::EntityId& inputGradientId);
        AZStd::unique_ptr<AZ::Entity> BuildTestReferenceGradient(float shapeHalfBounds, const AZ::EntityId& inputGradientId);
        AZStd::unique_ptr<AZ::Entity> BuildTestSmoothStepGradient(float shapeHalfBounds, const AZ::EntityId& inputGradientId);
        AZStd::unique_ptr<AZ::Entity> BuildTestThresholdGradient(float shapeHalfBounds, const AZ::EntityId& inputGradientId);

        AZStd::unique_ptr<AZ::Entity> BuildTestSurfaceAltitudeGradient(float shapeHalfBounds);
        AZStd::unique_ptr<AZ::Entity> BuildTestSurfaceMaskGradient(float shapeHalfBounds);
        AZStd::unique_ptr<AZ::Entity> BuildTestSurfaceSlopeGradient(float shapeHalfBounds);

    protected:
        AZStd::unique_ptr<UnitTest::StubRHI::Factory> m_rhiFactory;
        AZStd::unique_ptr<AZ::RPI::RPISystem> m_rpiSystem;
        AZStd::unique_ptr<AZ::RPI::ImageSystem> m_imageSystem;
    };

    struct GradientSignalTest
        : public GradientSignalBaseFixture
        , public ::testing::Test
    {
    protected:
        void SetUp() override;
        void TearDown() override;

        void TestFixedDataSampler(const AZStd::vector<float>& expectedOutput, int size, AZ::EntityId gradientEntityId);
        void TestFixedDataSampler(const AZStd::vector<float>& expectedOutput, int size, GradientSignal::GradientSampler& gradientSampler);
    };

#ifdef HAVE_BENCHMARK
    class GradientSignalBenchmarkFixture
        : public GradientSignalBaseFixture
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
