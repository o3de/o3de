/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef HAVE_BENCHMARK

#include <Tests/GradientSignalTestFixtures.h>
#include <Tests/GradientSignalTestHelpers.h>

#include <AzTest/AzTest.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

namespace UnitTest
{
    class GradientGetValues : public GradientSignalBenchmarkFixture
    {
    public:
        // Create an arbitrary size shape for creating our gradients for benchmark runs.
        const float TestShapeHalfBounds = 128.0f;
    };

    // --------------------------------------------------------------------------------------
    // Base Gradients

    BENCHMARK_DEFINE_F(GradientGetValues, BM_ConstantGradient)(benchmark::State& state)
    {
        auto entity = BuildTestConstantGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_ImageGradient)(benchmark::State& state)
    {
        auto entity = BuildTestImageGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }


    BENCHMARK_DEFINE_F(GradientGetValues, BM_PerlinGradient)(benchmark::State& state)
    {
        auto entity = BuildTestPerlinGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_RandomGradient)(benchmark::State& state)
    {
        auto entity = BuildTestRandomGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_ShapeAreaFalloffGradient)(benchmark::State& state)
    {
        auto entity = BuildTestShapeAreaFalloffGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_ConstantGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_ImageGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_PerlinGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_RandomGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_ShapeAreaFalloffGradient);

    // --------------------------------------------------------------------------------------
    // Gradient Modifiers

    BENCHMARK_DEFINE_F(GradientGetValues, BM_DitherGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestDitherGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_InvertGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestDitherGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_LevelsGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestLevelsGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_MixedGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto mixedEntity = BuildTestConstantGradient(TestShapeHalfBounds);
        auto entity = BuildTestMixedGradient(TestShapeHalfBounds, baseEntity->GetId(), mixedEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_PosterizeGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestPosterizeGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_ReferenceGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestReferenceGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_SmoothStepGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestSmoothStepGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_ThresholdGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestThresholdGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_DitherGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_InvertGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_LevelsGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_MixedGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_PosterizeGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_ReferenceGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_SmoothStepGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_ThresholdGradient);

    // --------------------------------------------------------------------------------------
    // Surface Gradients

    BENCHMARK_DEFINE_F(GradientGetValues, BM_SurfaceAltitudeGradient)(benchmark::State& state)
    {
        auto mockSurfaceDataSystem =
            CreateMockSurfaceDataSystem(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-TestShapeHalfBounds), AZ::Vector3(TestShapeHalfBounds)));

        auto entity = BuildTestSurfaceAltitudeGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_SurfaceMaskGradient)(benchmark::State& state)
    {
        auto mockSurfaceDataSystem =
            CreateMockSurfaceDataSystem(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-TestShapeHalfBounds), AZ::Vector3(TestShapeHalfBounds)));

        auto entity = BuildTestSurfaceMaskGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_SurfaceSlopeGradient)(benchmark::State& state)
    {
        auto mockSurfaceDataSystem =
            CreateMockSurfaceDataSystem(AZ::Aabb::CreateFromMinMax(AZ::Vector3(-TestShapeHalfBounds), AZ::Vector3(TestShapeHalfBounds)));

        auto entity = BuildTestSurfaceSlopeGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_SurfaceAltitudeGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_SurfaceMaskGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_SurfaceSlopeGradient);

#endif
}


