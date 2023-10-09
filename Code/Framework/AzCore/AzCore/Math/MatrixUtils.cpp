/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MatrixUtils.h>

namespace AZ
{
    namespace
    {
        const float FloatEpsilon = 0.0001f;
    }

    Matrix4x4* MakePerspectiveFovMatrixRH(Matrix4x4& out, float fovY, float aspectRatio, float nearDist, float farDist, bool reverseDepth)
    {
        AZ_Assert(nearDist > 0.0f, "near distance should be greater than 0.f");
        AZ_Assert(farDist > nearDist, "far distance should be greater than near");
        AZ_Assert(fovY > FloatEpsilon && aspectRatio > FloatEpsilon, "Both field of view in y direction and aspect ratio must be greater than 0");

        if (!(nearDist > 0 && farDist > nearDist && fovY > FloatEpsilon && aspectRatio > FloatEpsilon))
        {
            return nullptr;
        }

        float sinFov, cosFov;
        SinCos(0.5f * fovY, sinFov, cosFov);
        float yScale = cosFov / sinFov; //cot(fovY/2)
        float xScale = yScale / aspectRatio;

        if (reverseDepth)
        {
            AZStd::swap(nearDist, farDist);
        }
        
        // x right, y up, negative z forward
        out.SetRow(0,   xScale, 0.f,    0.f,                                0.f                                     );
        out.SetRow(1,   0.f,    yScale, 0.f,                                0.f                                     );
        out.SetRow(2,   0.f,    0.f,    farDist / (nearDist - farDist),     nearDist*farDist / (nearDist - farDist) );
        out.SetRow(3,   0.f,    0.f,    -1.f,                               0.f                                     );
        return &out;
    }

    void SetPerspectiveMatrixFOV(Matrix4x4& out, float fovY, float aspectRatio)
    {
        float sinFov, cosFov;
        SinCos(0.5f * fovY, sinFov, cosFov);
        float yScale = cosFov / sinFov; //cot(fovY/2)
        float xScale = yScale / aspectRatio;

        out.SetElement(0, 0, xScale);
        out.SetElement(1, 1, yScale);
    }

    float GetPerspectiveMatrixFOV(const Matrix4x4& m)
    {
        return 2.0f * AZStd::atan(1.0f / m.GetElement(1, 1));
    }

    void SetPerspectiveMatrixNearFar(Matrix4x4& out, float nearDist, float farDist, bool reverseDepth)
    {
        AZ_Assert(nearDist > FloatEpsilon, "Near distance should be greater than zero (float epsilon) in a perspective matrix");
        AZ_Assert(farDist > nearDist, "Far should be greater than near in a perspective matrix");

        if (reverseDepth)
        {
            AZStd::swap(nearDist, farDist);
        }

        out.SetElement(2, 2, farDist / (nearDist - farDist));
        out.SetElement(2, 3, nearDist * farDist / (nearDist - farDist));
    }

    Matrix4x4* MakeFrustumMatrixRH(Matrix4x4& out, float left, float right, float bottom, float top, float nearDist, float farDist, bool reverseDepth)
    {
        AZ_Assert(right > left, "right should be greater than left");
        AZ_Assert(top > bottom, "top should be greater than bottom");
        AZ_Assert(nearDist > FloatEpsilon, "near distance should be greater than 0.f");
        AZ_Assert(farDist > nearDist, "far should be greater than near");

        if (!(right > left && top > bottom && farDist > nearDist && nearDist > FloatEpsilon))
        {
            return nullptr;
        }

        // save near distance for x, y transform
        float savedNear = nearDist;

        if (reverseDepth)
        {
            AZStd::swap(nearDist, farDist);
        }

        out.SetRow(0,   2.f * savedNear / (right - left),   0.f,                                (left + right) / (right - left),    0                                       );
        out.SetRow(1,   0.f,                                2.f * savedNear / (top - bottom),   (top + bottom)/(top-bottom),        0                                       );
        out.SetRow(2,   0.f,                                0.f,                                farDist / (nearDist - farDist),     nearDist*farDist / (nearDist - farDist) );
        out.SetRow(3,   0.f,                                0.f,                                -1.f,                               0.f                                     );
        return &out;
    }

    Matrix4x4* MakeOrthographicMatrixRH(Matrix4x4& out, float left, float right, float bottom, float top, float nearDist, float farDist, bool reverseDepth)
    {
        AZ_Assert(right > left, "right should be greater than left");
        // valid to have matrix invert top/bottom and far/near
        //AZ_Assert(top > bottom, "top should be greater than bottom");
        //AZ_Assert(farDist > nearDist, "far should be greater than near");

        if (!(right > left /*&& top > bottom && farDist > nearDist*/))
        {
            return nullptr;
        }

        if (reverseDepth)
        {
            AZStd::swap(nearDist, farDist);
        }

        out.SetRow(0,   2.f/(right - left), 0.f,                    0.f,                        - (right + left) / (right - left)   );
        out.SetRow(1,   0.f,                2.f / (top - bottom),   0.f,                        - (top + bottom) / (top - bottom)   );
        out.SetRow(2,   0.f,                0.f,                    1 / (nearDist - farDist),   nearDist / (nearDist - farDist)     );
        out.SetRow(3,   0.f,                0.f,                    0,                          1.f                                 );
        return &out;
    }
    
    Vector3 MatrixTransformPosition(const Matrix4x4& matrix, const Vector3& inPosition)
    {
        Vector4 result;
        result = matrix * Vector4(Simd::Vec4::ReplaceIndex3(Simd::Vec4::FromVec3(inPosition.GetSimdValue()), 1.0f));
        AZ_Assert(!IsClose(result.GetW(), 0, FloatEpsilon), "w is too close to 0.f");
        return result.GetHomogenized();
    }
}
