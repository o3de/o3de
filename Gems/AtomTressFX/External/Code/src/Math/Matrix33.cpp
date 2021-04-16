//--------------------------------------------------------------------------------------
// File: Matrix33.cpp
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

#include <cassert>

#include <Math/Matrix33.h>
#include <Math/Vector3D.h>

namespace AMD
{
    const Matrix3 Matrix3::IDENTITY =
        Matrix3(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    const Matrix3 Matrix3::ZERO =
        Matrix3(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    Matrix3::Matrix3(void)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                m[i][j] = 0;
    }

    Matrix3::Matrix3(const Matrix3& other)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                m[i][j] = other.m[i][j];
    }

    Matrix3::Matrix3(float e00,
        float e01,
        float e02,
        float e10,
        float e11,
        float e12,
        float e20,
        float e21,
        float e22)
    {
        m[0][0] = e00;
        m[0][1] = e01;
        m[0][2] = e02;

        m[1][0] = e10;
        m[1][1] = e11;
        m[1][2] = e12;

        m[2][0] = e20;
        m[2][1] = e21;
        m[2][2] = e22;
    }

    Matrix3::~Matrix3(void) {}

    void Matrix3::SetIdentity()
    {
        m[0][0] = 1.0;
        m[0][1] = 0.0;
        m[0][2] = 0.0;

        m[1][0] = 0.0;
        m[1][1] = 1.0;
        m[1][2] = 0.0;

        m[2][0] = 0.0;
        m[2][1] = 0.0;
        m[2][2] = 1.0;
    }

    float Matrix3::GetElement(int i, int j) const
    {
        assert(0 <= i && i < 3);
        assert(0 <= i && j < 3);

        return m[i][j];
    }

    void Matrix3::SetElement(int i, int j, float val)
    {
        assert(0 <= i && i < 3);
        assert(0 <= i && j < 3);

        m[i][j] = val;
    }

    void Matrix3::Set(float e00,
        float e01,
        float e02,
        float e10,
        float e11,
        float e12,
        float e20,
        float e21,
        float e22)
    {
        m[0][0] = e00;
        m[0][1] = e01;
        m[0][2] = e02;

        m[1][0] = e10;
        m[1][1] = e11;
        m[1][2] = e12;

        m[2][0] = e20;
        m[2][1] = e21;
        m[2][2] = e22;
    }

    void Matrix3::SetRotation(const Vector3& axis, float ang)
    {
#define vsin(x) ((1.0f) - cos(x))

        float nx = axis.x;
        float ny = axis.y;
        float nz = axis.z;

        m[0][0] = nx * nx * vsin(ang) + cos(ang);
        m[0][1] = nx * ny * vsin(ang) - nz * sin(ang);
        m[0][2] = nx * nz * vsin(ang) + ny * sin(ang);

        m[1][0] = nx * ny * vsin(ang) + nz * sin(ang);
        m[1][1] = ny * ny * vsin(ang) + cos(ang);
        m[1][2] = ny * nz * vsin(ang) - nx * sin(ang);

        m[2][0] = nx * nz * vsin(ang) - ny * sin(ang);
        m[2][1] = ny * nz * vsin(ang) + nx * sin(ang);
        m[2][2] = nz * nz * vsin(ang) + cos(ang);

#undef vsin
    }

    void Matrix3::Inverse()
    {
        float det = m[0][0] * (m[2][2] * m[1][1] - m[2][1] * m[1][2]) -
            m[1][0] * (m[2][2] * m[0][1] - m[2][1] * m[0][2]) +
            m[2][0] * (m[1][2] * m[0][1] - m[1][1] * m[0][2]);

        m[0][0] = m[2][2] * m[1][1] - m[2][1] * m[1][2];
        m[0][1] = -m[2][2] * m[0][1] - m[2][1] * m[0][2];
        m[0][2] = m[1][2] * m[0][1] - m[1][1] * m[0][2];

        m[1][0] = -m[2][2] * m[1][0] - m[2][0] * m[1][2];
        m[1][1] = m[2][2] * m[0][0] - m[2][0] * m[0][2];
        m[1][2] = -m[1][2] * m[0][0] - m[1][0] * m[0][2];

        m[2][0] = m[2][1] * m[1][0] - m[2][0] * m[1][1];
        m[2][1] = -m[2][1] * m[0][0] - m[2][0] * m[0][1];
        m[2][2] = m[1][1] * m[0][0] - m[1][0] * m[0][1];

        (*this) *= 1 / det;
    }

    Matrix3 Matrix3::InverseOther() const
    {
        Matrix3 other(*this);
        other.Inverse();
        return other;
    }

    void Matrix3::Transpose() { *this = TransposeOther(); }

    Matrix3 Matrix3::TransposeOther() const
    {
        Matrix3 other;

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                other.m[i][j] = m[j][i];

        return other;
    }

    Vector3 Matrix3::operator*(const Vector3& vec) const
    {
        Vector3 ret;

        ret.x = m[0][0] * vec.x + m[0][1] * vec.y + m[0][2] * vec.z;
        ret.y = m[1][0] * vec.x + m[1][1] * vec.y + m[1][2] * vec.z;
        ret.z = m[2][0] * vec.x + m[2][1] * vec.y + m[2][2] * vec.z;

        return ret;
    }

    Matrix3 Matrix3::operator*(const Matrix3& other) const
    {
        Matrix3 ret;

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                ret.m[i][j] =
                m[i][0] * other.m[0][j] + m[i][1] * other.m[1][j] + m[i][2] * other.m[2][j];

        return ret;
    }

    Matrix3 Matrix3::operator+(const Matrix3& other) const
    {
        Matrix3 ret;

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                ret.m[i][j] = m[i][j] + other.m[i][j];

        return ret;
    }

    Matrix3 Matrix3::operator-(const Matrix3& other) const
    {
        Matrix3 ret;

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                ret.m[i][j] = m[i][j] - other.m[i][j];

        return ret;
    }

    Matrix3 Matrix3::operator*(float val) const
    {
        Matrix3 ret;

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                ret.m[i][j] = m[i][j] * val;

        return ret;
    }

    Matrix3 Matrix3::operator/(float val) const
    {
        Matrix3 ret;

        // assert(val != 0);

        float eps = 1e-10f;

        if (0 <= val && val <= eps)
            val += eps;
        else if (-eps <= val && val <= 0)
            val -= eps;

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                ret.m[i][j] = m[i][j] / val;

        return ret;
    }

    Matrix3& Matrix3::operator*=(float val)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                m[i][j] = val * m[i][j];

        return (*this);
    }

    Matrix3& Matrix3::operator-=(const Matrix3& other)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                m[i][j] = m[i][j] - other.m[i][j];

        return (*this);
    }

    Matrix3& Matrix3::operator+=(const Matrix3& other)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                m[i][j] = m[i][j] + other.m[i][j];

        return (*this);
    }

    Matrix3& Matrix3::operator=(const Matrix3& other)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                m[i][j] = other.m[i][j];

        return (*this);
    }

    // Matrix3& Matrix3::operator=(float a)
    //{
    //  for ( int i = 0; i < 3; i++ )
    //      for ( int j = 0; j < 3; j++ )
    //          m[i][j] = a;
    //
    //  return (*this);
    //}

    bool Matrix3::operator==(const Matrix3& other)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
            {
                if (m[i][j] != other.m[i][j])
                    return false;
            }

        return true;
    }

    bool Matrix3::operator!=(const Matrix3& other)
    {
        return !(*this).operator==(other);
    }

    bool Matrix3::operator==(float a)
    {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
            {
                if (m[i][j] != a)
                    return false;
            }

        return true;
    }

    bool Matrix3::operator!=(float a) { return !(*this).operator==(a); }

    float& Matrix3::operator()(int i, int j)
    {
        assert(0 <= i && i < 3);
        assert(0 <= i && j < 3);

        return m[i][j];
    }

    const float& Matrix3::operator()(int i, int j) const
    {
        assert(0 <= i && i < 3);
        assert(0 <= i && j < 3);

        return m[i][j];
    }

    Matrix3 operator*(float val, const Matrix3& other) { return other * val; }

    Matrix3 operator-(const Matrix3& other)
    {
        Matrix3 ret;

        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                ret.m[i][j] = -other.m[i][j];

        return ret;
    }

} // namespace AMD

