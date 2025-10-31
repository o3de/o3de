/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Vector4.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    template<typename TYPE>
    class PackedVector4
    {
    public:
        //===============================================================
        // Constructors.
        //===============================================================
        PackedVector4();
        explicit PackedVector4(TYPE initValue);
        PackedVector4(TYPE x, TYPE y, TYPE z, TYPE w);
        explicit PackedVector4(const AZ::Vector4& rhs);
        explicit PackedVector4(const TYPE* initData);

        //===============================================================
        // Conversions.
        //===============================================================
        explicit operator TYPE* ();
        explicit operator const TYPE* () const;
        explicit operator AZ::Vector4() const;

        //===============================================================
        // Interface.
        //======================
        void Set(TYPE x, TYPE y, TYPE z, TYPE w);

        TYPE GetElement(size_t index) const;
        void SetElement(size_t index, TYPE val);

        TYPE GetX() const;
        TYPE GetY() const;
        TYPE GetZ() const;
        TYPE GetW() const;
        void SetX(TYPE v);
        void SetY(TYPE v);
        void SetZ(TYPE v);
        void SetW(TYPE v);

    private:
        TYPE m_x;
        TYPE m_y;
        TYPE m_z;
        TYPE m_w;
    };

    AZ_TYPE_INFO_TEMPLATE(PackedVector4, "{E01FF5C3-0346-4B84-912A-967CB9A21EDB}", AZ_TYPE_INFO_TYPENAME);

    //===============================================================
    // Standard type variations.
    //===============================================================
    using PackedVector4f = PackedVector4<float>;
    using PackedVector4i = PackedVector4<int32_t>;


    //===============================================================
    // Implementation.
    //===============================================================

    template<typename TYPE>
    PackedVector4<TYPE>::PackedVector4()
    {}

    template<typename TYPE>
    PackedVector4<TYPE>::PackedVector4(TYPE initValue)
        : m_x(initValue)
        , m_y(initValue)
        , m_z(initValue)
        , m_w(initValue)
    {}

    template<typename TYPE>
    PackedVector4<TYPE>::PackedVector4(TYPE x, TYPE y, TYPE z, TYPE w)
        : m_x(x)
        , m_y(y)
        , m_z(z)
        , m_w(w)
    {}

    template<typename TYPE>
    PackedVector4<TYPE>::PackedVector4(const AZ::Vector4& rhs)
        : m_x(rhs.GetX())
        , m_y(rhs.GetY())
        , m_z(rhs.GetZ())
        , m_w(rhs.GetW())
    {}

    template<typename TYPE>
    PackedVector4<TYPE>::PackedVector4(const TYPE* initData)
        : m_x(initData[0])
        , m_y(initData[1])
        , m_z(initData[2])
        , m_w(initData[3])
    {}

    template<typename TYPE>
    PackedVector4<TYPE>::operator TYPE* ()
    {
        return &m_x;
    }

    template<typename TYPE>
    PackedVector4<TYPE>::operator const TYPE* () const
    {
        return &m_x;
    }

    template<typename TYPE>
    PackedVector4<TYPE>::operator AZ::Vector4() const
    {
        return AZ::Vector4(m_x, m_y, m_z, m_w);
    }

    template<typename TYPE>
    void PackedVector4<TYPE>::Set(TYPE x, TYPE y, TYPE z, TYPE w)
    {
        m_x = x;
        m_y = y;
        m_z = z;
        m_w = w;
    }

    template<typename TYPE>
    TYPE PackedVector4<TYPE>::GetElement(size_t index) const
    {
        AZ_MATH_ASSERT(index < 4, "access beyond bounds of PackedVector4");
        return reinterpret_cast<const TYPE*>(this)[index];
    }

    template<typename TYPE>
    void PackedVector4<TYPE>::SetElement(size_t index, TYPE val)
    {
        AZ_MATH_ASSERT(index < 4, "access beyond bounds of PackedVector4");
        reinterpret_cast<TYPE*>(this)[index] = val;
    }

    template<typename TYPE>
    TYPE PackedVector4<TYPE>::GetX() const
    {
        return m_x;
    }

    template<typename TYPE>
    void PackedVector4<TYPE>::SetX(TYPE v)
    {
        m_x = v;
    }

    template<typename TYPE>
    TYPE PackedVector4<TYPE>::GetY() const
    {
        return m_y;
    }

    template<typename TYPE>
    void PackedVector4<TYPE>::SetY(TYPE v)
    {
        m_y = v;
    }

    template<typename TYPE>
    TYPE PackedVector4<TYPE>::GetZ() const
    {
        return m_z;
    }


    template<typename TYPE>
    void PackedVector4<TYPE>::SetZ(TYPE v)
    {
        m_z = v;
    }

template<typename TYPE>
    void PackedVector4<TYPE>::SetW(TYPE v)
    {
        m_w = v;
    }

    template<typename TYPE>
    TYPE PackedVector4<TYPE>::GetW() const
    {
        return m_w;
    }
}

