//--------------------------------------------------------------------------------------
// File: Matrix44.cpp
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

#include "Math/Matrix44.h"
#include "Math/Vector3D.h"
#include <cmath>

const Matrix4 Matrix4::IDENTITY = Matrix4(1.0f,
                                                            0.0f,
                                                            0.0f,
                                                            0.0f,
                                                            0.0f,
                                                            1.0f,
                                                            0.0f,
                                                            0.0f,
                                                            0.0f,
                                                            0.0f,
                                                            1.0f,
                                                            0.0f,
                                                            0.0f,
                                                            0.0f,
                                                            0.0f,
                                                            1.0f);
const Matrix4 Matrix4::ZERO = Matrix4(0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f,
                                                        0.0f);

Matrix4::Matrix4()
{
    int i = 0;
    int j = 0;

    for (i = 0; i < 4; i++)
    {
        for (j      = 0; j < 4; j++)
            m[i][j] = 0.0f;
    }
}

Matrix4::Matrix4(const Matrix4& other)
{
    int i = 0;
    int j = 0;

    for (i = 0; i < 4; i++)
    {
        for (j      = 0; j < 4; j++)
            m[i][j] = other.m[i][j];
    }
}

Matrix4::Matrix4(float R1[4], float R2[4], float R3[4], float R4[4])
{
    int i;

    for (i = 0; i < 4; i++)
    {
        m[0][i] = R1[i];
    }

    for (i = 0; i < 4; i++)
    {
        m[1][i] = R2[i];
    }

    for (i = 0; i < 4; i++)
    {
        m[2][i] = R3[i];
    }

    for (i = 0; i < 4; i++)
    {
        m[3][i] = R4[i];
    }
}

Matrix4::Matrix4(float e00,
                                float e01,
                                float e02,
                                float e03,
                                float e10,
                                float e11,
                                float e12,
                                float e13,
                                float e20,
                                float e21,
                                float e22,
                                float e23,
                                float e30,
                                float e31,
                                float e32,
                                float e33)
{
    m[0][0] = e00;
    m[0][1] = e01;
    m[0][2] = e02;
    m[0][3] = e03;
    m[1][0] = e10;
    m[1][1] = e11;
    m[1][2] = e12;
    m[1][3] = e13;
    m[2][0] = e20;
    m[2][1] = e21;
    m[2][2] = e22;
    m[2][3] = e23;
    m[3][0] = e30;
    m[3][1] = e31;
    m[3][2] = e32;
    m[3][3] = e33;
}

Matrix4::~Matrix4() {}

void Matrix4::SetIdentity()
{
    m[0][0] = 1.0f;
    m[0][1] = 0.0f;
    m[0][2] = 0.0f;
    m[0][3] = 0.0f;
    m[1][0] = 0.0f;
    m[1][1] = 1.0f;
    m[1][2] = 0.0f;
    m[1][3] = 0.0f;
    m[2][0] = 0.0f;
    m[2][1] = 0.0f;
    m[2][2] = 1.0f;
    m[2][3] = 0.0f;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

void Matrix4::SetRotation(const Vector3& axis, float ang)
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

    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;

#undef vsin
}

void Matrix4::SetTranslate(float x, float y, float z)
{
    m[0][3] = x;
    m[1][3] = y;
    m[2][3] = z;
}

float Matrix4::GetElement(int i, int j) const { return m[i][j]; }

Vector3 Matrix4::operator*(const Vector3& vec) const
{
    Vector3 retVal;

    retVal.x = m[0][0] * vec.x + m[0][1] * vec.y + m[0][2] * vec.z + m[0][3];
    retVal.y = m[1][0] * vec.x + m[1][1] * vec.y + m[1][2] * vec.z + m[1][3];
    retVal.z = m[2][0] * vec.x + m[2][1] * vec.y + m[2][2] * vec.z + m[2][3];

    return retVal;
}

Matrix4 Matrix4::operator*(const Matrix4& other) const
{
    Matrix4 retMat;

    for (int i = 0; i < 4; i++)
    {
        for (int j         = 0; j < 4; j++)
            retMat.m[i][j] = m[i][0] * other.m[0][j] + m[i][1] * other.m[1][j] +
                                m[i][2] * other.m[2][j] + m[i][3] * other.m[3][j];
    }

    return retMat;
}

Matrix4& Matrix4::operator=(const Matrix4& other)
{
    int i = 0;
    int j = 0;

    for (i = 0; i < 4; i++)
    {
        for (j      = 0; j < 4; j++)
            m[i][j] = other.m[i][j];
    }

    return *this;
}