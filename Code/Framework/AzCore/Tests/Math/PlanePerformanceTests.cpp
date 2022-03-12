/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/Math/Plane.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <random>

namespace Benchmark
{
    class BM_MathPlane
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                m_normal = AZ::Vector3(unif(rng), unif(rng), unif(rng));
                m_normal.Normalize();
                m_normals.push_back(m_normal);

                m_point = AZ::Vector3(unif(rng), unif(rng), unif(rng));
                m_points.push_back(m_point);

                m_distance = unif(rng);
                m_dists.push_back(m_distance);

                // set these differently so they don't overlap with same values as other vectors
                m_normal = AZ::Vector3(unif(rng), unif(rng), unif(rng));
                m_normal.Normalize();
                m_distance = unif(rng);
                m_plane = AZ::Plane::CreateFromNormalAndDistance(m_normal, m_distance);
                m_planes.push_back(m_plane);
            }
        }
    public:
        BM_MathPlane()
        {
            const unsigned int seed = 1;
            rng = std::mt19937_64(seed);
        }

        void SetUp(const benchmark::State&) override
        {
            internalSetUp();
        }
        void SetUp(benchmark::State&) override
        {
            internalSetUp();
        }

        AZ::Plane m_plane;
        AZ::Vector3 m_normal;
        AZ::Vector3 m_point;
        float m_distance;
        const int m_numIters = 1000;

        //rng
        std::mt19937_64 rng;
        std::uniform_real_distribution<float> unif;
        std::vector<AZ::Vector3> m_normals;
        std::vector<AZ::Vector3> m_points;
        std::vector<float> m_dists;
        std::vector<AZ::Plane> m_planes;
    };

    BENCHMARK_F(BM_MathPlane, CreateFromNormalAndDistance)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                AZ::Plane result = AZ::Plane::CreateFromNormalAndDistance(m_normals[i], m_dists[i]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, GetDistance)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                float result = m_planes[i].GetDistance();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, GetNormal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                AZ::Vector3 result = m_planes[i].GetNormal();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, CreateFromNormalAndPoint)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                AZ::Plane result = AZ::Plane::CreateFromNormalAndPoint(m_normals[i], m_points[i]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, CreateFromCoefficients)(benchmark::State& state)
    {
        AZStd::vector<float> coeff;
        const int numCoeffPerCall = 4;
        for (int i = 0; i < m_numIters * numCoeffPerCall; ++i)
        {
            coeff.push_back(unif(rng));
        }

        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                int index = i * numCoeffPerCall;
                AZ::Plane result = AZ::Plane::CreateFromCoefficients(coeff[index], coeff[index + 1], coeff[index + 2], coeff[index + 3]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, SetCoefficients)(benchmark::State& state)
    {
        AZStd::vector<float> coeff;
        const int numCoeffPerCall = 4;
        for (int i = 0; i < m_numIters * numCoeffPerCall; ++i)
        {
            coeff.push_back(unif(rng));
        }

        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                int index = i * numCoeffPerCall;
                m_planes[i].Set(coeff[index], coeff[index + 1], coeff[index + 2], coeff[index + 3]);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, SetVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                AZ::Plane result;
                result.Set(m_normals[i], m_dists[i]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, SetVector4)(benchmark::State& state)
    {
        AZStd::vector<AZ::Vector4> vecs;
        for (int i = 0; i < m_numIters; ++i)
        {
            vecs.push_back(AZ::Vector4(unif(rng), unif(rng), unif(rng), unif(rng)));
        }

        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                AZ::Plane result;
                result.Set(vecs[i]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, SetNormal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                AZ::Plane result = m_planes[i];
                result.SetNormal(m_normals[i]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, SetDistance)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                AZ::Plane result = m_planes[i];
                result.SetDistance(m_dists[i]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, GetPlaneEquationCoefficients)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                AZ::Vector4 result = m_planes[i].GetPlaneEquationCoefficients();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, GetTransform)(benchmark::State& state)
    {
        AZStd::vector<AZ::Transform> trans;
        for (int i = 0; i < m_numIters; ++i)
        {
            trans.push_back(AZ::Transform::CreateRotationY(AZ::DegToRad(unif(rng))));
        }

        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                AZ::Plane result = m_planes[i].GetTransform(trans[i]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, ApplyTransform)(benchmark::State& state)
    {
        AZStd::vector<AZ::Transform> trans;
        for (int i = 0; i < m_numIters; ++i)
        {
            trans.push_back(AZ::Transform::CreateRotationY(AZ::DegToRad(unif(rng))));
        }

        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                m_planes[i].ApplyTransform(trans[i]);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, GetPointDist)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                float result = m_planes[i].GetPointDist(m_points[i]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, GetProjected)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                AZ::Vector3 result = m_planes[i].GetProjected(m_points[i]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, CastRay)(benchmark::State& state)
    {
        AZ::Vector3 rayResult;

        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                bool result = m_plane.CastRay(m_points[i], m_normals[i], rayResult);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, CastRayTime)(benchmark::State& state)
    {
        float rayResult;

        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                bool result = m_plane.CastRay(m_points[i], m_normals[i], rayResult);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, IntersectSegment)(benchmark::State& state)
    {
        AZ::Vector3 rayResult;

        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters - 1; ++i)
            {
                bool result = m_plane.IntersectSegment(m_points[i], m_points[i + 1], rayResult);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, IntersectSegmentTime)(benchmark::State& state)
    {
        float rayResult;

        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters - 1; ++i)
            {
                bool result = m_plane.IntersectSegment(m_points[i], m_points[i + 1], rayResult);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathPlane, IsFinite)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (int i = 0; i < m_numIters; ++i)
            {
                bool result = m_planes[i].IsFinite();
                benchmark::DoNotOptimize(result);
            }
        }
    }
}

#endif
