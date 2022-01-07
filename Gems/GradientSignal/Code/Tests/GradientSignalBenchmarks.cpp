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

#include <GradientSignal/Components/ImageGradientComponent.h>
#include <GradientSignal/Components/PerlinGradientComponent.h>
#include <GradientSignal/Components/RandomGradientComponent.h>
#include <GradientSignal/Components/GradientTransformComponent.h>

namespace UnitTest
{
    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_ConstantGradientEBusGetValue)(benchmark::State& state)
    {
        CreateTestConstantGradient(m_testEntity.get());
        RunEBusGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_ConstantGradientEBusGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_ConstantGradientEBusGetValues)(benchmark::State& state)
    {
        CreateTestConstantGradient(m_testEntity.get());
        RunEBusGetValuesBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_ConstantGradientEBusGetValues)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_ConstantGradientSamplerGetValue)(benchmark::State& state)
    {
        CreateTestConstantGradient(m_testEntity.get());
        RunSamplerGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_ConstantGradientSamplerGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_ConstantGradientSamplerGetValues)(benchmark::State& state)
    {
        CreateTestConstantGradient(m_testEntity.get());
        RunSamplerGetValuesBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_ConstantGradientSamplerGetValues)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);




    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_ImageGradientEBusGetValue)(benchmark::State& state)
    {
        CreateTestImageGradient(m_testEntity.get());
        RunEBusGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_ImageGradientEBusGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_ImageGradientEBusGetValues)(benchmark::State& state)
    {
        CreateTestImageGradient(m_testEntity.get());
        RunEBusGetValuesBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_ImageGradientEBusGetValues)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_ImageGradientSamplerGetValue)(benchmark::State& state)
    {
        CreateTestImageGradient(m_testEntity.get());
        RunSamplerGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_ImageGradientSamplerGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_ImageGradientSamplerGetValues)(benchmark::State& state)
    {
        CreateTestImageGradient(m_testEntity.get());
        RunSamplerGetValuesBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_ImageGradientSamplerGetValues)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_PerlinGradientEBusGetValue)(benchmark::State& state)
    {
        CreateTestPerlinGradient(m_testEntity.get());
        RunEBusGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_PerlinGradientEBusGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_PerlinGradientEBusGetValues)(benchmark::State& state)
    {
        CreateTestPerlinGradient(m_testEntity.get());
        RunEBusGetValuesBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_PerlinGradientEBusGetValues)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_PerlinGradientSamplerGetValue)(benchmark::State& state)
    {
        CreateTestPerlinGradient(m_testEntity.get());
        RunSamplerGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_PerlinGradientSamplerGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_PerlinGradientSamplerGetValues)(benchmark::State& state)
    {
        CreateTestPerlinGradient(m_testEntity.get());
        RunSamplerGetValuesBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_PerlinGradientSamplerGetValues)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_RandomGradientEBusGetValue)(benchmark::State& state)
    {
        CreateTestRandomGradient(m_testEntity.get());
        RunEBusGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_RandomGradientEBusGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_RandomGradientEBusGetValues)(benchmark::State& state)
    {
        CreateTestRandomGradient(m_testEntity.get());
        RunEBusGetValuesBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_RandomGradientEBusGetValues)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_RandomGradientSamplerGetValue)(benchmark::State& state)
    {
        CreateTestRandomGradient(m_testEntity.get());
        RunSamplerGetValueBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_RandomGradientSamplerGetValue)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(GradientSignalBenchmarkFixture, BM_RandomGradientSamplerGetValues)(benchmark::State& state)
    {
        CreateTestRandomGradient(m_testEntity.get());
        RunSamplerGetValuesBenchmark(state);
    }

    BENCHMARK_REGISTER_F(GradientSignalBenchmarkFixture, BM_RandomGradientSamplerGetValues)
        ->Args({ 1024, 1024 })
        ->Args({ 2048, 2048 })
        ->Args({ 4096, 4096 })
        ->Unit(::benchmark::kMillisecond);

#endif




}


