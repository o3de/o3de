/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// calculate the cube root
MCORE_INLINE float CubeRoot(float x)
{
    if (x > 0.0f)
    {
        return Math::Pow(x, 0.333333f);
    }
    else if (x < 0.0f)
    {
        return -Math::Pow(-x, 0.333333f);
    }

    return 0.0f;
}


// calculate the cosine interpolation weight value
MCORE_INLINE float CalcCosineInterpolationWeight(float linearValue)
{
    return (1.0f - Math::Cos(linearValue * Math::pi)) * 0.5f;
}


// linear interpolate
template <class T>
MCORE_INLINE T LinearInterpolate(const T& source, const T& target, float timeValue)
{
    return static_cast<T>(source * (1.0f - timeValue) + (timeValue * target));
}


// cosine interpolate
template <class T>
MCORE_INLINE T CosineInterpolate(const T& source, const T& target, float timeValue)
{
    const float weight = CalcCosineInterpolationWeight(timeValue);
    return source * (1.0f - weight) + (weight * target);
}


// barycentric interpolation
template <class T>
MCORE_INLINE T BarycentricInterpolate(float u, float v, const T& pointA, const T& pointB, const T& pointC)
{
    return (1.0f - u - v) * pointA + u * pointB + v * pointC;
}


// swap two values
template <class T>
MCORE_INLINE void Swap(T& a, T& b)
{
    // don't do anything when both items are the same
    if (&a == &b)
    {
        return;
    }

    const T temp(std::move(a));
    a = std::move(b);
    b = std::move(temp);

    /*  T tempObj = a;
        a = b;
        b = tempObj;*/
}


// get the minimum of two values
template <class T>
MCORE_INLINE T Min(T a, T b)
{
    return (a < b) ? a : b;
}


// get the maximum of two values
template <class T>
MCORE_INLINE T Max(T a, T b)
{
    return (a > b) ? a : b;
}


// get the minimum of 3 values
template <class T>
MCORE_INLINE T Min3(T a, T b, T c)
{
    return Min(Min(a, b), c);
}


// get the maximum of three values
template <class T>
MCORE_INLINE T Max3(T a, T b, T c)
{
    return Max(Max(a, b), c);
}


// return -1 in case of a negative value, 0 when zero, or +1 when positive
template <class T>
MCORE_INLINE T Sgn(T A)
{
    return (A > (T)0) ? (T)1 : ((A < (T)0) ? (T)-1 : (T)0);
}


// the square of the value
template <class T>
MCORE_INLINE T Square(T x)
{
    return x * x;
}


// the square of the value, same like Square, but with another name
template <class T>
MCORE_INLINE T Sqr(T x)
{
    return x * x;
}


// check if a value is negative or not
// TODO: use intel optimization here by checking the bits? (when compiling for PC)
template <class T>
MCORE_INLINE bool IsNegative(T x)
{
    return (x < 0);
}


// check if a value is positive or not
// TODO: use intel optimization here by checking the bits? (when compiling for PC)
template <class T>
MCORE_INLINE bool IsPositive(T x)
{
    return (x >= 0);
}


// clamp/clip a value to a given range
template <class T>
MCORE_INLINE T Clamp(T x, T min, T max)
{
    if (x < min)
    {
        x = min;
    }

    if (x > max)
    {
        x = max;
    }

    return x;
}


// check if value x is within a given range
template <class T>
MCORE_INLINE bool InRange(T x, T low, T high)
{
    return ((x >= low) && (x <= high));
}


// sample an ease-in/out curve
MCORE_INLINE float SampleEaseInOutCurve(float t, float k1, float k2)
{
    const float f = k1 * 2.0f / MCore::Math::pi + k2 - k1 + (1.0f - k2) * 2.0f / MCore::Math::pi;
    if (t < k1) // ease in section
    {
        return (k1 * (2.0f / MCore::Math::pi) * (MCore::Math::Sin((t / k1) * MCore::Math::halfPi - MCore::Math::halfPi) + 1.0f)) / f;
    }
    else
    if (t < k2) // mid section
    {
        return (k1 / MCore::Math::halfPi + t - k1) / f;
    }
    else    // ease out section
    {
        return ((k1 / MCore::Math::halfPi) + k2 - k1 + ((1.0f - k2) * (2.0f / MCore::Math::pi) * MCore::Math::Sin(((t - k2) / (1.0f - k2)) * MCore::Math::halfPi))) / f;
    }
}


// ease-in/out interpolation
template <class T>
MCORE_INLINE T EaseInOutInterpolate(const T& source, const T& target, float timeValue, float k1, float k2)
{
    const float t = SampleEaseInOutCurve(timeValue, k1, k2);
    return source + t * (target - source);
}


// sample an ease-in-out curve with smoothness control
// basically this is a much simplified TCB spline
MCORE_INLINE float SampleEaseInOutCurveWithSmoothness(float t, float easeInSmoothness, float easeOutSmoothness)
{
    const float continuity = -1.0f + easeInSmoothness;
    const float tangentA = -(1.0f + continuity) * 0.5f + (1.0f - continuity) * 0.5f;

    const float continuity2 = -1.0f + easeOutSmoothness;
    const float tangentB = -(1.0f + continuity2) * 0.5f + (1.0f - continuity2) * 0.5f;

    const float t2 = t * t;
    const float t3 = t2 * t;
    return (-2 * t3 +  3 * t2) +
           (t3 + -2 * t2 + t) * tangentA +
           (t3 + -t2)       * tangentB;
}


// interpolate with ease-in out with smoothness
template <class T>
MCORE_INLINE T EaseInOutWithSmoothnessInterpolate(const T& source, const T& target, float timeValue, float easeInSmoothness, float easeOutSmoothness)
{
    const float t = MCore::SampleEaseInOutCurveWithSmoothness(timeValue, easeInSmoothness, easeOutSmoothness);
    return source + t * (target - source);
}
