/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzFramework/AzFrameworkAPI.h>

namespace AzFramework
{
    //! \brief An easing curve is a collection of points that define a curve.
    //! The points are arranged by ascending time
    class AZF_API EasingCurve
    {
    public:
        enum class PointInterpolationMode: AZ::u8
        {
            LINEAR = 0,
            STEP,
            CUBIC_IN,
            CUBIC_OUT,
            SINE_IN,
            SINE_OUT,
            CIRCLE_IN,
            CIRCLE_OUT
        };

        //! \brief A single point in the easing curve.
        //! Both m_time and m_value are in the range [0, 1].
        struct Point
        {
            AZ_CLASS_ALLOCATOR(Point, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(Point, "{28F834C3-DA0C-4E0D-8D4E-BEDE1CE9D8FF}");

            static void Reflect(AZ::ReflectContext* context);
            float m_time = 0.0f;
            float m_value = 0.0f;
            PointInterpolationMode m_interpMode = PointInterpolationMode::LINEAR;
        };

        AZ_CLASS_ALLOCATOR(EasingCurve, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EasingCurve, "{D4B7E91C-DD02-423E-B8BC-699E0DFC249B}");
        static void Reflect(AZ::ReflectContext* context);

        EasingCurve() = default;
        ~EasingCurve() = default;
        EasingCurve(const EasingCurve&) = default;
        EasingCurve(EasingCurve&&) = default;
        EasingCurve& operator=(const EasingCurve&) = default;
        EasingCurve& operator=(EasingCurve&&) = default;

        //! \brief Construct an easing curve from a list of points.
        //!
        //! This constructor will add all points in the given list to the easing curve and then sort them.
        EasingCurve(AZStd::initializer_list<Point> points);

        //! Sets the default value for the easing curve.
        void SetDefaultValue();

        //! Returns the number of points in the easing curve.
        int64_t GetNumPoints() const;

        //! Returns the point at the given index.
        //! @param index The index of the point to retrieve.
        //! @return The point at the given index.
        Point GetPoint(int64_t index);

        //! @brief Updates the point at the given index with the provided point data.
        //! (The point's index might be changed if it no longer fits in the same position after the update,
        //! but the point will still be updated with the new data)
        //! @param index The index of the point to be updated
        //! @param point The updated point data to replace the existing point at the specified index
        int64_t UpdatePoint(int64_t index, Point point);

        //! Adds a new point to the easing curve.
        //! @param point The point to be added to the easing curve.
        int64_t AddPoint(Point point);

        //! Removes the point at the given index from the easing curve.
        //! @param index The index of the point to be removed.
        void RemovePoint(int64_t index);

        //! Removes all points from the easing curve.
        void Clear();

        //! Evaluates the easing curve at the given time and returns the corresponding value.
        //! @param time The time at which to evaluate the easing curve.
        //! @return The value of the easing curve at the given time.
        float Evaluate(float time) const;

        //! Finds the point in the curve that is closest to the given time and value.
        //! @param time The time to compare against the points in the curve.
        //! @param value The value to compare against the points in the curve.
        //! @return A reference to the point in the curve that is closest to the given time and value.
        int64_t GetClosetPoint(float time, float value);

        //! Interpolates between the start and end points based on the given time and the interpolation mode
        //! of the end point.
        //! @param start The start point of the interpolation.
        //! @param end The end point of the interpolation.
        //! @param time The time at which to interpolate between the start and end points.
        //! @return The interpolated value between the start and end points at the given time.
        static float Interpolate(const Point& start, const Point& end, float time);

    private:
        AZStd::vector<Point> m_points;
    };
}

