/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/MathTypes.h>
#include <AzCore/RTTI/TypeInfoSimple.h>

namespace AZ
{
    class Vector3;
    class Vector4;
    class ReflectContext;

    //! 2-dimensional vector class.
    class Vector2
    {
    public:

        AZ_TYPE_INFO(Vector2, "{3D80F623-C85C-4741-90D0-E4E66164E6BF}");

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default constructor, components are uninitialized.
        Vector2() = default;
        Vector2(const Vector2& v) = default;

        //! Constructs vector with all components set to the same specified value.
        explicit Vector2(float x);

        explicit Vector2(float x, float y);

        explicit Vector2(const Vector3& source);

        explicit Vector2(const Vector4& source);

        //! For internal use only, arrangement of values in SIMD type is not guaranteed.
        explicit Vector2(Simd::Vec2::FloatArgType value);

        //! Creates a vector with all components set to zero, more efficient than calling Vector2(0.0f).
        static Vector2 CreateZero();

        //! Creates a vector with all components set to one.
        static Vector2 CreateOne();

        static Vector2 CreateAxisX(float length = 1.0f);
        static Vector2 CreateAxisY(float length = 1.0f);

        //! Sets components from an array of 2 floats, stored in xy order
        static Vector2 CreateFromFloat2(const float* values);

        //! Creates a normalized Vector2 from an angle in radians.
        static Vector2 CreateFromAngle(float angle = 0.0f);

        //! operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component.
        static Vector2 CreateSelectCmpEqual(const Vector2& cmp1, const Vector2& cmp2, const Vector2& vA, const Vector2& vB);

        //! operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component.
        static Vector2 CreateSelectCmpGreaterEqual(const Vector2& cmp1, const Vector2& cmp2, const Vector2& vA, const Vector2& vB);

        //! operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component.
        static Vector2 CreateSelectCmpGreater(const Vector2& cmp1, const Vector2& cmp2, const Vector2& vA, const Vector2& vB);

        //! Stores the vector to an array of 2 floats.
        //! The floats need only be 4 byte aligned, 16 byte alignment is not required.
        void StoreToFloat2(float* values) const;

        float GetX() const;
        float GetY() const;
        void SetX(float x);
        void SetY(float y);

        float GetElement(int index) const;
        void SetElement(int index, float value);

        void Set(float x);
        void Set(float x, float y);

        //! Indexed access using operator().
        float operator()(int index) const;

        //! Returns squared length of the vector
        float GetLengthSq() const;

        //! Returns length of the vector, full accuracy.
        float GetLength() const;

        //! Returns length of the vector, fast but low accuracy, uses raw estimate instructions.
        float GetLengthEstimate() const;

        //! Returns 1/length, full accuracy.
        float GetLengthReciprocal() const;

        //! Returns 1/length of the vector, fast but low accuracy, uses raw estimate instructions.
        float GetLengthReciprocalEstimate() const;

        //! Returns normalized vector, full accuracy.
        Vector2 GetNormalized() const;

        //! Returns normalized vector, fast but low accuracy, uses raw estimate instructions.
        Vector2 GetNormalizedEstimate() const;

        //! Normalizes the vector in-place, full accuracy.
        void Normalize();

        //! Normalizes the vector in-place, fast but low accuracy, uses raw estimate instructions.
        void NormalizeEstimate();

        //! Normalizes the vector in-place and returns the previous length.
        //! This takes a few more instructions than calling just Normalize().
        //! @{
        float NormalizeWithLength();
        float NormalizeWithLengthEstimate();
        //! @}

        //! Safe normalization, returns (0,0) if the length of the vector is too small.
        //! @{
        Vector2 GetNormalizedSafe(float tolerance = Constants::Tolerance) const;
        Vector2 GetNormalizedSafeEstimate(float tolerance = Constants::Tolerance) const;
        void NormalizeSafe(float tolerance = Constants::Tolerance);
        void NormalizeSafeEstimate(float tolerance = Constants::Tolerance);
        float NormalizeSafeWithLength(float tolerance = Constants::Tolerance);
        float NormalizeSafeWithLengthEstimate(float tolerance = Constants::Tolerance);
        //! @}

        bool IsNormalized(float tolerance = Constants::Tolerance) const;

        //! Scales the vector to have the specified length, full accuracy.
        void SetLength(float length);

        //! Scales the vector to have the specified length, fast but low accuracy, uses raw estimate instructions.
        void SetLengthEstimate(float length);

        //! Returns squared distance to another Vector2.
        float GetDistanceSq(const Vector2& v) const;

        //! Returns distance to another Vector2.
        //! @{
        float GetDistance(const Vector2& v) const;
        float GetDistanceEstimate(const Vector2& v) const;
        //! @}

        //! Linear interpolation between this vector and a destination.
        //! @return (*this)*(1-t) + dest*t
        Vector2 Lerp(const Vector2& dest, float t) const;

        //! Spherical linear interpolation between normalized vectors.
        //! Interpolates along the great circle connecting the two vectors.
        Vector2 Slerp(const Vector2& dest, float t) const;

        //! Normalized linear interpolation between this vector and a destination.
        //! Linearly interpolates between the two vectors and normalizes the result.
        Vector2 Nlerp(const Vector2& dest, float t) const;

        //! Gets perpendicular vector, i.e. rotates through 90 degrees.
        //! The positive rotation direction is defined such that the x-axis is rotated into the y-axis.
        Vector2 GetPerpendicular() const;

        //! Checks the vector is equal to another within a floating point tolerance.
        bool IsClose(const Vector2& v, float tolerance = 0.001f) const;

        bool IsZero(float tolerance = AZ::Constants::FloatEpsilon) const;

        bool operator==(const Vector2& rhs) const;
        bool operator!=(const Vector2& rhs) const;

        //! Comparison functions, not implemented as operators since that would probably be a little dangerous.
        //! These functions return true only if all components pass the comparison test.
        //! @{
        bool IsLessThan(const Vector2& v) const;
        bool IsLessEqualThan(const Vector2& v) const;
        bool IsGreaterThan(const Vector2& v) const;
        bool IsGreaterEqualThan(const Vector2& v) const;
        //! @}

        //! Floor/Ceil/Round functions, operate on each component individually, result will be a new Vector2.
        //! @{
        Vector2 GetFloor() const;
        Vector2 GetCeil() const;
        Vector2 GetRound() const; // Ties to even (banker's rounding)
        //! @}

        //! Min/Max functions, operate on each component individually, result will be a new Vector2.
        //! @{
        Vector2 GetMin(const Vector2& v) const;
        Vector2 GetMax(const Vector2& v) const;
        Vector2 GetClamp(const Vector2& min, const Vector2& max) const;
        //! @}

        //! Vector select operation (vR = (vCmp==0) ? vA : vB).
        //! This is a special case (from CmpEqual) and it's a lot faster
        Vector2 GetSelect(const Vector2& vCmp, const Vector2& vB);

        void Select(const Vector2& vCmp, const Vector2& vB);

        Vector2 GetAbs() const;

        Vector2& operator+=(const Vector2& rhs);
        Vector2& operator-=(const Vector2& rhs);
        Vector2& operator*=(const Vector2& rhs);
        Vector2& operator/=(const Vector2& rhs);
        Vector2& operator*=(float multiplier);
        Vector2& operator/=(float divisor);

        Vector2 operator-() const;
        Vector2 operator+(const Vector2& rhs) const;
        Vector2 operator-(const Vector2& rhs) const;
        Vector2 operator*(const Vector2& rhs) const;
        Vector2 operator*(float multiplier) const;
        Vector2 operator/(float divisor) const;
        Vector2 operator/(const Vector2& rhs) const;

        //! Gets the sine of each component.
        Vector2 GetSin() const;

        //! Gets the cosine of each component.
        Vector2 GetCos() const;

        //! Gets the sine and cosine of each component, quicker than calling GetSin and GetCos separately.
        void GetSinCos(Vector2& sin, Vector2& cos) const;

        //! Gets the arccosine of each component.
        Vector2 GetAcos() const;

        //! Gets the arctangent of each component.
        Vector2 GetAtan() const;

        //! Gets the angle between the positive x-axis and the ray to the point (x,y). (-pi,pi].
        float GetAtan2() const;

        //! Wraps the angle in each component into the [-pi,pi] range.
        Vector2 GetAngleMod() const;

        //! Calculates the closest angle(radians) towards the given vector with in the [0, pi] range.
        //! Note: It's unsafe if any of the vectors are (0, 0)
        float Angle(const Vector2& v) const;

        //! Calculates the closest angle(degrees) towards the given vector with in the [0, 180] range.
        //! Note: It's unsafe if any of the vectors are (0, 0)
        float AngleDeg(const Vector2& v) const;

        //! Calculates the closest angle(radians) towards the given vector with in the [0, pi] range.
        float AngleSafe(const Vector2& v) const;

        //! Calculates the closest angle(degrees) towards the given vector with in the [0, 180] range.
        float AngleSafeDeg(const Vector2& v) const;

        //! Returns the reciprocal of each component of the vector, full accuracy.
        Vector2 GetReciprocal() const;

        //! Returns the reciprocal of each component of the vector, fast but low accuracy, uses raw estimate instructions.
        Vector2 GetReciprocalEstimate() const;

        //! Dot product of two vectors.
        float Dot(const Vector2& rhs) const;

        //! Perform multiply-add operator (this = this * mul + add).
        Vector2 GetMadd(const Vector2& mul, const Vector2& add);

        void Madd(const Vector2& mul, const Vector2& add);

        //! Project vector onto another.
        //! P = (a.Dot(b) / b.Dot(b)) * b
        void Project(const Vector2& rhs);

        //! Project vector onto a normal (faster function).
        //! P = (v.Dot(Normal) * normal)
        void ProjectOnNormal(const Vector2& normal);

        //! Project this vector onto another and return the resulting projection.
        //! P = (a.Dot(b) / b.Dot(b)) * b
        Vector2 GetProjected(const Vector2& rhs) const;

        //! Project this vector onto a normal and return the resulting projection.
        //! P = (v.Dot(Normal) * normal)
        Vector2 GetProjectedOnNormal(const Vector2& normal);

        //! Returns true if the vector contains no nan or inf values, false if at least one element is not finite.
        bool IsFinite() const;

        //! Returns the underlying SIMD vector.
        Simd::Vec2::FloatType GetSimdValue() const;

        //! Directly sets the underlying SIMD vector.
        void SetSimdValue(Simd::Vec2::FloatArgType value);

    private:

        union
        {
            Simd::Vec2::FloatType m_value;
            float m_values[2];

            struct
            {
                float m_x;
                float m_y;
            };
        };
    };

    //! Allows pre-multiplying by a float.
    Vector2 operator*(float multiplier, const Vector2& rhs);
}

#include <AzCore/Math/Vector2.inl>
