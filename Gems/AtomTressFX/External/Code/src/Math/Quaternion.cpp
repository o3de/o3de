//--------------------------------------------------------------------------------------
// File: Quaternion.cpp
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

#include <Math/Quaternion.h>
#include <cassert>
#include <math.h>
#include <memory.h>

namespace AMD
{
    Quaternion::Quaternion(float x /* = 0.0*/,
        float y /* = 0.0*/,
        float z /* = 0.0*/,
        float w /* = 1.0*/)
        : x(x), y(y), z(z), w(w)
    {
    }

    Quaternion::~Quaternion(void) {}

    Quaternion::Quaternion(const Quaternion& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
    }

    Quaternion::Quaternion(const Matrix3& rotMat) { SetRotation(rotMat); }

    Quaternion::Quaternion(const Vector3& axis, float angle_radian)
    {
        SetRotation(axis, angle_radian);
    }

    Quaternion::Quaternion(float* xyz) { memcpy(m, xyz, sizeof(Quaternion)); }

    Quaternion& Quaternion::Normalize()
    {
        float n = w * w + x * x + y * y + z * z;

        if (n == 0)
        {
            w = 1;
            return (*this);
        }

        n = 1.0f / sqrt(n);

        w *= n;
        x *= n;
        y *= n;
        z *= n;

        return (*this);
    }

    void Quaternion::SetRotation(const Vector3& axis, float angle_radian)
    {
        // This function assumes that the axis vector has been normalized.
        float halfAng = 0.5f * angle_radian;
        float sinHalf = sin(halfAng);
        w = cos(halfAng);

        x = sinHalf * axis.x;
        y = sinHalf * axis.y;
        z = sinHalf * axis.z;
    }

    void Quaternion::SetRotation(const Matrix3& rotMat)
    {
        float fTrace = rotMat.m[0][0] + rotMat.m[1][1] + rotMat.m[2][2];
        float fRoot;

        if (fTrace > 0.0f)
        {
            // |w| > 1/2, may as well choose w > 1/2
            fRoot = sqrt(fTrace + 1.0f);  // 2w
            w = 0.5f * fRoot;
            fRoot = 0.5f / fRoot;  // 1/(4w)
            x = (rotMat.m[2][1] - rotMat.m[1][2]) * fRoot;
            y = (rotMat.m[0][2] - rotMat.m[2][0]) * fRoot;
            z = (rotMat.m[1][0] - rotMat.m[0][1]) * fRoot;
        }
        else
        {
            // |w| <= 1/2
            static size_t s_iNext[3] = { 1, 2, 0 };
            size_t        i = 0;
            if (rotMat.m[1][1] > rotMat.m[0][0])
            {
                i = 1;
            }
            if (rotMat.m[2][2] > rotMat.m[i][i])
            {
                i = 2;
            }
            size_t j = s_iNext[i];
            size_t k = s_iNext[j];

            fRoot = sqrt(rotMat.m[i][i] - rotMat.m[j][j] - rotMat.m[k][k] + 1.0f);
            float* apkQuat[3] = { &x, &y, &z };
            *apkQuat[i] = 0.5f * fRoot;
            fRoot = 0.5f / fRoot;
            w = (rotMat.m[k][j] - rotMat.m[j][k]) * fRoot;
            *apkQuat[j] = (rotMat.m[j][i] + rotMat.m[i][j]) * fRoot;
            *apkQuat[k] = (rotMat.m[k][i] + rotMat.m[i][k]) * fRoot;
        }
    }

    void Quaternion::SetRotation(const Quaternion& quaternion) { *this = quaternion; }

    void Quaternion::GetRotation(Vector3* pAxis, float* pAngle_radian) const
    {
        *pAngle_radian = 2.0f * acos(w);

        float scale = sqrt(x * x + y * y + z * z);

        if (scale > 0)
        {
            pAxis->x = x / scale;
            pAxis->y = y / scale;
            pAxis->z = z / scale;
        }
        else
        {
            pAxis->x = 0;
            pAxis->y = 0;
            pAxis->z = 0;
        }
    }

    void Quaternion::GetRotation(Matrix3* pMat33) const
    {
        float nQ = x * x + y * y + z * z + w * w;
        float s = 0.0;

        if (nQ > 0.0)
        {
            s = 2.0f / nQ;
        }

        float xs = x * s;
        float ys = y * s;
        float zs = z * s;
        float wxs = w * xs;
        float wys = w * ys;
        float wzs = w * zs;
        float xxs = x * xs;
        float xys = x * ys;
        float xzs = x * zs;
        float yys = y * ys;
        float yzs = y * zs;
        float zzs = z * zs;

        pMat33->Set(1.0f - yys - zzs,
            xys - wzs,
            xzs + wys,
            xys + wzs,
            1.0f - xxs - zzs,
            yzs - wxs,
            xzs - wys,
            yzs + wxs,
            1.0f - xxs - yys);
    }

    Matrix3 Quaternion::GetMatrix33() const
    {
        Matrix3 mat;
        GetRotation(&mat);
        return mat;
    }

    float Quaternion::Length() const { return sqrt(x * x + y * y + z * z + w * w); }

    void Quaternion::SetIdentity()
    {
        x = y = z = 0.0;
        w = 1.0;
    }

    void Quaternion::Inverse()
    {
        float lengthSqr = x * x + y * y + z * z + w * w;

        assert(lengthSqr != 0.0);

        x = -x / lengthSqr;
        y = -y / lengthSqr;
        z = -z / lengthSqr;
        w = w / lengthSqr;
    }

    Quaternion Quaternion::InverseOther() const
    {
        Quaternion q(*this);
        q.Inverse();
        return q;
    }

    Quaternion& Quaternion::operator=(const Quaternion& other)
    {
        w = other.w;
        x = other.x;
        y = other.y;
        z = other.z;

        return (*this);
    }

    Quaternion& Quaternion::operator=(float* xyz)
    {
        memcpy(m, xyz, sizeof(Quaternion));
        return *this;
    }

    Quaternion Quaternion::operator+(const Quaternion& other) const
    {
        Quaternion q;

        q.w = w + other.w;
        q.x = x + other.x;
        q.y = y + other.y;
        q.z = z + other.z;

        return q;
    }

    Quaternion Quaternion::operator+(const Vector3& vec) const
    {
        Quaternion q;

        q.w = w;
        q.x = x + vec.x;
        q.y = y + vec.y;
        q.z = z + vec.z;

        return q;
    }

    Quaternion Quaternion::operator*(const Quaternion& other) const
    {
        Quaternion q(*this);

        q.w = w * other.w - x * other.x - y * other.y - z * other.z;
        q.x = w * other.x + x * other.w + y * other.z - z * other.y;
        q.y = w * other.y + y * other.w + z * other.x - x * other.z;
        q.z = w * other.z + z * other.w + x * other.y - y * other.x;

        return q;
    }

    Vector3 Quaternion::operator*(const Vector3& vec) const
    {
        Vector3 uv, uuv;
        Vector3 qvec(x, y, z);
        uv = qvec.Cross(vec);
        uuv = qvec.Cross(uv);
        uv *= (2.0f * w);
        uuv *= 2.0f;

        return vec + uv + uuv;
    }

    Vector3 operator*(const Vector3& vec, const Quaternion& q) { return q * vec; }

} // namespace AMD

