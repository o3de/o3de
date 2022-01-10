/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Tests/GradientSignalTestMocks.h>

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

        AZStd::unique_ptr<AZ::Entity> CreateTestEntity(float shapeHalfBounds);

        void CreateTestConstantGradient(AZ::Entity* entity);
        void CreateTestImageGradient(AZ::Entity* entity);
        void CreateTestPerlinGradient(AZ::Entity* entity);
        void CreateTestRandomGradient(AZ::Entity* entity);

        void CreateTestDitherGradient(AZ::Entity* entity, const AZ::EntityId& inputGradientId);
        void CreateTestInvertGradient(AZ::Entity* entity, const AZ::EntityId& inputGradientId);
        void CreateTestLevelsGradient(AZ::Entity* entity, const AZ::EntityId& inputGradientId);
        void CreateTestPosterizeGradient(AZ::Entity* entity, const AZ::EntityId& inputGradientId);
        void CreateTestReferenceGradient(AZ::Entity* entity, const AZ::EntityId& inputGradientId);

        AZStd::unique_ptr<AZ::ComponentApplication> m_app;
        AZ::Entity* m_systemEntity = nullptr;
        ImageAssetMockAssetHandler* m_mockHandler = nullptr;
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

            // Create a default test entity with bounds of 256 m x 256 m x 256 m.
            const float shapeHalfBounds = 128.0f;
            m_testEntity = CreateTestEntity(shapeHalfBounds);
        }

        void internalTearDown(const benchmark::State& state)
        {
            m_testEntity.reset();
            TearDownCoreSystems();
            UnitTest::AllocatorsBenchmarkFixture::TearDown(state);
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        void RunSamplerGetValueBenchmark(benchmark::State& state);
        void RunSamplerGetValuesBenchmark(benchmark::State& state);

        void RunEBusGetValueBenchmark(benchmark::State& state);
        void RunEBusGetValuesBenchmark(benchmark::State& state);

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

        AZStd::unique_ptr<AZ::Entity> m_testEntity;
    };
#endif
}
