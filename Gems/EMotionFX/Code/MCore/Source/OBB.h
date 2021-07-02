/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include "StandardHeaders.h"


namespace MCore
{
    // forward declarations
    class AABB;


    /**
     * 3D Oriented Bounding Box (OBB) template.
     * This is basically a AABB with an arbitrary rotation.
     */
    class MCORE_API OBB
    {
    public:
        /**
         * The constructor.
         * This automatically initializes the box. After initialization the box is invalid since it basically has no size yet.
         * The IsValid() method will return false.
         */
        MCORE_INLINE OBB()                                                                      { Init(); }

        /**
         * Construct the OBB from a given axis aligned bounding box and a transformation.
         * @param aabb The axis aligned bounding box.
         * @param transformation The transformation of the box.
         */
        MCORE_INLINE OBB(const AABB& aabb, const AZ::Transform& transformation)                 { Create(aabb, transformation); }

        /**
         * Construct the OBB from a center, extends and a rotation.
         * @param center The center of the box.
         * @param extents The extents of the box, which start at the center of the box.
         * @param rot The matrix, representing the transformation of the box.
         */
        MCORE_INLINE OBB(const AZ::Vector3& center, const AZ::Vector3& extents, const AZ::Transform& rot)
            : mRotation(rot)
            , mExtents(extents)
            , mCenter(center)    {}

        /**
         * Reset the OBB with as center 0,0,0, infinite negative extents and no rotation.
         * This makes the box an invalid box as well, because the extents have not been set.
         */
        MCORE_INLINE void Init();

        /**
         * Initialize the box from a set of points.
         * This uses the covariant matrix and eigen vectors to fit the box to the set of points.
         * @param points The set of points to fit the box to.
         * @param numPoints The number of points inside array specified as first parameter.
         */
        void InitFromPoints(const AZ::Vector3* points, uint32 numPoints);

        /**
         * Check if this box OBB contains a given point or not.
         * @param p The point to check.
         * @result Returns true when the point is inside this box, otherwise false is returned.
         */
        bool Contains(const AZ::Vector3& p) const;

        /**
         * Check if this OBB is inside another specified box.
         * @param box The OBB to check.
         * @result Returns true when this OBB is inside the box specified as parameter.
         */
        bool CheckIfIsInside(const OBB& box) const;

        /**
         * Create the OBB from a given AABB and a matrix.
         * @param aabb The axis aligned bounding box.
         * @param mat The matrix, which represents the orientation of the box.
         */
        void Create(const AABB& aabb, const AZ::Transform& mat);

        /**
         * Transform this OBB with a given matrix.
         * This means the transformation specified as parameter will be applied to the current transformation of the OBB.
         * So the transformation specified is NOT an absolute rotation, but a relative transformation.
         * @param transMatrix The relative transformation matrix, to be applied to the current transformation.
         */
        void Transform(const AZ::Transform& transMatrix);

        /**
         * Calculate the transformed version of this OBB.
         * @param rotMatrix The transformation matrix to be applied to the rotation of this OBB, so not an absolute rotation!
         * @param outOBB A pointer to the OBB to fill with the rotated version of this OBB.
         */
        void Transformed(const AZ::Transform& rotMatrix, OBB* outOBB) const;

        /**
         * Check if this is a valid OBB or not.
         * The box is only valid if the extents are non-negative.
         * @result Returns true when the OBB is valid, otherwise false is returned.
         */
        MCORE_INLINE bool CheckIfIsValid() const;

        /**
         * Set the center of the box.
         * @param center The new center of the box.
         */
        MCORE_INLINE void SetCenter(const AZ::Vector3& center)                                      { mCenter = center; }

        /**
         * Set the extents of the box.
         * @param extents The new extents of the box.
         */
        MCORE_INLINE void SetExtents(const AZ::Vector3& extents)                                    { mExtents = extents; }

        /**
         * Set the transformation of the box.
         * @param transform The new transformation of the box.
         */
        MCORE_INLINE void SetTransformation(const AZ::Transform& transform)                         { mRotation = transform; }

        /**
         * Get the center of the box.
         * @result The center point of the box.
         */
        MCORE_INLINE const AZ::Vector3& GetCenter() const                                           { return mCenter; }

        /**
         * Get the extents of the box.
         * @result The extents of the box, which start at the center.
         */
        MCORE_INLINE const AZ::Vector3& GetExtents() const                                          { return mExtents; }

        /**
         * Get the transformation of the box.
         * @result The transformation of the box.
         */
        MCORE_INLINE const AZ::Transform& GetTransformation() const                                 { return mRotation; }

        /**
         * Calculate the 8 corner points of the box.
         * The layout is as follows:
         * <pre>
         *
         *     7+------+6
         *     /|     /|
         *    / |    / |
         *   / 4+---/--+5
         * 3+------+2 /
         *  | /    | /
         *  |/     |/
         * 0+------+1
         *
         * </pre>
         * @param outPoints the array of at least 8 vectors to write the points in.
         */
        void CalcCornerPoints(AZ::Vector3* outPoints) const;

        /**
         * Calculate the rotated minimum and maximum points of the box.
         * After rotation it is possible that the min point is not really the min anymore though. The same goes for max.
         * But the main use for this method however is to quickly approximate an AABB from this OBB, without having to
         * calculate all 8 corner points.
         * <pre>
         *
         *        +------+MAX
         *       /|     /|
         *      / |    / |
         *     /  +---/--+
         *    +------+  /
         *    | /    | /
         *    |/     |/
         * MIN+------+
         *
         * </pre>
         * @param outMin The vector that we will write the minimum point to.
         * @param outMax The vector that we will write the maximum point to.
         */
        void CalcMinMaxPoints(AZ::Vector3* outMin, AZ::Vector3* outMax) const;

    private:
        AZ::Transform mRotation;  /**< The rotation of the box. */     // TODO: store the center inside the translation component and extents inside last column?
        AZ::Vector3   mExtents;   /**< The extents of the box. */
        AZ::Vector3   mCenter;    /**< The center of the box. */

        /**
         * Calculate the three eigen vectors for a symmetric matrix.
         * @param A The symmetric matrix values.
         * @param v1 The first output eigen vector.
         * @param v2 The second output eigen vector.
         * @param v3 The third output eigen vector.
         */
        void GetRealSymmetricEigenvectors(const float A[6], AZ::Vector3& v1, AZ::Vector3& v2, AZ::Vector3& v3);

        /**
         * Calculate the eigen vector from a symmetric matrix.
         * This assumes that the specified eigenvalue is of order 1.
         * @param A The symmetric matrix values.
         * @param eigenValue The eigen value.
         * @param v1 The output eigen vector.
         */
        void CalcSymmetricEigenVector(const float A[6], float eigenValue, AZ::Vector3& v1);

        /**
         * Calculate the pair of eigen vectors from a symmetric matrix.
         * This assumes that the specified eigen value is of order 2.
         * @param A The symmetric matrix values.
         * @param eigenValue The eigen value.
         * @param v1 The first output eigen vector.
         * @param v2 The second output eigen vector.
         */
        void CalcSymmetricEigenPair(const float A[6], float eigenValue, AZ::Vector3& v1, AZ::Vector3& v2);

        /**
         * Calculate the covariance matrix from a set of points.
         * @param points The set of points to calculate the covariance matrix from.
         * @param numPoints The number of points inside the specified set of points.
         * @param mean The statistical mean will be output in this vector.
         * @param C The covariance matrix values that will be written to. Since the matrix is symmetric we only output one triangle of the 3x3 matrix.
         */
        void CovarianceMatrix(const AZ::Vector3 * points, uint32 numPoints, AZ::Vector3 & mean, float C[6]);

        void InitFromPointsRange(const AZ::Vector3* points, uint32 numPoints, float xDegrees, float* outMinArea, AABB* outMinBox, AZ::Transform* outMinMatrix);
    };

    // include the inline code
#include "OBB.inl"
}   // namespace MCore
