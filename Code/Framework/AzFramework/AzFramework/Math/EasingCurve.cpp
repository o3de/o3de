/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/sort.h>
#include <AzFramework/Math/Easing.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Math/EasingCurve.h>

namespace AzFramework
{
    EasingCurve::EasingCurve()
    {
        Clear();
        Point first;
        Point last;
        first.m_time = 0.0f;
        first.m_value = 1.0f;
        last.m_time = 1.0f;
        last.m_value = 1.0f;
        m_points.emplace_back(first);
        m_points.emplace_back(last);
    }

    void EasingCurve::Point::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<Point>()
                ->Version(1)
                ->Field("time", &Point::m_time)
                ->Field("value", &Point::m_value);
        }
    }

    void EasingCurve::Reflect(AZ::ReflectContext* context)
    {
        Point::Reflect(context);
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EasingCurve>()
                ->Version(1)
                ->Field("points", &EasingCurve::m_points);
        }
    }

    size_t EasingCurve::GetNumPoints() const
    {
        return m_points.size();
    }
    
    EasingCurve::Point& EasingCurve::GetPoint(size_t index)
    {
        AZ_Assert(index < m_points.size(), "Point index is out of bound of the curve!");
        return m_points[index];
    }

    void EasingCurve::SetPoint(size_t index, EasingCurve::Point point)
    {
        AZ_Assert(index < m_points.size(), "Point index is out of bound of the curve!");
        m_points[index] = point;
    }
    
    void EasingCurve::AddPoint(EasingCurve::Point point)
    {
        m_points.emplace_back(point);
        SortPoints();
    }
    
    void EasingCurve::RemovePoint(size_t index)
    {
        AZ_Assert(index < m_points.size(), "Point index is out of bound of the curve!");
        m_points.erase(m_points.begin() + index);
    }

    void EasingCurve::Clear()
    {
        m_points.clear();
    }

    float EasingCurve::Evaluate(float time) const
    {
        AZ_Assert(time >= 0 && time <= 1, "time is out of range between 0 and 1");
        for (int i = 0; i < m_points.size(); i++)
        {
            if (m_points[i].m_time == time)
            {
                return m_points[i].m_value;
            }
            else if (m_points[i].m_time > time)
            {
                return Interpolate(m_points[i-1], m_points[i], time);
            }
        }
        return 0.0f;
    }

    void EasingCurve::SortPoints()
    {
        AZStd::sort(m_points.begin(), m_points.end(),
            [](Point& left, Point& right) {
                return left.m_time < right.m_time;
            });
    }

    float EasingCurve::Interpolate(const Point& start, const Point& end, float time)
    {
        EasingMethod easeMethod = EasingMethod::Linear;
        EasingType easeType = EasingType::In;
        float timeActive = time - start.m_time;
        float duration = end.m_time - start.m_time;

        switch(end.m_interpMode)
        {
        case PointInterpolationMode::STEP:  // this is not "easing" at all
            return start.m_value;
        case PointInterpolationMode::LINEAR:
            easeMethod = EasingMethod::Linear;
            break;
        case PointInterpolationMode::CUBIC_IN:
            easeMethod = EasingMethod::Cubic;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::CUBIC_OUT:
            easeMethod = EasingMethod::Cubic;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::SINE_IN:
            easeMethod = EasingMethod::Sine;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::SINE_OUT:
            easeMethod = EasingMethod::Cubic;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::CIRCLE_IN:
            easeMethod = EasingMethod::Circ;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::CIRCLE_OUT:
            easeMethod = EasingMethod::Circ;
            easeType = EasingType::Out;
            break;
        default:
            easeMethod = EasingMethod::Linear;
        }
        
        return EasingEquations::GetEasingResult(easeMethod, easeType, timeActive, duration, start.m_value, end.m_value);
    }

    EasingCurve::Point& EasingCurve::GetClosetPoint(float time, float value)
    {
        size_t min_index = AZStd::numeric_limits<size_t>::max();
        float min_sqdistance = AZStd::numeric_limits<float>::max();
        for (size_t index = 0; index < m_points.size(); index++)
        {
            float delta_time = time - m_points[index].m_time;
            float delta_value = value - m_points[index].m_value;
            float sqdistance = delta_time * delta_time + delta_value * delta_value;
            if (sqdistance < min_sqdistance)
            {
                min_index = index;
                min_sqdistance = sqdistance;
            }
        }
        return m_points[min_index];
    }
}

