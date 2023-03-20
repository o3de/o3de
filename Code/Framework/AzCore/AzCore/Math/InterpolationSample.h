/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Quaternion.h>

namespace AZ
{
    /**
    * Behavior types for smoothing of transform between network updates.
    */
    enum class InterpolationMode : u32
    {
        NoInterpolation,
        LinearInterpolation,
    };

    AZ_TYPE_INFO_SPECIALIZE(InterpolationMode, "{3095F41E-C2C6-474A-BB05-68B228097626}");

    template<typename Value>
    class Sample
    {
    public:
        virtual ~Sample() = default;
        using TimeType = unsigned int;

        Sample()
            : m_targetValue()
            , m_targetTimestamp(0)
            , m_previousValue()
            , m_previousTimestamp(0)
        {
        }

        void SetNewTarget(Value newValue, TimeType timestamp)
        {
            m_targetValue = newValue;
            m_targetTimestamp = timestamp;
        }

        virtual Value GetInterpolatedValue(TimeType time) = 0;

        Value GetTargetValue() const
        {
            return m_targetValue;
        }

        TimeType GetTargetTimestamp() const
        {
            return m_targetTimestamp;
        }

    protected:
        void SetPreviousValue(Value previousValue, TimeType previousTimestamp)
        {
            m_previousValue = previousValue;
            m_previousTimestamp = previousTimestamp;
        }

        Value m_targetValue;
        TimeType m_targetTimestamp;

        Value m_previousValue;
        TimeType m_previousTimestamp;
    };

    template<typename T>
    class LinearlyInterpolatedSample;

    template<>
    class LinearlyInterpolatedSample<Vector3> final
        : public Sample<Vector3>
    {
    public:
        Vector3 GetInterpolatedValue(TimeType time) final
        {
            Vector3 interpolatedValue = m_previousValue;
            if (m_targetTimestamp != 0)
            {
                if (m_targetTimestamp <= m_previousTimestamp || m_targetTimestamp <= time)
                {
                    interpolatedValue = m_targetValue;
                }
                else if (time > m_previousTimestamp)
                {
                    float t = float(time - m_previousTimestamp) / float(m_targetTimestamp - m_previousTimestamp);

                    // lerp translation
                    Vector3 deltaPos = t * (m_targetValue - m_previousValue);
                    interpolatedValue = m_previousValue + deltaPos;

                    AZ_MATH_ASSERT(interpolatedValue.IsFinite(), "interpolatedValue is not finite!");
                }
            }

            SetPreviousValue(interpolatedValue, time);
            return interpolatedValue;
        }
    };

    template<>
    class LinearlyInterpolatedSample<Quaternion> final
        : public Sample<Quaternion>
    {
    public:
        Quaternion GetInterpolatedValue(TimeType time) final
        {
            Quaternion interpolatedValue = m_previousValue;
            if (m_targetTimestamp != 0)
            {
                if (m_targetTimestamp <= m_previousTimestamp || m_targetTimestamp <= time)
                {
                    interpolatedValue = m_targetValue;
                }
                else if (time > m_previousTimestamp)
                {
                    float t = float(time - m_previousTimestamp) / float(m_targetTimestamp - m_previousTimestamp);

                    // slerp rotation
                    Quaternion r0 = m_previousValue;
                    Quaternion r1 = m_targetValue;
                    Quaternion rot = r0.Slerp(r1, t);
                    interpolatedValue = rot.GetNormalized();

                    AZ_MATH_ASSERT(interpolatedValue.IsFinite(), "interpolatedValue is not finite!");
                }
            }

            SetPreviousValue(interpolatedValue, time);
            return interpolatedValue;
        }
    };

    template<typename T>
    class UninterpolatedSample;

    template<>
    class UninterpolatedSample<Vector3> final
        : public Sample<Vector3>
    {
    public:
        Vector3 GetInterpolatedValue(TimeType /*time*/) final
        {
            return GetTargetValue();
        }
    };

    template<>
    class UninterpolatedSample<Quaternion> final
        : public Sample<Quaternion>
    {
    public:
        Quaternion GetInterpolatedValue(TimeType /*time*/) final
        {
            return GetTargetValue();
        }
    };
}
