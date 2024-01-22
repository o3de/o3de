/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/Math/PackedVector3.h>
#include <MCore/Source/Algorithms.h>
#include <MCore/Source/Vector.h>
#include <EMotionFX/Source/Transform.h>

// This file is "glue" code to convert math back-forward between MCore and AZ. It also has functions that MCore used to 
// have and AZ doesn't have. Eventually all the conversions should disappear (once MCore math is removed) and the functionality
// that AZ doesnt have has to be moved there.
//

namespace MCore
{
    AZ_FORCE_INLINE AZ::Transform EmfxTransformToAzTransform(const EMotionFX::Transform& emfxTransform)
    {
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(emfxTransform.m_rotation, emfxTransform.m_position);
        EMFX_SCALECODE
        (
            transform.MultiplyByUniformScale(emfxTransform.m_scale.GetMaxElement());
        )
        return transform;
    }

    AZ_FORCE_INLINE EMotionFX::Transform AzTransformToEmfxTransform(const AZ::Transform& azTransform)
    {
        return EMotionFX::Transform(azTransform);
    }

    // O3DE_DEPRECATION_NOTICE(GHI-10471)
    //! @deprecated AZ::Quaternion::CreateFromAxisAngle where the axis is normalized should be equivelent to MCore::CreateFromAxisAndAngle
    AZ_FORCE_INLINE AZ::Quaternion CreateFromAxisAndAngle(const AZ::Vector3& axis, const float angle)
    {
        const float squaredLength = axis.GetLengthSq();
        if (squaredLength > 0.0f)
        {
            const float halfAngle = angle * 0.5f;
            const float sinScale = Math::Sin(halfAngle) / Math::Sqrt(squaredLength);
            return AZ::Quaternion(axis.GetX() * sinScale,
                axis.GetY() * sinScale,
                axis.GetZ() * sinScale,
                Math::Cos(halfAngle));
        }
        else
        {
            return AZ::Quaternion::CreateIdentity();
        }
    }

    //! O3DE_DEPRECATION_NOTICE(GHI-10508)
    //! @deprecated use AZ::Quaternion::CreateFromEulerRadiansZYX
    AZ_FORCE_INLINE AZ::Quaternion AzEulerAnglesToAzQuat(float pitch, float yaw, float roll)
    {
        // In the LY coordinate system, pitch: X, yaw: Z, roll: Y.
        const float halfYaw = yaw * 0.5f;
        const float halfPitch = pitch * 0.5f;
        const float halfRoll = roll * 0.5f;

        const float cY = Math::Cos(halfYaw);
        const float sY = Math::Sin(halfYaw);
        const float cP = Math::Cos(halfPitch);
        const float sP = Math::Sin(halfPitch);
        const float cR = Math::Cos(halfRoll);
        const float sR = Math::Sin(halfRoll);

        // Method 1:
        return AZ::Quaternion(
            cY * sP * cR - sY * cP * sR,
            cY * sP * sR + sY * cP * cR,
            cY * cP * sR - sY * sP * cR,
            cY * cP * cR + sY * sP * sR
        );

        // Method 2:
        /*const AZ::Quaternion Qx = MCore::CreateFromAxisAndAngle(AZ::Vector3(sP, 0, 0), cP);
        const AZ::Quaternion Qy = MCore::CreateFromAxisAndAngle(AZ::Vector3(0, sY, 0), cY);
        const AZ::Quaternion Qz = MCore::CreateFromAxisAndAngle(AZ::Vector3(0, 0, sR), cR);

        return Qx * Qy * Qz;*/

        //  Normalize();    // we might be able to leave the normalize away, but better safe than not, this is more robust :)
    }

    AZ_FORCE_INLINE AZ::Quaternion AzEulerAnglesToAzQuat(const AZ::Vector3& eulerAngles)
    {
        return AzEulerAnglesToAzQuat(eulerAngles.GetX(), eulerAngles.GetY(), eulerAngles.GetZ());
    }

    AZ_FORCE_INLINE AZ::Vector3 AzQuaternionToEulerAngles(const AZ::Quaternion& q)
    {
        /*
            // METHOD #1:

            Vector3 euler;

            float matrix[3][3];
            float cx,sx;
            float cy,sy,yr;
            float cz,sz;

            matrix[0][0] = 1.0 - (2.0 * y * y) - (2.0 * z * z);
            matrix[1][0] = (2.0 * x * y) + (2.0 * w * z);
            matrix[2][0] = (2.0 * x * z) - (2.0 * w * y);
            matrix[2][1] = (2.0 * y * z) + (2.0 * w * x);
            matrix[2][2] = 1.0 - (2.0 * x * x) - (2.0 * y * y);

            sy = -matrix[2][0];
            cy = Math::Sqrt(1 - (sy * sy));
            yr = Math::ATan2(sy,cy);
            euler.y = yr;

            // avoid divide by zero only where y ~90 or ~270
            if (sy != 1.0 && sy != -1.0)
            {
                cx = matrix[2][2] / cy;
                sx = matrix[2][1] / cy;
                euler.x = Math::ATan2(sx,cx);

                cz = matrix[0][0] / cy;
                sz = matrix[1][0] / cy;
                euler.z = Math::ATan2(sz,cz);
            }
            else
            {
                matrix[1][1] = 1.0 - (2.0 * x * x) - (2.0 * z * z);
                matrix[1][2] = (2.0 * y * z) - (2.0 * w * x);
                cx = matrix[1][1];
                sx = -matrix[1][2];
                euler.x = Math::ATan2(sx,cx);

                cz = 1.0;
                sz = 0.0;
                euler.z = Math::ATan2(sz,cz);
            }

            return euler;
        */

        /*
            // METHOD #2:
            Matrix mat = ToMatrix();

            //
            float cy = Math::Sqrt(mat.m44[0][0]*mat.m44[0][0] + mat.m44[0][1]*mat.m44[0][1]);
            if (cy > 16.0*Math::epsilon)
            {
                result.x = -atan2(mat.m44[1][2], mat.m44[2][2]);
                result.y = -atan2(-mat.m44[0][2], cy);
                result.z = -atan2(mat.m44[0][1], mat.m44[0][0]);
            }
            else
            {
                result.x = -atan2(-mat.m44[2][1], mat.m44[1][1]);
                result.y = -atan2(-mat.m44[0][2], cy);
                result.z = 0.0;
            }

            return result;
        */

        // METHOD #3 (without conversion to matrix first):
        // TODO: safety checks?
        float m00 = 1.0f - (2.0f * ((q.GetY() * q.GetY()) + q.GetZ() * q.GetZ()));
        float m01 = 2.0f * (q.GetX() * q.GetY() + q.GetW() * q.GetZ());

        AZ::Vector3 result(
            Math::ATan2(2.0f * (q.GetY() * q.GetZ() + q.GetW() * q.GetX()), 1.0f - (2.0f * ((q.GetX() * q.GetX()) + (q.GetY() * q.GetY())))),
            Math::ATan2(-2.0f * (q.GetX() * q.GetZ() - q.GetW() * q.GetY()), Math::Sqrt((m00 * m00) + (m01 * m01))),
            Math::ATan2(m01, m00)
        );

        return result;
    }

    AZ_FORCE_INLINE AZ::Quaternion NLerp(const AZ::Quaternion& left, const AZ::Quaternion& right, float t)
    {
        const float omt = 1.0f - t;
        const float dot = left.Dot(right);
        if (dot < 0.0f)
        {
            t = -t;
        }

        // calculate the interpolated values
        const AZ::Quaternion newQuat = omt * left + t * right;

        // calculate the inverse length
        const float invLen = Math::InvSqrt(newQuat.Dot(newQuat));

        // return the normalized linear interpolation
        return invLen * newQuat;
    }

    AZ_FORCE_INLINE AZ::Vector3 CalcUpAxis(const AZ::Quaternion& q)
    {
        // TODO: make it more vector friendly, something like this:
        /*const AZ::Vector3 c1(0.0f, 0.0f, 1.0f);
        const AZ::Vector3 c2(2.0f, 2.0f, -2.0f);
        const AZ::Vector3 v1(q.GetX(), q.GetY(), q.GetZ());
        const AZ::Vector3 v2(q.GetZ(), q.GetZ(), q.GetX());
        const AZ::Vector3 c3(2.0f, -2.0f, -2.0f);
        const AZ::Vector3 v3(q.GetY(), q.GetX(), q.GetY());
        const AZ::Vector3 v4(q.GetW(), q.GetW(), q.GetY());
        return c1 + c2 * v1 * v2 + c3 * v3 * v4;*/

        return AZ::Vector3(2.0f * q.GetX() * q.GetZ() + 2.0f * q.GetY() * q.GetW(),
            2.0f * q.GetY() * q.GetZ() - 2.0f * q.GetX() * q.GetW(),
            1.0f - 2.0f * q.GetX() * q.GetX() - 2.0f * q.GetY() * q.GetY());
    }

    AZ_FORCE_INLINE AZ::Vector3 CalcForwardAxis(const AZ::Quaternion& q)
    {
        // TODO: make it more vector friendly, something like this:
        /*const AZ::Vector3 c1(0.0f, 1.0f, 0.0f);
        const AZ::Vector3 c2(2.0f, -2.0f, 2.0f);
        const AZ::Vector3 v1(q.GetX(), q.GetX(), q.GetY());
        const AZ::Vector3 v2(q.GetY(), q.GetX(), q.GetZ());
        const AZ::Vector3 c3(-2.0f, -2.0f, 2.0f);
        const AZ::Vector3 v3(q.GetZ(), q.GetZ(), q.GetX());
        const AZ::Vector3 v4(q.GetW(), q.GetZ(), q.GetW());
        return c1 + c2 * v1 * v2 + c3 * v3 * v4;*/

        return AZ::Vector3(2.0f * q.GetX() * q.GetY() - 2.0f * q.GetZ() * q.GetW(),
            1.0f - 2.0f * q.GetX() * q.GetX() - 2.0f * q.GetZ() * q.GetZ(),
            2.0f * q.GetY() * q.GetZ() + 2.0f * q.GetX() * q.GetW());
    }

    // TODO: replace in favor of AzFramework::ConvertQuaternionToAxisAngle
    AZ_FORCE_INLINE void ToAxisAngle(const AZ::Quaternion& q, AZ::Vector3& axis, float& angle)
    {
        angle = 2.0f * Math::ACos(q.GetW());

        const float sinHalfAngle = Math::Sin(angle * 0.5f);
        if (sinHalfAngle > 0.0f)
        {
            const float invS = 1.0f / sinHalfAngle;
            axis.Set(q.GetX() * invS, q.GetY() * invS, q.GetZ() * invS);
        }
        else
        {
            axis.Set(0.0f, 1.0f, 0.0f);
            angle = 0.0f;
        }
    }

    AZ_FORCE_INLINE float GetEulerZ(const AZ::Quaternion& q)
    {
        float m00 = 1.0f - (2.0f * ((q.GetY() * q.GetY()) + q.GetZ() * q.GetZ()));
        float m01 = 2.0f * (q.GetX() * q.GetY() + q.GetW() * q.GetZ());
        return Math::ATan2(m01, m00);
    }

    AZ_FORCE_INLINE AZ::Quaternion CreateDeltaRotation(const AZ::Vector3& fromVector, const AZ::Vector3& toVector)
    {
        const float dot = fromVector.Dot(toVector);
        if (dot < 0.99999f) // we have rotated compared to the forward direction
        {
            const float angleRadians = Math::ACos(dot);
            const AZ::Vector3 rotAxis = fromVector.Cross(toVector);
            return CreateFromAxisAndAngle(rotAxis, angleRadians);
        }
        else
        {
            return AZ::Quaternion::CreateIdentity();
        }
    }

    AZ_FORCE_INLINE void RotateFromTo(AZ::Quaternion& q, const AZ::Vector3& fromVector, const AZ::Vector3& toVector)
    {
        q = CreateDeltaRotation(fromVector, toVector) * q;
        q.Normalize();
    }

    AZ_FORCE_INLINE AZ::Quaternion LogN(const AZ::Quaternion& q)
    {
        const float r = Math::Sqrt(q.GetX() * q.GetX() + q.GetY() * q.GetY() + q.GetZ() * q.GetZ()); 
        const float t = (r > 0.00001f) ? Math::ATan2(r, q.GetW()) / r : 0.0f; 
        return AZ::Quaternion(t * q.GetX(), t * q.GetY(), t * q.GetZ(), 0.5f * Math::Log(q.GetLengthSq()));
    }

    MCORE_INLINE AZ::Quaternion Exp(const AZ::Quaternion& q) 
    { 
        const float r = Math::Sqrt(q.GetX() * q.GetX() + q.GetY() * q.GetY() + q.GetZ() * q.GetZ());
        const float expW = Math::Exp(q.GetW()); 
        const float s = (r >= 0.00001f) ? expW * Math::Sin(r) / r : 0.0f; 
        return AZ::Quaternion(s * q.GetX(), s * q.GetY(), s * q.GetZ(), expW * Math::Cos(r)); 
    }
    
    AZ_FORCE_INLINE void LookAt(AZ::Matrix4x4& matrix, const AZ::Vector3& view, const AZ::Vector3& target, const AZ::Vector3& up)
    {
        const AZ::Vector3 z = (target - view).GetNormalized();
        const AZ::Vector3 x = (up.Cross(z)).GetNormalized();
        const AZ::Vector3 y = z.Cross(x);

        matrix.SetRow(0, x, -x.Dot(view));
        matrix.SetRow(1, y, -y.Dot(view));
        matrix.SetRow(2, z, -z.Dot(view));
        matrix.SetRow(3, 0.0f, 0.0f, 0.0f, 1.0f);

        // DirectX:
        //  zaxis = normal(cameraTarget - cameraPosition)
        //  xaxis = normal(cross(cameraUpVector, zaxis))
        //  yaxis = cross(zaxis, xaxis)
        //  xaxis.x           yaxis.x           zaxis.x          0
        //  xaxis.y           yaxis.y           zaxis.y          0
        //  xaxis.z           yaxis.z           zaxis.z          0
        //  -dot(xaxis, cameraPosition)  -dot(yaxis, cameraPosition)  -dot(zaxis, cameraPosition)  1
    }

    // calculate a rotation matrix from two vectors
    AZ_FORCE_INLINE AZ::Matrix3x3 GetRotationMatrixFromTwoVectors(const AZ::Vector3& from, const AZ::Vector3& to)
    {
        // calculate intermediate values
        const float vzwy = (to.GetY() * from.GetZ()) - (to.GetZ() * from.GetY());
        const float wxuz = (to.GetZ() * from.GetX()) - (to.GetX() * from.GetZ());
        const float uyvx = (to.GetX() * from.GetY()) - (to.GetY() * from.GetX());
        const float A = vzwy * vzwy + wxuz * wxuz + uyvx * uyvx;

        // return identity if the cross product of the two vectors is small
        if (A < Math::epsilon)
        {
            return AZ::Matrix3x3::CreateIdentity();
        }

        const float lengths = SafeLength(to) * SafeLength(from);
        const float D = (lengths > Math::epsilon) ? 1.0f / lengths : 0.0f;
        const float C = (to.GetX() * from.GetX() + to.GetY() * from.GetY() + to.GetZ() * from.GetZ()) * D;

        // set the components of the rotation matrix
        const float t = (1.0f - C) / A;

        AZ::Matrix3x3 matrix;
        matrix.SetElement(0, 0, t * vzwy * vzwy + C);
        matrix.SetElement(1, 1, t * wxuz * wxuz + C);
        matrix.SetElement(2, 2, t * uyvx * uyvx + C);
        matrix.SetElement(1, 0, t * vzwy * wxuz + D * uyvx);
        matrix.SetElement(2, 0, t * vzwy * uyvx - D * wxuz);
        matrix.SetElement(2, 1, t * wxuz * uyvx + D * vzwy);
        matrix.SetElement(0, 1, t * vzwy * wxuz - D * uyvx);
        matrix.SetElement(0, 2, t * vzwy * uyvx + D * wxuz);
        matrix.SetElement(1, 2, t * wxuz * uyvx - D * vzwy);
        return matrix;
    }

    AZ_FORCE_INLINE AZ::Transform CreateFromQuaternionAndTranslationAndScale(const AZ::Quaternion& rotation, const AZ::Vector3& translation, const AZ::Vector3& scale)
    {
        AZ::Transform result;
        result.SetTranslation(translation);
        result.SetRotation(rotation);
        result.SetUniformScale(scale.GetMaxElement());
        return result;
    }

    AZ_FORCE_INLINE AZ::Transform GetRotationMatrixAxisAngle(const AZ::Vector3& axis, float angle)
    {
        const float length2 = axis.GetLengthSq();
        if (Math::Abs(length2) < 0.00001f)
        {
            return AZ::Transform::CreateIdentity();
        }

        return AZ::Transform::CreateFromQuaternion(AZ::Quaternion::CreateFromAxisAngle(axis.GetNormalized(), angle));
    }

    AZ_FORCE_INLINE void Skin(const AZ::Matrix3x4& inMat, const AZ::Vector3* inPos, const AZ::Vector3* inNormal, AZ::Vector3* outPos, AZ::Vector3* outNormal, float weight)
    {
        *outPos += weight * (inMat * (*inPos));
        *outNormal += weight * inMat.TransformVector(*inNormal);
    }

    AZ_FORCE_INLINE void Skin(const AZ::Matrix3x4& inMat, const AZ::Vector3* inPos, const AZ::Vector3* inNormal, const AZ::Vector4* inTangent, AZ::Vector3* outPos, AZ::Vector3* outNormal, AZ::Vector4* outTangent, float weight)
    {
        *outPos += weight * (inMat * (*inPos));
        *outNormal += weight * inMat.TransformVector(*inNormal);
        outTangent->Set(outTangent->GetAsVector3() + weight * inMat.TransformVector(inTangent->GetAsVector3()), inTangent->GetW());
    }

    AZ_FORCE_INLINE void Skin(const AZ::Matrix3x4& inMat, const AZ::Vector3* inPos, const AZ::Vector3* inNormal, const AZ::Vector4* inTangent, const AZ::Vector3* inBitangent, AZ::Vector3* outPos, AZ::Vector3* outNormal, AZ::Vector4* outTangent, AZ::Vector3* outBitangent, float weight)
    {
        *outPos += weight * (inMat * (*inPos));
        *outNormal += weight * inMat.TransformVector(*inNormal);
        outTangent->Set(outTangent->GetAsVector3() + weight * inMat.TransformVector(inTangent->GetAsVector3()), inTangent->GetW());
        *outBitangent += weight * inMat.TransformVector(*inBitangent);
    }


    AZ_FORCE_INLINE void LookAtRH(AZ::Matrix4x4& mat, const AZ::Vector3& view, const AZ::Vector3& target, const AZ::Vector3& up)
    {
        const AZ::Vector3 z = (view - target).GetNormalized();
        const AZ::Vector3 x = (up.Cross(z)).GetNormalized();
        const AZ::Vector3 y = z.Cross(x);

        mat.SetRow(0, x, -x.Dot(view));
        mat.SetRow(1, y, -y.Dot(view));
        mat.SetRow(2, z, -z.Dot(view));
        mat.SetRow(3, AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f));

        // DirectX:
        //  zaxis = normal(cameraPosition - cameraTarget)
        //  xaxis = normal(cross(cameraUpVector, zaxis))
        //  yaxis = cross(zaxis, xaxis)
        //  xaxis.x           yaxis.x           zaxis.x          0
        //  xaxis.y           yaxis.y           zaxis.y          0
        //  xaxis.z           yaxis.z           zaxis.z          0
        //  -dot(xaxis, cameraPosition)  -dot(yaxis, cameraPosition)  -dot(zaxis, cameraPosition)  1
    }


    AZ_FORCE_INLINE void PerspectiveRH(AZ::Matrix4x4& mat, float fov, float aspect, float zNear, float zFar)
    {
        const float yScale = 1.0f / Math::Tan(fov * 0.5f);
        const float xScale = yScale / aspect;
        const float d = zFar / (zNear - zFar);

        mat = AZ::Matrix4x4::CreateZero();
        mat.SetElement(0, 0, xScale);
        mat.SetElement(1, 1, yScale);
        mat.SetElement(2, 2, d);
        mat.SetElement(3, 2, -1.0f);
        mat.SetElement(2, 3, zNear * d);
    }


    // ortho projection matrix
    AZ_FORCE_INLINE void OrthoOffCenterRH(AZ::Matrix4x4& mat, float left, float right, float top, float bottom, float znear, float zfar)
    {
        mat = AZ::Matrix4x4::CreateIdentity();
        mat.SetElement(0, 0, 2.0f / (right - left));
        mat.SetElement(1, 1, 2.0f / (top - bottom));
        mat.SetElement(2, 2, 1.0f / (znear - zfar));
        
        mat.SetElement(0, 3, (left + right) / (left - right));
        mat.SetElement(1, 3, (top + bottom) / (bottom - top));
        mat.SetElement(2, 3, znear / (znear - zfar));
        mat.SetElement(3, 3, 1.0f);

        // DirectX:
        //  2/(right-left)         0                         0                                  0
        //  0                      2/(top-bottom)            0                                  0
        //  0                      0                         1/(znearPlane-zfarPlane)           0
        //  (l+right)/(l-right)  (top+bottom)/(bottom-top)  znearPlane/(znearPlane-zfarPlane)  1
    }

    AZ_FORCE_INLINE AZ::Vector3 GetRight(const AZ::Matrix4x4& mat)
    {
        return mat.GetColumnAsVector3(0);
    }

    AZ_FORCE_INLINE AZ::Vector3 GetForward(const AZ::Matrix4x4& mat)
    {
        return mat.GetColumnAsVector3(1);
    }

    AZ_FORCE_INLINE AZ::Vector3 GetUp(const AZ::Matrix4x4& mat)
    {
        return mat.GetColumnAsVector3(2);
    }

    AZ_FORCE_INLINE AZ::Vector3 GetTranslation(const AZ::Matrix4x4& mat)
    {
        return mat.GetColumnAsVector3(3);
    }

    AZ_FORCE_INLINE AZ::Vector3 GetRight(const AZ::Transform& mat)
    {
        return mat.GetBasisX();
    }

    AZ_FORCE_INLINE AZ::Vector3 GetForward(const AZ::Transform& mat)
    {
        return mat.GetBasisY();
    }

    AZ_FORCE_INLINE AZ::Vector3 GetUp(const AZ::Transform& mat)
    {
        return mat.GetBasisZ();
    }

    AZ_FORCE_INLINE AZ::Vector3 GetTranslation(const AZ::Transform& mat)
    {
        return mat.GetTranslation();
    }

    // MCore::Matrix was not doing a full invertion of the matrix, it was just inverting the 3x3 region and converting
    // the translation part. This method mimics what that inverse was doing
    AZ_FORCE_INLINE AZ::Matrix4x4 InvertProjectionMatrix(const AZ::Matrix4x4& mat)
    {
        AZ::Matrix3x3 m33 = AZ::Matrix3x3::CreateFromRows(mat.GetRowAsVector3(0), mat.GetRowAsVector3(1), mat.GetRowAsVector3(2));
        m33.InvertFull();
        
        const AZ::Vector3 translation = -(m33 * mat.GetTranslation());
        return AZ::Matrix4x4::CreateFromRows(
            AZ::Vector3ToVector4(m33.GetRow(0), translation.GetX()),
            AZ::Vector3ToVector4(m33.GetRow(1), translation.GetY()),
            AZ::Vector3ToVector4(m33.GetRow(2), translation.GetZ()),
            mat.GetRow(3));
    }
} // namespace MCore
