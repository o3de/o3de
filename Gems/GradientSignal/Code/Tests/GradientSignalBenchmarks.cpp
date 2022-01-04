/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef HAVE_BENCHMARK

#include <Tests/GradientSignalTestFixtures.h>

#include <AzTest/AzTest.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <Source/Components/ImageGradientComponent.h>
#include <Source/Components/PerlinGradientComponent.h>
#include <Source/Components/RandomGradientComponent.h>
#include <Source/Components/GradientTransformComponent.h>

namespace UnitTest
{
    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_ImageGradientGetValue)(benchmark::State& state)
    {
        // Create the Image Gradient Component with some default sizes and parameters.
        GradientSignal::ImageGradientConfig config;
        const uint32_t imageSize = 4096;
        const int32_t imageSeed = 12345;
        config.m_imageAsset = ImageAssetMockAssetHandler::CreateImageAsset(imageSize, imageSize, imageSeed);
        config.m_tilingX = 1.0f;
        config.m_tilingY = 1.0f;
        CreateComponent<GradientSignal::ImageGradientComponent>(m_testEntity.get(), config);

        // Create the Gradient Transform Component with some default parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        CreateComponent<GradientSignal::GradientTransformComponent>(m_testEntity.get(), gradientTransformConfig);

        // Run the benchmark
        RunGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_ImageGradientGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_PerlinGradientGetValue)(benchmark::State& state)
    {
        // Create the Perlin Gradient Component with some default sizes and parameters.
        GradientSignal::PerlinGradientConfig config;
        config.m_amplitude = 1.0f;
        config.m_frequency = 1.1f;
        config.m_octave = 4;
        config.m_randomSeed = 12345;
        CreateComponent<GradientSignal::PerlinGradientComponent>(m_testEntity.get(), config);

        // Create the Gradient Transform Component with some default parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        CreateComponent<GradientSignal::GradientTransformComponent>(m_testEntity.get(), gradientTransformConfig);

        // Run the benchmark
        RunGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_PerlinGradientGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_RandomGradientGetValue)(benchmark::State& state)
    {
        // Create the Random Gradient Component with some default parameters.
        GradientSignal::RandomGradientConfig config;
        config.m_randomSeed = 12345;
        CreateComponent<GradientSignal::RandomGradientComponent>(m_testEntity.get(), config);

        // Create the Gradient Transform Component with some default parameters.
        GradientSignal::GradientTransformConfig gradientTransformConfig;
        gradientTransformConfig.m_wrappingType = GradientSignal::WrappingType::None;
        CreateComponent<GradientSignal::GradientTransformComponent>(m_testEntity.get(), gradientTransformConfig);

        // Run the benchmark
        RunGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_RandomGradientGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

#endif




}


