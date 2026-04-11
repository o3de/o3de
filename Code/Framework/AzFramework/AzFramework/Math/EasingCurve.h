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

        void SetDefaultValue();
        size_t GetNumPoints() const;
        Point& GetPoint(size_t index);
        void UpdatePoint(size_t index, Point point);
        void AddPoint(Point point);
        void RemovePoint(size_t index);
        void Clear();
        float Evaluate(float time)   const;
        Point& GetClosetPoint(float time, float value);
        static float Interpolate(const Point& start, const Point& end, float time);

    private:
        void SortPoints();
        AZStd::vector<Point> m_points;
    };
}

