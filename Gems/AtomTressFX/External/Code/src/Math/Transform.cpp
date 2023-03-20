//--------------------------------------------------------------------------------------
// File: Transform.cpp
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

#include <Math/Transform.h>

namespace AMD
{
    TransformSet::TransformSet(void) {}

    TransformSet::TransformSet(const TransformSet& other)
    {
        m_Translation = other.m_Translation;
        m_Rotation = other.m_Rotation;
    }

    TransformSet::TransformSet(const Vector3& translation,
        const Quaternion& rotation)
    {
        m_Translation = translation;
        m_Rotation = rotation;
    }

    TransformSet::~TransformSet(void) {}

    void TransformSet::Inverse()
    {
        m_Rotation.Inverse();
        m_Translation = m_Rotation * (-m_Translation);
    }

    TransformSet TransformSet::InverseOther() const
    {
        TransformSet other(*this);
        other.Inverse();
        return other;
    }

    Vector3 TransformSet::operator*(const Vector3& vector) const
    {
        return m_Rotation * vector + m_Translation;
    }

    TransformSet TransformSet::operator*(const TransformSet& transform) const
    {
        return TransformSet(m_Rotation * transform.GetTranslation() + m_Translation,
            m_Rotation * transform.GetRotation());
    }

    TransformSet& TransformSet::operator=(const TransformSet& other)
    {
        if (this == &other)
        {
            return (*this);
        }

        m_Translation = other.m_Translation;
        m_Rotation = other.m_Rotation;

        return (*this);
    }

} // namespace AMD

