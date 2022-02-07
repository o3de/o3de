/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>

namespace AZ
{
    //! A vector class with 4 components.
    //! To convert back to a Vector3, call the GetHomogenized function.
    class Vector4
    {
    public:

        AZ_TYPE_INFO(Vector4, "{0CE9FA36-1E3A-4C06-9254-B7C73A732053}");

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default constructor, components are uninitialized.
        Vector4() = default;
        Vector4(const Vector4& v);

        //! Constructs vector with all components set to the same specified value.
        explicit Vector4(float x);

        explicit Vector4(float x, float y, float z, float w);

        ///Copies x,y, components from a Vector2, z = 0, w = 1.0
        explicit Vector4(const Vector2& source);

        ///Copies x,y,z components from a Vector3, w = 1.0
        explicit Vector4(const Vector3& source);
        //! For internal use only, arrangement of values in SIMD type is not guaranteed.
        explicit Vector4(Simd::Vec4::FloatArgType value);

        //! Creates a vector with all components set to zero, more efficient than calling Vector4(0.0f).
        static Vector4 CreateZero();

        //! Creates a vector with all components set to one.
        static Vector4 CreateOne();

        static Vector4 CreateAxisX(float length = 1.0f);
        static Vector4 CreateAxisY(float length = 1.0f);
        static Vector4 CreateAxisZ(float length = 1.0f);
        static Vector4 CreateAxisW(float length = 1.0f);

        //! Sets components from an array of 4 floats, stored in xyzw order.
        static Vector4 CreateFromFloat4(const float* values);

        //! Copies x,y,z components from a Vector3, sets w to 1.0.
        static Vector4 CreateFromVector3(const Vector3& v);

        //! Copies x,y,z components from a Vector3, specify w separately.
        static Vector4 CreateFromVector3AndFloat(const Vector3& v, float w);

        //! operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component.
        static Vector4 CreateSelectCmpEqual(const Vector4& cmp1, const Vector4& cmp2, const Vector4& vA, const Vector4& vB);

        //! operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component.
        static Vector4 CreateSelectCmpGreaterEqual(const Vector4& cmp1, const Vector4& cmp2, const Vector4& vA, const Vector4& vB);

        //! operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component.
        static Vector4 CreateSelectCmpGreater(const Vector4& cmp1, const Vector4& cmp2, const Vector4& vA, const Vector4& vB);

        //! Stores the vector to an array of 4 floats. The floats need only be 4 byte aligned, 16 byte alignment is not required.
        void StoreToFloat4(float* values) const;

        float GetX() const;
        float GetY() const;
        float GetZ() const;
        float GetW() const;
        void SetX(float x);
        void SetY(float y);
        void SetZ(float z);
        void SetW(float w);

        //! Access component by index.
        float GetElement(int32_t index) const;

        //! Sets all components to the same specified value.
        void Set(float x);

        void Set(float x, float y, float z, float w);

        //! Sets components from an array of 4 floats, stored in xyzw order.
        void Set(const float values[]);

        //! Sets x,y,z components from a Vector3, sets w to 1.0.
        void Set(const Vector3& v);

        //! Sets x,y,z components from a Vector3, specify w separately.
        void Set(const Vector3& v, float w);

        //! Sets x,y,z,w components using a single simd vector4 float type.
        void Set(Simd::Vec4::FloatArgType v);

        //! We recommend using SetX,Y,Z,W. SetElement can be slower.
        void SetElement(int32_t index, float v);

        Vector3 GetAsVector3() const;

        //! Indexed access using operator().
        //! \note This is a convenience method, as it can be slower than using GetX, GetY, etc.
        float operator()(int32_t index) const;

        //! Returns squared length of the vector.
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
        Vector4 GetNormalized() const;

        //! Returns normalized vector, fast but low accuracy, uses raw estimate instructions.
        Vector4 GetNormalizedEstimate() const;

        //! Normalizes the vector in-place, full accuracy.
        void Normalize();

        //! Normalizes the vector in-place, fast but low accuracy, uses raw estimate instructions.
        void NormalizeEstimate();

        //! Normalizes the vector in-place and returns the previous length.
        //! This takes a few more instructions than calling Normalize().
        //! @{
        float NormalizeWithLength();
        float NormalizeWithLengthEstimate();
        //! @}

        //! Safe normalization, returns (0,0,0,0) if the length of the vector is too small.
        //! @{
        Vector4 GetNormalizedSafe(float tolerance = Constants::Tolerance) const;
        Vector4 GetNormalizedSafeEstimate(float tolerance = Constants::Tolerance) const;
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

        //! Returns squared distance to another Vector4.
        float GetDistanceSq(const Vector4& v) const;

        //! Returns distance to another Vector4.
        //! @{
        float GetDistance(const Vector4& v) const;
        float GetDistanceEstimate(const Vector4& v) const;
        //! @}

        //! Checks the vector is equal to another within a floating point tolerance
        bool IsClose(const Vector4& v, float tolerance = Constants::Tolerance) const;

        bool IsZero(float tolerance = Constants::FloatEpsilon) const;

        bool operator==(const Vector4& rhs) const;
        bool operator!=(const Vector4& rhs) const;

        //! Comparison functions, not implemented as operators to improve code clarity.
        //! These functions return true only if all components pass the comparison test.
        //! @{
        bool IsLessThan(const Vector4& rhs) const;
        bool IsLessEqualThan(const Vector4& rhs) const;
        bool IsGreaterThan(const Vector4& rhs) const;
        bool IsGreaterEqualThan(const Vector4& rhs) const;
        //! @}

        //! Floor/Ceil/Round functions, operate on each component individually, result will be a new Vector4.
        //! @{
        Vector4 GetFloor() const;
        Vector4 GetCeil() const;
        Vector4 GetRound() const; // Ties to even (banker's rounding)
        //! @}

        //! Min/Max functions, operate on each component individually, result will be a new Vector4.
        //! @{
        Vector4 GetMin(const Vector4& v) const;
        Vector4 GetMax(const Vector4& v) const;
        Vector4 GetClamp(const Vector4& min, const Vector4& max) const;
        //! @}

        //! Linear interpolation between this vector and a destination.
        //! @return (*this)*(1-t) + dest*t
        Vector4 Lerp(const Vector4& dest, float t) const;

        //! Spherical linear interpolation between normalized vectors.
        //! Interpolates along the great circle connecting the two vectors.
        Vector4 Slerp(const Vector4& dest, float t) const;

        //! Normalized linear interpolation between this vector and a destination.
        //! Linearly interpolates between the two vectors and normalizes the result.
        Vector4 Nlerp(const Vector4& dest, float t) const;

        //! Dot product of two vectors, uses all 4 components.
        float Dot(const Vector4& rhs) const;

        //! Dot product of two vectors, using only the x,y,z components.
        float Dot3(const Vector3& rhs) const;

        //! Homogenizes the vector, i.e. divides all components by w.
        void Homogenize();

        //! Returns the homogenized vector, i.e. divides all components by w, return value is a Vector3.
        Vector3 GetHomogenized() const;

        Vector4& operator=(const Vector4& rhs);

        Vector4 operator-() const;
        Vector4 operator+(const Vector4& rhs) const;
        Vector4 operator-(const Vector4& rhs) const;
        Vector4 operator*(const Vector4& rhs) const;
        Vector4 operator/(const Vector4& rhs) const;
        Vector4 operator*(float multiplier) const;
        Vector4 operator/(float divisor) const;

        Vector4& operator+=(const Vector4& rhs);
        Vector4& operator-=(const Vector4& rhs);
        Vector4& operator*=(const Vector4& rhs);
        Vector4& operator/=(const Vector4& rhs);
        Vector4& operator*=(float multiplier);
        Vector4& operator/=(float divisor);

        //! Gets the sine of each component.
        Vector4 GetSin() const;

        //! Gets the cosine of each component.
        Vector4 GetCos() const;

        //! Gets the sine and cosine of each component, quicker than calling GetSin and GetCos separately.
        void GetSinCos(Vector4& sin, Vector4& cos) const;

        //! Gets the arccosine of each component.
        Vector4 GetAcos() const;

        //! Gets the arctangent of each component.
        Vector4 GetAtan() const;

        //! Wraps the angle in each component into the [-pi,pi] range.
        Vector4 GetAngleMod() const;

        //! Calculates the closest angle(radians) towards the given vector with in the [0, pi] range.
        //! Note: It's unsafe if any of the vectors are (0, 0, 0, 0)
        float Angle(const Vector4& v) const;

        //! Calculates the closest angle(degrees) towards the given vector with in the [0, 180] range.
        //! Note: It's unsafe if any of the vectors are (0, 0, 0, 0)
        float AngleDeg(const Vector4& v) const;

        //! Calculates the closest angle(radians) towards the given vector with in the [0, pi] range.
        float AngleSafe(const Vector4& v) const;

        //! Calculates the closest angle(degrees) towards the given vector with in the [0, 180] range.
        float AngleSafeDeg(const Vector4& v) const;

        //! Takes the absolute value of each component of the vector.
        Vector4 GetAbs() const;

        //! Returns the reciprocal of each component of the vector.
        Vector4 GetReciprocal() const;

        //! Returns the reciprocal of each component of the vector, fast but low accuracy, uses raw estimate instructions.
        Vector4 GetReciprocalEstimate() const;

        bool IsFinite() const;

        Simd::Vec4::FloatType GetSimdValue() const;

    protected:
        union
        {
            Simd::Vec4::FloatType m_value;
            float m_values[4];

            struct
            {
                float m_x;
                float m_y;
                float m_z;
                float m_w;
            };
        };
    };
}

#include <AzCore/Math/Vector4.inl>
