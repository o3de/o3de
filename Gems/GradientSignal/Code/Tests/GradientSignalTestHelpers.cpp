/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/GradientSignalTestHelpers.h>
#include <AzCore/Math/Aabb.h>
#include <GradientSignal/GradientSampler.h>

namespace UnitTest
{
    void GradientSignalTestHelpers::CompareGetValueAndGetValues(AZ::EntityId gradientEntityId, float shapeHalfBounds)
    {
        // Create a gradient sampler and run through a series of points to see if they match expectations.

        const AZ::Aabb queryRegion = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-shapeHalfBounds), AZ::Vector3(shapeHalfBounds));
        const AZ::Vector2 stepSize(1.0f, 1.0f);

        GradientSignal::GradientSampler gradientSampler;
        gradientSampler.m_gradientId = gradientEntityId;

        const size_t numSamplesX = aznumeric_cast<size_t>(ceil(queryRegion.GetExtents().GetX() / stepSize.GetX()));
        const size_t numSamplesY = aznumeric_cast<size_t>(ceil(queryRegion.GetExtents().GetY() / stepSize.GetY()));

        // Build up the list of positions to query.
        AZStd::vector<AZ::Vector3> positions(numSamplesX * numSamplesY);
        size_t index = 0;
        for (size_t yIndex = 0; yIndex < numSamplesY; yIndex++)
        {
            float y = queryRegion.GetMin().GetY() + (stepSize.GetY() * yIndex);
            for (size_t xIndex = 0; xIndex < numSamplesX; xIndex++)
            {
                float x = queryRegion.GetMin().GetX() + (stepSize.GetX() * xIndex);
                positions[index++] = AZ::Vector3(x, y, 0.0f);
            }
        }

        // Get the results from GetValues
        AZStd::vector<float> results(numSamplesX * numSamplesY);
        gradientSampler.GetValues(positions, results);

        // For each position, call GetValue and verify that the values match.
        for (size_t positionIndex = 0; positionIndex < positions.size(); positionIndex++)
        {
            GradientSignal::GradientSampleParams params;
            params.m_position = positions[positionIndex];
            float value = gradientSampler.GetValue(params);

            // We use ASSERT_NEAR instead of EXPECT_NEAR because if one value doesn't match, they probably all won't, so there's no
            // reason to keep running and printing failures for every value.
            ASSERT_NEAR(value, results[positionIndex], 0.000001f);
        }
    }

#ifdef HAVE_BENCHMARK

    void GradientSignalTestHelpers::FillQueryPositions(AZStd::vector<AZ::Vector3>& positions, float height, float width)
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

    void GradientSignalTestHelpers::RunEBusGetValueBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
    {
        AZ_PROFILE_FUNCTION(Entity);

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

    void GradientSignalTestHelpers::RunEBusGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
    {
        AZ_PROFILE_FUNCTION(Entity);

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

    void GradientSignalTestHelpers::RunSamplerGetValueBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
    {
        AZ_PROFILE_FUNCTION(Entity);

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

    void GradientSignalTestHelpers::RunSamplerGetValuesBenchmark(
        benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange)
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

    void GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId)
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
#endif
}


