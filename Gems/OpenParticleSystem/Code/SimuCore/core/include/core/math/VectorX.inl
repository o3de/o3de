/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef _MSC_VER
#pragma warning(push)
#endif

namespace SimuCore {
    namespace detail {
        template <typename T>
        struct Calculator;

        /*********************************Vector2*************************************/
        // vector2 has 2 member.
        template<>
        struct Calculator<VEC2_TYPE> {
            static inline void SetValue(VEC2_TYPE& value, float a, float b, float, float)
            {
                value = VEC2_TYPE(a, b);
            }

            static inline float Length(const VEC2_TYPE& v)
            {
                return v.GetLength();
            }

            static inline VEC2_TYPE ComponentMin(const VEC2_TYPE& lhs, const VEC2_TYPE& rhs)
            {
                return lhs.GetMin(rhs);
            }

            static inline VEC2_TYPE ComponentMax(const VEC2_TYPE& lhs, const VEC2_TYPE& rhs)
            {
                return lhs.GetMax(rhs);
            }

            static inline bool CmpAllEq(const VEC2_TYPE& lhs, const VEC2_TYPE& rhs)
            {
                return lhs == rhs;
            }

            static inline bool CmpAllLt(const VEC2_TYPE& lhs, const VEC2_TYPE& rhs)
            {
                // Note that this returns true if ANY are less than, whereas AZ Vector returns true only if ALL are less than,
                return (lhs.GetX() < rhs.GetX()) || (lhs.GetY() < rhs.GetY());
            }

            static inline bool CmpAllLtEq(const VEC2_TYPE& lhs, const VEC2_TYPE& rhs)
            {
                return (lhs.GetX() <= rhs.GetX()) || (lhs.GetY() <= rhs.GetY());
            }

            static inline bool CmpAllGt(const VEC2_TYPE& lhs, const VEC2_TYPE& rhs)
            {
                return (lhs.GetX() > rhs.GetX()) || (lhs.GetY() > rhs.GetY());
            }

            static inline bool CmpAllGtEq(const VEC2_TYPE& lhs, const VEC2_TYPE& rhs)
            {
                return (lhs.GetX() >= rhs.GetX()) || (lhs.GetY() >= rhs.GetY());
            }
        };

        /*********************************Vector3*************************************/
        // vector3 has 3 member.
        template <>
        struct Calculator<VEC3_TYPE> {
            static inline VEC3_TYPE Add(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return lhs + rhs;
            }

            static inline VEC3_TYPE Sub(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return lhs - rhs;
            }

            static inline VEC3_TYPE Mul(const VEC3_TYPE& lhs, float value)
            {
                return lhs * value;
            }

            static inline VEC3_TYPE Mul(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return lhs * rhs;
            }

            static inline VEC3_TYPE Div(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return lhs / rhs;
            }

            static inline float Dot(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return lhs.Dot(rhs);
            }

            static inline float Length(const VEC3_TYPE& v)
            {
                return v.GetLength();
            }

            static inline float Distance(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                auto dir = Sub(lhs, rhs);
                return Length(dir);
            }

            static inline void SetValue(VEC3_TYPE& value, float a)
            {
                value.Set(a);
            }

            static inline void SetValue(VEC3_TYPE& value, float a, float b, float c)
            {
                value.Set(a,b,c);
            }

            static inline VEC3_TYPE Normalize(const VEC3_TYPE& lhs)
            {
                return lhs.GetNormalized();
            }

            static inline VEC3_TYPE Cross(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return lhs.Cross(rhs);
            }

            static inline VEC3_TYPE ComponentMin(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return lhs.GetMin(rhs);
            }
       
            static inline VEC3_TYPE ComponentMax(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return lhs.GetMax(rhs);
            }

            static inline bool CmpAllEq(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return lhs == rhs;
            }

            static inline bool CmpAllLt(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return (lhs.GetX() < rhs.GetX())
                || (lhs.GetY() < rhs.GetY())
                || (lhs.GetZ() < rhs.GetZ());
            }

            static inline bool CmpAllLtEq(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return (lhs.GetX() <= rhs.GetX())
                || (lhs.GetY() <= rhs.GetY())
                || (lhs.GetZ() <= rhs.GetZ());
            }

            static inline bool CmpAllGt(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return (lhs.GetX() > rhs.GetX())
                || (lhs.GetY() > rhs.GetY())
                || (lhs.GetZ() > rhs.GetZ());
            }

            static inline bool CmpAllGtEq(const VEC3_TYPE& lhs, const VEC3_TYPE& rhs)
            {
                return (lhs.GetX() >= rhs.GetX())
                || (lhs.GetY() >= rhs.GetY())
                || (lhs.GetZ() >= rhs.GetZ());
            }
            
            static inline VEC3_TYPE Lerp(const VEC3_TYPE& src, const VEC3_TYPE& dest, const float& alpha)
            {
                return src.Lerp(dest, alpha);
            }
        };

        /*********************************Vector4*************************************/
        // vector4 has 4 member.
        template <>
        struct Calculator<VEC4_TYPE> {
            static inline VEC4_TYPE Add(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return lhs + rhs;
            }

            static inline VEC4_TYPE Sub(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return lhs - rhs;
            }

            static inline VEC4_TYPE Cross(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                // there's no such thing as the cross product of a vector in dimensions other than 3 and 7.
                // however, this is just computing the 3 dimensional cross as if it were a vector3.
                VEC3_TYPE lhs3(lhs.GetElement(VEC_X), lhs.GetElement(VEC_Y), lhs.GetElement(VEC_Z));
                VEC3_TYPE rhs3(rhs.GetElement(VEC_X), rhs.GetElement(VEC_Y), rhs.GetElement(VEC_Z));
                VEC3_TYPE result3 = Calculator<VEC3_TYPE>::Cross(lhs3, rhs3);

                return VEC4_TYPE(
                 result3.GetElement(VEC_X),
                 result3.GetElement(VEC_Y),
                 result3.GetElement(VEC_Z),
                 0.0f);
            }

            static inline VEC4_TYPE Mul(const VEC4_TYPE& lhs, float value)
            {
                return lhs * value;
            }

            static inline VEC4_TYPE Mul(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return lhs * rhs;
            }

            static inline VEC4_TYPE Div(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return lhs / rhs;
            }

            static inline float Dot(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return lhs.Dot(rhs);
            }

            static inline float Length(const VEC4_TYPE& v)
            {
                return v.GetLength();
            }

            static inline float Distance(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                auto dir = Sub(lhs, rhs);
                return Length(dir);
            }

            static inline void SetValue(VEC4_TYPE& value, float a)
            {
                value.Set(a);
            }

            static inline void SetValue(VEC4_TYPE& value, float a, float b, float c, float d)
            {
                value.Set(a,b,c,d);
            }

            static inline VEC4_TYPE Normalize(const VEC4_TYPE& lhs)
            {
                return lhs.GetNormalized();
            }

            static inline VEC4_TYPE ComponentMin(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return lhs.GetMin(rhs);
            }

            static inline VEC4_TYPE ComponentMax(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return lhs.GetMax(rhs);
            }

            static inline bool CmpAllEq(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return lhs == rhs;
            }

            static inline bool CmpAllLt(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return (lhs.GetElement(VEC_X) < rhs.GetElement(VEC_X)) 
                || (lhs.GetElement(VEC_Y) < rhs.GetElement(VEC_Y))
                || (lhs.GetElement(VEC_Z) < rhs.GetElement(VEC_Z)) 
                || (lhs.GetElement(VEC_W) < rhs.GetElement(VEC_W));
            }

            static inline bool CmpAllLtEq(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return (lhs.GetElement(VEC_X) <= rhs.GetElement(VEC_X)) || (lhs.GetElement(VEC_Y) <= rhs.GetElement(VEC_Y))
                || (lhs.GetElement(VEC_Z) <= rhs.GetElement(VEC_Z)) || (lhs.GetElement(VEC_W) <= rhs.GetElement(VEC_W));
            }

            static inline bool CmpAllGt(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return (lhs.GetElement(VEC_X) > rhs.GetElement(VEC_X)) || (lhs.GetElement(VEC_Y) > rhs.GetElement(VEC_Y))
                || (lhs.GetElement(VEC_Z) > rhs.GetElement(VEC_Z)) || (lhs.GetElement(VEC_W) > rhs.GetElement(VEC_W));
            }

            static inline bool CmpAllGtEq(const VEC4_TYPE& lhs, const VEC4_TYPE& rhs)
            {
                return (lhs.GetElement(VEC_X) >= rhs.GetElement(VEC_X)) || (lhs.GetElement(VEC_Y) >= rhs.GetElement(VEC_Y))
                || (lhs.GetElement(VEC_Z) >= rhs.GetElement(VEC_Z)) || (lhs.GetElement(VEC_W) >= rhs.GetElement(VEC_W));
            }

            static inline VEC4_TYPE GetConjugate(const VEC4_TYPE& lhs)
            {
                return VEC4_TYPE(-lhs.GetElement(VEC_X), -lhs.GetElement(VEC_Y), -lhs.GetElement(VEC_Z), lhs.GetElement(VEC_W));
            }

            static inline VEC4_TYPE Madd(const VEC4_TYPE& mul1, const VEC4_TYPE& mul2, const VEC4_TYPE& add)
            {
                return Add(Mul(mul1, mul2), add);
            }

            static inline VEC4_TYPE Lerp(const VEC4_TYPE& src, const VEC4_TYPE& dest, const float& alpha)
            {
                return src.Lerp(dest, alpha);
            }
        };

        template <>
        struct Calculator<AZ::Quaternion> 
        {
            static inline AZ::Quaternion Add(const AZ::Quaternion& lhs, const AZ::Quaternion& rhs)
            {
                return lhs + rhs;
            }

            static inline AZ::Quaternion Sub(const AZ::Quaternion& lhs, const AZ::Quaternion& rhs)
            {
                return lhs - rhs;
            }


            static inline  AZ::Quaternion Mul(const  AZ::Quaternion& lhs, float value)
            {
                return lhs * value;
            }

            static inline  AZ::Quaternion Mul(const  AZ::Quaternion& lhs, const  AZ::Quaternion& rhs)
            {
                return lhs * rhs;
            }

            static inline float Dot(const  AZ::Quaternion& lhs, const  AZ::Quaternion& rhs)
            {
                return lhs.Dot(rhs);
            }

            static inline float Length(const  AZ::Quaternion& v)
            {
                return v.GetLength();
            }

            static inline float Distance(const  AZ::Quaternion& lhs, const  AZ::Quaternion& rhs)
            {
                auto dir = Sub(lhs, rhs);
                return Length(dir);
            }

            static inline void SetValue(AZ::Quaternion& value, float a)
            {
                value.Set(a);
            }

            static inline void SetValue(AZ::Quaternion& value, float a, float b, float c, float d)
            {
                value.Set(a,b,c,d);
            }

            static inline AZ::Quaternion Normalize(const AZ::Quaternion& lhs)
            {
                return lhs.GetNormalized();
            }

            static inline bool CmpAllEq(const AZ::Quaternion& lhs, const AZ::Quaternion& rhs)
            {
                return lhs == rhs;
            }

            static inline VEC4_TYPE GetConjugate(const VEC4_TYPE& lhs)
            {
                return VEC4_TYPE(-lhs.GetElement(VEC_X), -lhs.GetElement(VEC_Y), -lhs.GetElement(VEC_Z), lhs.GetElement(VEC_W));
            }
       };
    }


    inline Vector3::Vector3()
    {
        detail::Calculator<MemType>::SetValue(value, 0, 0, 0);
    }

    inline Vector3::Vector3(float v)
    {
        detail::Calculator<MemType>::SetValue(value, v);
    }

    inline Vector3::Vector3(float tx, float ty, float tz)
    {
        detail::Calculator<MemType>::SetValue(value, tx, ty, tz);
    }

    inline Vector3::Vector3(const Vector3& v)
    {
        value = v.value;
    }

    inline Vector3::Vector3(const Vector4& v)
    {
        value = VEC3_TYPE(v.value);
    }

    inline Vector3& Vector3::operator=(const Vector3& v)
    {
        value = v.value;
        return *this;
    }

    inline Vector3& Vector3::operator+=(const Vector3& v)
    {
        value += v.value;
        return *this;
    }

    inline Vector3& Vector3::operator-=(const Vector3& v)
    {
        value -= v.value;
        return *this;
    }

    inline Vector3& Vector3::operator+=(float v)
    {
        value += VEC3_TYPE(v, v, v);
        return *this;
    }

    inline Vector3& Vector3::operator-=(float v)
    {
        value -= VEC3_TYPE(v, v, v);
        return *this;
    }

    inline Vector3& Vector3::operator*=(const Vector3& v)
    {
        value *= v.value;
        return *this;
    }

    inline Vector3& Vector3::operator/=(const Vector3& v)
    {
        value /= v.value;
        return *this;
    }

    inline Vector3& Vector3::operator*=(float v)
    {
        value *= VEC3_TYPE(v, v, v);
        return *this;
    }

    inline Vector3& Vector3::operator/=(float v)
    {
        value /= VEC3_TYPE(v, v, v);
        return *this;
    }

    inline bool Vector3::operator==(const Vector3& v) const
    {
        return detail::Calculator<MemType>::CmpAllEq(value, v.value);
    }

    inline bool Vector3::operator!=(const Vector3& v) const
    {
        return !(*this == v);
    }

    inline const float Vector3::operator[](size_t index) const
    {
        return value.GetElement(static_cast<int32_t>(index));
    }

    inline float Vector3::Dot(const Vector3& v) const
    {
        return detail::Calculator<MemType>::Dot(value, v.value);
    }

    inline float Vector3::Length() const
    {
        return detail::Calculator<MemType>::Length(value);
    }

    inline float Vector3::Distance(const Vector3& v) const
    {
        return detail::Calculator<MemType>::Distance(value, v.value);
    }

    inline Vector3 Vector3::Cross(const Vector3& v) const
    {
        Vector3 result;
        result.value = detail::Calculator<MemType>::Cross(value, v.value);
        return result;
    }

    inline Vector3 Vector3::Lerp(const Vector3& dest, float alpha) const
    {
        Vector3 result;
        result.value = detail::Calculator<MemType>::Lerp(value, dest.value, alpha);
        return result;
    }

    inline Vector3 Vector3::ComponentMin(const Vector3& v) const
    {
        Vector3 result;
        result.value = detail::Calculator<MemType>::ComponentMin(value, v.value);
        return result;
    }

    inline Vector3 Vector3::ComponentMax(const Vector3& v) const
    {
        Vector3 result;
        result.value = detail::Calculator<MemType>::ComponentMax(value, v.value);
        return result;
    }

    inline bool Vector3::IsLessThan(const Vector3& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllLt(value, rhs.value);
    }

    inline bool Vector3::IsLessEqualThan(const Vector3& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllLtEq(value, rhs.value);
    }

    inline bool Vector3::IsGreaterThan(const Vector3& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllGt(value, rhs.value);
    }

    inline bool Vector3::IsGreaterEqualThan(const Vector3& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllGtEq(value, rhs.value);
    }

    inline bool Vector3::IsEqual(const Vector3& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllEq(value, rhs.value);
    }

    inline float Vector3::GetMaxElement() const
    {
        return value.GetMaxElement();
    }

    inline float Vector3::GetMinElement() const
    {
        return value.GetMinElement();
    }

    inline bool Vector3::IsValid() const
    {
        if (std::isnan(value.GetX()) || std::isinf(value.GetX()) ||
            std::isnan(value.GetY()) || std::isinf(value.GetY()) ||
            std::isnan(value.GetZ()) || std::isinf(value.GetZ())) {
            return false;
        }
        return true;
    }

    inline Vector3& Vector3::Normalize()
    {
        value = detail::Calculator<MemType>::Normalize(value);
        return *this;
    }

    inline Vector3 Vector3::Reciprocal() const
    {
        return Vector3(1.0f / value.GetX(), 1.0f / value.GetY(), 1.0f / value.GetZ());
    }

    inline Vector4::Vector4()
    {
        detail::Calculator<MemType>::SetValue(value, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    inline Vector4::Vector4(const VEC4_TYPE& v)
    {
        value = v;
    }

    inline Vector4::Vector4(float v)
    {
        detail::Calculator<MemType>::SetValue(value, v, v, v, v);
    }

    inline Vector4::Vector4(float tx, float ty, float tz, float tw)
    {
        detail::Calculator<MemType>::SetValue(value, tx, ty, tz, tw);
    }

    inline Vector4::Vector4(const Vector3& v, float w)
    {
        value = VEC4_TYPE::CreateFromVector3AndFloat(v.value, w);
    }

    inline void Vector4::operator=(const Vector3& v)
    {
        value = VEC4_TYPE::CreateFromVector3(v.value);
    }

    inline Vector4& Vector4::operator+=(const Vector4& v)
    {
        value = detail::Calculator<MemType>::Add(value, v.value);
        return *this;
    }

    inline Vector4& Vector4::operator-=(const Vector4& v)
    {
        value = detail::Calculator<MemType>::Sub(value, v.value);
        return *this;
    }

    inline Vector4& Vector4::operator*=(const Vector4& v)
    {
        value = detail::Calculator<MemType>::Mul(value, v.value);
        return *this;
    }

    inline Vector4& Vector4::operator/=(const Vector4& v)
    {
        value = detail::Calculator<MemType>::Div(value, v.value);
        return *this;
    }

    inline Vector4& Vector4::operator*=(float v)
    {
        value = detail::Calculator<MemType>::Mul(value, Vector4{v, v, v, v}.value);
        return *this;
    }

    inline Vector4& Vector4::operator/=(float v)
    {
        value = detail::Calculator<MemType>::Div(value, Vector4{v, v, v, v}.value);
        return *this;
    }

    inline bool Vector4::operator==(const Vector4& v) const
    {
        return detail::Calculator<MemType>::CmpAllEq(value, v.value);
    }

    inline bool Vector4::operator!=(const Vector4& v) const
    {
        return !(*this == v);
    }

    inline const float Vector4::operator[](size_t index) const
    {
        return value.GetElement(static_cast<int32_t>(index));
    }

    inline float Vector4::Dot(const Vector4& v) const
    {
        return detail::Calculator<MemType>::Dot(value, v.value);
    }

    inline float Vector4::Length() const
    {
        return detail::Calculator<MemType>::Length(value);
    }

    inline float Vector4::Distance(const Vector4& v) const
    {
        return detail::Calculator<MemType>::Distance(value, v.value);
    }

    inline Vector4& Vector4::Normalize()
    {
        value = detail::Calculator<MemType>::Normalize(value);
        return *this;
    }

    inline Vector4 Vector4::ComponentMin(const Vector4& v) const
    {
        Vector4 result;
        result.value = detail::Calculator<MemType>::ComponentMin(value, v.value);
        return result;
    }

    inline Vector4 Vector4::ComponentMax(const Vector4& v) const
    {
        Vector4 result;
        result.value = detail::Calculator<MemType>::ComponentMax(value, v.value);
        return result;
    }

    inline bool Vector4::IsLessThan(const Vector4& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllLt(value, rhs.value);
    }

    inline bool Vector4::IsLessEqualThan(const Vector4& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllLtEq(value, rhs.value);
    }


    inline bool Vector4::IsGreaterThan(const Vector4& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllGt(value, rhs.value);
    }


    inline bool Vector4::IsGreaterEqualThan(const Vector4& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllGtEq(value, rhs.value);
    }

    inline bool Vector4::IsEqual(const Vector4& rhs) const
    {
        return detail::Calculator<MemType>::CmpAllEq(value, rhs.value);
    }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
