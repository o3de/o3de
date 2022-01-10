/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestFixtures.h>

namespace UnitTest
{
    void GradientSignalBaseFixture::SetupCoreSystems()
    {
        m_app = AZStd::make_unique<AZ::ComponentApplication>();
        ASSERT_TRUE(m_app != nullptr);

        AZ::ComponentApplication::Descriptor componentAppDesc;

        m_systemEntity = m_app->Create(componentAppDesc);
        ASSERT_TRUE(m_systemEntity != nullptr);
        m_app->AddEntity(m_systemEntity);

        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();
        AZ::Data::AssetManager::Descriptor desc;
        AZ::Data::AssetManager::Create(desc);
        m_mockHandler = new ImageAssetMockAssetHandler();
        AZ::Data::AssetManager::Instance().RegisterHandler(m_mockHandler, azrtti_typeid<GradientSignal::ImageAsset>());
    }

    void GradientSignalBaseFixture::TearDownCoreSystems()
    {
        AZ::Data::AssetManager::Instance().UnregisterHandler(m_mockHandler);
        delete m_mockHandler; // delete after removing from the asset manager
        AzFramework::LegacyAssetEventBus::ClearQueuedEvents();
        AZ::Data::AssetManager::Destroy();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();

        m_app->Destroy();
        m_app.reset();
        m_systemEntity = nullptr;
    }

    void GradientSignalTest::TestFixedDataSampler(const AZStd::vector<float>& expectedOutput, int size, AZ::EntityId gradientEntityId)
    {
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = gradientEntityId;

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                GradientSignal::GradientSampleParams params;
                params.m_position = AZ::Vector3(static_cast<float>(x), static_cast<float>(y), 0.0f);

                const int index = y * size + x;
                float actualValue = gradientSampler.GetValue(params);
                float expectedValue = expectedOutput[index];

                EXPECT_NEAR(actualValue, expectedValue, 0.01f);
            }
        }
    }

#ifdef HAVE_BENCHMARK
    void GradientSignalBenchmarkFixture::CreateTestEntity(float shapeHalfBounds)
    {
        // Create the base entity
        m_testEntity = CreateEntity();

        // Create a mock Shape component that describes the bounds that we're using to map our gradient into world space.
        CreateComponent<MockShapeComponent>(m_testEntity.get());
        MockShapeComponentHandler mockShapeHandler(m_testEntity->GetId());
        mockShapeHandler.m_GetLocalBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3(shapeHalfBounds), shapeHalfBounds);

        // Create a mock Transform component that locates our gradient in the center of our desired mock Shape.
        MockTransformHandler mockTransformHandler;
        mockTransformHandler.m_GetLocalTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(shapeHalfBounds));
        mockTransformHandler.m_GetWorldTMOutput = AZ::Transform::CreateTranslation(AZ::Vector3(shapeHalfBounds));
        mockTransformHandler.BusConnect(m_testEntity->GetId());
    }

    void GradientSignalBenchmarkFixture::DestroyTestEntity()
    {
        m_testEntity.reset();
    }

    void GradientSignalBenchmarkFixture::RunGetValueBenchmark(benchmark::State& state)
    {
        // All components are created, so activate the entity
        ActivateEntity(m_testEntity.get());

        // Create a gradient sampler and run through a series of points to see if they match expectations.
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = m_testEntity->GetId();

        // Get the height and width ranges for querying from our benchmark parameters
        float height = aznumeric_cast<float>(state.range(0));
        float width = aznumeric_cast<float>(state.range(1));

        // Call GetValue() for every height and width in our ranges.
        for (auto _ : state)
        {
            for (float y = 0.0f; y < height; y += 1.0f)
            {
                for (float x = 0.0f; x < width; x += 1.0f)
                {
                    GradientSignal::GradientSampleParams params;
                    params.m_position = AZ::Vector3(x, y, 0.0f);
                    float value = gradientSampler.GetValue(params);
                    benchmark::DoNotOptimize(value);
                }
            }
        }
    }

#endif

}

