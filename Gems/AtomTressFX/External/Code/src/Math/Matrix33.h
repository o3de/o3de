//--------------------------------------------------------------------------------------
// File: Matrix33.h
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

    class Matrix3
    {
        friend class Vector3;
        friend class Quaternion;

    public:
        Matrix3(void);
        Matrix3(const Matrix3& other);
        Matrix3(float e00,
            float e01,
            float e02,
            float e10,
            float e11,
            float e12,
            float e20,
            float e21,
            float e22);
        ~Matrix3(void);

    private:
        float m[3][3];

    public:
        static const Matrix3 IDENTITY;
        static const Matrix3 ZERO;

    public:
        void SetIdentity();
        void Set(float e00,
            float e01,
            float e02,
            float e10,
            float e11,
            float e12,
            float e20,
            float e21,
            float e22);
        float GetElement(int i, int j) const;
        void SetElement(int i, int j, float val);
        void SetRotation(const Vector3& axis, float ang);
        void          Inverse();
        Matrix3 InverseOther() const;
        void          Transpose();
        Matrix3 TransposeOther() const;

        Vector3 operator*(const Vector3& vec) const;
        Matrix3 operator*(const Matrix3& other) const;
        Matrix3 operator*(float val) const;
        Matrix3 operator+(const Matrix3& other) const;
        Matrix3 operator-(const Matrix3& other) const;
        Matrix3 operator/(float val) const;
        Matrix3& operator*=(float val);
        Matrix3& operator-=(const Matrix3& other);
        Matrix3& operator+=(const Matrix3& other);
        Matrix3& operator=(const Matrix3& other);

        bool operator==(const Matrix3& other);
        bool operator!=(const Matrix3& other);

        bool operator==(float a);
        bool operator!=(float a);

        float& operator()(int i, int j);
        const float& operator()(int i, int j) const;

        friend Matrix3 operator*(float val, const Matrix3& other);
        friend Matrix3 operator-(const Matrix3& other);
    };

} // namespace AMD

