/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef HAVE_BENCHMARK

#include <AzTest/AzTest.h>

#include <AzCore/Math/Vector3.h>
#include <FastNoiseGradientComponent.h>
#include <FastNoiseTest.h>
#include <GradientSignalTestHelpers.h>
#include <GradientSignal/Components/GradientTransformComponent.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformModifierRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>

namespace UnitTest
{
    class FastNoiseGetValues
        : public ::benchmark::Fixture
    {
    public:
        void RunGetValueOrGetValuesBenchmark(benchmark::State& state, FastNoise::NoiseType noiseType)
        {
            AZ::Entity* noiseEntity = aznew AZ::Entity("noise_entity");
            ASSERT_TRUE(noiseEntity != nullptr);
            noiseEntity->CreateComponent<AzFramework::TransformComponent>();
            noiseEntity->CreateComponent(LmbrCentral::BoxShapeComponentTypeId);
            noiseEntity->CreateComponent<GradientSignal::GradientTransformComponent>();

            // Set up a FastNoise component with the requested noise type
            FastNoiseGem::FastNoiseGradientConfig cfg;
            cfg.m_frequency = 0.01f;
            cfg.m_noiseType = noiseType;
            noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>(cfg);

            noiseEntity->Init();
            noiseEntity->Activate();

            UnitTest::GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, noiseEntity->GetId());
        }

    };

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient_Value)(benchmark::State& state)
    {
        RunGetValueOrGetValuesBenchmark(state, FastNoise::NoiseType::Value);
    }

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient_ValueFractal)(benchmark::State& state)
    {
        RunGetValueOrGetValuesBenchmark(state, FastNoise::NoiseType::ValueFractal);
    }

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient_Perlin)(benchmark::State& state)
    {
        RunGetValueOrGetValuesBenchmark(state, FastNoise::NoiseType::Perlin);
    }

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient_PerlinFractal)(benchmark::State& state)
    {
        RunGetValueOrGetValuesBenchmark(state, FastNoise::NoiseType::PerlinFractal);
    }

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient_Simplex)(benchmark::State& state)
    {
        RunGetValueOrGetValuesBenchmark(state, FastNoise::NoiseType::Simplex);
    }

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient_SimplexFractal)(benchmark::State& state)
    {
        RunGetValueOrGetValuesBenchmark(state, FastNoise::NoiseType::SimplexFractal);
    }

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient_Cellular)(benchmark::State& state)
    {
        RunGetValueOrGetValuesBenchmark(state, FastNoise::NoiseType::Cellular);
    }

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient_WhiteNoise)(benchmark::State& state)
    {
        RunGetValueOrGetValuesBenchmark(state, FastNoise::NoiseType::WhiteNoise);
    }

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient_Cubic)(benchmark::State& state)
    {
        RunGetValueOrGetValuesBenchmark(state, FastNoise::NoiseType::Cubic);
    }

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient_CubicFractal)(benchmark::State& state)
    {
        RunGetValueOrGetValuesBenchmark(state, FastNoise::NoiseType::CubicFractal);
    }

    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient_Value);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient_ValueFractal);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient_Perlin);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient_PerlinFractal);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient_Simplex);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient_SimplexFractal);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient_Cellular);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient_WhiteNoise);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient_Cubic);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient_CubicFractal);

#endif
}



