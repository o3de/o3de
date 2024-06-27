/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/math.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/std/typetraits/is_integral.h>
#include <AzCore/std/typetraits/is_signed.h>
#include <AzCore/std/typetraits/is_unsigned.h>
#include <AzCore/std/utils.h>

#include <float.h>
#include <limits>
#include <math.h>
#include <utility>
#include <inttypes.h>

// We have a separate inline define for math functions.
// The performance of these functions is very sensitive to inlining, and some compilers don't deal well with this.
#define AZ_MATH_INLINE AZ_FORCE_INLINE

// We have a separate macro for asserts within the math functions.
// Profile leaves asserts enabled, but in many cases we need highly performant profile builds (editor).
// This allows us to disable math asserts independently of asserts throughout the rest of the engine.
#ifdef AZ_DEBUG_BUILD
#   define AZ_MATH_ASSERT AZ_Assert
#else
#   define AZ_MATH_ASSERT(...)
#endif

namespace AZ
{
    class Quaternion;
    class SimpleLcgRandom;

    //! Important constants.
    namespace Constants
    {
        static constexpr float Pi = 3.14159265358979323846f;
        static constexpr float TwoPi = 6.28318530717958647692f;
        static constexpr float HalfPi = 1.57079632679489661923f;
        static constexpr float QuarterPi = 0.78539816339744830962f;
        static constexpr float TwoOverPi = 0.63661977236758134308f;
        static constexpr float MaxFloatBeforePrecisionLoss = 100000.f;
        static constexpr float Tolerance = 0.001f;
        static constexpr float FloatMax = FLT_MAX;
        static constexpr float FloatEpsilon = FLT_EPSILON;

        enum class Axis : AZ::u8
        {
            XPositive,
            XNegative,
            YPositive,
            YNegative,
            ZPositive,
            ZNegative
        };
    }

    //! Compile-time test for power of two, useful for static asserts
    //! usage: IsPowerOfTwo<(thing you're testing)>::Value
    constexpr bool IsPowerOfTwo(uint32_t testValue)
    {
        return (testValue & (testValue - 1)) == 0;
    }

    //! Calculates at compile-time the integral log base 2 of the input value.
    constexpr uint32_t Log2(uint64_t maxValue)
    {
        uint32_t bits = 0;
        do
        {
            maxValue >>= 1;
            ++bits;
        } while (maxValue > 0);
        return bits;
    }

    //! Calculates at compile-time the number of bytes required to represent the given set of input bits.
    template <uint64_t MAX_VALUE>
    constexpr uint32_t RequiredBytesForBitfield()
    {
        static_assert(MAX_VALUE <= 64, "Invalid value");
        return (MAX_VALUE > 32) ? 8
             : (MAX_VALUE > 16) ? 4
             : (MAX_VALUE >  8) ? 2
             : 1;
    }

    //! Calculates at compile-time the number of bytes required to represent the given max value.
    template <uint64_t MAX_VALUE>
    constexpr uint32_t RequiredBytesForValue()
    {
        static_assert(MAX_VALUE < 0xFFFFFFFFFFFFFFFF, "Invalid value");
        return (MAX_VALUE > 0x00000000FFFFFFFF) ? 8
             : (MAX_VALUE > 0x000000000000FFFF) ? 4
             : (MAX_VALUE > 0x00000000000000FF) ? 2
             : 1;
    }

    //! Compile-time check of address alignment.
    template <uint32_t Alignment>
    AZ_MATH_INLINE bool IsAligned(const void* addr)
    {
        static_assert(IsPowerOfTwo(Alignment), "Alignment must be a power of two");
        return (reinterpret_cast<uintptr_t>(addr) & (Alignment - 1)) == 0;
    }

    template <uint32_t BYTE_COUNT, bool IS_SIGNED = false> struct SizeType { };
    template <> struct SizeType<8, false> { using Type = uint64_t; };
    template <> struct SizeType<7, false> { using Type = uint64_t; };
    template <> struct SizeType<6, false> { using Type = uint64_t; };
    template <> struct SizeType<5, false> { using Type = uint64_t; };
    template <> struct SizeType<4, false> { using Type = uint32_t; };
    template <> struct SizeType<3, false> { using Type = uint32_t; };
    template <> struct SizeType<2, false> { using Type = uint16_t; };
    template <> struct SizeType<1, false> { using Type =  uint8_t; };
    template <> struct SizeType<0, false> { using Type =  uint8_t; };
    template <> struct SizeType<8, true > { using Type =  int64_t; };
    template <> struct SizeType<7, true > { using Type =  int64_t; };
    template <> struct SizeType<6, true > { using Type =  int64_t; };
    template <> struct SizeType<5, true > { using Type =  int64_t; };
    template <> struct SizeType<4, true > { using Type =  int32_t; };
    template <> struct SizeType<3, true > { using Type =  int32_t; };
    template <> struct SizeType<2, true > { using Type =  int16_t; };
    template <> struct SizeType<1, true > { using Type =   int8_t; };
    template <> struct SizeType<0, true > { using Type =   int8_t; };

    //! Enumerations to describe the sign and size relationship between two integer types.
    enum class IntegralTypeDiff
    {
        //! Left integer is signed, right integer is signed, both are equal in size
        LSignedRSignedEqSize,

        //! Left integer is signed, right integer is unsigned, both are equal in size
        LSignedRUnsignedEqSize,

        //! Left integer is unsigned, right integer is signed, both are equal in size
        LUnsignedRSignedEqSize,

        //! Left integer is unsigned, right integer is unsigned, both are equal in size
        LUnsignedRUnsignedEqSize,

        //! Left integer is signed, right integer is signed, left integer is wider
        LSignedRSignedLWider,

        //! Left integer is signed, right integer is unsigned, left integer is wider
        LSignedRUnsignedLWider,

        //! Left integer is unsigned, right integer is signed, left integer is wider
        LUnsignedRSignedLWider,

        //! Left integer is unsigned, right integer is unsigned, left integer is wider
        LUnsignedRUnsignedLWider,

        //! Left integer is signed, right integer is signed, right integer is wider
        LSignedRSignedRWider,

        //! Left integer is signed, right integer is unsigned, right integer is wider
        LSignedRUnsignedRWider,

        //! Left integer is unsigned, right integer is signed, right integer is wider
        LUnsignedRSignedRWider,

        //! Left integer is unsigned, right integer is unsigned, right integer is wider
        LUnsignedRUnsignedRWider,
    };

    //! Compares two integer types and returns their sign and size relationship.
    template <typename LeftType, typename RightType>
    constexpr IntegralTypeDiff IntegralTypeCompare()
    {
        static_assert(AZStd::is_signed<LeftType>::value || AZStd::is_unsigned<LeftType>::value, "LeftType must either be a signed or unsigned integral type");
        static_assert(AZStd::is_signed<RightType>::value || AZStd::is_unsigned<RightType>::value, "RightType must either be a signed or unsigned integral type");

        if constexpr (sizeof(LeftType) < sizeof(RightType))
        {
            if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRSignedRWider;
            }
            else if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_unsigned<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRUnsignedRWider;
            }
            else if constexpr(AZStd::is_unsigned<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LUnsignedRSignedRWider;
            }
            else
            {
                return IntegralTypeDiff::LUnsignedRUnsignedRWider;
            }
        }
        else if constexpr (sizeof(LeftType) > sizeof(RightType))
        {
            if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRSignedLWider;
            }
            else if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_unsigned<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRUnsignedLWider;
            }
            else if constexpr(AZStd::is_unsigned<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LUnsignedRSignedLWider;
            }
            else
            {
                return IntegralTypeDiff::LUnsignedRUnsignedLWider;
            }
        }
        else
        {
            if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRSignedEqSize;
            }
            else if constexpr(AZStd::is_signed<LeftType>::value && AZStd::is_unsigned<RightType>::value)
            {
                return IntegralTypeDiff::LSignedRUnsignedEqSize;
            }
            else if constexpr(AZStd::is_unsigned<LeftType>::value && AZStd::is_signed<RightType>::value)
            {
                return IntegralTypeDiff::LUnsignedRSignedEqSize;
            }
            else
            {
                return IntegralTypeDiff::LUnsignedRUnsignedEqSize;
            }
        }
    }

    //! Comparison result returned by SafeIntegralCompare.
    enum class IntegralCompare
    {
        LessThan,
        Equal,
        GreaterThan
    };

    //! Safely compares two integers of any sign and width combination and returns an IntegralCompare result
    //! that is the whether lhs is less than, equal to or greater than rhs.
    template<typename LeftType, typename RightType>
    constexpr IntegralCompare SafeIntegralCompare(LeftType lhs, RightType rhs);

    //! A collection of methods for clamping and constraining integer values and ranges to that of a reference integer type.
    template<typename SourceType, typename ClampType>
    struct ClampedIntegralLimits
    {
        //! If SourceType and ClampType are different, returns the greater value of 
        //! AZStd::numeric_limits<SourceType>::lowest() and AZStd::numeric_limits<ClampType>::lowest(),
        //! otherwise returns AZStd::numeric_limits<SourceType>::lowest().
        static constexpr SourceType Min();

        //! If SourceType and ClampType are different, returns the lesser value of 
        //! AZStd::numeric_limits<SourceType>::max() and AZStd::numeric_limits<ClampType>::max(),
        //! otherwise returns AZStd::numeric_limits<SourceType>::max().
        static constexpr SourceType Max();

        //! Safely clamps a value of type ValueType to the [Min(), Max()] range as determined by the
        //! numerical limit constraint rules described by Min() and Max() given SourceType and ClampType.
        //! @param first component is the clamped value which may or may not equal the original value.
        //! @param second is a Boolean flag signifying whether or not the the value was required to be clamped
        //! in order to stay within ClampType's numerical range.
        template<typename ValueType>
        static constexpr AZStd::pair<SourceType, bool> Clamp(ValueType value);        
    };

    //! Converts radians to degrees.
    constexpr float RadToDeg(float rad)
    {
        return rad * 180.0f / Constants::Pi;
    }

    //! Converts degrees to radians.
    constexpr float DegToRad(float deg)
    {
        return deg * Constants::Pi / 180.0f;
    }

    //! Simd optimized math functions.
    //! @{
    float Abs(float value);
    float Mod(float value, float divisor);
    float Wrap(float value, float maxValue);
    float Wrap(float value, float minValue, float maxValue);
    float AngleMod(float value);
    void SinCos(float angle, float& sin, float& cos);
    float Sin(float angle);
    float Cos(float angle);
    float Acos(float value);
    float Atan(float value);
    float Atan2(float y, float x);
    float Sqrt(float value);
    float InvSqrt(float value);
    //! @}

    AZ_MATH_INLINE bool IsClose(float a, float b, float tolerance = Constants::Tolerance)
    {
        return (AZStd::abs(a - b) <= tolerance);
    }

    AZ_MATH_INLINE bool IsClose(double a, double b, double tolerance = Constants::Tolerance)
    {
        return (AZStd::abs(a - b) <= tolerance);
    }

    //! Returns x >= 0.0f ? 1.0f : -1.0f.
    AZ_MATH_INLINE float GetSign(float x)
    {
        union FloatInt
        {
            float           f;
            unsigned int    u;
        } fi;
        fi.f = x;
        fi.u &= 0x80000000; // preserve sign
        fi.u |= 0x3f800000; // 1
        return fi.f;
    }

    //! Returns the clamped value.
    template<typename T>
    constexpr T GetClamp(T value, T min, T max)
    {
        if (value < min)
        {
            return min;
        }
        else if (value > max)
        {
            return max;
        }
        else
        {
            return value;
        }
    }

    //! Return the smaller of the 2 values.
    //! \note we don't use names like clamp, min and max because many implementations define it as a macro.
    template<typename T>
    constexpr T GetMin(T a, T b)
    {
        return a < b ? a : b;
    }

    //! Return the bigger of the 2 values.
    //! \note we don't use names like clamp, min and max because many implementations define it as a macro.
    template<typename T>
    constexpr T GetMax(T a, T b)
    {
        return a > b ? a : b;
    }

    //! Return the gcd of 2 values.
    //! \note we don't use names like clamp, min and max because many implementations define it as a macro.
    template<typename T>
    constexpr T GetGCD(T a, T b)
    {
        static_assert(std::is_integral<T>::value);
        T c = a % b;
        while (c != 0)
        {
            a = b;
            b = c;
            c = a % b;
        }
        return b;
    }

    //! Returns a linear interpolation between 2 values.
    template<typename T>
    constexpr T Lerp(const T& a, const T& b, float t)
    {
        return static_cast<T>(a + (b - a) * t);
    }

    //! Returns a linear interpolation between 2 values.
    constexpr float Lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    constexpr double Lerp(double a, double b, double t)
    {
        return a + (b - a) * t;
    }

    //! Returns a value t where Lerp(a, b, t) == value (or 0 if a == b).
    inline float LerpInverse(float a, float b, float value)
    {
        return IsClose(a, b, AZStd::numeric_limits<float>::epsilon()) ? 0.0f : (value - a) / (b - a);
    }

    inline double LerpInverse(double a, double b, double value)
    {
        return IsClose(a, b, AZStd::numeric_limits<double>::epsilon()) ? 0.0 : (value - a) / (b - a);
    }

    //! Returns true if the number provided is even.
    template<typename T>
    constexpr AZStd::enable_if_t<AZStd::is_integral<T>::value, bool> IsEven(T a)
    {
        return a % 2 == 0;
    }

    //! Returns true if the number provided is odd.
    template<typename T>
    constexpr AZStd::enable_if_t<AZStd::is_integral<T>::value, bool> IsOdd(T a)
    {
        return a % 2 != 0;
    }

    AZ_MATH_INLINE float GetAbs(float a)
    {
        return AZStd::abs(a);
    }

    AZ_MATH_INLINE double GetAbs(double a)
    {
        return AZStd::abs(a);
    }

    AZ_MATH_INLINE float GetMod(float a, float b)
    {
        return Mod(a, b);
    }

    AZ_MATH_INLINE double GetMod(double a, double b)
    {
        return fmod(a, b);
    }

    //! Wraps [0, maxValue].
    AZ_MATH_INLINE int Wrap(int value, int maxValue)
    {
        return (value % maxValue) + ((value < 0) ? maxValue : 0);
    }

    //! Wraps [minValue, maxValue].
    AZ_MATH_INLINE int Wrap(int value, int minValue, int maxValue)
    {
        return Wrap(value - minValue, maxValue - minValue) + minValue;
    }

    AZ_MATH_INLINE float GetFloatQNaN()
    {
        return AZStd::numeric_limits<float>::quiet_NaN();
    }

    //! IsCloseMag(x, y, epsilon) returns true if y and x are sufficiently close, taking magnitude of x and y into account in the epsilon
    template<typename T>
    AZ_MATH_INLINE bool IsCloseMag(T x, T y, T epsilonValue = AZStd::numeric_limits<T>::epsilon())
    {
        return (AZStd::abs(x - y) <= epsilonValue * GetMax<T>(GetMax<T>(T(1.0), AZStd::abs(x)), AZStd::abs(y)));
    }

    //! ClampIfCloseMag(x, y, epsilon) returns y when x and y are within epsilon of each other (taking magnitude into account).  Otherwise returns x.
    template<typename T>
    AZ_MATH_INLINE T ClampIfCloseMag(T x, T y, T epsilonValue = AZStd::numeric_limits<T>::epsilon())
    {
        return IsCloseMag<T>(x, y, epsilonValue) ? y : x;
    }

    // This wrapper function exists here to ensure the correct version of isnormal is used on certain platforms (namely Android) because of the
    // math.h and cmath include order can be somewhat dangerous.  For example, cmath undefines isnormal (and a number of other C99 math macros 
    // defined in math.h) in favor of their std:: variants.
    AZ_MATH_INLINE bool IsNormalDouble(double x)
    {
        return std::isnormal(x);
    }

    AZ_MATH_INLINE bool IsFiniteFloat(float x)
    {
        return (azisfinite(x) != 0);
    }

    //! Returns the value divided by alignment, where the result is rounded up if the remainder is non-zero.
    //! Example: alignment: 4
    //! Value:  0 1 2 3 4 5 6 7 8
    //! Result: 0 1 1 1 1 2 2 2 2
    //! @param value The numerator.
    //! @param alignment The denominator. The result will be rounded up to the nearest multiple of alignment. Must be non-zero or it will lead to undefined behavior
    template<typename T>
    constexpr T DivideAndRoundUp(T value, T alignment)
    {
        AZ_MATH_ASSERT(alignment != 0, "0 is an invalid multiple to round to.");
        AZ_MATH_ASSERT(
            AZStd::numeric_limits<T>::max() - value >= alignment,
            "value and alignment will overflow when added together during DivideAndRoundUp.");
        return (value + alignment - 1) / alignment;
    }
    
    //! Returns the value rounded up to a multiple of alignment.
    //! This function will work for non power of two alignments.
    //! If your alignment is guaranteed to be a power of two, SizeAlignUp in base.h is a more efficient implementation.
    //! Example: roundTo: 4
    //! Value:  0 1 2 3 4 5 6 7 8
    //! Result: 0 4 4 4 4 8 8 8 8
    //! @param value The number to round up.
    //! @param alignment Round up to the nearest multiple of alignment. Must be non-zero or it will lead to undefined behavior
    template<typename T>
    constexpr T RoundUpToMultiple(T value, T alignment)
    {
        return DivideAndRoundUp(value, alignment) * alignment;
    }

    //! Returns the maximum value for SourceType as constrained by the numerical range of ClampType.
    template <typename SourceType, typename ClampType>    
    constexpr SourceType ClampedIntegralLimits<SourceType, ClampType>::Min()
    {
        if constexpr (AZStd::is_unsigned<ClampType>::value || AZStd::is_unsigned<SourceType>::value)
        {
            // If either SourceType or ClampType is unsigned, the lower limit will always be 0
            return 0;
        }
        else
        {
            // Both SourceType and ClampType are signed, take the greater of the lower limits of each type
            return sizeof(SourceType) < sizeof(ClampType) ? 
                (AZStd::numeric_limits<SourceType>::lowest)() : 
                static_cast<SourceType>((AZStd::numeric_limits<ClampType>::lowest)());
        }
    }

    //! Returns the minimum value for SourceType as constrained by the numerical range of ClampType.
    template <typename SourceType, typename ClampType>
    constexpr SourceType ClampedIntegralLimits<SourceType, ClampType>::Max()
    {
        if constexpr (sizeof(SourceType) < sizeof(ClampType))
        {
            // If SourceType is narrower than ClampType, the upper limit will be SourceType's
            return (AZStd::numeric_limits<SourceType>::max)();
        }
        else if constexpr (sizeof(SourceType) > sizeof(ClampType))
        {
            // If SourceType is wider than ClampType, the upper limit will be ClampType's
            return static_cast<SourceType>((AZStd::numeric_limits<ClampType>::max)());
        }
        else
        {
            if constexpr (AZStd::is_signed<ClampType>::value)
            {
                // SourceType and ClampType are the same width, ClampType is signed
                // so our upper limit will be ClampType
                return static_cast<SourceType>((AZStd::numeric_limits<ClampType>::max)());
            }       
            else
            {
                // SourceType and ClampType are the same width, ClampType is unsigned
                // then our upper limit will be SourceType
                return (AZStd::numeric_limits<SourceType>::max)();
            }
        }
    }

    template <typename SourceType, typename ClampType>
    template <typename ValueType>
    constexpr AZStd::pair<SourceType, bool> ClampedIntegralLimits<SourceType, ClampType>::Clamp(ValueType value)
    {
        if (SafeIntegralCompare(value, Min()) == IntegralCompare::LessThan)
        {
            return { Min(), true };
        }
        if (SafeIntegralCompare(value, Max()) == IntegralCompare::GreaterThan)
        {
            return { Max(), true };
        }
        else
        {
            return { static_cast<SourceType>(value), false };
        }
    }

    template <typename LeftType, typename RightType>
    constexpr IntegralCompare SafeIntegralCompare(LeftType lhs, RightType rhs)
    {
        constexpr bool LeftSigned = AZStd::is_signed<LeftType>::value;
        constexpr bool RightSigned = AZStd::is_signed<RightType>::value;
        constexpr std::size_t LeftTypeSize = sizeof(LeftType);
        constexpr std::size_t RightTypeSize = sizeof(RightType);

        static_assert(AZStd::is_integral<LeftType>::value && AZStd::is_integral<RightType>::value,
                      "Template parameters must be integrals");

        auto comp = [lhs](auto value) -> IntegralCompare
        {
            if (lhs < value)
            {
                return IntegralCompare::LessThan;
            }
            else if (lhs > value)
            {
                return IntegralCompare::GreaterThan;
            }
            else
            {
                return IntegralCompare::Equal;
            }
        };

        if constexpr (LeftSigned == RightSigned)
        {
            return comp(rhs);
        }
        else if constexpr (LeftTypeSize > RightTypeSize)
        {
            if constexpr (LeftSigned)
            {
                // LeftTypeSize > RightTypeSize
                // LeftType is signed
                // RightType is unsigned
                return comp(static_cast<LeftType>(rhs));
            }
            else
            {
                // LeftTypeSize > RightTypeSize
                // LeftType is unsigned
                // RightType is signed
                if (rhs < 0)
                {
                    return IntegralCompare::GreaterThan;
                }
                else
                {
                    return comp(static_cast<LeftType>(rhs));
                }
            }
        }
        else if constexpr (LeftSigned)
        {
            // LeftTypeSize <= RightTypeSize
            // LeftType is signed
            // RightType is unsigned
            RightType max = static_cast<RightType>((AZStd::numeric_limits<LeftType>::max)());

            if (rhs > max)
            {
                return IntegralCompare::LessThan;
            }
            else
            {
                return comp(static_cast<LeftType>(rhs));
            }
        }
        else if constexpr (LeftTypeSize < RightTypeSize)
        {
            // LeftType < RightType
            // LeftType is unsigned
            // RightType is signed
            RightType max = static_cast<RightType>((AZStd::numeric_limits<LeftType>::max)());

            if (rhs < 0)
            {
                return IntegralCompare::GreaterThan;
            }
            else if (rhs > max)
            {
                return IntegralCompare::LessThan;
            }
            else
            {
                return comp(static_cast<LeftType>(rhs));
            }
        }
        else
        {
            // LeftType == RightType
            // LeftType is unsigned
            // RightType is signed
            if (rhs < 0)
            {
                return IntegralCompare::GreaterThan;
            }
            else
            {
                return comp(static_cast<LeftType>(rhs));
            }
        }
    }

    //! Creates a unit quaternion uniformly sampled from the space of all possible rotations.
    //! See Graphics Gems III, chapter 6.
    Quaternion CreateRandomQuaternion(SimpleLcgRandom& rng);
}
