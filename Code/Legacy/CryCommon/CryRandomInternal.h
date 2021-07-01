/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <algorithm>  // std::swap()
#include <limits>  // std::numeric_limits
#include <type_traits>  // std::make_unsigned
#include "BaseTypes.h"  // uint32, uint64
#include "CompileTimeAssert.h"
#include "Cry_Vector2.h"
#include "Cry_Vector3.h"
#include "Cry_Vector4.h"


namespace CryRandom_Internal
{
    template <class R, class T, size_t size>
    struct BoundedRandomUint
    {
        COMPILE_TIME_ASSERT(std::numeric_limits<T>::is_integer);
        COMPILE_TIME_ASSERT(!std::numeric_limits<T>::is_signed);
        COMPILE_TIME_ASSERT(sizeof(T) == size);
        COMPILE_TIME_ASSERT(sizeof(T) <= sizeof(uint32));

        inline static T Get(R& randomGenerator, const T maxValue)
        {
            const uint32 r = randomGenerator.GenerateUint32();
            // Note that the computation below is biased. An alternative computation
            // (also biased): uint32((uint64)r * ((uint64)maxValue + 1)) >> 32)
            return (T)((uint64)r % ((uint64)maxValue + 1));
        }
    };

    template <class R, class T>
    struct BoundedRandomUint<R, T, 8>
    {
        COMPILE_TIME_ASSERT(std::numeric_limits<T>::is_integer);
        COMPILE_TIME_ASSERT(!std::numeric_limits<T>::is_signed);
        COMPILE_TIME_ASSERT(sizeof(T) == sizeof(uint64));

        inline static T Get(R& randomGenerator, const T maxValue)
        {
            const uint64 r = randomGenerator.GenerateUint64();
            if (maxValue >= (std::numeric_limits<uint64>::max)())
            {
                return r;
            }
            // Note that the computation below is biased.
            return (T)(r % ((uint64)maxValue + 1));
        }
    };

    //////////////////////////////////////////////////////////////////////////

    template <class R, class T, bool bInteger = std::numeric_limits<T>::is_integer>
    struct BoundedRandom;

    template <class R, class T>
    struct BoundedRandom<R, T, true>
    {
        COMPILE_TIME_ASSERT(std::numeric_limits<T>::is_integer);
        typedef typename std::make_unsigned<T>::type UT;
        COMPILE_TIME_ASSERT(sizeof(T) == sizeof(UT));
        COMPILE_TIME_ASSERT(std::numeric_limits<UT>::is_integer);
        COMPILE_TIME_ASSERT(!std::numeric_limits<UT>::is_signed);

        inline static T Get(R& randomGenerator, T minValue, T maxValue)
        {
            if (minValue > maxValue)
            {
                std::swap(minValue, maxValue);
            }
            return (T)((UT)minValue + (UT)BoundedRandomUint<R, UT, sizeof(UT)>::Get(randomGenerator, (UT)(maxValue - minValue)));
        }
    };

    template <class R, class T>
    struct BoundedRandom<R, T, false>
    {
        COMPILE_TIME_ASSERT(!std::numeric_limits<T>::is_integer);

        inline static T Get(R& randomGenerator, const T minValue, const T maxValue)
        {
            return minValue + (maxValue - minValue) * randomGenerator.GenerateFloat();
        }
    };

    //////////////////////////////////////////////////////////////////////////

    template <class R, class VT, class T = typename VT::value_type, size_t componentCount = VT::component_count>
    struct BoundedRandomComponentwise;

    template <class R, class VT, class T>
    struct BoundedRandomComponentwise<R, VT, T, 2>
    {
        inline static VT Get(R& randomGenerator, const VT& minValue, const VT& maxValue)
        {
            const T x = BoundedRandom<R, T>::Get(randomGenerator, minValue.x, maxValue.x);
            const T y = BoundedRandom<R, T>::Get(randomGenerator, minValue.y, maxValue.y);
            return VT(x, y);
        }
    };

    template <class R, class VT, class T>
    struct BoundedRandomComponentwise<R, VT, T, 3>
    {
        inline static VT Get(R& randomGenerator, const VT& minValue, const VT& maxValue)
        {
            const T x = BoundedRandom<R, T>::Get(randomGenerator, minValue.x, maxValue.x);
            const T y = BoundedRandom<R, T>::Get(randomGenerator, minValue.y, maxValue.y);
            const T z = BoundedRandom<R, T>::Get(randomGenerator, minValue.z, maxValue.z);
            return VT(x, y, z);
        }
    };

    template <class R, class VT, class T>
    struct BoundedRandomComponentwise<R, VT, T, 4>
    {
        inline static VT Get(R& randomGenerator, const VT& minValue, const VT& maxValue)
        {
            const T x = BoundedRandom<R, T>::Get(randomGenerator, minValue.x, maxValue.x);
            const T y = BoundedRandom<R, T>::Get(randomGenerator, minValue.y, maxValue.y);
            const T z = BoundedRandom<R, T>::Get(randomGenerator, minValue.z, maxValue.z);
            const T w = BoundedRandom<R, T>::Get(randomGenerator, minValue.w, maxValue.w);
            return VT(x, y, z, w);
        }
    };

    //////////////////////////////////////////////////////////////////////////

    template <class R, class VT>
    inline VT GetRandomUnitVector(R& randomGenerator)
    {
        typedef typename VT::value_type T;
        COMPILE_TIME_ASSERT(!std::numeric_limits<T>::is_integer);

        VT res;
        T lenSquared;

        do
        {
            res = BoundedRandomComponentwise<R, VT>::Get(randomGenerator, VT(-1), VT(1));
            lenSquared = res.GetLengthSquared();
        } while (lenSquared > 1);

        if (lenSquared >= (std::numeric_limits<T>::min)())
        {
            return res * isqrt_tpl(lenSquared);
        }

        res = VT(ZERO);
        res.x = 1;
        return res;
    }
} // namespace CryRandom_Internal

// eof
