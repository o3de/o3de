//--------------------------------------------------------------------------------------
// File: Quaternion.h
//
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//--------------------------------------------------------------------------------------

#pragma once

#include <Math/Matrix33.h>
#include <Math/Vector3D.h>

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4201)  // disable warning C4201: nonstandard extension used : nameless struct/union
#endif

namespace AMD
{
    class Quaternion
    {
    public:
        Quaternion(float x = 0.0, float y = 0.0, float z = 0.0, float w = 1.0);
        Quaternion(const Quaternion& other);
        Quaternion(const Matrix3& rotMat);
        Quaternion(const Vector3& axis, float angle_radian);
        Quaternion(float* xyz);
        ~Quaternion(void);

    public:
        union
        {
            struct
            {
                float m[4];
            };  // x, y, z, w
            struct
            {
                float x, y, z, w;
            };
        };

        Quaternion& Normalize();

        void SetRotation(const Vector3& axis, float angle_radian);
        void SetRotation(const Matrix3& rotMat);
        void SetRotation(const Quaternion& quaternion);
        void GetRotation(Vector3* pAxis, float* pAngle_radian) const;
        void GetRotation(Matrix3* pMat33) const;
        Matrix3 GetMatrix33() const;
        float         Length() const;

        void         SetIdentity();
        void         Inverse();
        Quaternion InverseOther() const;

        Quaternion& operator=(const Quaternion& other);
        Quaternion& operator=(float* xyz);
        Quaternion operator+(const Quaternion& other) const;
        Quaternion operator+(const Vector3& vec) const;
        Quaternion operator*(const Quaternion& other) const;
        Vector3 operator*(const Vector3& vec) const;

        friend Vector3 operator*(const Vector3& vec, const Quaternion& q);
    };

} // namespace AMD

#ifdef _MSC_VER
    #pragma warning(pop)
#endif
