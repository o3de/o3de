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
    class Quaternion
    {
    public:

        AZ_TYPE_INFO(Quaternion, "{73103120-3DD3-4873-BAB3-9713FA2804FB}")

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default constructor, components are uninitialized.
        Quaternion() = default;

        //! Copy constructor, clones the provided quaternion.
        Quaternion(const Quaternion& q);

        //! Constructs quaternion with all components set to the same specified value.
        explicit Quaternion(float x);

        //! Constructs quaternion with all components set to the provided values.
        explicit Quaternion(float x, float y, float z, float w);

        //! Constructs quaternion from a vector3 and scalar w value.
        explicit Quaternion(const Vector3& v, float w);

        //! Constructs quaternion from a SIMD type.
        //! For internal use only, arrangement of values in SIMD type is not guaranteed.
        explicit Quaternion(Simd::Vec4::FloatArgType value);

        static Quaternion CreateIdentity();

        static Quaternion CreateZero();

        //! Sets components from an array of 4 floats, stored in xyzw order.
        static Quaternion CreateFromFloat4(const float* values);

        //! Sets components using a Vector3 for the imaginary part and setting the real part to zero.
        static Quaternion CreateFromVector3(const Vector3& v);

        //! Sets components using a Vector3 for the imaginary part and a float for the real part.
        static Quaternion CreateFromVector3AndValue(const Vector3& v, float w);

        //! Sets the quaternion to be a rotation around a specified axis in radians.
        //! @{
        static Quaternion CreateRotationX(float angleInRadians);
        static Quaternion CreateRotationY(float angleInRadians);
        static Quaternion CreateRotationZ(float angleInRadians);
        //! @}

        static Quaternion CreateFromEulerRadiansXYZ(const Vector3& eulerRadians);
        static Quaternion CreateFromEulerRadiansYXZ(const Vector3& eulerRadians);
        static Quaternion CreateFromEulerRadiansZYX(const Vector3& eulerRadians);

        static Quaternion CreateFromEulerDegreesXYZ(const Vector3& eulerDegrees);
        static Quaternion CreateFromEulerDegreesYXZ(const Vector3& eulerDegrees);
        static Quaternion CreateFromEulerDegreesZYX(const Vector3& eulerDegrees);

        //! Creates a quaternion from a Matrix3x3
        static Quaternion CreateFromMatrix3x3(const class Matrix3x3& m);

        //! Creates a quaternion using the left 3x3 part of a Matrix3x4.
        //! \note If the matrix has a scale other than (1, 1, 1) be sure to extract the scale first
        //! with AZ::Matrix3x4::ExtractScale or ::ExtractScaleExact.
        static Quaternion CreateFromMatrix3x4(const class Matrix3x4& m);

        //! Creates a quaternion using the rotation part of a Matrix4x4
        static Quaternion CreateFromMatrix4x4(const class Matrix4x4& m);

        //! Creates a quaternion from a set of basis vectors
        static Quaternion CreateFromBasis(const Vector3& basisX, const Vector3& basisY, const Vector3& basisZ);

        static Quaternion CreateFromAxisAngle(const Vector3& axis, float angle);

        //! Create a quaternion from a scaled axis-angle representation.
        static Quaternion CreateFromScaledAxisAngle(const Vector3& scaledAxisAngle);

        static Quaternion CreateShortestArc(const Vector3& v1, const Vector3& v2);

        //! O3DE_DEPRECATION_NOTICE(GHI-10929)
        //! @deprecated use CreateFromEulerDegreesXYZ()
        //! Creates a quaternion using rotation in degrees about the axes. First rotated about the X axis, followed by the Y axis, then the Z axis.
        static const Quaternion CreateFromEulerAnglesDegrees(const Vector3& anglesInDegrees);

        //! O3DE_DEPRECATION_NOTICE(GHI-10929)
        //! @deprecated use CreateFromEulerRadiansXYZ()
        //! Creates a quaternion using rotation in radians about the axes. First rotated about the X axis, followed by the Y axis, then the Z axis.
        static const Quaternion CreateFromEulerAnglesRadians(const Vector3& anglesInRadians);

        //! Stores the vector to an array of 4 floats. The floats need only be 4 byte aligned, 16 byte alignment is not required.
        void StoreToFloat4(float* values) const;

        //! Component access
        //! @{
        float GetX() const;
        float GetY() const;
        float GetZ() const;
        float GetW() const;
        void SetX(float x);
        void SetY(float y);
        void SetZ(float z);
        void SetW(float w);
        //! @}

        //! We recommend using GetX,Y,Z,W. GetElement can be slower.
        float GetElement(int index) const;

        //! We recommend using SetX,Y,Z,W. SetElement can be slower.
        void SetElement(int index, float v);

         //! Indexed access using operator(). It's just for convenience.
         //! \note This can be slower than using GetX,GetY, etc.
        float operator()(int index) const;

        void Set(float x);
        void Set(float x, float y, float z, float w);
        void Set(const Vector3& v, float w);
        void Set(const float values[]);

        //! The conjugate of a quaternion is (-x, -y, -z, w).
        Quaternion GetConjugate() const;

        //! For a unit quaternion, the inverse is just the conjugate. This function assumes a unit quaternion.
        //! @{
        Quaternion GetInverseFast() const;
        void InvertFast();
        //! @}

        //! This is the inverse for any quaternion, not just unit quaternions.
        //! @{
        Quaternion GetInverseFull() const;
        void InvertFull();
        //! @}

        //! Dot product.
        float Dot(const Quaternion& q) const;

        float GetLengthSq() const;

        //! Returns length of the quaternion, full accuracy.
        float GetLength() const;

        //! Returns length of the quaternion, fast but low accuracy, uses raw estimate instructions.
        float GetLengthEstimate() const;

        //! Returns 1/length, full accuracy.
        float GetLengthReciprocal() const;

        //! Returns 1/length of the quaternion, fast but low accuracy, uses raw estimate instructions.
        float GetLengthReciprocalEstimate() const;

        //! Returns normalized quaternion, full accuracy.
        Quaternion GetNormalized() const;

        //! Returns normalized quaternion, fast but low accuracy, uses raw estimate instructions.
        Quaternion GetNormalizedEstimate() const;

        //! Normalizes the quaternion in-place, full accuracy.
        void Normalize();

        //! Normalizes the quaternion in-place, fast but low accuracy, uses raw estimate instructions.
        void NormalizeEstimate();

        //! Normalizes the vector in-place and returns the previous length. This takes a few more instructions than calling just Normalize().
        //! @{
        float NormalizeWithLength();
        float NormalizeWithLengthEstimate();
        //! @}

        //! Get the shortest equivalent of the rotation.
        //! In case the w component of the quaternion is negative the rotation is > 180° and taking the longer path.
        //! The quaternion will be inverted in that case to take the shortest path of rotation.
        //! @{
        Quaternion GetShortestEquivalent() const;
        void ShortestEquivalent();
        //! @}

        //! Linearly interpolate towards a destination quaternion.
        //! @param[in] dest The quaternion to interpolate towards.
        //! @param[in] t Normalized interpolation value where 0.0 represents the current and 1.0 the destination value.
        //! @result The interpolated quaternion at the given interpolation point.
        Quaternion Lerp(const Quaternion& dest, float t) const;

        //! Linearly interpolate towards a destination quaternion, and normalize afterwards.
        //! @param[in] dest The quaternion to interpolate towards.
        //! @param[in] t Normalized interpolation value where 0.0 represents the current and 1.0 the destination value.
        //! @result The interpolated and normalized quaternion at the given interpolation point.
        Quaternion NLerp(const Quaternion& dest, float t) const;

        //! Spherical linear interpolation. Result is not normalized.
        Quaternion Slerp(const Quaternion& dest, float t) const;

        //! Spherical linear interpolation, but with in/out tangent quaternions.
        //! Squad(p,a,b,q,t) = Slerp(Slerp(p,q,t), Slerp(a,b,t); 2(1-t)t).
        Quaternion Squad(const Quaternion& dest, const Quaternion& in, const Quaternion& out, float t) const;

        //! Checks if the quaternion is close to another quaternion with a given floating point tolerance.
        bool IsClose(const Quaternion& q, float tolerance = Constants::Tolerance) const;

        bool IsIdentity(float tolerance = Constants::Tolerance) const;

        bool IsZero(float tolerance = Constants::FloatEpsilon) const;

        Quaternion& operator= (const Quaternion& rhs);

        Quaternion operator-() const;
        Quaternion operator+(const Quaternion& q) const;
        Quaternion operator-(const Quaternion& q) const;
        Quaternion operator*(const Quaternion& q) const;
        Quaternion operator*(float multiplier) const;
        Quaternion operator/(float divisor) const;

        Quaternion& operator+=(const Quaternion& q);
        Quaternion& operator-=(const Quaternion& q);
        Quaternion& operator*=(const Quaternion& q);
        Quaternion& operator*=(float multiplier);
        Quaternion& operator/=(float divisor);

        bool operator==(const Quaternion& rhs) const;
        bool operator!=(const Quaternion& rhs) const;

        //! Transforms a vector using the rotation described by this quaternion
        Vector3 TransformVector(const Vector3& v) const;

        //! Create, from the quaternion, a set of Euler (actually Tait-Bryan) angles of rotations around:
        //!  first x-axis, then y-axis and then z-axis.
        //! @return Vector3 A vector containing component-wise rotation angles in degrees.
        //! O3DE_DEPRECATION_NOTICE(GHI-10929)
        //! @deprecated use GetEulerDegreesXYZ()
        Vector3 GetEulerDegrees() const;

        //! Create, from the quaternion, a set of Euler (actually Tait-Bryan) angles of rotations around:
        //!  first x-axis, then y-axis and then z-axis.
        //! @return Vector3 A vector containing component-wise rotation angles in radians.
        //! O3DE_DEPRECATION_NOTICE(GHI-10929)
        //! @deprecated use GetEulerRadiansXYZ()
        Vector3 GetEulerRadians() const;

        //! Compute, from the quaternion, a set of Euler (actually Tait-Bryan) angles of rotations around:
        //!  first x-axis (roll), then y-axis (pitch) and then z-axis (yaw).
        //! The quaternion us supposed to be created with CreateFromEulerRadiansXYZ() or CreateFromEulerDegreesXYZ().
        //! Note that a gimbal lock occurs with the y-axis rotation equal to +-90 degrees, loosing information
        //!  on initial yaw and roll, and then roll is deliberately zeroed.
        //! @return Vector3 A vector containing component-wise rotation angles in degrees.
        Vector3 GetEulerDegreesXYZ() const;

        //! Compute, from the quaternion, a set of Euler (actually Tait-Bryan) angles of rotations around:
        //!  first x-axis (roll), then y-axis (pitch) and then z-axis (yaw).
        //! The quaternion us supposed to be created with CreateFromEulerRadiansXYZ() or CreateFromEulerDegreesXYZ().
        //! Note that a gimbal lock occurs with the y-axis rotation equal to +-90 degrees, loosing information
        //!  on initial yaw and roll, and then roll is deliberately zeroed.
        //! @return Vector3 A vector containing component-wise rotation angles in radians.
        Vector3 GetEulerRadiansXYZ() const;

        //! Compute, from the quaternion, a set of Euler (actually Tait-Bryan) angles of rotations around:
        //!  first y-axis (pitch), then x-axis (roll) and then z-axis (yaw).
        //! The quaternion us supposed to be created with CreateFromEulerRadiansYXZ() or CreateFromEulerDegreesYXZ().
        //! @return Vector3 A vector containing component-wise rotation angles in degrees.
        Vector3 GetEulerDegreesYXZ() const;

        //! Compute, from the quaternion, a set of Euler (actually Tait-Bryan) angles of rotations around:
        //!  first y-axis (pitch), then x-axis (roll) and then z-axis (yaw).
        //! The quaternion us supposed to be created with CreateFromEulerRadiansYXZ() or CreateFromEulerDegreesYXZ().
        //! @return Vector3 A vector containing component-wise rotation angles in radians.
        Vector3 GetEulerRadiansYXZ() const;

        //! Compute, from the quaternion, a set of Euler (actually Tait-Bryan) angles of rotations around:
        //!  first z-axis (yaw), then y-axis (pitch) and then x-axis (roll).
        //! The quaternion us supposed to be created with CreateFromEulerRadiansZYX() or CreateFromEulerDegreesZYX().
        //! Note that a gimbal lock occurs with the y-axis rotation equal to +-90 degrees, loosing information
        //!  on initial yaw and roll, and then roll is deliberately zeroed.
        //! @return Vector3 A vector containing component-wise rotation angles in degrees.
        Vector3 GetEulerDegreesZYX() const;

        //! Compute, from the quaternion, a set of Euler (actually Tait-Bryan) angles of rotations around:
        //!  first z-axis (yaw), then y-axis (pitch) and then x-axis (roll).
        //! The quaternion us supposed to be created with CreateFromEulerRadiansZYX() or CreateFromEulerDegreesZYX().
        //! Note that a gimbal lock occurs with the y-axis rotation equal to +-90 degrees, loosing information
        //!  on initial yaw and roll, and then roll is deliberately zeroed.
        //! @return Vector3 A vector containing component-wise rotation angles in radians.
        Vector3 GetEulerRadiansZYX() const;

        //! @param eulerRadians A vector containing component-wise rotation angles in radians.
        //! O3DE_DEPRECATION_NOTICE(GHI-10929)
        //! @deprecated use CreateFromEulerRadiansXYZ()
        void SetFromEulerRadians(const Vector3& eulerRadians);

        //! @param eulerDegrees A vector containing component-wise rotation angles in degrees.
        //! O3DE_DEPRECATION_NOTICE(GHI-10929)
        //! @deprecated use CreateFromEulerDegreesXYZ()
        void SetFromEulerDegrees(const Vector3& eulerDegrees);

        //! Populate axis and angle of rotation from Quaternion
        //! @param[out] outAxis A Vector3 defining the rotation axis.
        //! @param[out] outAngle A float rotation angle around the axis in radians.
        void ConvertToAxisAngle(Vector3& outAxis, float& outAngle) const;

        //! Convert the quaternion into scaled axis-angle representation.
        Vector3 ConvertToScaledAxisAngle() const;

        //! Returns the imaginary (X/Y/Z) portion of the quaternion.
        Vector3 GetImaginary() const;

        //! Return angle in radians.
        float GetAngle() const;

        bool IsFinite() const;

        Simd::Vec4::FloatType GetSimdValue() const;

    private:

        //! Takes the absolute value of each component of the quaternion.
        Quaternion GetAbs() const;

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

    //! Non-member functionality belonging to the AZ namespace

    //! O3DE_DEPRECATION_NOTICE(GHI-10929)
    //! @deprecated use GetEulerDegreesXYZ()
    //! Create, from a quaternion, a set of Euler angles of rotations around first z-axis, then y-axis and then x-axis.
    //! @param q a quaternion representing the rotation
    //! @return A vector containing component-wise rotation angles in degrees.
    Vector3 ConvertQuaternionToEulerDegrees(const Quaternion& q);

    //! O3DE_DEPRECATION_NOTICE(GHI-10929)
    //! @deprecated use GetEulerRadiansXYZ()
    //! Create, from a quaternion, a set of Euler angles of rotations around first z-axis, then y-axis and then x-axis.
    //! @param q a quaternion representing the rotation
    //! @return A vector containing component-wise rotation angles in radians.
    Vector3 ConvertQuaternionToEulerRadians(const Quaternion& q);

    //! O3DE_DEPRECATION_NOTICE(GHI-10929)
    //! @deprecated use Quaternion::CreateFromEulerRadiansXYZ()
    //! @param eulerRadians A vector containing component-wise rotation angles in radians.
    //! @return a quaternion made from composition of rotations around principle axes.
    Quaternion ConvertEulerRadiansToQuaternion(const Vector3& eulerRadians);

    //! O3DE_DEPRECATION_NOTICE(GHI-10929)
    //! @deprecated use Quaternion::CreateFromEulerDegreesXYZ()
    //! @param eulerDegrees A vector containing component-wise rotation angles in degrees.
    //! @return a quaternion made from composition of rotations around principle axes.
    Quaternion ConvertEulerDegreesToQuaternion(const Vector3& eulerDegrees);

    //! Populate axis and angle of rotation from Quaternion.
    //! @param[in] quat A source quaternion
    //! @param[out] outAxis A Vector3 defining the rotation axis.
    //! @param[out] outAngle A float rotation angle around the axis in radians.
    void ConvertQuaternionToAxisAngle(const Quaternion& quat, Vector3& outAxis, float& outAngle);

    //! Scalar multiplication of a quaternion, allows scalar * quaternion syntax.
    //! @param multiplier the scalar value to use.
    //! @param rhs the quaternion to scale.
    //! @return a quaternion where each component of rhs is scaled by multiplier.
    Quaternion operator*(float multiplier, const Quaternion& rhs);

    //! Helper method to quickly determine whether or not a given transform includes a scale
    bool IsUnit(const Transform& t);
}

#include <AzCore/Math/Quaternion.inl>
