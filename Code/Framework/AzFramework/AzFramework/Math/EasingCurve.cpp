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
    void EasingCurve::SetDefaultValue()
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
        m_extrapMode = ExtrapolationMode::CONSTANT;
    }

    void EasingCurve::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<Point>()
                ->Version(2)
                ->Field("time", &Point::m_time)
                ->Field("value", &Point::m_value)
                ->Field("interpMode", &Point::m_interpMode)
                ;
            serialize->Class<EasingCurve>()
                ->Version(2)
                ->Field("points", &EasingCurve::m_points)
                ->Field("extrapMode", &EasingCurve::m_extrapMode)
                ;
        }
    }

    EasingCurve::EasingCurve(AZStd::initializer_list<Point> points)
    {
        for (const auto& point : points)
        {
            AddPoint(point);
        }
    }

    int64_t EasingCurve::GetNumPoints() const
    {
        return aznumeric_cast<int64_t>(m_points.size());
    }
    
    EasingCurve::Point EasingCurve::GetPoint(int64_t index)
    {
        if (index < 0 || index >= aznumeric_cast<int64_t>(m_points.size()))
        {
            AZ_Error("EasingCurve", false, "Index %d does not exist in the curve!", index);
            return Point();
        }
        return m_points[index];
    }

    int64_t EasingCurve::UpdatePoint(int64_t index, EasingCurve::Point point)
    {
        if (index < 0 || index >= aznumeric_cast<int64_t>(m_points.size()))
        {
            AZ_Error("EasingCurve", false, "Index %d does not exist in the curve!", index);
            return -1;
        }

        if (point.m_time < 0.0f || point.m_time > 1.0f)
        {
            AZ_Error("EasingCurve", false, "Point time must be between 0 and 1!");
            return  -1;
        }

        if (point.m_value < 0.0f || point.m_value > 1.0f)
        {
            AZ_Error("EasingCurve", false, "Point value must be between 0 and 1!");
            return -1;
        }

        m_points.erase(m_points.begin() + index);
        return AddPoint(point);
    }
    
    int64_t EasingCurve::AddPoint(EasingCurve::Point point)
    {
        
        if (point.m_time < 0.0f || point.m_time > 1.0f)
        {
            AZ_Error("EasingCurve", false, "Point time must be between 0 and 1!");
            return -1;
        }

        if (point.m_value < 0.0f || point.m_value > 1.0f)
        {
            AZ_Error("EasingCurve", false, "Point value must be between 0 and 1!");
            return -1;
        }
        if (m_points.empty())
        {
            m_points.emplace_back(point);
            return 0;
        }

        auto iter = AZStd::lower_bound(m_points.begin(), m_points.end(), point.m_time,
            [](Point& point, float time) {
                return point.m_time < time;
            });
        return AZStd::distance(m_points.begin(), m_points.emplace(iter, point));
    }
    
    void EasingCurve::RemovePoint(int64_t index)
    {
        if (index < 0 || index >= aznumeric_cast<int64_t>(m_points.size()))
        {
            AZ_Error("EasingCurve", false, "Index %d does not exist in the curve!", index);
            return;
        }
        m_points.erase(m_points.begin() + index);
    }

    void EasingCurve::Clear()
    {
        m_points.clear();
    }

    float EasingCurve::Evaluate(float time) const
    {
        if (m_points.empty())
        {
            return 0.0f;
        }

        if (m_points.size() == 1)
        {
            return m_points.front().m_value;
        }

        float offset = 0.0f;

        if (m_extrapMode == ExtrapolationMode::CONSTANT)
        {
            if (time < m_points.front().m_time)
            {
                return m_points.front().m_value;
            }
            else if (time > m_points.back().m_time)
            {
                return m_points.back().m_value;
            }
        }
        else if (m_extrapMode == ExtrapolationMode::CYCLE)
        {
            float duration = m_points.back().m_time - m_points.front().m_time;
            if (duration <= 0.0f)
            {
                return m_points.front().m_value;
            }
            time = AZStd::fmod(time - m_points.front().m_time, duration);
            if (time < 0.0f)
            {
                time += duration;
            }
            time += m_points.front().m_time;
        }
        else // CYCLE_WITH_OFFSET
        {
            float duration = m_points.back().m_time - m_points.front().m_time;
            if (duration <= 0.0f)
            {
                return m_points.front().m_value;
            }
            float valueRange = m_points.back().m_value - m_points.front().m_value;
            int cycleCount = aznumeric_cast<int>((time - m_points.front().m_time) / duration); // this rounds towards zero, which is what we want for negative time values
            time = AZStd::fmod(time - m_points.front().m_time, duration);
            if (time < 0.0f)
            {
                time += duration;
                cycleCount--;
            }
            time += m_points.front().m_time;
            offset = cycleCount * valueRange;
        }

        // potentially faster than binary search as elements are typically few
        for (AZStd::size_t i = 0; i < m_points.size(); i++)
        {
            if (AZ::IsClose(m_points[i].m_time, time))
            {
                return m_points[i].m_value;
            }
            else if (m_points[i].m_time > time)
            {
                return Interpolate(m_points[i-1], m_points[i], time) + offset;
            }
        }
        return 0.0f;
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
        case PointInterpolationMode::QUADRATIC_IN:
            easeMethod = EasingMethod::Quad;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::QUADRATIC_OUT:
            easeMethod = EasingMethod::Quad;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::QUADRATIC_IN_OUT:
            easeMethod = EasingMethod::Quad;
            easeType = EasingType::InOut;
            break;
        case PointInterpolationMode::CUBIC_IN:
            easeMethod = EasingMethod::Cubic;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::CUBIC_OUT:
            easeMethod = EasingMethod::Cubic;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::CUBIC_IN_OUT:
            easeMethod = EasingMethod::Cubic;
            easeType = EasingType::InOut;
            break;
        case PointInterpolationMode::QUARTIC_IN:
            easeMethod = EasingMethod::Quart;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::QUARTIC_OUT:
            easeMethod = EasingMethod::Quart;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::QUARTIC_IN_OUT:
            easeMethod = EasingMethod::Quart;
            easeType = EasingType::InOut;
            break;
        case PointInterpolationMode::QUINTIC_IN:
            easeMethod = EasingMethod::Quint;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::QUINTIC_OUT:
            easeMethod = EasingMethod::Quint;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::QUINTIC_IN_OUT:
            easeMethod = EasingMethod::Quint;
            easeType = EasingType::InOut;
            break;
        case PointInterpolationMode::SINE_IN:
            easeMethod = EasingMethod::Sine;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::SINE_OUT:
            easeMethod = EasingMethod::Cubic;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::SINE_IN_OUT:
            easeMethod = EasingMethod::Sine;
            easeType = EasingType::InOut;
            break;
        case PointInterpolationMode::EXPONENTIAL_IN:
            easeMethod = EasingMethod::Expo;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::EXPONENTIAL_OUT:
            easeMethod = EasingMethod::Expo;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::EXPONENTIAL_IN_OUT:
            easeMethod = EasingMethod::Expo;
            easeType = EasingType::InOut;
            break;
        case PointInterpolationMode::CIRCLE_IN:
            easeMethod = EasingMethod::Circ;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::CIRCLE_OUT:
            easeMethod = EasingMethod::Circ;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::CIRCLE_IN_OUT:
            easeMethod = EasingMethod::Circ;
            easeType = EasingType::InOut;
            break;
        case PointInterpolationMode::ELASTIC_IN:
            easeMethod = EasingMethod::Elastic;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::ELASTIC_OUT:
            easeMethod = EasingMethod::Elastic;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::ELASTIC_IN_OUT:
            easeMethod = EasingMethod::Elastic;
            easeType = EasingType::InOut;
            break;
        case PointInterpolationMode::BACK_IN:
            easeMethod = EasingMethod::Back;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::BACK_OUT:
            easeMethod = EasingMethod::Back;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::BACK_IN_OUT:
            easeMethod = EasingMethod::Back;
            easeType = EasingType::InOut;
            break;
        case PointInterpolationMode::BOUNCE_IN:
            easeMethod = EasingMethod::Bounce;
            easeType = EasingType::In;
            break;
        case PointInterpolationMode::BOUNCE_OUT:
            easeMethod = EasingMethod::Bounce;
            easeType = EasingType::Out;
            break;
        case PointInterpolationMode::BOUNCE_IN_OUT:
            easeMethod = EasingMethod::Bounce;
            easeType = EasingType::InOut;
            break;
        default:
            easeMethod = EasingMethod::Linear;
        }
        
        return EasingEquations::GetEasingResult(easeMethod, easeType, timeActive, duration, start.m_value, end.m_value);
    }

    int64_t EasingCurve::GetClosetPoint(float time, float value)
    {
        AZStd::size_t min_index = AZStd::numeric_limits<AZStd::size_t>::max();
        float min_sqdistance = AZStd::numeric_limits<float>::max();
        for (AZStd::size_t index = 0; index < m_points.size(); index++)
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
        if (min_index == AZStd::numeric_limits<AZStd::size_t>::max())
        {
            return -1;
        }
        return aznumeric_cast<int64_t>(min_index);
    }
}

