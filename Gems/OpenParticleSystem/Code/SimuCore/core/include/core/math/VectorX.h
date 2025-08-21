/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cmath>
#include "core/platform/Platform.h"
#include "core/math/SimdType.h"
#include "core/math/Math.h"

namespace SimuCore {
    enum VectorEnum {
        VEC_X = 0,
        VEC_Y,
        VEC_Z,
        VEC_W
    };

    struct Vector4;

    struct Vector3 {
        using MemType = VEC4_TYPE; // simd requires 4byte memory

        union {
            MemType value;
            struct {
                float x;
                float y;
                float z;
            };
            
        };

        inline const float& GetElement(size_t index) const
        {
            return (&x)[index];
        }

        inline void SetElement(size_t index, float v)
        {
            (&x)[index] = v;
        }

        inline Vector3();

        inline explicit Vector3(float v);

        inline Vector3(float tx, float ty, float tz);

        inline Vector3(const Vector3& v);

        inline explicit Vector3(const Vector4& v);

        inline Vector3& operator=(const Vector3& v);
        inline Vector3& operator+=(const Vector3& v);
        inline Vector3& operator-=(const Vector3& v);
        inline Vector3& operator+=(float v);
        inline Vector3& operator-=(float v);
        inline Vector3& operator*=(const Vector3& v);
        inline Vector3& operator/=(const Vector3& v);
        inline Vector3& operator*=(float v);
        inline Vector3& operator/=(float v);
        inline bool operator==(const Vector3& v) const;
        inline bool operator!=(const Vector3& v) const;

        inline bool IsValid() const;
        inline Vector3& Normalize();
        inline Vector3 Reciprocal() const;
        //! Creates a vector with all components set to zero, more efficient than calling Vector3(0.0f).
        static inline Vector3 CreateZero();
        //! Creates a vector with all components set to one.
        static inline Vector3 CreateOne();
        // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        static inline Vector3 CreateSelectCmpGreaterEqual(const Vector3& cmp1,
            const Vector3& cmp2, const Vector3& vA, const Vector3& vB);
        inline float& operator[](size_t index);
        inline const float& operator[](size_t index) const;
        [[nodiscard]] inline float Dot(const Vector3& v) const;
        [[nodiscard]] inline float Length() const;
        [[nodiscard]] inline float Distance(const Vector3& v) const;
        [[nodiscard]] inline Vector3 Cross(const Vector3& v) const;
        [[nodiscard]] inline Vector3 Lerp(const Vector3& dest, float alpha) const;
        [[nodiscard]] inline Vector3 ComponentMin(const Vector3& v) const;
        [[nodiscard]] inline Vector3 ComponentMax(const Vector3& v) const;
        [[nodiscard]] inline bool IsLessThan(const Vector3& rhs) const;
        [[nodiscard]] inline bool IsLessEqualThan(const Vector3& rhs) const;
        [[nodiscard]] inline bool IsGreaterThan(const Vector3& rhs) const;
        [[nodiscard]] inline bool IsGreaterEqualThan(const Vector3& rhs) const;
        [[nodiscard]] inline bool IsEqual(const Vector3& rhs) const;
        [[nodiscard]] inline float GetMaxElement() const;
        [[nodiscard]] inline float GetMinElement() const;

        [[nodiscard]] inline Vector3 GetMax(const Vector3& v) const;
        [[nodiscard]] inline Vector3 GetMin(const Vector3& v) const;

        inline Vector3 operator-() const
        {
            return Vector3(-x, -y, -z);
        }
    };

    // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
    inline Vector3 Vector3::CreateSelectCmpGreaterEqual(const Vector3 &cmp1,
        const Vector3 &cmp2, const Vector3 &vA, const Vector3 &vB)
    {
        return {(cmp1.x >= cmp2.x) ? vA.x : vB.x, (cmp1.y >= cmp2.y) ? vA.y : vB.y, (cmp1.z >= cmp2.z) ? vA.z : vB.z};
    }

    inline Vector3 operator+(const Vector3& lhs, const Vector3& rhs)
    {
        return Vector3(lhs) += rhs;
    }

    inline Vector3 operator-(const Vector3& lhs, const Vector3& rhs)
    {
        return Vector3(lhs) -= rhs;
    }

    inline Vector3 operator+(const Vector3& lhs, float rhs)
    {
        return Vector3(lhs) += rhs;
    }

    inline Vector3 operator-(const Vector3& lhs, float rhs)
    {
        return Vector3(lhs) -= rhs;
    }

    inline Vector3 operator*(const Vector3& lhs, const Vector3& rhs)
    {
        return Vector3(lhs) *= rhs;
    }

    inline Vector3 operator/(const Vector3& lhs, const Vector3& rhs)
    {
        return Vector3(lhs) /= rhs;
    }

    inline Vector3 operator*(const Vector3& lhs, float rhs)
    {
        return Vector3(lhs) *= rhs;
    }

    inline Vector3 operator*(float lhs, const Vector3& rhs)
    {
        return Vector3(lhs) *= rhs;
    }

    inline Vector3 operator/(const Vector3& lhs, float rhs)
    {
        return Vector3(lhs) /= rhs;
    }

    inline Vector3 Vector3::GetMin(const Vector3& v) const
    {
        return Vector3(Math::Min(x, v.x), Math::Min(y, v.y), Math::Min(z, v.z));
    }


    inline Vector3 Vector3::GetMax(const Vector3& v) const
    {
        return Vector3(Math::Max(x, v.x), Math::Max(y, v.y), Math::Max(z, v.z));
    }

    struct Vector4 {
        using MemType = VEC4_TYPE;
        union {
            struct {
                float x;
                float y;
                float z;
                float w;
            };
            MemType value;
        };

        inline Vector4();

        inline explicit Vector4(float v);

        inline Vector4(float tx, float ty, float tz, float tw);

        inline explicit Vector4(const Vector3& v, float w = 1.0f);

        inline void operator=(const Vector3& v);

        inline Vector4& operator+=(const Vector4& v);
        inline Vector4& operator-=(const Vector4& v);
        inline Vector4& operator*=(const Vector4& v);
        inline Vector4& operator/=(const Vector4& v);
        inline Vector4& operator*=(float v);
        inline Vector4& operator/=(float v);
        inline bool operator==(const Vector4& v) const;
        inline bool operator!=(const Vector4& v) const;

        inline Vector4& Normalize();
        inline float& operator[](size_t index);
        inline const float& operator[](size_t index) const;
        [[nodiscard]] inline float Dot(const Vector4& v) const;
        [[nodiscard]] inline float Length() const;
        [[nodiscard]] inline float Distance(const Vector4& v) const;
        [[nodiscard]] inline Vector4 ComponentMin(const Vector4& v) const;
        [[nodiscard]] inline Vector4 ComponentMax(const Vector4& v) const;
        [[nodiscard]] inline bool IsLessThan(const Vector4& rhs) const;
        [[nodiscard]] inline bool IsLessEqualThan(const Vector4& rhs) const;
        [[nodiscard]] inline bool IsGreaterThan(const Vector4& rhs) const;
        [[nodiscard]] inline bool IsGreaterEqualThan(const Vector4& rhs) const;
        [[nodiscard]] inline bool IsEqual(const Vector4& rhs) const;
    };

    inline Vector4 operator+(const Vector4& lhs, const Vector4& rhs)
    {
        return Vector4(lhs) += rhs;
    }

    inline Vector4 operator-(const Vector4& lhs, const Vector4& rhs)
    {
        return Vector4(lhs) -= rhs;
    }

    inline Vector4 operator*(const Vector4& lhs, const Vector4& rhs)
    {
        return Vector4(lhs) *= rhs;
    }

    inline Vector4 operator/(const Vector4& lhs, const Vector4& rhs)
    {
        return Vector4(lhs) /= rhs;
    }

    inline Vector4 operator*(const Vector4& lhs, float rhs)
    {
        return Vector4(lhs) *= rhs;
    }

    inline Vector4 operator*(float lhs, const Vector4& rhs)
    {
        return Vector4(lhs) *= rhs;
    }

    inline Vector4 operator/(const Vector4& lhs, float rhs)
    {
        return Vector4(lhs) /= rhs;
    }

    template <typename T, typename = std::enable_if<std::is_same<T, Vector3>::value || std::is_same<T, Vector4>::value>>
    inline float Dot(const T& lhs, const T& rhs)
    {
        return lhs.Dot(rhs);
    }

    template<typename T, typename = std::enable_if<std::is_same<T, Vector3>::value || std::is_same<T, Vector4>::value>>
    inline T Normalize(const T& lhs)
    {
        return T(lhs).Normalize();
    }

    inline Vector3 Cross(const Vector3& lhs, const Vector3& rhs)
    {
        return lhs.Cross(rhs);
    }
}

#include "core/math/VectorX.inl"
