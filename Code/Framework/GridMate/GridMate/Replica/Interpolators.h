/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_INTERPOLATOR_H
#define GM_INTERPOLATOR_H

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/array.h>

namespace GridMate
{
    template<typename T>
    struct SampleInfo
    {
        T m_v;
        unsigned int m_t;
        bool m_cantBreak;
    };

    template<typename T>
    struct SimpleValueInterpolator
    {
        /// \param time is [0.0,1.0]
        static T Interpolate(const T& from, const T& to, float time) { return T(from + (to - from) * time); }
    };

    //-----------------------------------------------------------------------------
    // Interpolators
    //-----------------------------------------------------------------------------
    //-----------------------------------------------------------------------------
    // PointSample
    //-----------------------------------------------------------------------------
    template<typename T, int k_maxSamples = 8>
    class PointSample
    {
    public:
        struct Sample
        {
            T m_v;
            unsigned int m_t;
        };


        PointSample()
            : m_curIdx(-1)
            , m_count(0) { }

        void AddSample(const T& sample, unsigned int time)
        {
            // discard old data points
            if (m_count > 0 && m_samples[m_curIdx].m_t > time)
            {
                return;
            }
            // only add new point if time moved forward
            if (m_count == 0 || time > m_samples[m_curIdx].m_t)
            {
                m_curIdx = (m_curIdx + 1) % k_maxSamples;
                m_count = AZ::GetMin(m_count + 1, k_maxSamples);
            }
            m_samples[m_curIdx].m_v = sample;
            m_samples[m_curIdx].m_t = time;
        }

        T GetInterpolatedValue(unsigned int time) const
        {
            AZ_Assert(m_count > 0, "No samples available.");
            int first = (m_curIdx + k_maxSamples - m_count + 1) % k_maxSamples;
            int beyond = (m_curIdx + 1) % k_maxSamples;
            for (int i = (first + 1) % k_maxSamples; i != beyond; i = (i + 1) % k_maxSamples)
            {
                if (m_samples[i].m_t > time)
                {
                    return m_samples[first].m_v;
                }
                first = i;
            }
            return m_samples[first].m_v;
        }

        T GetLastValue() const
        {
            AZ_Assert(m_count > 0, "No samples available.");
            return m_samples[m_curIdx].m_v;
        }

        void Break() { }
        void Clear() { m_curIdx = -1; m_count = 0; }

        // Debug info
        int GetSampleCount() const { return m_count; }
        SampleInfo<T> GetSampleInfo(int i) const
        {
            AZ_Assert(i >= 0 && i < m_count, "Out of bounds.");
            int _i = (m_curIdx + k_maxSamples - m_count + 1 + i) % k_maxSamples;
            SampleInfo<T> info;
            info.m_v = m_samples[_i].m_v;
            info.m_t = m_samples[_i].m_t;
            info.m_cantBreak = false;
            return info;
        }

    private:
        AZStd::array<Sample, k_maxSamples> m_samples;
        int m_curIdx, m_count;
    };
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // LinearInterp
    //-----------------------------------------------------------------------------
    template<typename T, int k_maxSamples = 8, typename Interpolator = SimpleValueInterpolator<T> >
    class LinearInterp
    {
    public:
        struct Sample
        {
            T m_v;
            unsigned int m_t;
            bool m_cantBreak;
        };

        LinearInterp()
            : m_curIdx(-1)
            , m_count(0)
            , m_cantBreak(false) { }

        void AddSample(const T& sample, unsigned int time)
        {
            // discard old data points
            if (m_count > 0 && m_samples[m_curIdx].m_t > time)
            {
                return;
            }
            // only add new point if time moved forward
            if (m_count == 0 || time > m_samples[m_curIdx].m_t)
            {
                m_curIdx = (m_curIdx + 1) % k_maxSamples;
                m_count = AZ::GetMin(m_count + 1, k_maxSamples);
            }
            m_samples[m_curIdx].m_v = sample;
            m_samples[m_curIdx].m_t = time;
            m_samples[m_curIdx].m_cantBreak = m_cantBreak;
            m_cantBreak = false;
        }

        T GetInterpolatedValue(unsigned int time) const
        {
            AZ_Assert(m_count > 0, "No samples available.");
            int first = (m_curIdx + k_maxSamples - m_count + 1) % k_maxSamples;
            //int last = (curIdx + maxSamples - count - 1) % maxSamples;
            int beyond = (m_curIdx + 1) % k_maxSamples;
            if (time < m_samples[first].m_t)
            {
                return m_samples[first].m_v;
            }
            float interval, t;
            T v;
            for (int second = (first + 1) % k_maxSamples; second != beyond; second = (second + 1) % k_maxSamples)
            {
                if (m_samples[second].m_t > time)
                {
                    // interpolation
                    if (m_samples[second].m_cantBreak)
                    {
                        v = m_samples[first].m_v;
                    }
                    else
                    {
                        interval = float(m_samples[second].m_t - m_samples[first].m_t);
                        AZ_Assert(interval > 0.001f, "non-incrementing timestamps!");
                        t = float(time - m_samples[first].m_t) / interval;
                        AZ_Assert(t >= 0.f && t < 1.f, "we should be interpolating!");
                        v = Interpolator::Interpolate(m_samples[first].m_v, m_samples[second].m_v, t);
                    }
                    return v;
                }
                first = second;
            }
            // return last value
            return m_samples[first].m_v;
        }

        const T& GetLastValue() const
        {
            AZ_Assert(m_count > 0, "No samples available.");
            return m_samples[m_curIdx].m_v;
        }

        const Sample& GetLastSample() const
        {
            AZ_Assert(m_count > 0, "No samples available.");
            return m_samples[m_curIdx];
        }

        const Sample& GetFirstSample() const
        {
            AZ_Assert(m_count > 0, "No samples available.");
            int first = (m_curIdx + k_maxSamples - m_count + 1) % k_maxSamples;
            return m_samples[first];
        }

        void Break() { m_cantBreak = true; }
        void Clear() { m_curIdx = -1; m_count = 0; m_cantBreak = false; }

        // Debug info
        int GetSampleCount() const { return m_count; }
        SampleInfo<T> GetSampleInfo(int i) const
        {
            AZ_Assert(i >= 0 && i < m_count, "Out of bounds.");
            int _i = (m_curIdx + k_maxSamples - m_count + 1 + i) % k_maxSamples;
            SampleInfo<T> info;
            info.m_v = m_samples[_i].m_v;
            info.m_t = m_samples[_i].m_t;
            info.m_cantBreak = m_samples[_i].m_cantBreak;
            return info;
        }

    private:
        AZStd::array<Sample, k_maxSamples> m_samples;
        int m_curIdx, m_count;
        bool m_cantBreak;
    };
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // LinearInterpExtrap
    // Interpolates/Extrapolates using the 2 closest samples
    //-----------------------------------------------------------------------------
    template<typename T, int k_maxSamples = 8, typename Interpolator = SimpleValueInterpolator<T> >
    class LinearInterpExtrap
    {
    public:
        struct Sample
        {
            T m_v;
            unsigned int m_t;
            bool m_cantBreak;
        };

        LinearInterpExtrap()
            : m_curIdx(-1)
            , m_count(0)
            , m_cantBreak(false) { }

        void AddSample(const T& sample, unsigned int time)
        {
            // discard old data points
            if (m_count > 0 && m_samples[m_curIdx].m_t > time)
            {
                return;
            }
            // only add new point if time moved forward
            if (m_count == 0 || time > m_samples[m_curIdx].m_t)
            {
                m_curIdx = (m_curIdx + 1) % k_maxSamples;
                m_count = AZ::GetMin(m_count + 1, k_maxSamples);
            }
            m_samples[m_curIdx].m_v = sample;
            m_samples[m_curIdx].m_t = time;
            m_samples[m_curIdx].m_cantBreak = m_cantBreak;
            m_cantBreak = false;
        }

        T GetInterpolatedValue(unsigned int time) const
        {
            AZ_Assert(m_count > 0, "No samples available.");
            if (m_count == 1 || k_maxSamples == 1)
            {
                return m_samples[0].m_v;
            }
            int first = (m_curIdx + k_maxSamples - m_count + 1) % k_maxSamples;
            int beyond = (m_curIdx + 1) % k_maxSamples;
            if (time < m_samples[first].m_t)
            {
                return m_samples[first].m_v;
            }
            int second, previous = 0;
            float interval, t;
            T v;
            for (second = (first + 1) % k_maxSamples; second != beyond; second = (second + 1) % k_maxSamples)
            {
                if (m_samples[second].m_t > time)
                {
                    // interpolation
                    if (m_samples[second].m_cantBreak)
                    {
                        v = m_samples[first].m_v;
                    }
                    else
                    {
                        interval = static_cast<float>(m_samples[second].m_t - m_samples[first].m_t);
                        AZ_Assert(interval > 0.001f, "non-incrementing timestamps!");
                        t = (time - m_samples[first].m_t) / interval;
                        AZ_Assert(t >= 0.f && t < 1.f, "we should be interpolating!");
                        v = Interpolator::Interpolate(m_samples[first].m_v, m_samples[second].m_v, t);
                    }
                    return v;
                }
                previous = first;
                first = second;
            }
            // extrapolation
            second = first;
            first = previous;
            if (m_samples[second].m_cantBreak)
            {
                v = m_samples[second].m_v;
            }
            else
            {
                interval = static_cast<float>(m_samples[second].m_t - m_samples[first].m_t);
                AZ_Assert(interval > 0.001f, "non-incrementing timestamps!");
                t = (time - m_samples[first].m_t) / interval;
                AZ_Assert(t > 0.99f, "we should be extrapolating!");
                v = Interpolator::Interpolate(m_samples[first].m_v, m_samples[second].m_v, t);
            }
            return v;
        }

        void Break()
        {
            m_cantBreak = true;
        }

        T GetLastValue() const
        {
            AZ_Assert(m_count > 0, "No samples available.");
            return m_samples[m_curIdx].m_v;
        }

        void Clear()
        {
            m_count = 0;
            m_curIdx = -1;
            m_cantBreak = false;
        }

        // Debug info
        int GetSampleCount() const { return m_count; }
        SampleInfo<T> GetSampleInfo(int i) const
        {
            AZ_Assert(i >= 0 && i < m_count, "Out of bounds.");
            int _i = (m_curIdx + k_maxSamples - m_count + 1 + i) % k_maxSamples;
            SampleInfo<T> info;
            info.m_v = m_samples[_i].m_v;
            info.m_t = m_samples[_i].m_t;
            info.m_cantBreak = m_samples[_i].m_cantBreak;
            return info;
        }
    private:
        AZStd::array<Sample, k_maxSamples> m_samples;
        int m_curIdx, m_count;
        bool m_cantBreak;
    };
    //-----------------------------------------------------------------------------
} // namespace GridMate

#endif // GM_INTERPOLATOR_H
