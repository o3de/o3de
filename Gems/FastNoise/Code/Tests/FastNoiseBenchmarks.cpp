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
#include <GradientSignal/Components/GradientTransformComponent.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientTransformModifierRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <FastNoiseSystemComponent.h>
#include <FastNoiseGradientComponent.h>
#include <FastNoiseModule.h>
#include <FastNoiseTest.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>

namespace UnitTest
{
    class FastNoiseGetValues
        : public ::benchmark::Fixture
    {
    public:
        // We use an enum to list out the different types of GetValue() benchmarks to run so that way we can condense our test cases
        // to just take the value in as a benchmark argument and switch on it. Otherwise, we would need to write a different benchmark
        // function for each test case for each gradient.
        enum GetValuePermutation : int64_t
        {
            EBUS_GET_VALUE,
            EBUS_GET_VALUES,
            SAMPLER_GET_VALUE,
            SAMPLER_GET_VALUES,
        };

        // Create an arbitrary size shape for creating our gradients for benchmark runs.
        const float TestShapeHalfBounds = 128.0f;

        void FillQueryPositions(AZStd::vector<AZ::Vector3>& positions, float height, float width)
        {
            size_t index = 0;
            for (float y = 0.0f; y < height; y += 1.0f)
            {
                for (float x = 0.0f; x < width; x += 1.0f)
                {
                    positions[index++] = AZ::Vector3(x, y, 0.0f);
                }
            }
        }

        void RunEBusGetValueBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
        {
            //AZ_PROFILE_FUNCTION(Entity);

            GradientSignal::GradientSampleParams params;

            // Get the height and width ranges for querying from our benchmark parameters
            const float height = aznumeric_cast<float>(queryRange);
            const float width = aznumeric_cast<float>(queryRange);

            // Call GetValue() on the EBus for every height and width in our ranges.
            for (auto _ : state)
            {
                for (float y = 0.0f; y < height; y += 1.0f)
                {
                    for (float x = 0.0f; x < width; x += 1.0f)
                    {
                        float value = 0.0f;
                        params.m_position = AZ::Vector3(x, y, 0.0f);
                        GradientSignal::GradientRequestBus::EventResult(
                            value, gradientId, &GradientSignal::GradientRequestBus::Events::GetValue, params);
                        benchmark::DoNotOptimize(value);
                    }
                }
            }
        }

        void RunEBusGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
        {
            //AZ_PROFILE_FUNCTION(Entity);

            // Get the height and width ranges for querying from our benchmark parameters
            float height = aznumeric_cast<float>(queryRange);
            float width = aznumeric_cast<float>(queryRange);
            int64_t totalQueryPoints = queryRange * queryRange;

            // Call GetValues() for every height and width in our ranges.
            for (auto _ : state)
            {
                // Set up our vector of query positions. This is done inside the benchmark timing since we're counting the work to create
                // each query position in the single GetValue() call benchmarks, and will make the timing more directly comparable.
                AZStd::vector<AZ::Vector3> positions(totalQueryPoints);
                FillQueryPositions(positions, height, width);

                // Query and get the results.
                AZStd::vector<float> results(totalQueryPoints);
                GradientSignal::GradientRequestBus::Event(
                    gradientId, &GradientSignal::GradientRequestBus::Events::GetValues, positions, results);
            }
        }

        void RunSamplerGetValueBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
        {
            //AZ_PROFILE_FUNCTION(Entity);

            // Create a gradient sampler to use for querying our gradient.
            GradientSignal::GradientSampler gradientSampler;
            gradientSampler.m_gradientId = gradientId;

            // Get the height and width ranges for querying from our benchmark parameters
            const float height = aznumeric_cast<float>(queryRange);
            const float width = aznumeric_cast<float>(queryRange);

            // Call GetValue() through the GradientSampler for every height and width in our ranges.
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

        void RunSamplerGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
        {
            AZ_PROFILE_FUNCTION(Entity);

            // Create a gradient sampler to use for querying our gradient.
            GradientSignal::GradientSampler gradientSampler;
            gradientSampler.m_gradientId = gradientId;

            // Get the height and width ranges for querying from our benchmark parameters
            const float height = aznumeric_cast<float>(queryRange);
            const float width = aznumeric_cast<float>(queryRange);
            const int64_t totalQueryPoints = queryRange * queryRange;

            // Call GetValues() through the GradientSampler for every height and width in our ranges.
            for (auto _ : state)
            {
                // Set up our vector of query positions. This is done inside the benchmark timing since we're counting the work to create
                // each query position in the single GetValue() call benchmarks, and will make the timing more directly comparable.
                AZStd::vector<AZ::Vector3> positions(totalQueryPoints);
                FillQueryPositions(positions, height, width);

                // Query and get the results.
                AZStd::vector<float> results(totalQueryPoints);
                gradientSampler.GetValues(positions, results);
            }
        }

        void RunGetValueOrGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId)
        {
            switch (state.range(0))
            {
            case GetValuePermutation::EBUS_GET_VALUE:
                RunEBusGetValueBenchmark(state, gradientId, state.range(1));
                break;
            case GetValuePermutation::EBUS_GET_VALUES:
                RunEBusGetValuesBenchmark(state, gradientId, state.range(1));
                break;
            case GetValuePermutation::SAMPLER_GET_VALUE:
                RunSamplerGetValueBenchmark(state, gradientId, state.range(1));
                break;
            case GetValuePermutation::SAMPLER_GET_VALUES:
                RunSamplerGetValuesBenchmark(state, gradientId, state.range(1));
                break;
            default:
                AZ_Assert(false, "Benchmark permutation type not supported.");
            }
        }
    };

    // Because there's no good way to label different enums in the output results (they just appear as integer values), we work around it by
    // registering one set of benchmark runs for each enum value and use ArgNames() to give it a friendly name in the results.
#define GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(Fixture, Func)                                                                     \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                    \
        ->Args({ FastNoiseGetValues::GetValuePermutation::EBUS_GET_VALUE, 1024 })                                                           \
        ->Args({ FastNoiseGetValues::GetValuePermutation::EBUS_GET_VALUE, 2048 })                                                           \
        ->Args({ FastNoiseGetValues::GetValuePermutation::EBUS_GET_VALUE, 4096 })                                                           \
        ->ArgNames({ "EbusGetValue", "size" })                                                                                             \
        ->Unit(::benchmark::kMillisecond);                                                                                                 \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                    \
        ->Args({ FastNoiseGetValues::GetValuePermutation::EBUS_GET_VALUES, 1024 })                                                          \
        ->Args({ FastNoiseGetValues::GetValuePermutation::EBUS_GET_VALUES, 2048 })                                                          \
        ->Args({ FastNoiseGetValues::GetValuePermutation::EBUS_GET_VALUES, 4096 })                                                          \
        ->ArgNames({ "EbusGetValues", "size" })                                                                                            \
        ->Unit(::benchmark::kMillisecond);                                                                                                 \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                    \
        ->Args({ FastNoiseGetValues::GetValuePermutation::SAMPLER_GET_VALUE, 1024 })                                                        \
        ->Args({ FastNoiseGetValues::GetValuePermutation::SAMPLER_GET_VALUE, 2048 })                                                        \
        ->Args({ FastNoiseGetValues::GetValuePermutation::SAMPLER_GET_VALUE, 4096 })                                                        \
        ->ArgNames({ "SamplerGetValue", "size" })                                                                                          \
        ->Unit(::benchmark::kMillisecond);                                                                                                 \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                    \
        ->Args({ FastNoiseGetValues::GetValuePermutation::SAMPLER_GET_VALUES, 1024 })                                                       \
        ->Args({ FastNoiseGetValues::GetValuePermutation::SAMPLER_GET_VALUES, 2048 })                                                       \
        ->Args({ FastNoiseGetValues::GetValuePermutation::SAMPLER_GET_VALUES, 4096 })                                                       \
        ->ArgNames({ "SamplerGetValues", "size" })                                                                                         \
        ->Unit(::benchmark::kMillisecond);

    // --------------------------------------------------------------------------------------
    // Base Gradients

    BENCHMARK_DEFINE_F(FastNoiseGetValues, BM_FastNoiseGradient)(benchmark::State& state)
    {
        AZ::Entity* noiseEntity = aznew AZ::Entity("noise_entity");
        ASSERT_TRUE(noiseEntity != nullptr);
        noiseEntity->CreateComponent<AzFramework::TransformComponent>();
        noiseEntity->CreateComponent(LmbrCentral::BoxShapeComponentTypeId);
        noiseEntity->CreateComponent<GradientSignal::GradientTransformComponent>();
        noiseEntity->CreateComponent<FastNoiseGem::FastNoiseGradientComponent>();

        noiseEntity->Init();
        noiseEntity->Activate();

        RunGetValueOrGetValuesBenchmark(state, noiseEntity->GetId());
    }

    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(FastNoiseGetValues, BM_FastNoiseGradient);

#endif
}



