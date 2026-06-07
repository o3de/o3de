/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/CurveData.h>

#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/sort.h>

namespace AZ
{
    // =====================================================================
    // Bezier Math Helpers (file-local)
    // =====================================================================

    namespace
    {
        //! Cubic Bezier control coordinates for a single segment, laid out as
        //! separate X (time) and Y (value) channels.
        struct SegmentControls
        {
            float m_x0, m_x1, m_x2, m_x3;
            float m_y0, m_y1, m_y2, m_y3;
        };

        //! Builds the four Bezier control points for the segment [a, b].
        //!
        //! The X channel is the time axis. To guarantee the segment remains a
        //! function of time (one value per time), the two arm weights are scaled
        //! so their combined horizontal reach never exceeds the segment, which
        //! keeps the inner control points ordered (x1 <= x2) and X monotonic.
        SegmentControls BuildSegmentControls(const CurveData::Point& a, const CurveData::Point& b)
        {
            const float dt = b.m_time - a.m_time;

            float outWeight = AZ::GetClamp(a.m_outWeight, 0.0f, 1.0f);
            float inWeight = AZ::GetClamp(b.m_inWeight, 0.0f, 1.0f);
            const float weightSum = outWeight + inWeight;
            if (weightSum > 1.0f)
            {
                outWeight /= weightSum;
                inWeight /= weightSum;
            }

            SegmentControls c;
            c.m_x0 = a.m_time;
            c.m_x3 = b.m_time;
            c.m_x1 = a.m_time + outWeight * dt;
            c.m_x2 = b.m_time - inWeight * dt;

            c.m_y0 = a.m_value;
            c.m_y3 = b.m_value;
            c.m_y1 = a.m_value + a.m_outTangent * (outWeight * dt);
            c.m_y2 = b.m_value - b.m_inTangent * (inWeight * dt);
            return c;
        }

        //! Evaluates a cubic Bezier scalar at parameter u in [0,1].
        float CubicBezier(float p0, float p1, float p2, float p3, float u)
        {
            const float v = 1.0f - u;
            return (v * v * v) * p0 + (3.0f * v * v * u) * p1 + (3.0f * v * u * u) * p2 + (u * u * u) * p3;
        }

        //! Derivative of the cubic Bezier scalar with respect to u.
        float CubicBezierDerivative(float p0, float p1, float p2, float p3, float u)
        {
            const float v = 1.0f - u;
            return 3.0f * v * v * (p1 - p0) + 6.0f * v * u * (p2 - p1) + 3.0f * u * u * (p3 - p2);
        }
    } // namespace

    // =====================================================================
    // Reflection
    // =====================================================================

    void CurveData::Point::Reflect(ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Point>()
                ->Version(1)
                ->Field("time", &Point::m_time)
                ->Field("value", &Point::m_value)
                ->Field("inTangent", &Point::m_inTangent)
                ->Field("outTangent", &Point::m_outTangent)
                ->Field("inWeight", &Point::m_inWeight)
                ->Field("outWeight", &Point::m_outWeight)
                ->Field("inMode", &Point::m_inMode)
                ->Field("outMode", &Point::m_outMode)
                ->Field("broken", &Point::m_broken)
                ;
        }
    }

    void CurveData::Reflect(ReflectContext* context)
    {
        Point::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CurveData>()
                ->Version(1)
                ->Field("points", &CurveData::m_points)
                ;
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<CurveData>("CurveData")
                ->Attribute(AZ::Script::Attributes::Category, "Math/Curves")
                ->Method("Evaluate", &CurveData::Evaluate)
                ->Method("EvaluateTime", &CurveData::EvaluateTime)
                ->Method("GetNumPoints", &CurveData::GetNumPoints)
                ->Method("GetMinTime", &CurveData::GetMinTime)
                ->Method("GetMaxTime", &CurveData::GetMaxTime)
                ->Method("Clear", &CurveData::Clear)
                ->Method("SetDefaultValue", &CurveData::SetDefaultValue)
                ;
        }
    }

    // =====================================================================
    // Construction
    // =====================================================================

    CurveData::CurveData(AZStd::initializer_list<Point> points)
    {
        m_points.reserve(points.size());
        for (const auto& point : points)
        {
            if (point.m_time >= 0.0f)
            {
                auto iter = AZStd::lower_bound(m_points.begin(), m_points.end(), point.m_time,
                    [](const Point& existing, float time) { return existing.m_time < time; });
                m_points.emplace(iter, point);
            }
        }
        RecomputeDerivedTangents();
    }

    // =====================================================================
    // Authoring
    // =====================================================================

    void CurveData::SetDefaultValue()
    {
        m_points.clear();

        // Default to the identity ramp across the unit square (0,0) -> (1,1) so a
        // fresh curve is immediately visible and editable.
        Point first;
        first.m_time = 0.0f;
        first.m_value = 0.0f;

        Point last;
        last.m_time = 1.0f;
        last.m_value = 1.0f;

        m_points.emplace_back(first);
        m_points.emplace_back(last);
        RecomputeDerivedTangents();
    }

    int64_t CurveData::GetNumPoints() const
    {
        return aznumeric_cast<int64_t>(m_points.size());
    }

    CurveData::Point CurveData::GetPoint(int64_t index) const
    {
        if (index < 0 || index >= aznumeric_cast<int64_t>(m_points.size()))
        {
            AZ_Error("CurveData", false, "Index %lld does not exist in the curve!", static_cast<long long>(index));
            return Point();
        }
        return m_points[index];
    }

    int64_t CurveData::AddPoint(Point point)
    {
        if (point.m_time < 0.0f)
        {
            AZ_Error("CurveData", false, "Point time must be non-negative!");
            return -1;
        }

        auto iter = AZStd::lower_bound(m_points.begin(), m_points.end(), point.m_time,
            [](const Point& existing, float time) { return existing.m_time < time; });
        const int64_t index = aznumeric_cast<int64_t>(AZStd::distance(m_points.begin(), m_points.emplace(iter, point)));
        RecomputeDerivedTangents();
        return index;
    }

    int64_t CurveData::UpdatePoint(int64_t index, Point point)
    {
        if (index < 0 || index >= aznumeric_cast<int64_t>(m_points.size()))
        {
            AZ_Error("CurveData", false, "Index %lld does not exist in the curve!", static_cast<long long>(index));
            return -1;
        }
        if (point.m_time < 0.0f)
        {
            AZ_Error("CurveData", false, "Point time must be non-negative!");
            return -1;
        }

        m_points.erase(m_points.begin() + index);
        return AddPoint(point);
    }

    void CurveData::RemovePoint(int64_t index)
    {
        if (index < 0 || index >= aznumeric_cast<int64_t>(m_points.size()))
        {
            AZ_Error("CurveData", false, "Index %lld does not exist in the curve!", static_cast<long long>(index));
            return;
        }
        m_points.erase(m_points.begin() + index);
        RecomputeDerivedTangents();
    }

    void CurveData::Clear()
    {
        m_points.clear();
    }

    const AZStd::vector<CurveData::Point>& CurveData::GetPoints() const
    {
        return m_points;
    }

    void CurveData::SetPoints(const AZStd::vector<Point>& points)
    {
        m_points.clear();
        m_points.reserve(points.size());
        for (const Point& point : points)
        {
            if (point.m_time < 0.0f)
            {
                continue;
            }
            auto iter = AZStd::lower_bound(m_points.begin(), m_points.end(), point.m_time,
                [](const Point& existing, float time) { return existing.m_time < time; });
            m_points.emplace(iter, point);
        }
        RecomputeDerivedTangents();
    }

    // =====================================================================
    // Evaluation
    // =====================================================================

    float CurveData::Evaluate(float t01) const
    {
        if (m_points.empty())
        {
            return 0.0f;
        }
        if (m_points.size() == 1)
        {
            return m_points.front().m_value;
        }

        // Remap the normalized parameter across the full authored time span,
        // then sample in absolute time. Values outside [0,1] extrapolate.
        const float time = AZ::Lerp(m_points.front().m_time, m_points.back().m_time, t01);
        return EvaluateTime(time);
    }

    float CurveData::EvaluateTime(float time) const
    {
        if (m_points.empty())
        {
            return 0.0f;
        }
        if (m_points.size() == 1)
        {
            return m_points.front().m_value;
        }

        const Point& front = m_points.front();
        const Point& back = m_points.back();

        // Extrapolate beyond the authored range along each end's FREE arm: the
        // first key's incoming arm (which shapes nothing inside the curve) drives
        // the pre-extrapolation, and the last key's outgoing arm drives the
        // post-extrapolation. For unified tangents both arms match, so this still
        // continues the curve's natural slope; when an end key is broken, the user
        // can aim the free arm to steer the extrapolation.
        if (time < front.m_time)
        {
            // A broken first key aims its free incoming arm; an unified key
            // continues along the curve's natural departure slope (outgoing arm),
            // which is correct even for Linear/Auto ends where the free arm is
            // computed toward a non-existent neighbour.
            const float slope = front.m_broken ? front.m_inTangent : front.m_outTangent;
            return front.m_value + slope * (time - front.m_time);
        }
        if (time > back.m_time)
        {
            // A broken last key aims its free outgoing arm; an unified key
            // continues along the curve's natural arrival slope (incoming arm).
            const float slope = back.m_broken ? back.m_outTangent : back.m_inTangent;
            return back.m_value + slope * (time - back.m_time);
        }

        // Locate the segment containing the time and evaluate it.
        for (size_t i = 0; i + 1 < m_points.size(); ++i)
        {
            if (time <= m_points[i + 1].m_time)
            {
                return EvaluateSegment(m_points[i], m_points[i + 1], time);
            }
        }
        return back.m_value;
    }

    float CurveData::EvaluateSegment(const Point& start, const Point& end, float time)
    {
        if (time <= start.m_time)
        {
            return start.m_value;
        }
        if (time >= end.m_time)
        {
            return end.m_value;
        }
        // A constant outgoing arm holds the left value until the next point.
        if (start.m_outMode == TangentMode::Constant)
        {
            return start.m_value;
        }

        const SegmentControls c = BuildSegmentControls(start, end);
        const float u = SolveSegmentParameter(start, end, time);
        return CubicBezier(c.m_y0, c.m_y1, c.m_y2, c.m_y3, u);
    }

    // =====================================================================
    // Queries
    // =====================================================================

    int64_t CurveData::GetClosestPoint(float time, float value) const
    {
        size_t closestIndex = AZStd::numeric_limits<size_t>::max();
        float closestSqDistance = AZStd::numeric_limits<float>::max();
        for (size_t index = 0; index < m_points.size(); ++index)
        {
            const float deltaTime = time - m_points[index].m_time;
            const float deltaValue = value - m_points[index].m_value;
            const float sqDistance = deltaTime * deltaTime + deltaValue * deltaValue;
            if (sqDistance < closestSqDistance)
            {
                closestIndex = index;
                closestSqDistance = sqDistance;
            }
        }
        if (closestIndex == AZStd::numeric_limits<size_t>::max())
        {
            return -1;
        }
        return aznumeric_cast<int64_t>(closestIndex);
    }

    float CurveData::GetMinTime() const
    {
        return m_points.empty() ? 0.0f : m_points.front().m_time;
    }

    float CurveData::GetMaxTime() const
    {
        return m_points.empty() ? 0.0f : m_points.back().m_time;
    }

    // =====================================================================
    // Internals
    // =====================================================================

    void CurveData::RecomputeDerivedTangents()
    {
        const int64_t count = aznumeric_cast<int64_t>(m_points.size());
        for (int64_t i = 0; i < count; ++i)
        {
            Point& point = m_points[i];

            // Slope aiming straight at each neighbour (used by Linear, and as the
            // one-sided fallback for Auto at the curve ends).
            float linearIn = 0.0f;
            if (i > 0)
            {
                const Point& prev = m_points[i - 1];
                const float dtPrev = point.m_time - prev.m_time;
                linearIn = (dtPrev > 0.0f) ? (point.m_value - prev.m_value) / dtPrev : 0.0f;
            }

            float linearOut = 0.0f;
            if (i + 1 < count)
            {
                const Point& next = m_points[i + 1];
                const float dtNext = next.m_time - point.m_time;
                linearOut = (dtNext > 0.0f) ? (next.m_value - point.m_value) / dtNext : 0.0f;
            }

            // Auto slope: central difference across both neighbours, one-sided at
            // the ends.
            float autoSlope;
            if (i > 0 && i + 1 < count)
            {
                const Point& prev = m_points[i - 1];
                const Point& next = m_points[i + 1];
                const float span = next.m_time - prev.m_time;
                autoSlope = (span > 0.0f) ? (next.m_value - prev.m_value) / span : 0.0f;
            }
            else if (i + 1 < count)
            {
                autoSlope = linearOut;
            }
            else
            {
                autoSlope = linearIn;
            }

            auto resolveSlope = [&](TangentMode mode, float linearSlope, float currentSlope) -> float
            {
                switch (mode)
                {
                case TangentMode::Flat:
                    return 0.0f;
                case TangentMode::Linear:
                    return linearSlope;
                case TangentMode::Auto:
                    return autoSlope;
                case TangentMode::Constant:
                    return 0.0f; // ignored by evaluation, kept tidy.
                case TangentMode::Free:
                default:
                    return currentSlope; // authored, leave untouched.
                }
            };

            point.m_inTangent = resolveSlope(point.m_inMode, linearIn, point.m_inTangent);
            point.m_outTangent = resolveSlope(point.m_outMode, linearOut, point.m_outTangent);

            // Derived arms use the default weight; Free arms keep their authored
            // length.
            if (point.m_inMode != TangentMode::Free)
            {
                point.m_inWeight = DefaultWeight;
            }
            if (point.m_outMode != TangentMode::Free)
            {
                point.m_outWeight = DefaultWeight;
            }
        }
    }

    float CurveData::SolveSegmentParameter(const Point& start, const Point& end, float time)
    {
        const SegmentControls c = BuildSegmentControls(start, end);

        const float span = c.m_x3 - c.m_x0;
        if (span <= 0.0f)
        {
            return 0.0f;
        }

        const float target = AZ::GetClamp(time, c.m_x0, c.m_x3);

        // Safe Newton-Raphson: a Newton step when it stays inside the bracket,
        // bisection otherwise. X is monotonic by construction (see
        // BuildSegmentControls), so the bracket is always valid.
        constexpr int MaxIterations = 24;
        constexpr float Tolerance = 1e-6f;

        float lo = 0.0f;
        float hi = 1.0f;
        float u = (target - c.m_x0) / span; // linear first guess

        for (int iteration = 0; iteration < MaxIterations; ++iteration)
        {
            const float x = CubicBezier(c.m_x0, c.m_x1, c.m_x2, c.m_x3, u) - target;
            if (AZ::GetAbs(x) < Tolerance)
            {
                break;
            }

            // X is non-decreasing: if the sampled X overshoots the target the
            // root lies to the left, so tighten the upper bound.
            if (x > 0.0f)
            {
                hi = u;
            }
            else
            {
                lo = u;
            }

            const float dx = CubicBezierDerivative(c.m_x0, c.m_x1, c.m_x2, c.m_x3, u);
            float next = (AZ::GetAbs(dx) > 1e-8f) ? (u - x / dx) : (0.5f * (lo + hi));
            if (next <= lo || next >= hi)
            {
                next = 0.5f * (lo + hi);
            }
            u = next;
        }

        return AZ::GetClamp(u, 0.0f, 1.0f);
    }
}
