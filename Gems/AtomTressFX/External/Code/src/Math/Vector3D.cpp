//--------------------------------------------------------------------------------------
// File: Vector3D.cpp
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

#include <Math/Vector3D.h>
#include <assert.h>
#include <memory.h>

namespace AMD
{
    Vector3::Vector3(const Vector3& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
    }

    Vector3::Vector3(const Vector3& begin, const Vector3& end)
    {
        x = end.x - begin.x;
        y = end.y - begin.y;
        z = end.z - begin.z;
    }

    Vector3::Vector3(float* xyz) { memcpy(m, xyz, sizeof(Vector3)); }

    Vector3& Vector3::Normalize()
    {
        float d = sqrt(x * x + y * y + z * z);

        if (d == 0)
        {
            return *this;
        }

        x = x / d;
        y = y / d;
        z = z / d;

        return *this;
    }

    Vector3 Vector3::NormalizeOther() const
    {
        Vector3 n(*this);
        return n.Normalize();
    }

    Vector3 Vector3::operator-(const Vector3& other) const
    {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 Vector3::operator+(const Vector3& other) const
    {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 Vector3::operator/(float val) const
    {
        if (val != 0.0f)
        {
            return Vector3(x / val, y / val, z / val);
        }
        else
        {
            return Vector3(0.0f, 0.0f, 0.0f);
        }
    }

    Vector3& Vector3::operator=(const Vector3& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;

        return *this;
    }

    Vector3& Vector3::operator=(float val)
    {
        x = val;
        y = val;
        z = val;

        return *this;
    }

    Vector3& Vector3::operator=(float* xyz)
    {
        memcpy(m, xyz, sizeof(Vector3));
        return *this;
    }

    bool Vector3::operator<(float val) const { return (LengthSqr() < val * val); }

    bool Vector3::operator>(float val) const { return (LengthSqr() > val * val); }

    bool Vector3::operator!=(float val) const { return (x != val || y != val || z != val); }

    bool Vector3::operator==(float val) const { return (x == val && y == val && z == val); }

    bool Vector3::operator==(const Vector3& other) const
    {
        return (x == other.x && y == other.y && z == other.z);
    }

    bool Vector3::operator!=(const Vector3& other) const
    {
        return (x != other.x || y != other.y || z != other.z);
    }

    Vector3& Vector3::operator-=(const Vector3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;

        return *this;
    }

    Vector3& Vector3::operator+=(const Vector3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;

        return *this;
    }

    Vector3& Vector3::operator*=(float val)
    {
        x *= val;
        y *= val;
        z *= val;

        return *this;
    }

    Vector3 Vector3::operator*(float val) const
    {
        Vector3 vec(*this);

        vec.x *= val;
        vec.y *= val;
        vec.z *= val;

        return vec;
    }

    float Vector3::operator*(const Vector3& other) const { return Dot(other); }


    Vector3 operator*(float val, const Vector3& other) { return other * val; }

    Vector3 operator-(const Vector3& other)
    {
        Vector3 vec(other);

        vec.x = -vec.x;
        vec.y = -vec.y;
        vec.z = -vec.z;

        return vec;
    }

} // namespace AMD
