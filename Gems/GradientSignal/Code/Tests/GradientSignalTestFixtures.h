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

namespace UnitTest
{
    // Base test fixture used for GradientSignal unit tests and benchmark tests
    class GradientSignalBaseFixture
    {
    public:
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

        template<typename Component, typename Configuration>
        Component* CreateComponent(AZ::Entity* entity, const Configuration& config)
        {
            m_app->RegisterComponentDescriptor(Component::CreateDescriptor());
            return entity->CreateComponent<Component>(config);
        }

        template<typename Component>
        Component* CreateComponent(AZ::Entity* entity)
        {
            m_app->RegisterComponentDescriptor(Component::CreateDescriptor());
            return entity->CreateComponent<Component>();
        }

        // Create a mock shape that will respond to the shape bus with proper responses for the given input box.
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockShapeComponentRequests>> CreateMockShape(
            const AZ::Aabb& spawnerBox, const AZ::EntityId& shapeEntityId);

        // Create a mock SurfaceDataSystem that will respond to requests for surface points with mock responses for points inside
        // the given input box.
        AZStd::unique_ptr<MockSurfaceDataSystem> CreateMockSurfaceDataSystem(const AZ::Aabb& spawnerBox);

        // Create an entity with a mock shape and a transform. It won't be activated yet though, because we expect a gradient component
        // to also get added to it first before activation.
        AZStd::unique_ptr<AZ::Entity> CreateTestEntity(float shapeHalfBounds);

        // Create and activate an entity with a gradient component of the requested type, initialized with test data.
        AZStd::unique_ptr<AZ::Entity> BuildTestConstantGradient(float shapeHalfBounds);
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

        AZStd::unique_ptr<AZ::ComponentApplication> m_app;
        AZ::Entity* m_systemEntity = nullptr;
        ImageAssetMockAssetHandler* m_mockHandler = nullptr;
        AZStd::vector<AZStd::unique_ptr<testing::NiceMock<UnitTest::MockShapeComponentRequests>>>* m_mockShapeHandlers = nullptr;
    };

    struct GradientSignalTest
        : public GradientSignalBaseFixture
        , public UnitTest::AllocatorsTestFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();
            SetupCoreSystems();
        }

        void TearDown() override
        {
            TearDownCoreSystems();
            UnitTest::AllocatorsTestFixture::TearDown();
        }

        void TestFixedDataSampler(const AZStd::vector<float>& expectedOutput, int size, AZ::EntityId gradientEntityId);
    };

#ifdef HAVE_BENCHMARK
    class GradientSignalBenchmarkFixture
        : public GradientSignalBaseFixture
        , public UnitTest::AllocatorsBenchmarkFixture
        , public UnitTest::TraceBusRedirector
    {
    public:
        void internalSetUp(const benchmark::State& state)
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
            UnitTest::AllocatorsBenchmarkFixture::SetUp(state);
            SetupCoreSystems();
        }

        void internalTearDown(const benchmark::State& state)
        {
            TearDownCoreSystems();
            UnitTest::AllocatorsBenchmarkFixture::TearDown(state);
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

    protected:
        void SetUp(const benchmark::State& state) override
        {
            internalSetUp(state);
        }
        void SetUp(benchmark::State& state) override
        {
            internalSetUp(state);
        }

        void TearDown(const benchmark::State& state) override
        {
            internalTearDown(state);
        }
        void TearDown(benchmark::State& state) override
        {
            internalTearDown(state);
        }
    };
#endif
}
