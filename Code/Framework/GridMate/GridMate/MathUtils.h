/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_MATHUTILS_H
#define GM_MATHUTILS_H

#include <AzCore/base.h>

namespace GridMate
{
    //-------------------------------------------------------------------------
    // helper functions to encode float as int while preserving relative-order
    // between +/- numbers and equality between +0.f and -0.f
    // http://www.cygnus-software.com/papers/comparingfloats/Comparing%20floating%20point%20numbers.htm
    //-------------------------------------------------------------------------
    AZ_FORCE_INLINE int encode_float_as_int(float v)
    {
        union
        {
            int m_i;
            float m_f;
        };

        m_f = v;
        return m_i < 0 ? 0x80000000 - m_i : m_i;
    }
    //-------------------------------------------------------------------------
    AZ_FORCE_INLINE float decode_float_as_int(int v)
    {
        union
        {
            int m_i;
            float m_f;
        };

        m_i = v < 0 ? 0x80000000 - v : v;
        return m_f;
    }
    //-------------------------------------------------------------------------
    AZ_FORCE_INLINE AZ::u32 encode_int_as_uint(AZ::s32 val)
    {
        AZ::u64 val64 = (~static_cast<AZ::u32>(val)) + 0x80000000u; // packing signed int into unsigned: 0x00(127)..0x7F(0), 0x80(-1)..0xFE(-127)
        return static_cast<AZ::u32>(val64);
    }
    //-------------------------------------------------------------------------
    AZ_FORCE_INLINE AZ::s32 decode_int_as_uint(AZ::u32 val)
    {
        AZ::u64 val64 = val - 0x80000000u;
        return static_cast<AZ::s32>(~val64);
    }
    //-------------------------------------------------------------------------
    template<class T, unsigned int size>
    class RollingSum
    {
    public:
        AZ_FORCE_INLINE RollingSum()
            : m_sum(T())
            , m_pos(0)
            , m_accumDt(0.f)
            , m_accumValue(T())
        {
            for (T& val : m_history)
            {
                val = T();
            }
        }

        AZ_FORCE_INLINE T GetSum() const
        {
            return m_sum;
        }

        AZ_FORCE_INLINE void Update(float dt, T value)
        {
            m_accumDt += dt;
            m_accumValue += value;

            const float thresholdDt = 1.f / size;
            if (m_accumDt < thresholdDt)
            {
                return;
            }

            const float maxDt = size * thresholdDt; // clamping in case there was delay >1sec
            m_accumValue = AZStd::GetMin(m_accumValue, m_accumValue * static_cast<T>(maxDt / m_accumDt));
            m_accumDt = AZStd::GetMin(m_accumDt, maxDt);

            unsigned int bytes = static_cast<unsigned int>(thresholdDt * m_accumValue / m_accumDt);
            while (m_accumDt >= thresholdDt)
            {
                Add(bytes);
                m_accumValue -= bytes;
                m_accumDt -= thresholdDt;
            }
        }

    private:
        T m_sum;
        T m_history[size];
        size_t m_pos;

        float m_accumDt;
        T m_accumValue;

        AZ_FORCE_INLINE void Add(T value)
        {
            m_sum -= m_history[m_pos];
            m_history[m_pos] = value;
            m_sum += value;
            m_pos = (m_pos + 1) % size;
        }
    };
}   // namespace GridMate

#endif // GM_MATHUTILS_H
