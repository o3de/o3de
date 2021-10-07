/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector2.h>
#include "StandardHeaders.h"
#include "FastMath.h"
#include "Vector.h"
#include "Algorithms.h"


namespace MCore
{
    /**
     * The dual quaternion class.
     * Dual quaternions contain two internal vectors. The real and dual part. They behave very similar to regular quaternions in usage.
     * The difference is that the dual quaternion can represent both a rotation and translation or displacement.
     * The dual quaternion consists of a real part, which you can see as the rotation quaternion, and a dual part, which you can see as the translation part.
     * One of the advantages of dual quaternions is that they can be used to replace the linear skin deformations with a volume preserving method.
     * Using dual quaternions in skinning fixes issues such as the famous candy-wrapper effect. It handles twisting of bones much nicer compared to the linear method.
     * This is often referred to as dual quaternion skinning.
     */
    class MCORE_API DualQuaternion
    {
    public:
        /**
         * The default constructor.
         * This automatically initializes the dual quaternion to identity.
         */
        MCORE_INLINE DualQuaternion()
            : m_real(0.0f, 0.0f, 0.0f, 1.0f)
            , m_dual(0.0f, 0.0f, 0.0f, 0.0f) {}

        /**
         * Copy constructor.
         * @param other The dual quaternion to copy the data from.
         */
        MCORE_INLINE DualQuaternion(const DualQuaternion& other)
            : m_real(other.m_real)
            , m_dual(other.m_dual) {}

        /**
         * Extended constructor.
         * @param real The real part, which is the rotation part.
         * @param dual The dual part, which is you can see as the translation part.
         * @note Do not directly put in the translation into the dual part, if you want to convert from a rotation and translation, please use the other constructor
         *       or use the FromRotationTranslation method.
         */
        MCORE_INLINE DualQuaternion(const AZ::Quaternion& real, const AZ::Quaternion& dual)
            : m_real(real)
            , m_dual(dual) {}

        /**
         * Constructor which takes a matrix as input parameter.
         * This converts the rotation of the specified matrix into a quaternion. Please keep in mind that the matrix may NOT contain
         * any scaling, so if it does, please normalize your matrix first!
         * @param matrix The matrix to initialize the quaternion from.
         */
        MCORE_INLINE DualQuaternion(const AZ::Transform& transform)                                       { FromTransform(transform); }

        /**
         * Extended constructor, which initializes this dual quaternion from a rotation and translation.
         * @param rotation The rotation quaternion, which does not need to be normalized, unless you want this to be a normalized dual quaternion.
         * @param translation The translation vector.
         */
        MCORE_INLINE DualQuaternion(const AZ::Quaternion& rotation, const AZ::Vector3& translation);

        /**
         * Set the real and dual part of the dual quaternion.
         * @param real The real part, which represents the rotation.
         * @param dual The dual part, which represents the translation.
         * @note Please keep in mind that you should not set the translation directly into the dual part. If you want to initialize the dual quaternion from
         *       a rotation and translation, please use the special constructor for this, or the FromRotationTranslation method.
         */
        MCORE_INLINE void Set(const AZ::Quaternion& real, const AZ::Quaternion& dual)                   { m_real = real; m_dual = dual; }

        /**
         * Normalize the dual quaternion.
         * @result A reference to this same quaternion, but now normalized.
         * @note Please keep in mind that zero length quaternions will result in a division by zero!
         */
        DualQuaternion& Normalize();

        /**
         * Calculate the normalized version of this dual quaternion.
         * @result A copy of this dual quaternion, but then normalized.
         * @note Please keep in mind that zero length quaternions will result in a division by zero!
         */
        MCORE_INLINE DualQuaternion Normalized() const                                          { DualQuaternion result(*this); result.Normalize(); return result; }

        /**
         * Set the dual quaternion to identity, so that it has basically no transform.
         * The default constructor already puts the dual quaternion in its identity transform.
         * @result A reference to this quaternion, but now having an identity transform.
         */
        MCORE_INLINE DualQuaternion& Identity()                                                 { m_real = AZ::Quaternion::CreateIdentity(); m_dual.Set(0.0f, 0.0f, 0.0f, 0.0f); return *this; }

        /**
         * Get the dot product between the two dual quaternions.
         * This basically performs two dot products: One on the real part and one on the dual part.
         * That means that it also results in two float values, which is why a Vector2 is returned.
         * @param other The other dual quaternion to perform the dot product with.
         * @result A 2D vector containing the result of the dot products. The x component contains the result of the dot between the real part and the y component
         *         contains the result of the dot product between the dual parts.
         */
        MCORE_INLINE AZ::Vector2 Dot(const DualQuaternion& other) const                             { return AZ::Vector2(m_real.Dot(other.m_real), m_dual.Dot(other.m_dual)); }

        /**
         * Calculate the length of the dual quaternion.
         * This results in a 2D vector, because it calculates the length for both the real and dual parts.
         * The result of the real part will be stored in the x component of the 2D vector, and the result of the dual part will be stored in the y component.
         * @result The 2D vector containing the length of the real and dual part.
         */
        MCORE_INLINE AZ::Vector2 Length() const                                                     { const float realLen = m_real.GetLength(); return AZ::Vector2(realLen, m_real.Dot(m_dual) / realLen); }

        /**
         * Inverse this dual quaternion.
         * @result A reference to this dual quaternion, but now inversed.
         */
        DualQuaternion& Inverse();

        /**
         * Calculate an inversed version of this dual quaternion.
         * @result A copy of this dual quaternion, but then inversed.
         */
        DualQuaternion Inversed() const;

        /**
         * Conjugate this dual quaternion.
         * @result A reference to this dual quaternion, but now conjugaged.
         * @note If you want to inverse a unit quaternion, you can use the conjugate instead, as that gives the same result, but is much faster to calculate.
         */
        MCORE_INLINE DualQuaternion& Conjugate()                                                { m_real = m_real.GetConjugate(); m_dual = m_dual.GetConjugate(); return *this; }

        /**
         * Calculate a conjugated version of this dual quaternion.
         * @result A copy of this dual quaternion, but conjugated.
         * @note If you want to inverse a unit quaternion, you can use the conjugate instead, as that gives the same result, but is much faster to calculate.
         */
        MCORE_INLINE DualQuaternion Conjugated() const                                          { return DualQuaternion(m_real.GetConjugate(), m_dual.GetConjugate()); }

        /**
         * Initialize the current quaternion from a specified matrix.
         * Please note that the matrix may not contain any scaling!
         * So make sure the matrix has been normalized before, if it contains any scale.
         * @param m The matrix to initialize the quaternion from.
         */
        MCORE_INLINE void FromTransform(const AZ::Transform& transform)                                    { *this = DualQuaternion::ConvertFromTransform(transform); }

        /**
         * Initialize this dual quaternion from a rotation and translation.
         * @param rot The rotation quaternion.
         * @param pos The translation vector.
         * @note It is allowed to pass an un-normalized quaternion to the rotation parameter. This however will also result in a non-normalized dual quaternion.
         */
        MCORE_INLINE void FromRotationTranslation(const AZ::Quaternion& rot, const AZ::Vector3& pos)    { *this = DualQuaternion::ConvertFromRotationTranslation(rot, pos); }

        /**
         * Convert this dual quaternion into a 4x4 matrix.
         * @result A matrix, representing the same transformation as this dual quaternion.
         */
        AZ::Transform ToTransform() const;

        /**
         * Transform a 3D point with this dual quaternion.
         * This applies both a rotation and possible translation to the point.
         * @param point The 3D point to transform.
         * @result The transformed point.
         * @note If you want to transform a vector instead of a point, please use the TransformVector method.
         */
        MCORE_INLINE AZ::Vector3 TransformPoint(const AZ::Vector3& point) const;

        /**
         * Transform a 3D vector with this dual quaternion.
         * This will apply only the rotation part of this dual quaternion. So it will not apply any displacement caused by the dual part of this dual quaternion.
         * You should use this method when transforming normals and tangents for example.
         * You can use the TransformPoint method to transform 3D points.
         * This vector transform is also faster than the TransformPoint method, as it has to do fewer calculations.
         * @param v The input vector.
         * @result The transformed vector.
         */
        MCORE_INLINE AZ::Vector3 TransformVector(const AZ::Vector3& v) const;

        /**
         * Extract the rotation and translation from this dual quaternion.
         * This method handles non-unit dual quaternions fine. If you are sure your dual quaternion is normalized, you should use
         * the faster version of this method, which is called NormalizedToRotationTranslation.
         * @param outRot A pointer to the quaternion in which we will store the output rotation.
         * @param outPos A pointer to the vector in which we will store the output translation.
         */
        void ToRotationTranslation(AZ::Quaternion* outRot, AZ::Vector3* outPos) const;

        /**
         * Extract the rotation and translation from this dual normalized quaternion.
         * This method assumes that this dual quaternion is normalized. If it isn't, the resulting output will be incorrect!
         * If you are not sure this quaternion is not normalized, or if you know it is not, please use the ToRotationTranslation method.
         * @param outRot A pointer to the quaternion in which we will store the output rotation.
         * @param outPos A pointer to the vector in which we will store the output translation.
         */
        void NormalizedToRotationTranslation(AZ::Quaternion* outRot, AZ::Vector3* outPos) const;

        /**
         * Convert a matrix into a quaternion.
         * Please keep in mind that the specified matrix may NOT contain any scaling!
         * So make sure the matrix has been normalized before, if it contains any scale.
         * @param m The matrix to extract the rotation from.
         * @result The quaternion, now containing the rotation of the given matrix, in quaternion form.
         */
        static DualQuaternion ConvertFromTransform(const AZ::Transform& transform);

        /**
         * Convert a rotation and translation into a dual quaternion.
         * @param rotation The rotation quaternion, which does not need to be normalized.
         * @param translation The translation.
         * @result A dual quaternion representing the same rotation and translation.
         * @note If your input quaternion is not normalized, then the dual quaternion will also not be normalized.
         */
        static DualQuaternion ConvertFromRotationTranslation(const AZ::Quaternion& rotation, const AZ::Vector3& translation);

        // operators
        MCORE_INLINE const DualQuaternion&  operator=(const AZ::Transform& transform)           { FromTransform(transform); return *this; }
        MCORE_INLINE const DualQuaternion&  operator=(const DualQuaternion& other)              { m_real = other.m_real; m_dual = other.m_dual; return *this; }
        MCORE_INLINE DualQuaternion         operator-() const                                   { return DualQuaternion(-m_real, -m_dual); }
        MCORE_INLINE const DualQuaternion&  operator+=(const DualQuaternion& q)                 { m_real += q.m_real; m_dual += q.m_dual; return *this; }
        MCORE_INLINE const DualQuaternion&  operator-=(const DualQuaternion& q)                 { m_real -= q.m_real; m_dual -= q.m_dual; return *this; }
        MCORE_INLINE const DualQuaternion&  operator*=(const DualQuaternion& q)                 { const AZ::Quaternion orgReal(m_real); m_real *= q.m_real; m_dual = orgReal * q.m_dual + q.m_real * m_dual; return *this; }
        MCORE_INLINE const DualQuaternion&  operator*=(float f)                                 { m_real *= f; m_dual *= f; return *this; }

        // attributes
        AZ::Quaternion  m_real;  /**< The real value, which you can see as the regular rotation quaternion. */
        AZ::Quaternion  m_dual;  /**< The dual part, which you can see as the translation part. */
    };


    // operators
    MCORE_INLINE DualQuaternion    operator*(const DualQuaternion& a, float f)                         { return DualQuaternion(a.m_real * f, a.m_dual * f); }
    MCORE_INLINE DualQuaternion    operator*(float f, const DualQuaternion& b)                         { return DualQuaternion(b.m_real * f, b.m_dual * f); }
    MCORE_INLINE DualQuaternion    operator+(const DualQuaternion& a, const DualQuaternion& b)         { return DualQuaternion(a.m_real + b.m_real, a.m_dual + b.m_dual); }
    MCORE_INLINE DualQuaternion    operator-(const DualQuaternion& a, const DualQuaternion& b)         { return DualQuaternion(a.m_real - b.m_real, a.m_dual - b.m_dual); }
    MCORE_INLINE DualQuaternion    operator*(const DualQuaternion& a, const DualQuaternion& b)
    {
        return DualQuaternion(a.m_real * b.m_real, a.m_real * b.m_dual + b.m_real * a.m_dual);
    }

    // include the inline code
#include "DualQuaternion.inl"
} // namespace MCore
