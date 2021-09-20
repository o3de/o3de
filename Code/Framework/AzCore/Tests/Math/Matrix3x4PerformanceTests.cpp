/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <random>
#include <benchmark/benchmark.h>

namespace Benchmark
{
    class BM_MathMatrix3x4
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_testDataArray.resize(1000);

            const unsigned int seed = 1;
            std::mt19937_64 rng(seed);
            std::uniform_real_distribution<float> distFloat;
            std::uniform_int_distribution<AZ::u32> distInt(0, std::numeric_limits<AZ::u32>::max());

            std::generate(m_testDataArray.begin(), m_testDataArray.end(), [&distFloat, &distInt, &rng]()
            {
                TestData testData;
                const AZ::Quaternion q1 = AZ::Quaternion(distFloat(rng), distFloat(rng), distFloat(rng), distFloat(rng)).GetNormalized();
                testData.m1 = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(q1, AZ::Vector3(distFloat(rng), distFloat(rng), distFloat(rng)));
                const AZ::Quaternion q2 = AZ::Quaternion(distFloat(rng), distFloat(rng), distFloat(rng), distFloat(rng)).GetNormalized();
                testData.m2 = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(q2, AZ::Vector3(distFloat(rng), distFloat(rng), distFloat(rng)));
                testData.m3x3 = AZ::Matrix3x3::CreateFromQuaternion(q1);
                testData.q = AZ::Quaternion(distFloat(rng), distFloat(rng), distFloat(rng), distFloat(rng)).GetNormalized();
                testData.t = AZ::Transform::CreateFromQuaternionAndTranslation(q1, AZ::Vector3(distFloat(rng), distFloat(rng), distFloat(rng)));
                for (int i = 0; i < 16; ++i)
                {
                    testData.value[i] = distFloat(rng);
                }
                testData.row = distInt(rng) % 3;
                testData.col = distInt(rng) % 4;
                for (int row = 0; row < 3; ++row)
                {
                    testData.rows[row] = AZ::Vector4(distFloat(rng), distFloat(rng), distFloat(rng), distFloat(rng));
                }
                for (int col = 0; col < 4; ++col)
                {
                    testData.cols[col] = AZ::Vector3(distFloat(rng), distFloat(rng), distFloat(rng));
                }
                return testData;
            });
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

        struct TestData
        {
            AZ::Matrix3x4 m1;
            AZ::Matrix3x4 m2;
            AZ::Matrix3x3 m3x3;
            AZ::Quaternion q;
            AZ::Transform t;
            AZ::Vector4 rows[3];
            AZ::Vector3 cols[4];
            float value[16];
            AZ::u32 row;
            AZ::u32 col;
        };

        std::vector<TestData> m_testDataArray;
    };

    BENCHMARK_F(BM_MathMatrix3x4, CreateIdentity)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateIdentity();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateZero)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateZero();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromValue)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromValue(testData.value[0]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromRowMajorFloat12)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromRowMajorFloat12(testData.value);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromRows)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromRows(testData.rows[0], testData.rows[1], testData.rows[2]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromColumnMajorFloat12)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromColumnMajorFloat12(testData.value);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromColumns)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromColumns(testData.cols[0], testData.cols[1], testData.cols[2], testData.cols[3]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromColumnMajorFloat16)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromColumnMajorFloat16(testData.value);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateRotationX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateRotationX(testData.value[0]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateRotationY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateRotationY(testData.value[0]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateRotationZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateRotationZ(testData.value[0]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromQuaternion)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromQuaternion(testData.q);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromQuaternionAndTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(testData.q, testData.cols[3]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromMatrix3x3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromMatrix3x3(testData.m3x3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromMatrix3x3AndTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(testData.m3x3, testData.cols[3]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromTransform)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateFromTransform(testData.t);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateScale(testData.cols[0]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateFromTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateTranslation(testData.cols[3]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, CreateLookAt)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = AZ::Matrix3x4::CreateLookAt(testData.cols[0], testData.cols[1]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, StoreToRowMajorFloat12)(benchmark::State& state)
    {
        float testFloats[12];

        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.StoreToRowMajorFloat12(testFloats);
                benchmark::DoNotOptimize(testFloats);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, StoreToColumnMajorFloat12)(benchmark::State& state)
    {
        float testFloats[12];

        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.StoreToColumnMajorFloat12(testFloats);
                benchmark::DoNotOptimize(testFloats);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, StoreToColumnMajorFloat16)(benchmark::State& state)
    {
        float testFloats[16];

        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.StoreToColumnMajorFloat16(testFloats);
                benchmark::DoNotOptimize(testFloats);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetElement)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                float result = testData.m1.GetElement(testData.row, testData.col);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetElement)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetElement(testData.row, testData.col, testData.value[0]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetRow)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                const auto& result = testData.m1.GetRow(testData.row);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetRowAsVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                const auto& result = testData.m1.GetRowAsVector3(testData.row);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetRowWithFloats)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetRow(testData.row, testData.value[0], testData.value[1], testData.value[2], testData.value[3]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetRowWithVector3AndFloat)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetRow(testData.row, testData.cols[0], testData.value[0]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetRowWithVector4)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetRow(testData.row, testData.rows[0]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetRows)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector4 rows[3];
                testData.m1.GetRows(&rows[0], &rows[1], &rows[2]);
                benchmark::DoNotOptimize(rows[0]);
                benchmark::DoNotOptimize(rows[1]);
                benchmark::DoNotOptimize(rows[2]);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetRows)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetRows(testData.rows[0], testData.rows[1], testData.rows[2]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetColumn)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                auto result = testData.m1.GetColumn(testData.col);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetColumnWithFloats)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetColumn(testData.col, testData.value[0], testData.value[1], testData.value[2]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetColumnWithVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetColumn(testData.col, testData.cols[0]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetColumns)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 cols[4];
                testData.m1.GetColumns(&cols[0], &cols[1], &cols[2], &cols[3]);
                benchmark::DoNotOptimize(cols[0]);
                benchmark::DoNotOptimize(cols[1]);
                benchmark::DoNotOptimize(cols[2]);
                benchmark::DoNotOptimize(cols[3]);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetColumns)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix;
                testMatrix.SetColumns(testData.cols[0], testData.cols[1], testData.cols[2], testData.cols[3]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetBasisX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                auto result = testData.m1.GetBasisX();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetBasisXWithFloats)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetBasisX(testData.value[0], testData.value[1], testData.value[2]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetBasisXWithVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetBasisX(testData.cols[0]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetBasisY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                auto result = testData.m1.GetBasisY();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetBasisYWithFloats)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetBasisY(testData.value[0], testData.value[1], testData.value[2]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetBasisYWithVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetBasisY(testData.cols[1]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetBasisZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                auto result = testData.m1.GetBasisZ();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetBasisZWithFloats)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetBasisZ(testData.value[0], testData.value[1], testData.value[2]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetBasisZWithVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetBasisZ(testData.cols[2]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.GetTranslation();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetTranslationWithFloats)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m2;
                testMatrix.SetTranslation(testData.value[0], testData.value[1], testData.value[2]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetTranslationWithVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m2;
                testMatrix.SetTranslation(testData.cols[0]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetBasisAndTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 basis[4];
                testData.m1.GetBasisAndTranslation(&basis[0], &basis[1], &basis[2], &basis[3]);
                benchmark::DoNotOptimize(basis[0]);
                benchmark::DoNotOptimize(basis[1]);
                benchmark::DoNotOptimize(basis[2]);
                benchmark::DoNotOptimize(basis[3]);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetBasisAndTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetBasisAndTranslation(testData.cols[0], testData.cols[1], testData.cols[2], testData.cols[3]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, OperatorMultiplyMatrix3x4)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = testData.m1 * testData.m2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, OperatorMultiplyEqualsMatrix3x4)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m2;
                testMatrix *= testData.m1;
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, TransformPointVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1 * testData.cols[0];
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, TransformVectorVector4)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector4 result = testData.m1 * testData.rows[0];
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, Multiply3x3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.Multiply3x3(testData.cols[0]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetTranspose)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = testData.m1.GetTranspose();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, Transpose)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.Transpose();
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetTranspose3x3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = testData.m1.GetTranspose3x3();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, Transpose3x3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.Transpose3x3();
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetInverseFull)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = testData.m1.GetInverseFull();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, InvertFull)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m2;
                testMatrix.InvertFull();
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetInverseFast)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = testData.m1.GetInverseFast();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, InvertFast)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m2;
                testMatrix.InvertFast();
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, RetrieveScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.RetrieveScale();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, ExtractScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                AZ::Vector3 result = testMatrix.ExtractScale();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, MultiplyByScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.MultiplyByScale(testData.cols[0]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, IsOrthogonal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = testData.m1.IsOrthogonal();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetOrthogonalized)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 result = testData.m1.GetOrthogonalized();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, Orthogonalize)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.Orthogonalize();
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, IsCloseExactAndDifferent)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = testData.m1.IsClose(testData.m1);
                benchmark::DoNotOptimize(result);

                result = testData.m2.IsClose(testData.m1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, OperatorEqualEqual)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = (testData.m1 == testData.m2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, OperatorNotEqual)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = (testData.m1 != testData.m2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetEulerDegrees)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.GetEulerDegrees();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetEulerRadians)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.GetEulerRadians();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetFromEulerDegrees)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix;
                testMatrix.SetFromEulerDegrees(testData.cols[0]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetFromEulerRadians)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix;
                testMatrix.SetFromEulerRadians(testData.cols[0]);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, SetRotationPartFromQuaternion)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x4 testMatrix = testData.m1;
                testMatrix.SetRotationPartFromQuaternion(testData.q);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, GetDeterminant3x3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                float result = testData.m1.GetDeterminant3x3();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x4, IsFinite)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = testData.m1.IsFinite();
                benchmark::DoNotOptimize(result);
            }
        }
    }
} // namespace Benchmark

#endif
