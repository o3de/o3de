/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Random.h>

namespace AZ
{
    AZ_MATH_INLINE VectorN::VectorN(AZStd::size_t numElements)
        : m_numElements(numElements)
    {
        OnSizeChanged();
    }

    AZ_MATH_INLINE VectorN::VectorN(AZStd::size_t numElements, float x)
        : VectorN(numElements)
    {
        Simd::Vec4::FloatType xVec = Simd::Vec4::Splat(x);
        for (Vector4& element : m_values)
        {
            element.SetSimdValue(xVec);
        }
        FixLastVectorElement();
    }

    AZ_MATH_INLINE VectorN::VectorN(VectorN&& v)
        : m_numElements(v.m_numElements)
        , m_values(AZStd::move(v.m_values))
    {
    }

    AZ_MATH_INLINE VectorN VectorN::CreateZero(AZStd::size_t numElements)
    {
        return VectorN(numElements, 0.0f);
    }

    AZ_MATH_INLINE VectorN VectorN::CreateOne(AZStd::size_t numElements)
    {
        return VectorN(numElements, 1.0f);
    }

    AZ_MATH_INLINE VectorN VectorN::CreateFromFloats(AZStd::size_t numElements, const float* inputs)
    {
        VectorN returnValue(numElements);
        for (AZStd::size_t iter = 0; iter < numElements; ++iter)
        {
            returnValue.SetElement(iter, inputs[iter]);
        }
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::CreateRandom(AZStd::size_t numElements)
    {
        SimpleLcgRandomVec4 randGen;
        VectorN returnValue(numElements);
        for (Vector4& element : returnValue.m_values)
        {
            element.SetSimdValue(randGen.GetRandomFloat4());
        }
        returnValue.FixLastVectorElement();
        return returnValue;
    }

    AZ_MATH_INLINE AZStd::size_t VectorN::GetDimensionality() const
    {
        return m_numElements;
    }

    AZ_MATH_INLINE void VectorN::Resize(AZStd::size_t size)
    {
        m_numElements = size;
        OnSizeChanged();
    }

    AZ_MATH_INLINE float VectorN::GetElement(AZStd::size_t index) const
    {
        const AZStd::size_t element = index / 4;
        return m_values[element].GetElement(index % 4);
    }

    AZ_MATH_INLINE void VectorN::SetElement(AZStd::size_t index, float value)
    {
        const AZStd::size_t element = index / 4;
        m_values[element].SetElement(index % 4, value);
    }

    AZ_MATH_INLINE bool VectorN::IsClose(const VectorN& v, float tolerance) const
    {
        AZ_Assert(m_numElements == v.m_numElements, "Dimensionality must be equal");
        Simd::Vec4::FloatType vecTolerance = Simd::Vec4::Splat(tolerance);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            Simd::Vec4::FloatType dist = Simd::Vec4::Abs(Simd::Vec4::Sub(m_values[i].GetSimdValue(), v.m_values[i].GetSimdValue()));
            if (!Simd::Vec4::CmpAllLtEq(dist, vecTolerance))
            {
                return false;
            }
        }
        return true;
    }

    AZ_MATH_INLINE bool VectorN::IsZero(float tolerance) const
    {
        Simd::Vec4::FloatType vecTolerance = Simd::Vec4::Splat(tolerance);
        for (const Vector4& element : m_values)
        {
            if (!Simd::Vec4::CmpAllLtEq(Simd::Vec4::Abs(element.GetSimdValue()), vecTolerance))
            {
                return false;
            }
        }
        return true;
    }

    AZ_MATH_INLINE bool VectorN::IsLessThan(const VectorN& v) const
    {
        AZ_Assert(m_numElements == v.m_numElements, "Dimensionality must be equal");
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            if (!Simd::Vec4::CmpAllLt(m_values[i].GetSimdValue(), v.m_values[i].GetSimdValue()))
            {
                return false;
            }
        }
        return true;
    }

    AZ_MATH_INLINE bool VectorN::IsLessEqualThan(const VectorN& v) const
    {
        AZ_Assert(m_numElements == v.m_numElements, "Dimensionality must be equal");
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            if (!Simd::Vec4::CmpAllLtEq(m_values[i].GetSimdValue(), v.m_values[i].GetSimdValue()))
            {
                return false;
            }
        }
        return true;
    }

    AZ_MATH_INLINE bool VectorN::IsGreaterThan(const VectorN& v) const
    {
        AZ_Assert(m_numElements == v.m_numElements, "Dimensionality must be equal");
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            if (!Simd::Vec4::CmpAllGt(m_values[i].GetSimdValue(), v.m_values[i].GetSimdValue()))
            {
                return false;
            }
        }
        return true;
    }

    AZ_MATH_INLINE bool VectorN::IsGreaterEqualThan(const VectorN& v) const
    {
        AZ_Assert(m_numElements == v.m_numElements, "Dimensionality must be equal");
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            if (!Simd::Vec4::CmpAllGtEq(m_values[i].GetSimdValue(), v.m_values[i].GetSimdValue()))
            {
                return false;
            }
        }
        return true;

    }

    AZ_MATH_INLINE VectorN VectorN::GetFloor() const
    {
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i].GetFloor();
        }
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::GetCeil() const
    {
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i].GetCeil();
        }
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::GetRound() const
    {
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i].GetRound();
        }
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::GetMin(const VectorN& v) const
    {
        AZ_Assert(m_numElements == v.m_numElements, "Dimensionality must be equal");
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i].GetMin(v.m_values[i]);
        }
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::GetMax(const VectorN& v) const
    {
        AZ_Assert(m_numElements == v.m_numElements, "Dimensionality must be equal");
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i].GetMax(v.m_values[i]);
        }
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::GetClamp(const VectorN& min, const VectorN& max) const
    {
        AZ_Assert(m_numElements == min.m_numElements, "Dimensionality must be equal");
        AZ_Assert(m_numElements == max.m_numElements, "Dimensionality must be equal");
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i].GetClamp(min.m_values[i], max.m_values[i]);
        }
        return returnValue;
    }

    AZ_MATH_INLINE float VectorN::L1Norm() const
    {
        AZ::Vector4 partialLengths = AZ::Vector4::CreateZero();
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            partialLengths += m_values[i].GetAbs();
        }
        return partialLengths.Dot(AZ::Vector4::CreateOne());
    }

    AZ_MATH_INLINE float VectorN::L2Norm() const
    {
        return AZ::Sqrt(Dot(*this));
    }

    AZ_MATH_INLINE VectorN VectorN::GetNormalized() const
    {
        VectorN returnValue(*this);
        returnValue.Normalize();
        return returnValue;
    }

    AZ_MATH_INLINE void VectorN::Normalize()
    {
        const float length = L2Norm();
        *this /= length;
    }

    AZ_MATH_INLINE VectorN VectorN::GetAbs() const
    {
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i].GetAbs();
        }
        return returnValue;
    }

    AZ_MATH_INLINE void VectorN::Absolute()
    {
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] = m_values[i].GetAbs();
        }
    }

    AZ_MATH_INLINE VectorN VectorN::GetSquare() const
    {
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] * m_values[i];
        }
        return returnValue;
    }

    AZ_MATH_INLINE void VectorN::Square()
    {
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] *= m_values[i];
        }
    }

    AZ_MATH_INLINE float VectorN::Dot(const VectorN& rhs) const
    {
        AZ_Assert(m_numElements == rhs.m_numElements, "Dimensionality must be equal");
        AZ::Vector4 partialSums = AZ::Vector4::CreateZero();
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            partialSums.SetSimdValue(Simd::Vec4::Madd(m_values[i].GetSimdValue(), rhs.m_values[i].GetSimdValue(), partialSums.GetSimdValue()));
        }
        return partialSums.Dot(AZ::Vector4::CreateOne());
    }

    AZ_MATH_INLINE void VectorN::SetZero()
    {
        AZ::Vector4* data = m_values.data();
        memset(data, 0, sizeof(AZ::Vector4) * m_values.size());
    }

    AZ_MATH_INLINE VectorN& VectorN::operator+=(const VectorN& rhs)
    {
        AZ_Assert(m_numElements == rhs.m_numElements, "Dimensionality must be equal");
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] += rhs.m_values[i];
        }
        FixLastVectorElement();
        return *this;
    }

    AZ_MATH_INLINE VectorN& VectorN::operator-=(const VectorN& rhs)
    {
        AZ_Assert(m_numElements == rhs.m_numElements, "Dimensionality must be equal");
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] -= rhs.m_values[i];
        }
        FixLastVectorElement();
        return *this;
    }

    AZ_MATH_INLINE VectorN& VectorN::operator*=(const VectorN& rhs)
    {
        AZ_Assert(m_numElements == rhs.m_numElements, "Dimensionality must be equal");
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] *= rhs.m_values[i];
        }
        return *this;
    }

    AZ_MATH_INLINE VectorN& VectorN::operator/=(const VectorN& rhs)
    {
        AZ_Assert(m_numElements == rhs.m_numElements, "Dimensionality must be equal");
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] /= rhs.m_values[i];
        }
        FixLastVectorElement();
        return *this;
    }

    AZ_MATH_INLINE VectorN& VectorN::operator+=(float sum)
    {
        Vector4 sumVec = Vector4(sum);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] += sumVec;
        }
        FixLastVectorElement();
        return *this;
    }

    AZ_MATH_INLINE VectorN& VectorN::operator-=(float difference)
    {
        Vector4 diffVec = Vector4(difference);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] -= diffVec;
        }
        FixLastVectorElement();
        return *this;
    }

    AZ_MATH_INLINE VectorN& VectorN::operator*=(float multiplier)
    {
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] *= multiplier;
        }
        return *this;
    }

    AZ_MATH_INLINE VectorN& VectorN::operator/=(float divisor)
    {
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            m_values[i] /= divisor;
        }
        FixLastVectorElement();
        return *this;
    }

    AZ_MATH_INLINE VectorN VectorN::operator-() const
    {
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = -m_values[i];
        }
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::operator+(const VectorN& rhs) const
    {
        AZ_Assert(m_numElements == rhs.m_numElements, "Dimensionality must be equal");
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] + rhs.m_values[i];
        }
        returnValue.FixLastVectorElement();
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::operator-(const VectorN& rhs) const
    {
        AZ_Assert(m_numElements == rhs.m_numElements, "Dimensionality must be equal");
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] - rhs.m_values[i];
        }
        returnValue.FixLastVectorElement();
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::operator*(const VectorN& rhs) const
    {
        AZ_Assert(m_numElements == rhs.m_numElements, "Dimensionality must be equal");
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] * rhs.m_values[i];
        }
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::operator/(const VectorN& rhs) const
    {
        AZ_Assert(m_numElements == rhs.m_numElements, "Dimensionality must be equal");
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] / rhs.m_values[i];
        }
        returnValue.FixLastVectorElement();
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::operator*(float multiplier) const
    {
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] * multiplier;
        }
        return returnValue;
    }

    AZ_MATH_INLINE VectorN VectorN::operator/(float divisor) const
    {
        VectorN returnValue(m_numElements);
        for (AZStd::size_t i = 0; i < m_values.size(); ++i)
        {
            returnValue.m_values[i] = m_values[i] / divisor;
        }
        returnValue.FixLastVectorElement();
        return returnValue;
    }

    AZ_MATH_INLINE const AZStd::vector<Vector4>& VectorN::GetVectorValues() const
    {
        return m_values;
    }

    AZ_MATH_INLINE AZStd::vector<Vector4>& VectorN::GetVectorValues()
    {
        return m_values;
    }

    AZ_MATH_INLINE void VectorN::FixLastVectorElement()
    {
        if (m_values.empty())
        {
            return;
        }

        const uint32_t masks[] =
        {
            0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
            0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
            0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
            0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000
        };

        const AZStd::size_t lastElement = m_values.size() - 1;
        const AZStd::size_t trailingZeroElements = 4 * (m_numElements % 4);
        const Simd::Vec4::FloatType mask = Simd::Vec4::LoadAligned(reinterpret_cast<const float*>(&masks[trailingZeroElements]));

        m_values[lastElement].SetSimdValue(Simd::Vec4::And(m_values[lastElement].GetSimdValue(), mask));
    }

    AZ_MATH_INLINE void VectorN::OnSizeChanged()
    {
        m_values.resize((m_numElements + 3) / 4);
        FixLastVectorElement();
    }

    AZ_MATH_INLINE VectorN operator+(float lhs, const VectorN& rhs)
    {
        VectorN returnValue(rhs.GetDimensionality());
        const AZ::Vector4 lhsVec = AZ::Vector4(lhs);
        for (AZStd::size_t i = 0; i < rhs.GetVectorValues().size(); ++i)
        {
            returnValue.GetVectorValues()[i] = lhsVec + rhs.GetVectorValues()[i];
        }
        returnValue.FixLastVectorElement();
        return returnValue;
    }

    AZ_MATH_INLINE VectorN operator-(float lhs, const VectorN& rhs)
    {
        VectorN returnValue(rhs.GetDimensionality());
        const AZ::Vector4 lhsVec = AZ::Vector4(lhs);
        for (AZStd::size_t i = 0; i < rhs.GetVectorValues().size(); ++i)
        {
            returnValue.GetVectorValues()[i] = lhsVec - rhs.GetVectorValues()[i];
        }
        returnValue.FixLastVectorElement();
        return returnValue;
    }

    AZ_MATH_INLINE VectorN operator*(float lhs, const VectorN& rhs)
    {
        VectorN returnValue(rhs.GetDimensionality());
        const AZ::Vector4 lhsVec = AZ::Vector4(lhs);
        for (AZStd::size_t i = 0; i < rhs.GetVectorValues().size(); ++i)
        {
            returnValue.GetVectorValues()[i] = lhsVec * rhs.GetVectorValues()[i];
        }
        return returnValue;
    }
}
