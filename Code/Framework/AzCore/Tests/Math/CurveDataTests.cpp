/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/CurveData.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

#include <cmath>

namespace UnitTest
{
    using AZ::CurveData;
    using TangentMode = AZ::CurveData::TangentMode;

    namespace
    {
        CurveData::Point MakePoint(float time, float value, TangentMode mode = TangentMode::Auto)
        {
            CurveData::Point point;
            point.m_time = time;
            point.m_value = value;
            point.m_inMode = mode;
            point.m_outMode = mode;
            return point;
        }
    } // namespace

    // -----------------------------------------------------------------
    // Defaults and authoring guards
    // -----------------------------------------------------------------

    TEST(CurveDataTests, SetDefaultValue_IsIdentityRamp)
    {
        CurveData curve;
        curve.SetDefaultValue();

        EXPECT_EQ(curve.GetNumPoints(), 2);
        EXPECT_NEAR(curve.Evaluate(0.0f), 0.0f, 1e-4f);
        EXPECT_NEAR(curve.Evaluate(0.5f), 0.5f, 1e-3f);
        EXPECT_NEAR(curve.Evaluate(1.0f), 1.0f, 1e-4f);
    }

    TEST(CurveDataTests, AddPoint_RejectsNegativeTime)
    {
        CurveData curve;
        AZ_TEST_START_TRACE_SUPPRESSION;
        const int64_t index = curve.AddPoint(MakePoint(-0.5f, 0.0f));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1); // the rejection emits one AZ_Error
        EXPECT_EQ(index, -1);
        EXPECT_EQ(curve.GetNumPoints(), 0);
    }

    TEST(CurveDataTests, AddPoint_AcceptsValuesOutsideUnitRange)
    {
        CurveData curve;
        EXPECT_GE(curve.AddPoint(MakePoint(0.0f, -5.0f)), 0);
        EXPECT_GE(curve.AddPoint(MakePoint(1.0f, 12.0f)), 0);
        EXPECT_EQ(curve.GetNumPoints(), 2);
        EXPECT_NEAR(curve.EvaluateTime(0.0f), -5.0f, 1e-4f);
        EXPECT_NEAR(curve.EvaluateTime(1.0f), 12.0f, 1e-4f);
    }

    TEST(CurveDataTests, AddPoint_KeepsPointsTimeSorted)
    {
        CurveData curve;
        curve.AddPoint(MakePoint(2.0f, 0.0f));
        curve.AddPoint(MakePoint(0.0f, 0.0f));
        curve.AddPoint(MakePoint(1.0f, 0.0f));

        EXPECT_NEAR(curve.GetPoint(0).m_time, 0.0f, 1e-4f);
        EXPECT_NEAR(curve.GetPoint(1).m_time, 1.0f, 1e-4f);
        EXPECT_NEAR(curve.GetPoint(2).m_time, 2.0f, 1e-4f);
    }

    // -----------------------------------------------------------------
    // Invariants
    // -----------------------------------------------------------------

    TEST(CurveDataTests, EvaluateTime_PassesThroughControlPoints)
    {
        CurveData curve({ MakePoint(0.0f, 0.0f), MakePoint(1.0f, 3.0f), MakePoint(2.5f, -1.0f) });
        for (int64_t i = 0; i < curve.GetNumPoints(); ++i)
        {
            const CurveData::Point point = curve.GetPoint(i);
            EXPECT_NEAR(curve.EvaluateTime(point.m_time), point.m_value, 1e-3f)
                << "Curve must interpolate through its own keys at index " << i;
        }
    }

    TEST(CurveDataTests, SinglePoint_EvaluatesToThatValue)
    {
        CurveData curve;
        curve.AddPoint(MakePoint(0.4f, 7.0f));
        EXPECT_NEAR(curve.Evaluate(0.0f), 7.0f, 1e-4f);
        EXPECT_NEAR(curve.Evaluate(1.0f), 7.0f, 1e-4f);
        EXPECT_NEAR(curve.EvaluateTime(99.0f), 7.0f, 1e-4f);
    }

    // -----------------------------------------------------------------
    // Tangent modes
    // -----------------------------------------------------------------

    TEST(CurveDataTests, LinearTangents_ProduceStraightLine)
    {
        CurveData curve({ MakePoint(0.0f, 0.0f, TangentMode::Linear),
                            MakePoint(1.0f, 1.0f, TangentMode::Linear) });
        EXPECT_NEAR(curve.EvaluateTime(0.25f), 0.25f, 1e-3f);
        EXPECT_NEAR(curve.EvaluateTime(0.50f), 0.50f, 1e-3f);
        EXPECT_NEAR(curve.EvaluateTime(0.75f), 0.75f, 1e-3f);
    }

    TEST(CurveDataTests, FlatTangents_EaseInOutIsSymmetricAndSlowAtEnds)
    {
        CurveData curve({ MakePoint(0.0f, 0.0f, TangentMode::Flat),
                            MakePoint(1.0f, 1.0f, TangentMode::Flat) });
        // Symmetric S-curve crosses the midpoint at 0.5.
        EXPECT_NEAR(curve.EvaluateTime(0.5f), 0.5f, 1e-3f);
        // Flat start means the early value lags behind the straight line.
        EXPECT_LT(curve.EvaluateTime(0.25f), 0.25f);
        // Flat end means the late value leads the straight line.
        EXPECT_GT(curve.EvaluateTime(0.75f), 0.75f);
    }

    TEST(CurveDataTests, ConstantTangent_HoldsLeftValue)
    {
        CurveData curve;
        curve.AddPoint(MakePoint(0.0f, 0.0f, TangentMode::Constant));
        curve.AddPoint(MakePoint(1.0f, 1.0f, TangentMode::Constant));

        EXPECT_NEAR(curve.EvaluateTime(0.0f), 0.0f, 1e-4f);
        EXPECT_NEAR(curve.EvaluateTime(0.5f), 0.0f, 1e-4f); // held
        EXPECT_NEAR(curve.EvaluateTime(0.99f), 0.0f, 1e-4f); // still held
        EXPECT_NEAR(curve.EvaluateTime(1.0f), 1.0f, 1e-4f); // snaps at the next key
    }

    // -----------------------------------------------------------------
    // Extrapolation beyond the authored range
    // -----------------------------------------------------------------

    TEST(CurveDataTests, EvaluateTime_ExtrapolatesLinearlyBeyondEnds)
    {
        // End slopes are 1 (straight line through the unit segment).
        CurveData curve({ MakePoint(0.0f, 0.0f, TangentMode::Linear),
                            MakePoint(1.0f, 1.0f, TangentMode::Linear) });
        EXPECT_NEAR(curve.EvaluateTime(2.0f), 2.0f, 1e-3f);   // forward extension
        EXPECT_NEAR(curve.EvaluateTime(-1.0f), -1.0f, 1e-3f); // backward extension
    }

    // -----------------------------------------------------------------
    // Normalized scrub vs absolute time
    // -----------------------------------------------------------------

    TEST(CurveDataTests, Evaluate_NormalizesAcrossFullTimeSpan)
    {
        // Curve spans time [0, 2]; a linear ramp to value 4.
        CurveData curve({ MakePoint(0.0f, 0.0f, TangentMode::Linear),
                            MakePoint(2.0f, 4.0f, TangentMode::Linear) });

        EXPECT_NEAR(curve.GetMinTime(), 0.0f, 1e-4f);
        EXPECT_NEAR(curve.GetMaxTime(), 2.0f, 1e-4f);

        // Normalized 0.5 maps to absolute time 1.0 -> value 2.0.
        EXPECT_NEAR(curve.Evaluate(0.5f), 2.0f, 1e-3f);
        EXPECT_NEAR(curve.Evaluate(0.5f), curve.EvaluateTime(1.0f), 1e-3f);
        // Normalized 1.0 maps to the final key.
        EXPECT_NEAR(curve.Evaluate(1.0f), 4.0f, 1e-3f);
    }

    // -----------------------------------------------------------------
    // Robustness
    // -----------------------------------------------------------------

    TEST(CurveDataTests, HeavyFreeWeights_StayFiniteAndHitEndpoints)
    {
        CurveData::Point a = MakePoint(0.0f, 0.0f, TangentMode::Free);
        a.m_outTangent = 4.0f;
        a.m_outWeight = 1.0f;
        CurveData::Point b = MakePoint(1.0f, 1.0f, TangentMode::Free);
        b.m_inTangent = -4.0f;
        b.m_inWeight = 1.0f;

        CurveData curve({ a, b });

        // Endpoints are exact regardless of arm weights.
        EXPECT_NEAR(curve.EvaluateTime(0.0f), 0.0f, 1e-3f);
        EXPECT_NEAR(curve.EvaluateTime(1.0f), 1.0f, 1e-3f);
        // Mid samples remain finite (the time solve never diverges).
        for (float t = 0.0f; t <= 1.0f; t += 0.1f)
        {
            EXPECT_TRUE(std::isfinite(curve.EvaluateTime(t)));
        }
    }

    TEST(CurveDataTests, UpdatePoint_ReordersWhenTimeCrossesNeighbour)
    {
        CurveData curve({ MakePoint(0.0f, 0.0f), MakePoint(1.0f, 1.0f), MakePoint(2.0f, 2.0f) });
        // Move the middle key past the last key.
        const int64_t newIndex = curve.UpdatePoint(1, MakePoint(3.0f, 5.0f));
        EXPECT_EQ(newIndex, 2);
        EXPECT_NEAR(curve.GetPoint(2).m_time, 3.0f, 1e-4f);
        EXPECT_NEAR(curve.GetPoint(2).m_value, 5.0f, 1e-4f);
    }

    TEST(CurveDataTests, RemovePoint_DropsTheKey)
    {
        CurveData curve({ MakePoint(0.0f, 0.0f), MakePoint(1.0f, 1.0f), MakePoint(2.0f, 2.0f) });
        curve.RemovePoint(1);
        EXPECT_EQ(curve.GetNumPoints(), 2);
        EXPECT_NEAR(curve.GetPoint(1).m_time, 2.0f, 1e-4f);
    }

    // -----------------------------------------------------------------
    // Free-arm extrapolation (broken end keys)
    // -----------------------------------------------------------------

    TEST(CurveDataTests, Extrapolation_BrokenEndKeyFollowsOutArm)
    {
        // Last key: in-arm slope 0 shapes the final segment; out-arm slope 5 is a
        // broken/free forward arm. Post-extrapolation must follow the OUT arm.
        CurveData::Point a = MakePoint(0.0f, 0.0f, TangentMode::Free);
        a.m_inTangent = 0.0f;
        a.m_outTangent = 0.0f;

        CurveData::Point b;
        b.m_time = 1.0f;
        b.m_value = 1.0f;
        b.m_inMode = TangentMode::Free;
        b.m_outMode = TangentMode::Free;
        b.m_broken = true;
        b.m_inTangent = 0.0f;  // arrival slope (in the curve)
        b.m_outTangent = 5.0f; // free forward arm (drives extrapolation)

        CurveData curve({ a, b });
        EXPECT_NEAR(curve.EvaluateTime(2.0f), 1.0f + 5.0f, 1e-3f); // follows out-arm, not in-arm
    }

    TEST(CurveDataTests, Extrapolation_UnifiedEndKeyContinuesNaturalSlope)
    {
        // Unified ends: in == out, so extrapolation continues the curve's slope.
        CurveData curve({ MakePoint(0.0f, 0.0f, TangentMode::Linear),
                          MakePoint(1.0f, 1.0f, TangentMode::Linear) });
        EXPECT_NEAR(curve.EvaluateTime(3.0f), 3.0f, 1e-3f);
    }

    // -----------------------------------------------------------------
    // Bulk point replacement
    // -----------------------------------------------------------------

    TEST(CurveDataTests, SetPoints_SortsByTimeAndDropsNegative)
    {
        CurveData curve;
        AZStd::vector<CurveData::Point> points;
        points.push_back(MakePoint(2.0f, 2.0f));
        points.push_back(MakePoint(0.0f, 0.0f));
        points.push_back(MakePoint(1.0f, 1.0f));
        curve.SetPoints(points);

        EXPECT_EQ(curve.GetNumPoints(), 3);
        EXPECT_NEAR(curve.GetPoint(0).m_time, 0.0f, 1e-4f);
        EXPECT_NEAR(curve.GetPoint(1).m_time, 1.0f, 1e-4f);
        EXPECT_NEAR(curve.GetPoint(2).m_time, 2.0f, 1e-4f);

        points.push_back(MakePoint(-1.0f, 9.0f));
        curve.SetPoints(points);
        EXPECT_EQ(curve.GetNumPoints(), 3); // negative-time point dropped
    }

    TEST(CurveDataTests, GetPoints_ExposesSortedControlPoints)
    {
        CurveData curve({ MakePoint(1.0f, 1.0f), MakePoint(0.0f, 0.0f) });
        const AZStd::vector<CurveData::Point>& points = curve.GetPoints();
        EXPECT_EQ(points.size(), 2u);
        EXPECT_NEAR(points[0].m_time, 0.0f, 1e-4f);
        EXPECT_NEAR(points[1].m_time, 1.0f, 1e-4f);
    }

    // -----------------------------------------------------------------
    // Reflection round-trip (serialization)
    // -----------------------------------------------------------------

    class CurveDataReflectionFixture : public ::UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();
            // SerializeContext's constructor invokes MathReflect internally, so
            // CurveData and CurveData::Point are already registered.
            m_context = AZStd::make_unique<AZ::SerializeContext>();
        }

        void TearDown() override
        {
            m_context.reset();
            UnitTest::LeakDetectionFixture::TearDown();
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_context;
    };

    TEST_F(CurveDataReflectionFixture, SerializeRoundTrip_PreservesAllPointFields)
    {
        // Author a point with every field exercised (notably the TangentMode
        // enums, the weights, and the broken flag), including a Free arm whose
        // authored slope/weight survives the tangent recompute.
        CurveData::Point a;
        a.m_time = 0.0f;
        a.m_value = 0.25f;
        a.m_inMode = TangentMode::Linear;
        a.m_outMode = TangentMode::Free;
        a.m_outTangent = 2.0f;
        a.m_outWeight = 0.6f;
        a.m_broken = true;

        CurveData::Point b;
        b.m_time = 1.5f;
        b.m_value = 3.0f;
        b.m_inMode = TangentMode::Constant;
        b.m_outMode = TangentMode::Auto;

        const CurveData original({ a, b });

        AZStd::vector<char> buffer;
        AZ::IO::ByteContainerStream<decltype(buffer)> stream(&buffer);
        EXPECT_TRUE(AZ::Utils::SaveObjectToStream(stream, AZ::DataStream::ST_BINARY, &original, m_context.get()));

        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        CurveData restored;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(stream, restored, m_context.get()));

        ASSERT_EQ(restored.GetNumPoints(), original.GetNumPoints());
        for (int64_t i = 0; i < original.GetNumPoints(); ++i)
        {
            const CurveData::Point e = original.GetPoint(i);
            const CurveData::Point r = restored.GetPoint(i);
            EXPECT_NEAR(r.m_time, e.m_time, 1e-4f);
            EXPECT_NEAR(r.m_value, e.m_value, 1e-4f);
            EXPECT_NEAR(r.m_inTangent, e.m_inTangent, 1e-4f);
            EXPECT_NEAR(r.m_outTangent, e.m_outTangent, 1e-4f);
            EXPECT_NEAR(r.m_inWeight, e.m_inWeight, 1e-4f);
            EXPECT_NEAR(r.m_outWeight, e.m_outWeight, 1e-4f);
            EXPECT_EQ(r.m_inMode, e.m_inMode);   // enum round-trip
            EXPECT_EQ(r.m_outMode, e.m_outMode);
            EXPECT_EQ(r.m_broken, e.m_broken);
        }

        // Behaviour is identical after a round-trip.
        EXPECT_NEAR(restored.EvaluateTime(0.75f), original.EvaluateTime(0.75f), 1e-4f);
    }
} // namespace UnitTest
