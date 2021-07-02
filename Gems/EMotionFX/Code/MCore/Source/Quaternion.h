/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Math/Vector2.h>
#include "StandardHeaders.h"
#include "FastMath.h"
#include "Vector.h"
#include "Matrix4.h"
#include "Algorithms.h"


namespace MCore
{
    /**
     * Depracated. Please use AZ::Quaternion instead.
     * The quaternion class in MCore. 
     * Quaternions are mostly used to represent rotations in 3D applications.
     * The advantages of quaternions over matrices are that they take up less space and that interpolation between
     * two quaternions is easier to perform. Instead of a 3x3 rotation matrix, which is 9 floats or doubles, a quaternion
     * only uses 4 floats or doubles. This template/class provides you with methods to perform all kind of operations on
     * these quaternions, from interpolation to conversion to matrices and other rotation representations.
     */
    class MCORE_API Quaternion
    {
    public:
        AZ_TYPE_INFO(MCore::Quaternion, "{1807CD22-EBB5-45E8-8113-3B1DABB53F12}")

        /**
         * Default constructor. Sets x, y and z to 0 and w to 1.
         */
        MCORE_INLINE Quaternion()
            : x(0.0f)
            , y(0.0f)
            , z(0.0f)
            , w(1.0f) {}

        /**
         * Constructor which sets the x, y, z and w.
         * @param xVal The value of x.
         * @param yVal The value of y.
         * @param zVal The value of z.
         * @param wVal The value of w.
         */
        MCORE_INLINE Quaternion(float xVal, float yVal, float zVal, float wVal)
            : x(xVal)
            , y(yVal)
            , z(zVal)
            , w(wVal) {}

        /**
         * Copy constructor. Copies the x, y, z, w values from the other quaternion.
         * @param other The quaternion to copy the attributes from.
         */
        MCORE_INLINE Quaternion(const Quaternion& other)
            : x(other.x)
            , y(other.y)
            , z(other.z)
            , w(other.w) {}

        /**
         * Constructor which creates a quaternion from a pitch, yaw and roll.
         * @param pitch Rotation around the x-axis, in radians.
         * @param yaw Rotation around the y-axis, in radians.
         * @param roll Rotation around the z-axis, in radians.
         */
        MCORE_INLINE Quaternion(float pitch, float yaw, float roll)                             { SetEuler(pitch, yaw, roll); }

        /**
         * Constructor which takes a matrix as input parameter.
         * This converts the rotation of the specified matrix into a quaternion. Please keep in mind that the matrix may NOT contain
         * any scaling, so if it does, please normalize your matrix first!
         * @param matrix The matrix to initialize the quaternion from.
         */
        MCORE_INLINE Quaternion(const Matrix& matrix)                                           { FromMatrix(matrix); }

        /**
         * Constructor which creates a quaternion from a spherical rotation.
         * @param spherical The spherical coordinates in radians, which creates an axis to rotate around.
         * @param angle The angle to rotate around this axis.
         */
        Quaternion(const AZ::Vector2& spherical, float angle);

        /**
         * Constructor which creates a quaternion from an axis and angle.
         * @param axis The axis to rotate around.
         * @param angle The angle in radians to rotate around the given axis.
         */
        Quaternion(const AZ::Vector3& axis, float angle);

        /**
         * Set the quaternion x/y/z/w component values.
         * @param vx The value of x.
         * @param vy The value of y.
         * @param vz The value of z.
         * @param vw The value of w.
         */
        MCORE_INLINE void Set(float vx, float vy, float vz, float vw)                           { x = vx; y = vy; z = vz; w = vw; }

        /**
         * Calculates the square length of the quaternion.
         * @result The square length (length*length).
         */
        MCORE_INLINE float SquareLength() const                                                 { return (x * x + y * y + z * z + w * w); }

        /**
         * Calculates the length of the quaternion.
         * It's safe, since it prevents a division by 0.
         * @result The length of the quaternion.
         */
        MCORE_INLINE float Length() const;

        /**
         * Performs a dot product on the quaternions.
         * @param q The quaternion to multiply (dot product) this quaternion with.
         * @result The quaternion which is the result of the dot product.
         */
        MCORE_INLINE float Dot(const Quaternion& q) const                                       { return (x * q.x + y * q.y + z * q.z + w * q.w); }

        /**
         * Normalize the quaternion.
         * @result The normalized quaternion. It modifies itself, so no new quaternion is returned.
         */
        MCORE_INLINE Quaternion& Normalize();

        /**
         * Sets the quaternion to identity. Where x, y and z are set to 0 and w is set to 1.
         * @result The quaternion, now set to identity.
         */
        MCORE_INLINE Quaternion& Identity()                                                     { x = 0.0f; y = 0.0f; z = 0.0f; w = 1.0f; return *this; }

        /**
         * Calculate the inversed version of this quaternion.
         * @result The inversed version of this quaternion.
         */
        MCORE_INLINE Quaternion& Inverse()                                                      { const float len = 1.0f / SquareLength(); x = -x * len; y = -y * len; z = -z * len; w = w * len; return *this; }

        /**
         * Conjugate this quaternion.
         * @result Returns itself Conjugated.
         */
        MCORE_INLINE Quaternion& Conjugate()                                                    { x = -x; y = -y; z = -z; return *this; }

        /**
         * Calculate the inversed version of this quaternion.
         * @result The inversed version of this quaternion.
         */
        MCORE_INLINE Quaternion Inversed() const                                                { const float len = 1.0f / SquareLength(); return Quaternion(-x * len, -y * len, -z * len, w * len); }

        /**
         * Returns the normalized version of this quaternion.
         * @result The normalized version of this quaternion.
         */
        MCORE_INLINE Quaternion Normalized() const                                              { Quaternion result(*this); result.Normalize(); return result; }

        /**
         * Return the conjugated version of this quaternion.
         * @result The conjugated version of this quaternion.
         */
        MCORE_INLINE Quaternion Conjugated() const                                              { return Quaternion(-x, -y, -z, w); }

        /**
         * Calculate the exponent of this quaternion.
         * @result The resulting quaternion of the exp.
         */
        MCORE_INLINE Quaternion Exp() const                                                     { const float r = Math::Sqrt(x * x + y * y + z * z); const float expW = Math::Exp(w); const float s = (r >= 0.00001f) ? expW* Math::Sin(r) / r : 0.0f; return Quaternion(s * x, s * y, s * z, expW * Math::Cos(r)); }

        /**
         * Calculate the log of the quaternion.
         * @result The resulting quaternion of the log.
         */
        MCORE_INLINE Quaternion LogN() const                                                    { const float r = Math::Sqrt(x * x + y * y + z * z); float t = (r > 0.00001f) ? Math::ATan2(r, w) / r : 0.0f; return Quaternion(t * x, t * y, t * z, 0.5f * Math::Log(SquareLength())); }

        /**
         * Calculate and get the right basis vector.
         * @result The basis vector pointing to the right. This assumes x+ points to the right.
         */
        MCORE_INLINE AZ::Vector3 CalcRightAxis() const;

        /**
         * Calculate and get the up basis vector.
         * @result The basis vector pointing upwards. This assumes z+ points up.
         */
        MCORE_INLINE AZ::Vector3 CalcUpAxis() const;

        /**
         * Calculate and get the forward basis vector.
         * @result The basis vector pointing forward. This assumes y+ points forward, into the depth.
         */
        MCORE_INLINE AZ::Vector3 CalcForwardAxis() const;

        /**
         * Initialize the current quaternion from a specified matrix.
         * Please note that the matrix may not contain any scaling!
         * So make sure the matrix has been normalized before, if it contains any scale.
         * @param m The matrix to initialize the quaternion from.
         */
        MCORE_INLINE void FromMatrix(const Matrix& m)                                       { *this = Quaternion::ConvertFromMatrix(m); }

        /**
         * Setup the quaternion from a pitch, yaw and roll.
         * @param pitch The rotation around the x-axis, in radians.
         * @param yaw The rotation around the y-axis, in radians.
         * @param roll The rotation around the z-axis in radians.
         * @result The quaternion, now initialized with the given pitch, yaw, roll rotation.
         */
        Quaternion& SetEuler(float pitch, float yaw, float roll);

        /**
         * Convert the quaternion to an axis and angle. Which represents a rotation of the resulting angle around the resulting axis.
         * @param axis Pointer to the vector to store the axis in.
         * @param angle Pointer to the variable to store the angle in (will be in radians).
         */
        void ToAxisAngle(AZ::Vector3* axis, float* angle) const;

        /**
         * Convert the quaternion to a spherical rotation.
         * @param spherical A pointer to the 2D vector to store the spherical coordinates in radians, which build the axis.
         * @param angle The pointer to the variable to store the angle around this axis in radians.
         */
        void ToSpherical(AZ::Vector2* spherical, float* angle) const;

        /**
         * Extract the euler angles in radians.
         * The x component of the resulting vector represents the rotation around the x-axis (pitch).
         * The y component results the rotation around the y-axis (yaw) and the z component represents
         * the rotation around the z-axis (roll).
         * @result The 3D vector containing the euler angles in radians, around each axis.
         */
        AZ::Vector3 ToEuler() const;

        /**
        * Returns the angle of rotation about the z axis. This is same as
        * the z component of the vector returned by the ToEuler method.  It
        * is just more efficient to call this when one is interested only in rotation about the z axis.
        * @result The angle of rotation about z axis in radians.
        */
        float GetEulerZ() const;

        /**
         * Convert this quaternion into a matrix.
         * @result The matrix representing the rotation of this quaternion.
         */
        Matrix ToMatrix() const;

        /**
         * Convert a matrix into a quaternion.
         * Please keep in mind that the specified matrix may NOT contain any scaling!
         * So make sure the matrix has been normalized before, if it contains any scale.
         * @param m The matrix to extract the rotation from.
         * @result The quaternion, now containing the rotation of the given matrix, in quaternion form.
         */
        static Quaternion ConvertFromMatrix(const Matrix& m);

        /**
         * Create a delta rotation that rotates one vector onto another vector.
         * @param fromVector The normalized vector to start from. This must be normalized!
         * @param toVector The normalized vector to rotate towards. This must be normalized as well!
         * @result The delta rotation quaternion.
         */
        static Quaternion CreateDeltaRotation(const AZ::Vector3& fromVector, const AZ::Vector3& toVector);

        /**
         * Create a delta rotation that rotates one vector onto another vector.
         * If the angle is bigger than the max allowed angle that is specified it will rotate with an angle of the maximum specified angle.
         * So if the angle between the vectors is 40 degrees and you maxAngleRadians equals 10 degrees (in radians) it will only rotate 10 degrees.
         * @param fromVector The normalized vector to start from. This must be normalized!
         * @param toVector The normalized vector to rotate towards. This must be normalized as well!
         * @param maxAngleRadians The maximum rotation angle on the plane defined by the two vectors. This cannot be more than Math::pi (180 degrees).
         * @result The delta rotation quaternion.
         */
        static Quaternion CreateDeltaRotation(const AZ::Vector3& fromVector, const AZ::Vector3& toVector, float maxAngleRadians);

        /**
         * Init this quaternion as a delta rotation that rotates one vector onto another vector.
         * @param fromVector The normalized vector to start from. This must be normalized!
         * @param toVector The normalized vector to rotate towards. This must be normalized as well!
         */
        void SetAsDeltaRotation(const AZ::Vector3& fromVector, const AZ::Vector3& toVector);

        /**
         * Init this quaternion as a delta rotation that rotates one vector onto another vector.
         * If the angle is bigger than the max allowed angle that is specified it will rotate with an angle of the maximum specified angle.
         * So if the angle between the vectors is 40 degrees and you maxAngleRadians equals 10 degrees (in radians) it will only rotate 10 degrees.
         * @param fromVector The normalized vector to start from. This must be normalized!
         * @param toVector The normalized vector to rotate towards. This must be normalized as well!
         * @param maxAngleRadians The maximum rotation angle on the plane defined by the two vectors. This cannot be more than Math::pi (180 degrees).
         */
        void SetAsDeltaRotation(const AZ::Vector3& fromVector, const AZ::Vector3& toVector, float maxAngleRadians);

        /**
         * Rotate this current quaternion using a given delta that is calculated from two vectors.
         * The rotation axis used is the cross product between the from and to vector. The rotation angle is the angle between these two vectors.
         * @param fromVector The current direction vector, must be normalized.
         * @param toVector The desired new direction vector, must be normalized.
         */
        void RotateFromTo(const AZ::Vector3& fromVector, const AZ::Vector3& toVector);

        /**
         * Decompose into swing and twist.
         * The original rotation quat can be reassembled by doing swing * twist.
         * @param direction The direction vector to get the twist from.
         * @param outSwing This will contain the swing quaternion.
         * @param outTwist This will contain the twist quaternion.
         */
        void DecomposeSwingTwist(const AZ::Vector3& direction, Quaternion* outSwing, Quaternion* outTwist) const;

        /**
         * Linear interpolate between this and another quaternion.
         * @param to The quaternion to interpolate towards.
         * @param t The time value, between 0 and 1.
         * @result The quaternion at the given time in the interpolation process.
         */
        Quaternion Lerp(const Quaternion& to, float t) const;

        /**
         * Linear interpolate between this and another quaternion, and normalize afterwards.
         * @param to The quaternion to interpolate towards.
         * @param t The time value, between 0 and 1.
         * @result The normalized quaternion at the given time in the interpolation process.
         */
        Quaternion NLerp(const Quaternion& to, float t) const;

        /**
         * Spherical Linear interpolate between this and another quaternion.
         * @param to The quaternion to interpolate towards.
         * @param t The time value, between 0 and 1.
         * @result The quaternion at the given time in the interpolation process.
         */
        Quaternion Slerp(const Quaternion& to, float t) const;

        /**
         * Spherical cubic interpolate.
         * @param p The first quaternion.
         * @param a The second quaternion.
         * @param b The third quaternion.
         * @param q The fourth quaternion.
         * @param t The time value, between 0 and 1.
         * @result The quaternion at the given time in the interpolation process.
         */
        static Quaternion Squad(const Quaternion& p, const Quaternion& a, const Quaternion& b, const Quaternion& q, float t);

        // operators
        MCORE_INLINE const Quaternion&  operator=(const Matrix& m)                              { FromMatrix(m); return *this; }
        MCORE_INLINE const Quaternion&  operator=(const Quaternion& other)                      { x = other.x; y = other.y; z = other.z; w = other.w; return *this; }
        MCORE_INLINE Quaternion         operator-() const                                       { return Quaternion(-x, -y, -z, -w); }
        MCORE_INLINE const Quaternion&  operator+=(const Quaternion& q)                         { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
        MCORE_INLINE const Quaternion&  operator-=(const Quaternion& q)                         { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
        MCORE_INLINE const Quaternion&  operator*=(const Quaternion& q);
        MCORE_INLINE const Quaternion&  operator*=(float f)                                     { x *= f; y *= f; z *= f; w *= f; return *this; }
        //MCORE_INLINE const Quaternion&    operator*=(double f)                                    { x*=f; y*=f; z*=f; w*=f; return *this; }
        MCORE_INLINE bool               operator==(const Quaternion& q) const                   { return ((q.x == x) && (q.y == y) && (q.z == z) && (q.w == w)); }
        MCORE_INLINE bool               operator!=(const Quaternion& q) const                   { return ((q.x != x) || (q.y != y) || (q.z != z) || (q.w != w)); }

        //MCORE_INLINE float&           operator[](int32 row)                                   { return ((float*)&x)[row]; }
        MCORE_INLINE operator           float*()                                                { return (float*)&x; }
        MCORE_INLINE operator           const float*() const                                    { return (const float*)&x; }

        MCORE_INLINE AZ::Vector3            operator*(const AZ::Vector3& p) const;                      // multiply a vector by a quaternion
        MCORE_INLINE Quaternion         operator/(const Quaternion& q) const;                   // returns the ratio of two quaternions

        // attributes
        float   x, y, z, w;
    };


    // operators
    MCORE_INLINE Quaternion operator*(const Quaternion& a, float f)                 { return Quaternion(a.x * f, a.y * f, a.z * f, a.w * f); }
    MCORE_INLINE Quaternion operator*(float f, const Quaternion& b)                 { return Quaternion(f * b.x, f * b.y, f * b.z, f * b.w); }
    //MCORE_INLINE Quaternion   operator*(const Quaternion& a, double f)                { return Quaternion(a.x*f, a.y*f, a.z*f, a.w*f); }
    //MCORE_INLINE Quaternion   operator*(double f, const Quaternion& b)                { return Quaternion(f*b.x, f*b.y, f*b.z, f*b.w); }
    MCORE_INLINE Quaternion operator+(const Quaternion& a, const Quaternion& b)     { return Quaternion(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
    MCORE_INLINE Quaternion operator-(const Quaternion& a, const Quaternion& b)     { return Quaternion(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
    MCORE_INLINE Quaternion operator*(const Quaternion& a, const Quaternion& b)     { return Quaternion(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,  a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z,  a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x,  a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z); }

    // include the inline code
#include "Quaternion.inl"
} // namespace MCore
