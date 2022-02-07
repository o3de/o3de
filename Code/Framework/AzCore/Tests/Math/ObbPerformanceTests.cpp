/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace Benchmark
{
    class BM_MathObb
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_position.Set(1.0f, 2.0f, 3.0f);
            m_rotation = AZ::Quaternion::CreateRotationZ(AZ::Constants::QuarterPi);
            m_halfLengths = AZ::Vector3(0.5f);
            m_obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(m_position, m_rotation, m_halfLengths);
        }

    public:
        void SetUp(const benchmark::State&) override
        {
            internalSetUp();
        }
        void SetUp(benchmark::State&) override
        {
            internalSetUp();
        }

        AZ::Obb m_obb;
        AZ::Vector3 m_position;
        AZ::Quaternion m_rotation;
        AZ::Vector3 m_halfLengths;

        const int numIters = 1000;
    };

    BENCHMARK_F(BM_MathObb, CreateFromPositionRotationAndHalfLengths)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                AZ::Obb result =
                    AZ::Obb::CreateFromPositionRotationAndHalfLengths(m_position, m_rotation, m_halfLengths);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, SetPosition)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                m_obb.SetPosition(m_position);
                benchmark::DoNotOptimize(m_obb);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, GetPosition)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                AZ::Vector3 result = m_obb.GetPosition();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, GetAxisX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                AZ::Vector3 result = m_obb.GetAxisX();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, GetAxisY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                AZ::Vector3 result = m_obb.GetAxisY();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, GetAxisZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                AZ::Vector3 result = m_obb.GetAxisZ();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, GetAxisIndex3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                AZ::Vector3 result = m_obb.GetAxis(0);
                benchmark::DoNotOptimize(result);
                result = m_obb.GetAxis(1);
                benchmark::DoNotOptimize(result);
                result = m_obb.GetAxis(2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, SetHalfLengthX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                m_obb.SetHalfLengthX(2.0f);
                benchmark::DoNotOptimize(m_obb);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, GetHalfLengthX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                float result = m_obb.GetHalfLengthX();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, SetHalfLengthY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                m_obb.SetHalfLengthY(2.0f);
                benchmark::DoNotOptimize(m_obb);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, GetHalfLengthY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                float result = m_obb.GetHalfLengthY();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, SetHalfLengthZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                m_obb.SetHalfLengthZ(2.0f);
                benchmark::DoNotOptimize(m_obb);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, GetHalfLengthZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                float result = m_obb.GetHalfLengthZ();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, SetHalfLengthIndex3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                m_obb.SetHalfLength(0, 2.0f);
                m_obb.SetHalfLength(1, 2.0f);
                m_obb.SetHalfLength(2, 2.0f);
                benchmark::DoNotOptimize(m_obb);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, GetHalfLengthIndex3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                float result = m_obb.GetHalfLength(0);
                benchmark::DoNotOptimize(result);
                result = m_obb.GetHalfLength(1);
                benchmark::DoNotOptimize(result);
                result = m_obb.GetHalfLength(2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, CreateFromAabb)(benchmark::State& state)
    {
        AZ::Vector3 min(-100.0f, 50.0f, 0.0f);
        AZ::Vector3 max(120.0f, 300.0f, 50.0f);
        AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(min, max);

        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                AZ::Obb result = AZ::Obb::CreateFromAabb(aabb);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, TransformMultiply)(benchmark::State& state)
    {
        AZ::Transform transform = AZ::Transform::CreateRotationY(AZ::DegToRad(90.0f));

        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                AZ::Obb result = transform * m_obb;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, Equal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                AZ::Obb result = m_obb;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathObb, IsFinite)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < numIters; ++i)
            {
                bool result = m_obb.IsFinite();
                benchmark::DoNotOptimize(result);
            }
        }
    }
}

#endif
