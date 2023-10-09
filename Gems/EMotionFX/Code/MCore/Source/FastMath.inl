/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// check if a float is zero
MCORE_INLINE bool Math::IsFloatZero(float x)
{
    return (Math::Abs(x) < Math::epsilon);
}


// compare two floats
MCORE_INLINE bool Math::IsFloatEqual(float x, float y)
{
    return (Math::Abs(x - y) < Math::epsilon);
}


MCORE_INLINE float Math::Floor(float x)
{
    return floorf(x);
}


MCORE_INLINE bool Math::IsPositive(float x)
{
    if (x > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}


MCORE_INLINE bool Math::IsNegative(float x)
{
    if (x < 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}


MCORE_INLINE float Math::SignOfFloat(float x)
{
    if (x > 0)
    {
        return 1;
    }
    else if (x < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}


MCORE_INLINE float Math::Abs(float x)
{
    return fabsf(x);
}


MCORE_INLINE float Math::Ceil(float x)
{
    return ceilf(x);
}


MCORE_INLINE float Math::RadiansToDegrees(float rad)
{
    return (rad * 57.2957795130823208767f);
}


MCORE_INLINE float Math::DegreesToRadians(float deg)
{
    return (deg * 0.01745329251994329576f);
}


MCORE_INLINE float Math::Sin(float x)
{
    return AZ::Sin(x);
}


MCORE_INLINE float Math::Cos(float x)
{
    return AZ::Cos(x);
}


MCORE_INLINE float Math::Tan(float x)
{
    return tanf(x);
}


MCORE_INLINE float Math::ASin(float x)
{
    return asinf(x);
}


MCORE_INLINE float Math::ACos(float x)
{
    return AZ::Acos(x);
}


MCORE_INLINE float Math::ATan(float x)
{
    return atanf(x);
}


MCORE_INLINE float Math::ATan2(float y, float x)
{
    return atan2f(y, x);
}


MCORE_INLINE float Math::SignOfCos(float y)
{
    if (y > Math::pi || y < -Math::pi)
    {
        y = fmodf(y, Math::pi);
    }

    if (y < Math::halfPi && y > -Math::halfPi)
    {
        return 1;
    }

    if (IsFloatEqual(y, Math::halfPi) || IsFloatEqual(y, -Math::halfPi))
    {
        return 0;
    }

    return -1;
}


MCORE_INLINE float Math::SignOfSin(float y)
{
    if (y > Math::pi || y < -Math::pi)
    {
        y = fmodf(y, Math::pi);
    }
    if (y > 0)
    {
        return 1.0f;
    }
    if (y < 0)
    {
        return -1.0f;
    }
    return 0;
}


MCORE_INLINE float Math::Exp(float x)
{
    return expf(x);
}


MCORE_INLINE float Math::Log(float x)
{
    return logf(x);
}


MCORE_INLINE float Math::Log2(float x)
{
    return logf(x) / logf(2.0f);
}


MCORE_INLINE float Math::Pow(float base, float exponent)
{
    return powf(base, exponent);
}


MCORE_INLINE float Math::Sqrt(float x)
{
    return AZ::Sqrt(x);
}


MCORE_INLINE float Math::SafeSqrt(float x)
{
    return (x > Math::epsilon) ? AZ::Sqrt(x) : 0.0f;
}


MCORE_INLINE float Math::InvSqrt(float x)
{
    return AZ::InvSqrt(x);
}


MCORE_INLINE uint32 Math::NextPowerOfTwo(uint32 x)
{
    uint32 nextPowerOfTwo = x;

    nextPowerOfTwo--;
    nextPowerOfTwo = (nextPowerOfTwo >> 1) | nextPowerOfTwo;
    nextPowerOfTwo = (nextPowerOfTwo >> 2) | nextPowerOfTwo;
    nextPowerOfTwo = (nextPowerOfTwo >> 4) | nextPowerOfTwo;
    nextPowerOfTwo = (nextPowerOfTwo >> 8) | nextPowerOfTwo;
    nextPowerOfTwo = (nextPowerOfTwo >> 16) | nextPowerOfTwo;
    nextPowerOfTwo++;

    return nextPowerOfTwo;
}


// returns an approximation to 1/sqrt(x)
MCORE_INLINE float Math::FastInvSqrt(float x)
{
    return AZ::Simd::Vec1::SelectIndex0(AZ::Simd::Vec1::SqrtInvEstimate(AZ::Simd::Vec1::Splat(x)));
}

/*
// returns an approximation to 1/sqrt(x)
MCORE_INLINE float Math::FastInvSqrt2(float x)
{
    const float xhalf = 0.5f * x;       // half of x
    int i = (int*)&x;                   // get bits for floating value
    i = 0x5f3759df - (i >> 1);          // gives initial guess y0
    x = *(float*)&i;                    // convert bits back to float
    x = x * (1.5f - xhalf * x * x);     // Newton step, repeating increases accuracy
    return x;
}


// returns an more accurate approximation to 1/sqrt(x) than FastInvSqrt2
MCORE_INLINE float Math::FastInvSqrt3(float x)
{
    const float xhalf = 0.5f * x;       // half of x
    int i = *(int*)&x;                  // get bits for floating value
    i = 0x5f3759df - (i >> 1);          // gives initial guess y0
    x = *(float*)&i;                    // convert bits back to float
    x = x * (1.5f - xhalf * x * x);     // Newton step, repeating increases accuracy
    x = x * (1.5f - xhalf * x * x);     // repeated step
    return x;
}
*/


// 11 bit precision sqrt
MCORE_INLINE float Math::FastSqrt2(float x)
{
    /*  #if AZ_TRAIT_USE_PLATFORM_SIMD_SSE
            float result;
            __m128 input        = _mm_load_ss( &x );
            __m128 oneOverSqrt  = _mm_rsqrt_ss( input );
            __m128 sqrtApprox   = _mm_rcp_ss( oneOverSqrt );
            _mm_store_ss( &result, sqrtApprox );
            return result;
        #else*/
    return Math::Sqrt(x);
    ///#endif
}


// 22 bit precision sqrt
MCORE_INLINE float Math::FastSqrt(float x)
{
    /*
        #if AZ_TRAIT_USE_PLATFORM_SIMD_SSE
            const float halfFloat = 0.5f;
            float result;

            // calculate the sqrt approximation first
            __m128 input        = _mm_load_ss( &x );
            __m128 oneOverSqrt  = _mm_rsqrt_ss( input );
            __m128 sqrtApprox   = _mm_rcp_ss( oneOverSqrt );

            __m128 half         = _mm_load_ss( &halfFloat );
            __m128 sqrInput     = _mm_mul_ss( sqrtApprox, sqrtApprox );
            __m128 a            = _mm_mul_ss( oneOverSqrt, half );
            __m128 b            = _mm_sub_ss( sqrInput, input );
            __m128 c            = _mm_mul_ss( b, a );
            __m128 d            = _mm_sub_ss( sqrtApprox, c );

            _mm_store_ss( &result, d );
            return result;
        #else*/
    return Math::Sqrt(x);
    //#endif
}


// align a value
template<typename T>
MCORE_INLINE T Math::Align(T inValue, T alignment)
{
    const T modValue = inValue % alignment;
    if (modValue > 0)
    {
        return inValue + (alignment - modValue);
    }
    return inValue;
}


// dest = source * sgn(source)
MCORE_INLINE void Math::MulSign(float& dest, float const& source)
{
    const int32 signMask = ((int32&)dest ^ (int32&)source) & 0x80000000;

    // XOR and mask
    (int32&)dest &= 0x7FFFFFFF; // clear sign bit
    (int32&)dest |= signMask;   // set sign bit if necessary
}

/*
// check if the float is positive
MCORE_INLINE bool Math::IsFloatPositive(float f)
{
    #if (MCORE_COMPILER == MCORE_COMPILER_GCC)
        return (f >= 0.0f)
    #else
        int32 signBit = *((int32*)(&f)) & 0x80000000;
        return signBit == 0;
    #endif
}


// check if the float is positive
MCORE_INLINE bool Math::IsFloatNegative(float f)
{
    #if (MCORE_COMPILER == MCORE_COMPILER_GCC)
        return (f < 0);
    #else
        int32 signBit = *((int32*)(&f)) & 0x80000000;
        return signBit != 0;
    #endif
}
*/

// fmod
MCORE_INLINE float Math::FMod(float x, float y)
{
    return fmodf(x, y);
}


// safe fmod
MCORE_INLINE float Math::SafeFMod(float x, float y)
{
    if (y < -epsilon || y > epsilon)
    {
        return fmodf(x, y);
    }
    else
    {
        return 0.0f;
    }
}
