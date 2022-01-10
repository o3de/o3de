/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestFixtures.h>

#include <AzFramework/Components/TransformComponent.h>
#include <GradientSignal/Components/GradientTransformComponent.h>

#include <GradientSignal/Components/ConstantGradientComponent.h>
#include <GradientSignal/Components/ImageGradientComponent.h>
#include <GradientSignal/Components/PerlinGradientComponent.h>
#include <GradientSignal/Components/RandomGradientComponent.h>

#include <GradientSignal/Components/DitherGradientComponent.h>
#include <GradientSignal/Components/InvertGradientComponent.h>
#include <GradientSignal/Components/LevelsGradientComponent.h>
#include <GradientSignal/Components/PosterizeGradientComponent.h>
#include <GradientSignal/Components/ReferenceGradientComponent.h>

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

    AZStd::unique_ptr<AZ::Entity> GradientSignalBaseFixture::CreateTestEntity(float shapeHalfBounds)
    {
        // Create the base entity
        AZStd::unique_ptr<AZ::Entity> testEntity = CreateEntity();

        // Create a mock Shape component that describes the bounds that we're using to map our gradient into world space.
        CreateComponent<MockShapeComponent>(testEntity.get());
        MockShapeComponentHandler mockShapeHandler(testEntity->GetId());
        mockShapeHandler.m_GetLocalBounds = AZ::Aabb::CreateCenterRadius(AZ::Vector3(shapeHalfBounds), shapeHalfBounds);

        // Create a transform that locates our gradient in the center of our desired mock Shape.
        auto transform = CreateComponent<AzFramework::TransformComponent>(testEntity.get());
        transform->SetLocalTM(AZ::Transform::CreateTranslation(AZ::Vector3(shapeHalfBounds)));
        transform->SetWorldTM(AZ::Transform::CreateTranslation(AZ::Vector3(shapeHalfBounds)));

        return testEntity;
    }

    void GradientSignalBaseFixture::CreateTestConstantGradient(AZ::Entity* entity)
    {
        // Create the Image Gradient Component with some default sizes and parameters.
        GradientSignal::ConstantGradientConfig config;
        config.m_value = 0.75f;
        CreateComponent<GradientSignal::ConstantGradientComponent>(entity, config);
    }

    void GradientSignalBaseFixture::CreateTestImageGradient(AZ::Entity* entity)
    {
        // Create the Image Gradient Component with some default sizes and parameters.
        GradientSignal::ImageGradientConfig config;
        const uint32_t imageSize = 4096;
        const int32_t imageSeed = 12345;
        config.m_imageAsset = ImageAssetMockAssetHandler::CreateImageAsset(imageSize, imageSize, imageSeed);
        config.m_tilingX = 1.0f;
        config.m_tilingY = 1.0f;
        CreateComponent<GradientSignal::ImageGradientComponent>(entity, config);

        // Create the Gradient Transform Component with some default parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        CreateComponent<GradientSignal::GradientTransformComponent>(entity, gradientTransformConfig);
    }

    void GradientSignalBaseFixture::CreateTestPerlinGradient(AZ::Entity* entity)
    {
        // Create the Perlin Gradient Component with some default sizes and parameters.
        GradientSignal::PerlinGradientConfig config;
        config.m_amplitude = 1.0f;
        config.m_frequency = 1.1f;
        config.m_octave = 4;
        config.m_randomSeed = 12345;
        CreateComponent<GradientSignal::PerlinGradientComponent>(entity, config);

        // Create the Gradient Transform Component with some default parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        CreateComponent<GradientSignal::GradientTransformComponent>(entity, gradientTransformConfig);
    }

    void GradientSignalBaseFixture::CreateTestRandomGradient(AZ::Entity* entity)
    {
        // Create the Random Gradient Component with some default parameters.
        GradientSignal::RandomGradientConfig config;
        config.m_randomSeed = 12345;
        CreateComponent<GradientSignal::RandomGradientComponent>(entity, config);

        // Create the Gradient Transform Component with some default parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        CreateComponent<GradientSignal::GradientTransformComponent>(entity, gradientTransformConfig);
    }

    void GradientSignalBaseFixture::CreateTestDitherGradient(AZ::Entity* entity, const AZ::EntityId& inputGradientId)
    {
        // Create the Dither Gradient Component with some default parameters.
        GradientSignal::DitherGradientConfig config;
        config.m_useSystemPointsPerUnit = false;
        config.m_pointsPerUnit = 1.0f;
        config.m_patternOffset = AZ::Vector3::CreateZero();
        config.m_patternType = GradientSignal::DitherGradientConfig::BayerPatternType::PATTERN_SIZE_4x4;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        CreateComponent<GradientSignal::DitherGradientComponent>(entity, config);
    }

    void GradientSignalBaseFixture::CreateTestInvertGradient(AZ::Entity* entity, const AZ::EntityId& inputGradientId)
    {
        // Create the Invert Gradient Component.
        GradientSignal::InvertGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        CreateComponent<GradientSignal::InvertGradientComponent>(entity, config);
    }

    void GradientSignalBaseFixture::CreateTestLevelsGradient(AZ::Entity* entity, const AZ::EntityId& inputGradientId)
    {
        // Create the Levels Gradient Component with some default parameters.
        GradientSignal::LevelsGradientConfig config;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        config.m_inputMin = 0.1f;
        config.m_inputMid = 0.3f;
        config.m_inputMax = 0.9f;
        config.m_outputMin = 0.0f;
        config.m_outputMax = 1.0f;
        CreateComponent<GradientSignal::LevelsGradientComponent>(entity, config);
    }

    void GradientSignalBaseFixture::CreateTestPosterizeGradient(AZ::Entity* entity, const AZ::EntityId& inputGradientId)
    {
        // Create the Posterize Gradient Component with some default parameters.
        GradientSignal::PosterizeGradientConfig config;
        config.m_mode = GradientSignal::PosterizeGradientConfig::ModeType::Ps;
        config.m_bands = 5;
        config.m_gradientSampler.m_gradientId = inputGradientId;
        CreateComponent<GradientSignal::PosterizeGradientComponent>(entity, config);
    }

    void GradientSignalBaseFixture::CreateTestReferenceGradient(AZ::Entity* entity, const AZ::EntityId& inputGradientId)
    {
        // Create the Reference Gradient Component.
        GradientSignal::ReferenceGradientConfig config;
        config.m_gradientSampler.m_ownerEntityId = entity->GetId();
        config.m_gradientSampler.m_gradientId = inputGradientId;
        CreateComponent<GradientSignal::ReferenceGradientComponent>(entity, config);
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
    void GradientSignalBenchmarkFixture::RunSamplerGetValueBenchmark(benchmark::State& state)
    {
        AZ_PROFILE_FUNCTION(Entity);

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

    void GradientSignalBenchmarkFixture::RunSamplerGetValuesBenchmark(benchmark::State& state)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // All components are created, so activate the entity
        ActivateEntity(m_testEntity.get());

        // Create a gradient sampler and run through a series of points to see if they match expectations.
        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = m_testEntity->GetId();

        // Get the height and width ranges for querying from our benchmark parameters
        float height = aznumeric_cast<float>(state.range(0));
        float width = aznumeric_cast<float>(state.range(1));
        int64_t totalQueryPoints = state.range(0) * state.range(1);

        // Call GetValues() for every height and width in our ranges.
        for (auto _ : state)
        {
            // Set up our vector of query positions.
            AZStd::vector<AZ::Vector3> positions(totalQueryPoints);
            size_t index = 0;
            for (float y = 0.0f; y < height; y += 1.0f)
            {
                for (float x = 0.0f; x < width; x += 1.0f)
                {
                    positions[index++] = AZ::Vector3(x, y, 0.0f);
                }
            }

            // Query and get the results.
            AZStd::vector<float> results(totalQueryPoints);
            gradientSampler.GetValues(positions, results);
        }
    }

    void GradientSignalBenchmarkFixture::RunEBusGetValueBenchmark(benchmark::State& state)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // All components are created, so activate the entity
        ActivateEntity(m_testEntity.get());

        GradientSignal::GradientSampleParams params;

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
                    float value = 0.0f;
                    params.m_position = AZ::Vector3(x, y, 0.0f);
                    GradientSignal::GradientRequestBus::EventResult(
                        value, m_testEntity->GetId(), &GradientSignal::GradientRequestBus::Events::GetValue, params);
                    benchmark::DoNotOptimize(value);
                }
            }
        }
    }

    void GradientSignalBenchmarkFixture::RunEBusGetValuesBenchmark(benchmark::State& state)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // All components are created, so activate the entity
        ActivateEntity(m_testEntity.get());

        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = m_testEntity->GetId();

        // Get the height and width ranges for querying from our benchmark parameters
        float height = aznumeric_cast<float>(state.range(0));
        float width = aznumeric_cast<float>(state.range(1));
        int64_t totalQueryPoints = state.range(0) * state.range(1);

        // Call GetValues() for every height and width in our ranges.
        for (auto _ : state)
        {
            // Set up our vector of query positions.
            AZStd::vector<AZ::Vector3> positions(totalQueryPoints);
            size_t index = 0;
            for (float y = 0.0f; y < height; y += 1.0f)
            {
                for (float x = 0.0f; x < width; x += 1.0f)
                {
                    positions[index++] = AZ::Vector3(x, y, 0.0f);
                }
            }

            // Query and get the results.
            AZStd::vector<float> results(totalQueryPoints);
            GradientSignal::GradientRequestBus::Event(
                m_testEntity->GetId(), &GradientSignal::GradientRequestBus::Events::GetValues, positions, results);
        }
    }
#endif

}

