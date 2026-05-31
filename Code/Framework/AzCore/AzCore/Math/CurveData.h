/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;

    //! A 1D curve: a time-ordered set of control points connected by weighted
    //! cubic Bezier segments. Each point owns an incoming and an outgoing tangent
    //! arm (slope + weight) that can be driven automatically or authored by hand,
    //! and the two arms can be unified (mirrored slope) or broken (edited
    //! independently).
    //!
    //! Time is the curve's X axis and is non-negative and strictly increasing.
    //! Value is the Y axis and is unbounded (positive or negative). Sampling
    //! outside the authored time range extrapolates linearly along the end
    //! tangents rather than clamping.
    class AZCORE_API CurveData
    {
    public:
        // =================================================================
        // Tangent Types
        // =================================================================

        //! How a single tangent arm derives its slope. Free and Constant carry
        //! authored data; the rest are recomputed from neighbouring points.
        enum class TangentMode : AZ::u8
        {
            Free = 0,   //!< User-authored slope and weight (draggable handle).
            Flat,       //!< Zero slope (horizontal arm).
            Linear,     //!< Aims straight at the adjacent point.
            Constant,   //!< Step: holds the left point's value across the segment (out arm only).
            Auto        //!< Smoothed automatic slope (Catmull-Rom style).
        };

        //! A single control point on the curve.
        struct AZCORE_API Point
        {
            AZ_CLASS_ALLOCATOR(Point, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(Point, "{6D2A8F31-5C49-4E7B-91A2-3F8C0B6E4D17}");

            static void Reflect(ReflectContext* context);

            float       m_time       = 0.0f;            //!< X position, >= 0, strictly increasing across the curve.
            float       m_value      = 0.0f;            //!< Y position, unbounded.
            float       m_inTangent  = 0.0f;            //!< Slope (dValue/dTime) of the incoming arm.
            float       m_outTangent = 0.0f;            //!< Slope (dValue/dTime) of the outgoing arm.
            float       m_inWeight   = DefaultWeight;   //!< Incoming arm length as a fraction [0,1] of the segment.
            float       m_outWeight  = DefaultWeight;   //!< Outgoing arm length as a fraction [0,1] of the segment.
            TangentMode m_inMode     = TangentMode::Auto;
            TangentMode m_outMode    = TangentMode::Auto;
            bool        m_broken     = false;           //!< false: arms share one slope; true: arms are independent.
        };

        //! Default arm length as a fraction of the segment (one third gives an
        //! even, well-behaved cubic Bezier for unit-slope handles).
        static constexpr float DefaultWeight = 1.0f / 3.0f;

        AZ_CLASS_ALLOCATOR(CurveData, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CurveData, "{1B9E4C7A-2F86-4D53-A0E1-9C3B7D5F2A48}");
        static void Reflect(ReflectContext* context);

        CurveData() = default;
        ~CurveData() = default;
        CurveData(const CurveData&) = default;
        CurveData(CurveData&&) = default;
        CurveData& operator=(const CurveData&) = default;
        CurveData& operator=(CurveData&&) = default;

        //! Builds a curve from a list of points, sorting them by time.
        CurveData(AZStd::initializer_list<Point> points);

        // =================================================================
        // Authoring
        // =================================================================

        //! Resets the curve to the canonical default: a flat line at value 1.0
        //! from time 0 to time 1.
        void SetDefaultValue();

        //! Number of control points.
        int64_t GetNumPoints() const;

        //! Returns a copy of the point at the given index, or a default-
        //! constructed point if the index is out of range.
        Point GetPoint(int64_t index) const;

        //! Inserts a point at its time-sorted position. Negative time is rejected.
        //! @return the index the point was inserted at, or -1 on rejection.
        int64_t AddPoint(Point point);

        //! Replaces the point at the given index. The point may move to a new
        //! index if its time changed relative to its neighbours.
        //! @return the new index of the updated point, or -1 on rejection.
        int64_t UpdatePoint(int64_t index, Point point);

        //! Removes the point at the given index.
        void RemovePoint(int64_t index);

        //! Removes all points.
        void Clear();

        // =================================================================
        // Evaluation
        // =================================================================

        //! Samples the curve using a normalized parameter that scrubs the whole
        //! authored time span: t01 = 0 maps to the first point's time, t01 = 1 to
        //! the last point's time. Values outside [0,1] extrapolate.
        float Evaluate(float t01) const;

        //! Samples the curve at an absolute authored time. Times beyond the
        //! authored range extrapolate linearly along the nearest end tangent.
        float EvaluateTime(float time) const;

        // =================================================================
        // Queries
        // =================================================================

        //! Index of the point closest (in time/value space) to the given sample,
        //! or -1 when the curve is empty.
        int64_t GetClosestPoint(float time, float value) const;

        //! Time of the first point (0 when empty).
        float GetMinTime() const;

        //! Time of the last point (0 when empty).
        float GetMaxTime() const;

        //! Evaluates a single Bezier segment between two adjacent points at an
        //! absolute time in [start.m_time, end.m_time]. Exposed for editor curve
        //! tessellation.
        static float EvaluateSegment(const Point& start, const Point& end, float time);

        //! Read-only access to the raw control points (time-sorted). Useful for
        //! editor group operations that snapshot the whole curve.
        const AZStd::vector<Point>& GetPoints() const;

        //! Replaces all control points at once (negative-time points dropped,
        //! result time-sorted, derived tangents recomputed). Lets the editor
        //! apply a group move/scale atomically without per-point churn.
        void SetPoints(const AZStd::vector<Point>& points);

    private:
        // =================================================================
        // Internals
        // =================================================================

        //! Recomputes the slope and weight of every Auto/Flat/Linear/Constant arm
        //! from neighbouring points. Free arms are left untouched. Called after
        //! any structural edit.
        void RecomputeDerivedTangents();

        //! Solves the Bezier parameter u in [0,1] whose X (time) coordinate maps
        //! to the requested absolute time along the segment [start, end].
        static float SolveSegmentParameter(const Point& start, const Point& end, float time);

        AZStd::vector<Point> m_points;
    };
}
