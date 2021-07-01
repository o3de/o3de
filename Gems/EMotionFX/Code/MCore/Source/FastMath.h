/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

//  include required headers
#include "StandardHeaders.h"
#include <AzCore/Math/Internal/MathTypes.h>

namespace MCore
{
    /**
     * The math class.
     * All methods and attributes are static.
     */
    class MCORE_API Math
    {
    public:
        /**
         * Calculates the floored version of a value.
         * The floor function returns a value representing the largest
         * integer that is less than or equal to x.
         * Examples:
         * The floor of 2.8 is 2.000000
         * The floor of -2.8 is -3.000000
         * @param x The value to floor.
         * @return The value representing the largest integer that is less than or equal to x.
         */
        static MCORE_INLINE float Floor(float x);

        /**
         * Tests if a number is positive.
         * @param x The tested value.
         * @return True if the number is positive, false if it is negative or zero.
         */
        static MCORE_INLINE bool IsPositive(float x);

        /**
         * Tests if a number is negative.
         * @param x The tested value.
         * @return True if the number is negative, false if it is positive or zero.
         */
        static MCORE_INLINE bool IsNegative(float x);

        /**
         * Tests if a number is equal to zero. This uses the epsilon value as tolerance.
         * Basically it does "return (Abs(x) < epsilon)".
         * @param x The tested value.
         * @return True if x is zero, otherwise false is returned.
         */
        static MCORE_INLINE bool IsFloatZero(float x);

        /**
         * Tests if two numbers are equal to each other. This uses the epsilon value as tolerance.
         * Basically it does "return (Abs(x-y) < epsilon)".
         * @param x The first value.
         * @param y The second value.
         * @return True if x is equal to y, otherwise false is returned.
         */
        static MCORE_INLINE bool IsFloatEqual(float x, float y);

        /**
         * Returns the sign of a real number.
         * @param x The tested number.
         * @return 1 for positive, -1 for negative, 0 for zero.
         */
        static MCORE_INLINE float SignOfFloat(float x);

        /**
         * Calculates the absolute value.
         * The Abs function returns the absolute value of its parameter.
         * Examples:
         * The absolute value of -4 is 4
         * The absolute value of -41567 is 41567
         * The absolute value of -3.141593 is 3.141593
         * @param x The value to calculate the absolute value from.
         * @return The absolute value of the parameter.
         */
        static MCORE_INLINE float Abs(float x);

        /**
         * Calculates the ceiling of a value.
         * The ceil function returns a value representing the smallest integer
         * that is greater than or equal to x.
         * Examples:
         * The ceil of 2.8 is 3.000000
         * The ceil of -2.8 is -2.000000
         * @param x The value to ceil.
         * @return The value representing the smallest integer that is greater than or equal to x.
         */
        static MCORE_INLINE float Ceil(float x);

        /**
         * Radians to degree Conversion.
         * @param rad The angle(rads) to convert.
         * @return The converted angle in degrees.
         */
        static MCORE_INLINE float RadiansToDegrees(float rad);

        /**
         * Degree to radians Conversion.
         * @param deg The angle (degrees) to convert.
         * @return The converted angle in radians.
         */
        static MCORE_INLINE float DegreesToRadians(float deg);

        /**
         * Sine function.
         * @param x Angle in radians.
         * @return The sine of x.
         */
        static MCORE_INLINE float Sin(float x);

        /**
         * Cosine function.
         * @param x Angle in radians.
         * @return The cosine of x.
         */
        static MCORE_INLINE float Cos(float x);

        /**
         * Tangent function.
         * @param x Angle in radians.
         * @return The tangent of x.
         */
        static MCORE_INLINE float Tan(float x);

        /**
         * Arcsine function.
         * @param x Angle in radians.
         * @return The arcsine of x.
         */
        static MCORE_INLINE float ASin(float x);

        /**
         * Arccosine function.
         * @param x Angle in radians.
         * @return The arccosine of x.
         */
        static MCORE_INLINE float ACos(float x);

        /**
         * Arctangent function.
         * @param x Angle in radians.
         * @return The arctangent of x.
         */
        static MCORE_INLINE float ATan(float x);

        /**
         * Calculate the arctangent of y/x.
         * @param y The value of y.
         * @param x The value of x.
         * @return The arctangent of y/x.
         */
        static MCORE_INLINE float ATan2(float y, float x);

        /**
         * Returns the sign of the cosine of the argument.
         */
        static MCORE_INLINE float SignOfCos(float y);

        /**
         * Returns the sign of the sine of the argument.
         */
        static MCORE_INLINE float SignOfSin(float y);

        /**
         * Calculates the exponential.
         * @param x The value.
         * @return The exponential of x.
         */
        static MCORE_INLINE float Exp(float x);

        /**
         * Calculates logarithms.
         * The log functions return the logarithm of x. If x is negative,
         * these functions return an indefinite, by default. If x is 0, they
         * return INF (infinite).
         * @param x The value.
         * @return The logarithm of x.
         */
        static MCORE_INLINE float Log(float x);

        /**
         * Calculates logarithms.
         * The log2 functions return the logarithm of x. If x is negative,
         * these functions return an indefinite, by default. If x is 0, they
         * return INF (infinite).
         * @param x The value.
         * @return The logarithm of x.
         */
        static MCORE_INLINE float Log2(float x);

        /**
         * Calculates x raised to the power of y.
         * @param base The base.
         * @param exponent The exponent.
         * @return X raised to the power of y.
         */
        static MCORE_INLINE float Pow(float base, float exponent);

        /**
         * Calculates the square root.
         * Parameter has to be nonnegative and non-zero.
         * @param x Nonnegative value.
         * @return The square root of x.
         */
        static MCORE_INLINE float Sqrt(float x);

        /**
         * Calculates the square root.
         * Parameter has to be nonnegative.
         * The difference between Sqrt and SafeSqrt is that SafeSqrt checks if the parameter value
         * is bigger than epsilon or not, otherwise it returns zero.
         * @param x Nonnegative value.
         * @return The square root of x.
         */
        static MCORE_INLINE float SafeSqrt(float x);

        /**
         * Fast square root.
         * On the PC and MSVC compiler an optimized SSE version is used.
         * On other platforms or other compilers it uses the standard square root.
         * The precision is 22 bits here.
         * @result The square root of x.
         */
        static MCORE_INLINE float FastSqrt(float x);

        /**
         * Fast square root method 2.
         * On the PC and MSVC compiler an optimized SSE version is used.
         * On other platforms or other compilers it uses the standard square root.
         * The precision is 11 bits here.
         * @result The square root of x.
         */
        static MCORE_INLINE float FastSqrt2(float x);

        /**
         * Calculates the inverse square root (1 / Sqrt(x)).
         * Parameter has to be nonnegative.
         * @param x Nonnegative value.
         * @return The inverse square root.
         */
        static MCORE_INLINE float InvSqrt(float x);

        /**
         * Calculates a fast approximation (very good approx) to 1/sqrt(x).
         * This one is more accurate than FastInvSqrt2, but a bit slower, but still much faster than the standard 1.0f / sqrt(x).
         * @param x The value in the equation 1/sqrt(x).
         * @result The inverse square root of x.
         */
        static MCORE_INLINE float FastInvSqrt(float x);

        /**
         * Calculates a fast (but less accurate) approximation to 1/sqrt(x).
         * The precision starts to show changes after the second digit in most cases.
         * @param x The value in the equation 1/sqrt(x).
         * @result The inverse square root of x.
         */
        //static MCORE_INLINE float FastInvSqrt2(float x);

        /**
         * Calculates a fast (but less accurate) approximation to 1/sqrt(x).
         * This version is more accurate (but a little slower) than the FastInvSqrt2.
         * @param x The value in the equation 1/sqrt(x).
         * @result The inverse square root of x.
         */
        //static MCORE_INLINE float FastInvSqrt3(float x);

        /**
         * Calculates the next power of two.
         * @param x The value.
         * @return The next power of two of the parameter.
         */
        static MCORE_INLINE uint32 NextPowerOfTwo(uint32 x);

        /**
         * Perform the modulus calculation on a float.
         * Please keep in mind that the value of y is not allowed to be zero.
         * If you don't know if y will be zero, use SafeFMod instead.
         * @param x The value to take the modulus on.
         * @param y The modulus value, which is NOT allowed to be zero.
         * @result The result of the calculation.
         */
        static MCORE_INLINE float FMod(float x, float y);

        /**
         * Perform the modulus calculation on a float.
         * The value of y is allowed to be zero.
         * If you know that the value of y will never be zero, please use FMod instead, as that is a bit faster.
         * @param x The value to take the modulus on.
         * @param y The modulus value, which is allowed to be zero as well.
         * @result The result of the calculation.
         */
        static MCORE_INLINE float SafeFMod(float x, float y);

        /**
         * Align a given uint32 value to a given alignment.
         * For example when the input value of inOutValue contains a value of 50, and the alignment is set to 16, then the
         * value is modified to be 64.
         * @param inOutValue The input value to align. This will also be the output, so the value is modified.
         * @param alignment The alignment to use, for example 16, 32 or 64, etc.
         */
        static MCORE_INLINE void Align(uint32* inOutValue, uint32 alignment);


        /**
         * Align a given uint32 value to a given alignment.
         * For example when the input value of inOutValue contains a value of 50, and the alignment is set to 16, then the
         * aligned return value would be 64.
         * @param inValue The input value, which would be 50 in our above example.
         * @param alignment The alignment touse, which would be 16 in our above example.
         * @result The value returned is the input value aligned to the given alignment. In our example it would return a value of 64.
         */
        static MCORE_INLINE uint32 Align(uint32 inValue, uint32 alignment);

        /**
         * Multiply a float value by its sign.
         * This basically does:
         * <pre>
         * dest = source > 0.0f ? source : -source;
         * or
         * dest = source * sgn(source);
         * </pre>
         * Using bit operations the required if statement is eliminated.
         */
        static MCORE_INLINE void MulSign(float& dest, float const& source);

        /**
         * Check if a given float is positive.
         * @param f The float to check.
         * @result Returns true when positive, otherwise false is returned.
         */
        //static MCORE_INLINE bool IsFloatPositive(float f);

        /**
         * Check if a given float is negative.
         * @param f The float to check.
         * @result Returns true when negative, otherwise false is returned.
         */
        //static MCORE_INLINE bool IsFloatNegative(float f);


    public:
        static const float pi;              /**< The value of PI, which is 3.1415926 */
        static const float twoPi;           /**< PI multiplied by two */
        static const float halfPi;          /**< PI divided by two */
        static const float invPi;           /**< 1.0 divided by PI */
        static const float epsilon;         /**< A very small number, almost equal to zero (0.000001). */
        static const float sqrtHalf;        /**< The square root of 0.5. */
        static const float sqrt2;           /**< The square root of 2. */
        static const float sqrt3;           /**< The square root of 3. */
        static const float halfSqrt2;       /**< Half the square root of two. */
        static const float halfSqrt3;       /**< Half the square root of three. */
    };

    // include the inline code
#include "FastMath.inl"
}   // namespace MCore
