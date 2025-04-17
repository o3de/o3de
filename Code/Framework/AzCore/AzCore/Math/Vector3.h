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
    class Vector2;
    class Vector4;
    class Transform;
    class Quaternion;
    class Matrix3x3;
    class Matrix3x4;
    class Matrix4x4;
    class ReflectContext;

    //! 3-dimensional vector class.
    class Vector3
    {
    public:

        AZ_TYPE_INFO(Vector3, "{8379EB7D-01FA-4538-B64B-A6543B4BE73D}");

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default constructor, components are uninitialized.
        Vector3() = default;
        Vector3(const Vector3& v) = default;

        //! Constructs vector with all components set to the same specified value.
        explicit Vector3(float x);

        Vector3(float x, float y, float z);

        //! Sets x,y components from a Vector2, sets z to 0.0.
        explicit Vector3(const Vector2& source);

        //! Sets x,y components from a Vector2, specify z separately.
        Vector3(const Vector2& source, float z);

        //! Sets x,y,z components from a Vector4.
        explicit Vector3(const Vector4& source);

        //! For internal use only, arrangement of values in SIMD type is not guaranteed.
        explicit Vector3(Simd::Vec3::FloatArgType value);

        //! Creates a vector with all components set to zero, more efficient than calling Vector3(0.0f).
        static Vector3 CreateZero();

        //! Creates a vector with all components set to one.
        static Vector3 CreateOne();

        static Vector3 CreateAxisX(float length = 1.0f);
        static Vector3 CreateAxisY(float length = 1.0f);
        static Vector3 CreateAxisZ(float length = 1.0f);

        //! Sets components from an array of 3 floats, stored in xyz order
        static Vector3 CreateFromFloat3(const float* values);

        //! operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        static Vector3 CreateSelectCmpEqual(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB);

        //! operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        static Vector3 CreateSelectCmpGreaterEqual(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB);

        //! operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        static Vector3 CreateSelectCmpGreater(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB);

        //! Stores the vector to an array of 3 floats.
        //! The floats need only be 4 byte aligned, 16 byte alignment is NOT required.
        //! Note that StoreToFloat4 is usually faster, as the extra padding allows more efficient instructions to be used.
        void StoreToFloat3(float* values) const;

        //! Stores the vector to an array of 4 floats.
        //! The floats need only be 4 byte aligned, 16 byte alignment is NOT required.
        //! The 4th float is generally set to garbage, but its presence allows more efficient instructions to be used.
        void StoreToFloat4(float* values) const;

        float GetX() const;
        float GetY() const;
        float GetZ() const;
        void SetX(float x);
        void SetY(float y);
        void SetZ(float z);

        //! We recommend using GetX,Y,Z. GetElement can be slower.
        float GetElement(int32_t index) const;

        //! We recommend using SetX,Y,Z. SetElement can be slower.
        void SetElement(int32_t index, float v);

        //! Sets all components to the same specified value.
        void Set(float x);

        void Set(float x, float y, float z);

        //! Sets components from an array of 3 floats in xyz order.
        void Set(const float values[]);

        //! Indexed access using operator(), just for convenience.
        float operator()(int32_t index) const;

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
        Vector3 GetNormalized() const;

        //! Returns normalized vector, fast but low accuracy, uses raw estimate instructions.
        Vector3 GetNormalizedEstimate() const;

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

        //! Safe normalization, returns (0,0,0) if the length of the vector is too small.
        //! @{
        Vector3 GetNormalizedSafe(float tolerance = Constants::Tolerance) const;
        Vector3 GetNormalizedSafeEstimate(float tolerance = Constants::Tolerance) const;
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

        //! Returns squared distance to another Vector3.
        float GetDistanceSq(const Vector3& v) const;

        //! Returns distance to another Vector3.
        //! @{
        float GetDistance(const Vector3& v) const;
        float GetDistanceEstimate(const Vector3& v) const;
        //! @}

        //! Linear interpolation between this vector and a destination.
        //! @return (*this)*(1-t) + dest*t
        Vector3 Lerp(const Vector3& dest, float t) const;

        //! Spherical linear interpolation between normalized vectors.
        //! Interpolates along the great circle connecting the two vectors.
        Vector3 Slerp(const Vector3& dest, float t) const;

        //! Normalized linear interpolation between this vector and a destination.
        //! Linearly interpolates between the two vectors and normalizes the result.
        Vector3 Nlerp(const Vector3& dest, float t) const;

        //! Dot product of two vectors.
        float Dot(const Vector3& rhs) const;

        //! Cross product of two vectors.
        Vector3 Cross(const Vector3& rhs) const;

        //! Cross product with specified axis.
        //! @{
        Vector3 CrossXAxis() const;
        Vector3 CrossYAxis() const;
        Vector3 CrossZAxis() const;
        Vector3 XAxisCross() const;
        Vector3 YAxisCross() const;
        Vector3 ZAxisCross() const;
        //! @}

        //! Checks the vector is equal to another within a floating point tolerance.
        bool IsClose(const Vector3& v, float tolerance = Constants::Tolerance) const;

        bool IsZero(float tolerance = Constants::FloatEpsilon) const;

        bool operator==(const Vector3& rhs) const;
        bool operator!=(const Vector3& rhs) const;

        //! Comparison functions, not implemented as operators since that would probably be a little dangerous.
        //! These functions return true only if all components pass the comparison test.
        //! @{
        bool IsLessThan(const Vector3& rhs) const;
        bool IsLessEqualThan(const Vector3& rhs) const;
        bool IsGreaterThan(const Vector3& rhs) const;
        bool IsGreaterEqualThan(const Vector3& rhs) const;
        //! @}

        //! Floor/Ceil/Round functions, operate on each component individually, result will be a new Vector3.
        //! @{
        Vector3 GetFloor() const;
        Vector3 GetCeil() const;
        Vector3 GetRound() const; // Ties to even (banker's rounding)
        //! @}

        //! Min/Max functions, operate on each component individually, result will be a new Vector3.
        //! @{
        Vector3 GetMin(const Vector3& v) const;
        Vector3 GetMax(const Vector3& v) const;
        Vector3 GetClamp(const Vector3& min, const Vector3& max) const;
        //! @}

        float GetMaxElement() const;
        float GetMinElement() const;

        Vector3 operator-() const;
        Vector3 operator+(const Vector3& rhs) const;
        Vector3 operator-(const Vector3& rhs) const;
        Vector3 operator*(const Vector3& rhs) const;
        Vector3 operator/(const Vector3& rhs) const;
        Vector3 operator*(float multiplier) const;
        Vector3 operator/(float divisor) const;

        Vector3& operator+=(const Vector3& rhs);
        Vector3& operator-=(const Vector3& rhs);
        Vector3& operator*=(const Vector3& rhs);
        Vector3& operator/=(const Vector3& rhs);
        Vector3& operator*=(float multiplier);
        Vector3& operator/=(float divisor);

        //! Gets the sine of each component.
        Vector3 GetSin() const;

        //! Gets the cosine of each component.
        Vector3 GetCos() const;

        //! Gets the sine and cosine of each component, quicker than calling GetSin and GetCos separately.
        void GetSinCos(Vector3& sin, Vector3& cos) const;

        //! Gets the arccosine of each component.
        Vector3 GetAcos() const;

        //! Gets the arctangent of each component.
        Vector3 GetAtan() const;

        //! Wraps the angle in each component into the [-pi,pi] range
        Vector3 GetAngleMod() const;

        //! Calculates the closest angle(radians) towards the given vector with in the [0, pi] range.
        //! Note: It's unsafe if any of the vectors are (0, 0, 0)
        float Angle(const Vector3& v) const;

        //! Calculates the closest angle(degrees) towards the given vector with in the [0, 180] range.
        //! Note: It's unsafe if any of the vectors are (0, 0, 0)
        float AngleDeg(const Vector3& v) const;

        //! Calculates the closest angle(radians) towards the given vector with in the [0, pi] range.
        float AngleSafe(const Vector3& v) const;

        //! Calculates the closest angle(degrees) towards the given vector with in the [0, 180] range.
        float AngleSafeDeg(const Vector3& v) const;

        //! Takes the absolute value of each component of the vector.
        Vector3 GetAbs() const;

        //! Returns the reciprocal of each component of the vector, full accuracy.
        Vector3 GetReciprocal() const;

        //! Returns the reciprocal of each component of the vector, fast but low accuracy, uses raw estimate instructions.
        Vector3 GetReciprocalEstimate() const;

        //! Builds a tangent basis with this vector as the normal.
        void BuildTangentBasis(Vector3& tangent, Vector3& bitangent) const;

        //! Perform multiply-add operator, returns this * mul + add.
        Vector3 GetMadd(const Vector3& mul, const Vector3& add);

        //! Perform multiply-add operator (this = this * mul + add).
        void Madd(const Vector3& mul, const Vector3& add);

        bool IsPerpendicular(const Vector3& v, float tolerance = Constants::Tolerance) const;

        //! Returns an (unnormalized) arbitrary vector which is orthogonal to this vector.
        Vector3 GetOrthogonalVector() const;

        //! Project vector onto another. P = (a.Dot(b) / b.Dot(b)) * b
        void Project(const Vector3& rhs);

        //! Project vector onto a normal (faster function). P = (v.Dot(Normal) * normal)
        void ProjectOnNormal(const Vector3& normal);

        //! Project this vector onto another and return the resulting projection. P = (a.Dot(b) / b.Dot(b)) * b
        Vector3 GetProjected(const Vector3& rhs) const;

        //! Project this vector onto a normal and return the resulting projection. P = v - (v.Dot(Normal) * normal)
        Vector3 GetProjectedOnNormal(const Vector3& normal);

        //! Returns true if the vector contains no nan or inf values, false if at least one element is not finite.
        bool IsFinite() const;

        //! Returns the underlying SIMD vector.
        Simd::Vec3::FloatType GetSimdValue() const;

        //! Directly sets the underlying SIMD vector.
        void SetSimdValue(Simd::Vec3::FloatArgType value);

    private:

        union
        {
            Simd::Vec3::FloatType m_value;
            float m_values[3];

            struct
            {
                float m_x;
                float m_y;
                float m_z;
            };
        };
    };

    //! Non member functionality belonging to the AZ namespace.
    //! Degrees-Radians conversions on AZ::Vector3
    AZ::Vector3 Vector3RadToDeg(const AZ::Vector3& radians);
    AZ::Vector3 Vector3DegToRad(const AZ::Vector3& degrees);
}

#include <AzCore/Math/Vector3.inl>
