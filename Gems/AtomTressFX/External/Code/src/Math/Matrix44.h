//--------------------------------------------------------------------------------------
// File: Matrix44.h
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

namespace AMD
{
    class Vector3;

    class Matrix4
    {
    public:
        Matrix4();
        Matrix4(const Matrix4& other);
        Matrix4(float r1[4], float r2[4], float r3[4], float r4[4]);
        Matrix4(float e00,
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
            float e33);
        ~Matrix4();

        float m[4][4];

    public:
        static const Matrix4 IDENTITY;
        static const Matrix4 ZERO;

    public:
        void SetIdentity();
        void SetRotation(const Vector3& axis, float ang);
        void SetTranslate(float x, float y, float z);

        float GetElement(int i, int j) const;

        Vector3 operator*(const Vector3& vec) const;
        Matrix4 operator*(const Matrix4& other) const;
        Matrix4& operator=(const Matrix4& other);

        operator float* () const { return (float*)m; }
        operator const float* () const { return (const float*)m; }
    };

} // namespace AMD

