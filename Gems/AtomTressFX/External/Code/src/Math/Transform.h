//--------------------------------------------------------------------------------------
// File: Transform.h
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

#include <Math/Quaternion.h>
#include <Math/Vector3D.h>

namespace AMD
{
    class TransformSet
    {
    public:
        TransformSet(void);
        TransformSet(const TransformSet& other);
        TransformSet(const Vector3& translation, const Quaternion& rotation);
        ~TransformSet(void);

    private:
        Vector3 m_Translation;
        Quaternion m_Rotation;

    public:
        const Vector3& GetTranslation() const { return m_Translation; }
        const Quaternion& GetRotation() const { return m_Rotation; }
        Vector3& GetTranslation() { return m_Translation; }
        Quaternion& GetRotation() { return m_Rotation; }
        void                Inverse();
        TransformSet   InverseOther() const;

        Vector3 operator*(const Vector3& vector) const;
        TransformSet operator*(const TransformSet& transform) const;
        TransformSet& operator=(const TransformSet& other);
    };
} // namespace AMD

