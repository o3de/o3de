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
            return AZ::Quaternion::CreateFromAxisAngle(rotAxis.Normalize(), angleRadians);
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
