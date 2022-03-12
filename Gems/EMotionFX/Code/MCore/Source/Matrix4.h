/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// includes
#include <AzCore/Math/Vector4.h>
#include "StandardHeaders.h"
#include "Vector.h"
#include "PlaneEq.h"
#include "LogManager.h"
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>

// matrix order
#define MCORE_MATRIX_ROWMAJOR

// matrix element access
#ifdef MCORE_MATRIX_ROWMAJOR
    #define MMAT(matrix, row, col)  matrix.m44[row][col]
    #define TMAT(row, col)          m44[row][col]
#else
    #define MMAT(matrix, row, col)  matrix.m44[col][row]
    #define TMAT(row, col)          m44[col][row]
#endif


namespace AZ { class Quaternion; }

namespace MCore
{
    /**
     * Depracated. Please use AZ::Transform instead.
     * A 4x4 matrix class.
     * Matrices can for example be used to transform points or vectors.
     * Transforming means moving to another coordinate system. With matrices you can do things like: translate (move), rotate and scale.
     * One single matrix can store a translation, rotation and scale. If we have only a rotation inside the matrix, this means that if we
     * multiply the matrix with a vector, the vector will rotate by the rotation inside the matrix! The cool thing is that you can also
     * concatenate matrices. In other words, you can multiply matrices with eachother. If you have a rotation matrix, like described above, and
     * also have a translation matrix, then multiplying these two matrices with eachother will result in a new matrix, which contains both the
     * rotation and the translation! So when you multiply this resulting matrix with a vector, it will both translate and rotate.
     * But, does it first rotate and then translate in the rotated space? Or does it first translate and then rotate?
     * For example if you want to rotate a planet, while it's moving, you want to rotate it in it's local coordinate system. Like when you spin
     * a globe you can have on your desk. So where it spins around it's own center. However, if the planet is at location (10,10,10) in 3D space for example
     * it is also possible that rotates around the origin in world space (0,0,0). The order of multiplication between matrices matters.
     * This means that (matrixA * matrixB) does not have to not result in the same as (matrixB * matrixA).
     *
     * Here is some information about how the matrices are stored internally:
     *
     * [00 01 02 03] // m16 offsets <br>
     * [04 05 06 07] <br>
     * [08 09 10 11] <br>
     * [12 13 14 15] <br>
     *
     * [00 01 02 03] // m44 offsets --> [row][column] <br>
     * [10 11 12 13] <br>
     * [20 21 22 23] <br>
     * [30 31 32 33] <br>
     *
     * [Xx Xy Xz 0] // right <br>
     * [Yx Yy Yz 0] // up <br>
     * [Zx Zy Zz 0] // forward <br>
     * [Tx Ty Tz 1] // translation <br>
     *
     */
    class MCORE_API alignas(16) Matrix
    {
    public:
        /**
         * Default constructor.
         * This leaves the matrix uninitialized.
         */
        MCORE_INLINE Matrix()  {}

        /**
         * Init the matrix using given float data.
         * The number of elements stored at the float pointer location that is used as parameter must be at least 16 floats in size.
         * @param elementData A pointer to the matrix float data, which must be 16 floats in size, or more, although only the first 16 floats are used.
         */
        MCORE_INLINE explicit Matrix(const float* elementData)                                          { MCore::MemCopy(m_m16, elementData, sizeof(float) * 16); }

        /**
         * Copy constructor.
         * @param m The matrix to copy the data from.
         */
        MCORE_INLINE Matrix(const Matrix& m);

        MCORE_INLINE Matrix(const AZ::Matrix4x4& m)
        {
            TMAT(0, 0) = m.GetElement(0, 0);
            TMAT(0, 1) = m.GetElement(0, 1);
            TMAT(0, 2) = m.GetElement(0, 2);
            TMAT(0, 3) = m.GetElement(0, 3);
            TMAT(1, 0) = m.GetElement(1, 0);
            TMAT(1, 1) = m.GetElement(1, 1);
            TMAT(1, 2) = m.GetElement(1, 2);
            TMAT(1, 3) = m.GetElement(1, 3);
            TMAT(2, 0) = m.GetElement(2, 0);
            TMAT(2, 1) = m.GetElement(2, 1);
            TMAT(2, 2) = m.GetElement(2, 2);
            TMAT(2, 3) = m.GetElement(2, 3);
            TMAT(3, 0) = m.GetElement(3, 0);
            TMAT(3, 1) = m.GetElement(3, 1);
            TMAT(3, 2) = m.GetElement(3, 2);
            TMAT(3, 3) = m.GetElement(3, 3);
        }

        void InitFromPosRot(const AZ::Vector3& pos, const AZ::Quaternion& rot);
        void InitFromPosRotScale(const AZ::Vector3& pos, const AZ::Quaternion& rot, const AZ::Vector3& scale);
        void InitFromNoScaleInherit(const AZ::Vector3& translation, const AZ::Quaternion& rotation, const AZ::Vector3& scale, const AZ::Vector3& invParentScale);
        void InitFromPosRotScaleScaleRot(const AZ::Vector3& pos, const AZ::Quaternion& rot, const AZ::Vector3& scale, const AZ::Quaternion& scaleRot);
        void InitFromPosRotScaleShear(const AZ::Vector3& pos, const AZ::Quaternion& rot, const AZ::Vector3& scale, const AZ::Vector3& shear);

        /**
         * Sets the matrix to identity.
         * When a matrix is an identity matrix it will have no influence.
         */
        void Identity();

        /**
         * Makes the matrix a scaling matrix.
         * Values of 1.0 would have no influence on the scale. Values of for example 2.0 would scale up by a factor of two.
         * @param s The vector containing the scale values for each axis.
         */
        void SetScaleMatrix(const AZ::Vector3& s);

        /**
         * Makes this matrix a shear matrix from three different shear matrices: XY, XZ and YZ.
         * The multiplication order is YZ * XZ * XY.
         * @param s The shear values (x=XY, y=XZ, z=YZ)
         */
        void SetShearMatrix(const AZ::Vector3& s);

        /**
         * Makes this matrix a translation matrix.
         * @param t The translation value.
         */
        void SetTranslationMatrix(const AZ::Vector3& t);

        /**
         * Initialize this matrix into a rotation matrix from a AZ::Quaternion.
         * @param rotation The AZ::Quaternion representing the rotatation.
         */
        void SetRotationMatrix(const AZ::Quaternion& rotation);

        /**
         * Makes the matrix an rotation matrix along the x-axis.
         * @param angle The angle to rotate around this axis, in radians.
         */
        void SetRotationMatrixX(float angle);

        /**
         * Makes the matrix a rotation matrix along the y-axis.
         * @param angle The angle to rotate around this axis, in radians.
         */
        void SetRotationMatrixY(float angle);

        /**
         * Makes the matrix a rotation matrix along the z-axis.
         * @param angle The angle to rotate around this axis, in radians.
         */
        void SetRotationMatrixZ(float angle);

        /**
         * Makes the matrix a rotation matrix around a given axis with a given angle.
         * @param axis The axis to rotate around.
         * @param angle The angle to rotate around this axis, in radians.
         */
        void SetRotationMatrixAxisAngle(const AZ::Vector3& axis, float angle);

        /**
         * Makes the matrix a rotation matrix given the euler angles.
         * The multiplication order is RotMatrix(v.z) * RotMatrix(v.y) * RotMatrix(v.x).
         * @param anglevec The vector containing the angles for each axis, in radians, so (pitch, yaw, roll) as (x,y,z).
         */
        void SetRotationMatrixEulerZYX(const AZ::Vector3& anglevec);

        /**
         * Initialize the matrix from a yaw, pitch and roll.
         * Pitch is the rotation around the x-axis.
         * Yaw is the rotation aroudn the y-axis.
         * Roll is the rotation around the z-axis.
         * All angles are in radians.
         * @param pitch The pitch angle (rotation around x-axis), in radians.
         * @param yaw The yaw angle (rotation around y-axis), in radians.
         * @param roll The roll angle (rotation around z-axis), in radians.
         */
        void SetRotationMatrixPitchYawRoll(float pitch, float yaw, float roll);

        /**
         * Initialize the matrix from a yaw, pitch and roll.
         * Pitch is the rotation around the x-axis.
         * Yaw is the rotation aroudn the y-axis.
         * Roll is the rotation around the z-axis.
         * All angles are in radians.
         * @param angles The angle for each axis, in radians.
         */
        MCORE_INLINE void SetRotationMatrixPitchYawRoll(const AZ::Vector3& angles)              { SetRotationMatrixPitchYawRoll(angles.GetX(), angles.GetY(), angles.GetZ()); }

        /**
         * Makes the matrix a rotation matrix given the euler angles.
         * The multiplication order is RotMatrix(v.x) * RotMatrix(v.y) * RotMatrix(v.z).
         * @param anglevec The vector containing the angles for each axis, in radians, so (pitch, yaw, roll) as (x,y,z).
         */
        void SetRotationMatrixEulerXYZ(const AZ::Vector3& anglevec);

        /**
         * Inverse rotate a vector with this matrix.
         * This means that the vector will be multiplied with the inverse rotation of this matrix.
         * @param v The vector to rotate.
         * @result The rotated vector.
         */
        AZ::Vector3 InverseRot(const AZ::Vector3& v);

        /**
         * Multiply this matrix with another matrix and stores the result in itself.
         * @param right The matrix to multiply with.
         */
        void MultMatrix(const Matrix& right);

        /**
         * Multiply this matrix with another matrix, but only multiply the 4x3 part.
         * So treat the other matrix as 4x3 matrix instead of 4x4 matrix. Stores the result in itself.
         * @param right The matrix to multiply with.
         */
        void MultMatrix4x3(const Matrix& right);

        /**
         * Multiply two matrices together, but only the 4x3 part, and store the result in itself.
         * @param left The left matrix.
         * @param right The right matrix.
         */
        void MultMatrix4x3(const Matrix& left, const Matrix& right);

        /**
         * Multiply the left matrix with the right matrix and store the result in this matrix object.
         * So basically this is a fast version of:<br>
         * <pre>
         * Matrix result = left * right;    // where left and right are also Matrix objects
         * </pre>
         *
         * @param left The left matrix of the matrix multiply.
         * @param right The right matrix of the matrix multiply.
         */
        void MultMatrix(const Matrix& left, const Matrix& right);

        /**
         * Multiply this matrix with the 3x3 rotation part of the other given matrix, and modify itself.
         * @param right The matrix to multiply with.
         */
        void MultMatrix3x3(const Matrix& right);

        MCORE_INLINE void Skin(const AZ::Vector3* inPos, const AZ::Vector3* inNormal, AZ::Vector3* outPos, AZ::Vector3* outNormal, float weight);
        MCORE_INLINE void Skin(const AZ::Vector3* inPos, const AZ::Vector3* inNormal, const AZ::Vector4* inTangent, AZ::Vector3* outPos, AZ::Vector3* outNormal, AZ::Vector4* outTangent, float weight);
        MCORE_INLINE void Skin(const AZ::Vector3* inPos, const AZ::Vector3* inNormal, const AZ::Vector4* inTangent, const AZ::Vector3* inBitangent, AZ::Vector3* outPos, AZ::Vector3* outNormal, AZ::Vector4* outTangent, AZ::Vector3* outBitangent, float weight);

        /**
         * Perform skinning on an input vertex, and add the result to the output, weighted by a weight value.
         * The calculation performed is:
         *
         * <pre>
         * out += (in * thisMatrix) * weight;
         * </pre>
         *
         * Only the 4x3 part of the matrix is used during the matrix multiply.
         * So this should be used when skinning for example positions.
         * @param in The input vector to skin.
         * @param out The output vector. Keep in mind that the result will be added to the output vector.
         * @param weight The weight value.
         */
        MCORE_INLINE void Skin4x3(const AZ::Vector3& in, AZ::Vector3& out, float weight);

        /**
         * Perform skinning on an input vertex, and add the result to the output, weighted by a weight value.
         * The calculation performed is:
         *
         * <pre>
         * out += (in * thisMatrix) * weight;
         * </pre>
         *
         * Only the 3x3 part of the matrix is used during the matrix multiply.
         * So this should be used to skin normals and tangents.
         * @param in The input vector to skin.
         * @param out The output vector. Keep in mind that the result will be added to the output vector.
         * @param weight The weight value.
         */
        MCORE_INLINE void Skin3x3(const AZ::Vector3& in, AZ::Vector3& out, float weight);

        /**
         * Transpose the matrix (swap rows with columns).
         */
        void Transpose();

        /**
         * Transpose the translation 1x3 translation part.
         * Leaves the rotation in tact.
         */
        void TransposeTranslation();

        /**
         * Adjoint this matrix.
         */
        void Adjoint();

        /**
         * Inverse this matrix.
         */
        void Inverse();

        /**
         * Makes this the inverse tranpose version of this matrix.
         * The inverse transpose is the transposed version of the inverse.
         */
        MCORE_INLINE void InverseTranspose();

        /**
         * Returns the inverse transposed version of this matrix.
         * The inverse transpose is the transposed version of the inverse.
         * @result The inverse transposed version of this matrix.
         */
        MCORE_INLINE Matrix InverseTransposed() const;

        /**
         * Orthonormalize this matrix (to prevent skewing or other errors).
         * This normalizes the x, y and z vectors of the matrix.
         * It makes sure that the axis vectors are perpendicular to eachother.
         */
        void OrthoNormalize();

        /**
         * Normalizes the matrix, which means that all axis vectors (right, up, forward)
         * will be made of unit length.
         */
        void Normalize();

        /**
         * Returns a normalized version of this matrix.
         * @result The normalized version of this matrix, where the  right, up and forward vectors are of unit length.
         */
        MCORE_INLINE Matrix Normalized() const;

        /**
         * Scale this matrix.
         * @param scale The scale factors for each axis.
         */
        MCORE_INLINE void Scale(const AZ::Vector3& scale);

        /**
         * Scale only the upper left 3x3 part of this matrix.
         * So this doesn't scale the translation part.
         * @param scale The scale factors for each axis.
         */
        void Scale3x3(const AZ::Vector3& scale);

        AZ::Vector3 ExtractScale();

        /**
         * Rotate this matrix around the x-axis.
         * @param angle The rotation in radians.
         */
        void RotateX(float angle);

        /**
         * Rotate this matrix around the y-axis.
         * @param angle The rotation in radians.
         */
        void RotateY(float angle);

        /**
         * Rotate this matrix around the z-axis.
         * @param angle The rotation in radians.
         */
        void RotateZ(float angle);

        /**
         * Initialize the matrix as a rotation matrix given two vectors. The resulting matrix rotates the vector 'from' such that it points
         * in the same direction as the vector 'to'.
         * @param from The vector that the resulting matrix rotates from.
         * @param to The vector that the resulting matrix rotates to.
         */
        void SetRotationMatrixTwoVectors(const AZ::Vector3& from, const AZ::Vector3& to);

        /**
         * Multiply a vector with the 3x3 rotation part of this matrix.
         * @param v The vector to transform.
         * @result The transformed (rotated) vector.
         */
        MCORE_INLINE AZ::Vector3 Mul3x3(const AZ::Vector3& v) const;

        /**
         * Returns the inversed version of this matrix.
         * @result The inversed version of this matrix.
         */
        MCORE_INLINE Matrix Inversed() const                                                    { Matrix m(*this); m.Inverse(); return m;   }

        /**
         * Returns the transposed version of this matrix.
         * @result The transposed version of this matrix.
         */
        MCORE_INLINE Matrix Transposed() const                                                  { Matrix m(*this); m.Transpose(); return m; }

        /**
         * Returns the adjointed version of this matrix.
         * @result The adjointed version of this matrix.
         */
        MCORE_INLINE Matrix Adjointed() const                                                   { Matrix m(*this); m.Adjoint(); return m;   }

        /**
         * Translate the matrix.
         * @param x The number of units to add to the current translation along the x-axis.
         * @param y The number of units to add to the current translation along the y-axis.
         * @param z The number of units to add to the current translation along the z-axis.
         */
        MCORE_INLINE void Translate(float x, float y, float z)                                  { TMAT(3, 0) += x; TMAT(3, 1) += y; TMAT(3, 2) += z; }

        /**
         * Translate the matrix.
         * @param t The vector containing the translation to add to the current translation of the matrix.
         */
        MCORE_INLINE void Translate(const AZ::Vector3& t)                                           { TMAT(3, 0) += t.GetX(); TMAT(3, 1) += t.GetY(); TMAT(3, 2) += t.GetZ(); }

        /**
         * Set the values for a given row, using a 3D vector.
         * Only the first 3 values on the row will be touched, so the 4th value will remain untouched inside the specified row of the matrix.
         * @param row A zero-based index of the row.
         * @param value The values to set in the row.
         */
        MCORE_INLINE void SetRow(uint32 row, const AZ::Vector3& value)                      { TMAT(row, 0) = value.GetX(); TMAT(row, 1) = value.GetY(); TMAT(row, 2) = value.GetZ(); }

        /**
         * Set the values in the given row, using a 4D vector.
         * @param row A zero-based index of the row.
         * @param value The values to set in the row.
         */
        MCORE_INLINE void SetRow(uint32 row, const AZ::Vector4& value)                      { TMAT(row, 0) = value.GetX(); TMAT(row, 1) = value.GetY(); TMAT(row, 2) = value.GetZ(); TMAT(row, 3) = value.GetW(); }

        /**
         * Set the values for a given column, using a 3D vector.
         * Only the first 3 values on the column will be touched, so the 4th value will remain untouched inside the specified column of the matrix.
         * @param column A zero-based index of the column.
         * @param value The values to set in the column.
         */
        MCORE_INLINE void SetColumn(uint32 column, const AZ::Vector3& value)                    { TMAT(0, column) = value.GetX(); TMAT(1, column) = value.GetY(); TMAT(2, column) = value.GetZ(); }

        /**
         * Set the values for a given column, using a 4D vector.
         * @param column A zero-based index of the column.
         * @param value The values to set in the column.
         */
        MCORE_INLINE void SetColumn(uint32 column, const AZ::Vector4& value)                    { TMAT(0, column) = value.GetX(); TMAT(1, column) = value.GetY(); TMAT(2, column) = value.GetZ(); TMAT(3, column) = value.GetW(); }

        /**
         * Get the values of a given row as 3D vector.
         * @param row A zero-based index of the row number ot get.
         * @result The vector containing the values of the specified row.
         */
        MCORE_INLINE AZ::Vector3 GetRow(uint32 row) const                                       { return AZ::Vector3(TMAT(row, 0), TMAT(row, 1), TMAT(row, 2)); }

        /**
         * Get the values of a given row as 4D vector.
         * @param column A zero-based index of the row number ot get.
         * @result The vector containing the values of the specified row.
         */
        MCORE_INLINE AZ::Vector3 GetColumn(uint32 column) const                             { return AZ::Vector3(TMAT(0, column), TMAT(1, column), TMAT(2, column)); }

        /**
         * Get the values of a given column as 3D vector.
         * @param row A zero-based index of the column number ot get.
         * @result The vector containing the values of the specified column.
         */
        MCORE_INLINE AZ::Vector4 GetRow4D(uint32 row) const                                 { return AZ::Vector4(TMAT(row, 0), TMAT(row, 1), TMAT(row, 2), TMAT(row, 3)); }

        /**
         * Get the values of a given column as 4D vector.
         * @param column A zero-based index of the column number ot get.
         * @result The vector containing the values of the specified column.
         */
        MCORE_INLINE AZ::Vector4 GetColumn4D(uint32 column) const                               { return AZ::Vector4(TMAT(0, column), TMAT(1, column), TMAT(2, column), TMAT(3, column)); }

        /**
         * Set the right vector (must be normalized).
         * @param xx The x component of the right vector.
         * @param xy The y component of the right vector.
         * @param xz The z component of the right vector.
         */
        MCORE_INLINE void SetRight(float xx, float xy, float xz);

        /**
         * Set the right vector.
         * @param x The right vector, must be normalized.
         */
        MCORE_INLINE void SetRight(const AZ::Vector3& x);

        /**
         * Set the up vector (must be normalized).
         * @param yx The x component of the up vector.
         * @param yy The y component of the up vector.
         * @param yz The z component of the up vector.
         */
        MCORE_INLINE void SetUp(float yx, float yy, float yz);

        /**
         * Set the up vector (must be normalized).
         * @param y The up vector.
         */
        MCORE_INLINE void SetUp(const AZ::Vector3& y);

        /**
         * Set the forward vector (must be normalized).
         * @param zx The x component of the forward vector.
         * @param zy The y component of the forward vector.
         * @param zz The z component of the forward vector.
         */
        MCORE_INLINE void SetForward(float zx, float zy, float zz);

        /**
         * Set the forward vector (must be normalized).
         * @param z The forward vector.
         */
        MCORE_INLINE void SetForward(const AZ::Vector3& z);

        /**
         * Set the translation part of the matrix.
         * @param tx The translation along the x-axis.
         * @param ty The translation along the y-axis.
         * @param tz The translation along the z-axis.
         */
        MCORE_INLINE void SetTranslation(float tx, float ty, float tz);

        /**
         * Set the translation part of the matrix.
         * @param t The translation vector.
         */
        MCORE_INLINE void SetTranslation(const AZ::Vector3& t);

        /**
         * Get the right vector.
         * @result The right vector (x-axis).
         */
        MCORE_INLINE AZ::Vector3 GetRight() const;

        /**
         * Get the up vector.
         * @result The up vector (z-axis).
         */
        MCORE_INLINE AZ::Vector3 GetUp() const;

        /**
         * Get the forward vector.
         * @result The forward vector (y-axis).
         */
        MCORE_INLINE AZ::Vector3 GetForward() const;

        /**
         * Get the translation part of the matrix.
         * @result The vector containing the translation.
         */
        MCORE_INLINE AZ::Vector3 GetTranslation() const;

        /**
         * Calculates the determinant of the matrix.
         * @result The determinant.
         */
        float CalcDeterminant() const;

        /**
         * Calculates the euler angles.
         * @result The euler angles, describing the rotation along each axis, in radians.
         */
        AZ::Vector3 CalcEulerAngles() const;

        /**
         * Calculate the pitch, yaw and roll.
         * Pitch is the rotation around the x axis.
         * Yaw is the rotation around the y axis.
         * Roll is the rotation around the z axis.
         * All angles returned are in radians.
         * @result The vector containing the rotation around each axis (x=pitch, y=yaw, z=roll).
         */
        AZ::Vector3 CalcPitchYawRoll() const;

        /**
         * Makes this matrix a mirrored version of a specified matrix.
         * After executing this operation this matrix is the mirrored version of the specified matrix.
         * @param transform The transformation matrix to mirror (so the original matrix).
         * @param plane The plane to use as mirror.
         */
        void Mirror(const Matrix& transform, const PlaneEq& plane);

        /**
         * Makes this matrix a lookat matrix (also known as camera or view matrix).
         * @param view The view position, so the position of the camera.
         * @param target The target position, so where the camera is looking at.
         * @param up The up vector, describing the roll of the camera, where (0,1,0) would mean the camera is straight up and has no roll and
         * where (0,-1,0) would mean the camera is upside down, etc.
         */
        void LookAt(const AZ::Vector3& view, const AZ::Vector3& target, const AZ::Vector3& up);

        /**
         * Makes this matrix a lookat matrix (also known as camera or view matrix), in right handed mode.
         * @param view The view position, so the position of the camera.
         * @param target The target position, so where the camera is looking at.
         * @param up The up vector, describing the roll of the camera, where (0,1,0) would mean the camera is straight up and has no roll and
         * where (0,-1,0) would mean the camera is upside down, etc.
         */
        void LookAtRH(const AZ::Vector3& view, const AZ::Vector3& target, const AZ::Vector3& up);

        /**
         * Makes this matrix a perspective projection matrix.
         * @param fov The field of view, in radians.
         * @param aspect The aspect ratio which is the width divided by height.
         * @param zNear The distance to the near plane.
         * @param zFar The distance to the far plane.
         */
        void Perspective(float fov, float aspect, float zNear, float zFar);

        /**
         * Makes this matrix a perspective projection matrix, in right handed mode.
         * @param fov The field of view, in radians.
         * @param aspect The aspect ratio which is the width divided by height.
         * @param zNear The distance to the near plane.
         * @param zFar The distance to the far plane.
         */
        void PerspectiveRH(float fov, float aspect, float zNear, float zFar);

        /**
         * Makes this matrix an ortho projection matrix, so without perspective.
         * @param left The left of the image plane.
         * @param right The right of the image plane.
         * @param top The top of the image plane.
         * @param bottom The bottom of the image plane.
         * @param znear The distance to the near plane.
         * @param zfar The distance to the far plane.
         */
        void Ortho(float left, float right, float top, float bottom, float znear, float zfar);

        /**
         * Makes this matrix an ortho projection matrix, so without perspective.
         * @param left The left of the image plane.
         * @param right The right of the image plane.
         * @param top The top of the image plane.
         * @param bottom The bottom of the image plane.
         * @param znear The distance to the near plane.
         * @param zfar The distance to the far plane.
         */
        void OrthoRH(float left, float right, float top, float bottom, float znear, float zfar);

        /**
         * Makes this matrix an ortho projection matrix, so without perspective.
         * @param left The left of the image plane.
         * @param right The right of the image plane.
         * @param top The top of the image plane.
         * @param bottom The bottom of the image plane.
         * @param znear The distance to the near plane.
         * @param zfar The distance to the far plane.
         */
        void OrthoOffCenter(float left, float right, float top, float bottom, float znear, float zfar);

        /**
         * Makes this matrix an ortho projection matrix, so without perspective.
         * @param left The left of the image plane.
         * @param right The right of the image plane.
         * @param top The top of the image plane.
         * @param bottom The bottom of the image plane.
         * @param znear The distance to the near plane.
         * @param zfar The distance to the far plane.
         */
        void OrthoOffCenterRH(float left, float right, float top, float bottom, float znear, float zfar);

        /**
         * Makes this matrix a frustum matrix.
         * @param left The left of the image plane.
         * @param right The right of the image plane.
         * @param top The top of the image plane.
         * @param bottom The bottom of the image plane.
         * @param znear The distance to the near plane.
         * @param zfar The distance to the far plane.
         */
        void Frustum(float left, float right, float top, float bottom, float znear, float zfar);


        // QR Gram-Schmidt decomposition
        void DecomposeQRGramSchmidt(AZ::Vector3& translation, Matrix& rot) const;
        void DecomposeQRGramSchmidt(AZ::Vector3& translation, Matrix& rot, AZ::Vector3& scale) const;
        void DecomposeQRGramSchmidt(AZ::Vector3& translation, Matrix& rot, AZ::Vector3& scale, AZ::Vector3& shear) const;

        static Matrix OuterProduct(const AZ::Vector4& column, const AZ::Vector4& row);

        /**
         * Get the handedness of the matrix, which described if the matrix is left- or right-handed.
         * The value returned by this method is the dot product between the forward vector and the result of the
         * cross product between the right and up vector. So: DotProduct( Cross(right, up), forward );
         * If the value returned by this method is positive we are dealing with a matrix which is in a right-handed
         * coordinate system, otherwise we are dealing with a left-handed coordinate system.
         * Performing an odd number of reflections reverses the handedness. An even number of reflections is always
         * equivalent to a rotation, so any series of reflections can always be regarded as a single rotation followed
         * by at most one reflection.
         * If a reflection is present (think of a mirror) the handedness will be reversed. A reflection can be detected
         * by looking at the determinant of the matrix. If the determinant is negative, then a reflection is present.
         * @result The handedness of the matrix. If this value is positive we are dealing with a matrix in a right handed
         *         coordinate system. Otherwise we are dealing with one in a left-handed coordinate system.
         * @see IsRightHanded()
         * @see IsLeftHanded()
         */
        float CalcHandedness() const;

        /**
         * Check if this matrix is symmetric or not.
         * A materix is said to be symmetric if and only if M(i, j) = M(j, i).
         * That is, a matrix whose entries are symmetric about the main diagonal.
         * @param tolerance The maximum difference tolerance between the M(i, j) and M(j, i) entries.
         *                   The reason for having this tolerance is of course floating point inaccuracy which might have
         *                   caused some entries to be a bit different.
         * @result Returns true when the matrix is symmetric, or false when not.
         */
        bool CheckIfIsSymmetric(float tolerance = 0.00001f) const;

        /**
         * Check if this matrix is a diagonal matrix or not.
         * A matrix is said to be a diagonal matrix when only the entries on the diagonal contain non-zero values.
         * The tolerance value is needed because of possible floating point inaccuracies.
         * @param tolerance The maximum difference between 0 and the entry on the diagonal.
         * @result Returns true when the matrix is a diagonal matrix, otherwise false is returned.
         */
        bool CheckIfIsDiagonal(float tolerance = 0.00001f) const;

        /**
         * Check if the matrix is orthogonal or not.
         * A matrix is orthogonal if the vectors in the matrix form an orthonormal set.
         * This is when the vectors (right, up and forward) are perpendicular to eachother.
         * If a matrix is orthogonal, the inverse of the matrix is equal to the transpose of the matrix.
         * This assumption can be used to optimize specific calculations, since the inverse is slower to calculate than
         * the transpose of the matrix. Also it can speed up by transforming normals with the matrix.
         * In that example instead of having to use the inverse transpose matrix, you could just use the transpose of the matrix.
         * @param tolerance The maximum tolerance in the orthonormal test.
         * @result Returns true when the matrix is orthogonal, otherwise false is returned.
         */
        bool CheckIfIsOrthogonal(float tolerance = 0.00001f) const;

        /**
         * Check if the matrix is an identity matrix or not.
         * @param tolerance The maximum error value per entry in the matrix.
         * @result Returns true if this matrix is an identity matrix, otherwise false is returned.
         */
        bool CheckIfIsIdentity(float tolerance = 0.00001f) const;

        /**
         * Check if the matrix is left handed or not.
         * @result Returns true when the matrix is left handed, otherwise false is returned (so then it is right handed).
         */
        bool CheckIfIsLeftHanded() const;

        /**
         * Check if the matrix is right handed or not.
         * @result Returns true when the matrix is right handed, otherwise false is returned (so then it is left handed).
         */
        bool CheckIfIsRightHanded() const;

        /**
         * Check if this matrix is a pure rotation matrix or not.
         * @param tolerance The maximum error in the measurement.
         * @result Returns true when the matrix represents only a rotation, otherwise false is returned.
         */
        bool CheckIfIsPureRotationMatrix(float tolerance = 0.00001f) const;

        /**
         * Check if this matrix contains a reflection or not.
         * @result Returns true when the matrix represents a reflection, otherwise false is returned.
         */
        bool CheckIfIsReflective() const;

        AZ::Matrix4x4 ToAzMatrix() const
        {
#ifdef MCORE_MATRIX_ROWMAJOR
            return AZ::Matrix4x4::CreateFromRowMajorFloat16(m_m16);
#else
            return AZ::Matrix4x4::CreateFromColumnMajorFloat16(m16);
#endif
        }

        /**
         * Prints the matrix into the logfile or debug output, using MCore::LogDetailedInfo().
         * Please note that the values are printed using floats or doubles. So it is not possible
         * to use this method for printing matrices of vectors or something other than real numbers.
         */
        void Log() const;

        /**
         * Returns a translation matrix.
         * @param v The translation of the matrix.
         * @result The translation matrix having the specified translation.
         */
        static MCORE_INLINE Matrix TranslationMatrix(const AZ::Vector3& v)                      { Matrix m; m.SetTranslationMatrix(v); return m; }

        /**
         * Returns a rotation matrix from a AZ::Quaternion.
         * @param rot The AZ::Quaternion that represents the rotation.
         * @result A rotation matrix.
         */
        static MCORE_INLINE Matrix RotationMatrix(const AZ::Quaternion& rot)                    { Matrix m; m.SetRotationMatrix(rot); return m; }

        /**
         * Returns a rotation matrix, including a translation, where the rotation is represented by a AZ::Quaternion.
         * @param rot The AZ::Quaternion that represents the rotation.
         * @param trans The translation of the matrix.
         * @result The rotation matrix, that includes a translation as well.
         */
        static MCORE_INLINE Matrix RotationTranslationMatrix(const AZ::Quaternion& rot, const AZ::Vector3& trans)       { Matrix m; m.InitFromPosRot(trans, rot); return m; }

        /**
         * Returns a rotation matrix along the x-axis.
         * @param rad The angle of rotation, in radians.
         * @result A rotation matrix.
         */
        static MCORE_INLINE Matrix RotationMatrixX(float rad)                                   { Matrix m; m.SetRotationMatrixX(rad); return m; }

        /**
         * Returns a rotation matrix along the y-axis.
         * @param rad The angle of rotation, in radians.
         * @result A rotation matrix.
         */
        static MCORE_INLINE Matrix RotationMatrixY(float rad)                                   { Matrix m; m.SetRotationMatrixY(rad); return m; }

        /**
         * Returns a rotation matrix along the z-axis.
         * @param rad The angle of rotation, in radians.
         * @result A rotation matrix.
         */
        static MCORE_INLINE Matrix RotationMatrixZ(float rad)                                   { Matrix m; m.SetRotationMatrixZ(rad); return m; }

        /**
         * Returns a rotation matrix along the x, y and z-axis.
         * The multiplication order is RotMatrix(v.x) * RotMatrix(v.y) * RotMatrix(v.z).
         * @param eulerAngles The euler angles in radians.
         * @result A rotation matrix.
         */
        static MCORE_INLINE Matrix RotationMatrixEulerXYZ(const AZ::Vector3& eulerAngles)       { Matrix m; m.SetRotationMatrixEulerXYZ(eulerAngles); return m; }

        /**
         * Returns a rotation matrix along the x, y and z-axis.
         * The multiplication order is RotMatrix(v.z) * RotMatrix(v.y) * RotMatrix(v.x).
         * @param eulerAngles The euler angles in radians.
         * @result A rotation matrix.
         */
        static MCORE_INLINE Matrix RotationMatrixEulerZYX(const AZ::Vector3& eulerAngles)       { Matrix m; m.SetRotationMatrixEulerZYX(eulerAngles); return m; }

        /**
         * Returns a rotation matrix given a pitch, yaw and roll.
         * Pitch is the rotation around the x-axis.
         * Yaw is the rotation aroudn the y-axis.
         * Roll is the rotation around the z-axis.
         * @param angles The rotation angles for each axis, in radians.
         * @result A rotation matrix.
         */
        static MCORE_INLINE Matrix RotationMatrixPitchYawRoll(const AZ::Vector3& angles)        { Matrix m; m.SetRotationMatrixPitchYawRoll(angles); return m; }

        /**
         * Constructs a rotation matrix given two vectors. The resulting matrix rotates the vector 'from' such that it points
         * in the same direction as the vector 'to'.
         * @param from The vector that the resulting matrix rotates to.
         * @param to The vector that the resulting matrix rotates from.
         * @result The rotation matrix that rotates the vector 'from' into the vector 'to'.
         */
        static MCORE_INLINE Matrix RotationMatrixTwoVectors(const AZ::Vector3& from, const AZ::Vector3& to)     { Matrix m; m.SetRotationMatrixTwoVectors(from, to); return m; }

        /**
         * Returns a rotation matrix from a given axis and angle.
         * @param axis The axis to rotate around.
         * @param angle The angle of rotation, in radians.
         * @result A rotation matrix.
         */
        static MCORE_INLINE Matrix RotationMatrixAxisAngle(const AZ::Vector3& axis, float angle)    { Matrix m; m.SetRotationMatrixAxisAngle(axis, angle); return m; }

        /**
         * Returns a scale matrix from a given scaling factor.
         * @param s The vector containing the scaling factors for each axis.
         * @result A scaling matrix.
         */
        static MCORE_INLINE Matrix ScaleMatrix(const AZ::Vector3& s)                                { Matrix m; m.SetScaleMatrix(s); return m; }

        /**
         * Returns a shear matrix created from three different shear matrices: XY, XZ and YZ.
         * The multiplication order is YZ * XZ * XY.
         * @param s The shear values (x=XY, y=XZ, z=YZ)
         * @result The shear matrix.
         */
        static MCORE_INLINE Matrix ShearMatrix(const AZ::Vector3& s)                                { Matrix m; m.SetShearMatrix(s); return m; }

        // operators
        Matrix  operator +  (const Matrix& right) const;
        Matrix  operator -  (const Matrix& right) const;
        Matrix  operator *  (const Matrix& right) const;
        Matrix  operator *  (float value) const;
        Matrix& operator += (const Matrix& right);
        Matrix& operator -= (const Matrix& right);
        Matrix& operator *= (const Matrix& right);
        Matrix& operator *= (float value);
        MCORE_INLINE void operator = (const Matrix& right);

        // attributes
        union
        {
            float   m_m16[16];        // 16 floats as 1D array
            float   m44[4][4];      // as 2D array
        };
    };


    // include inline code
#include "Matrix4.inl"
}   // namespace MCore
