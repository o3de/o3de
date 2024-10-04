/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Vector2.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    template<typename TYPE>
    class PackedVector2
    {
    public:
        //===============================================================
        // Constructors.
        //===============================================================
        PackedVector2();
        explicit PackedVector2(TYPE initValue);
        PackedVector2(TYPE x, TYPE y);
        explicit PackedVector2(const AZ::Vector2& rhs);
        explicit PackedVector2(const TYPE* initData);

        //===============================================================
        // Conversions.
        //===============================================================
        explicit operator TYPE* ();
        explicit operator const TYPE* () const;
        explicit operator AZ::Vector2() const;

        //===============================================================
        // Interface.
        //===============================================================
        void Set(TYPE x, TYPE y);

        TYPE GetElement(size_t index) const;
        void SetElement(size_t index, TYPE val);

        TYPE GetX() const;
        TYPE GetY() const;
        void SetX(TYPE v);
        void SetY(TYPE v);

    private:
        TYPE m_x;
        TYPE m_y;
    };

    AZ_TYPE_INFO_TEMPLATE(PackedVector2, "{C20DBD4B-5542-4FAE-9BE2-BD1DD3027417}", AZ_TYPE_INFO_TYPENAME);

    //===============================================================
    // Standard type variations.
    //===============================================================
    using PackedVector2f = PackedVector2<float>;
    using PackedVector2i = PackedVector2<int32_t>;


    //===============================================================
    // Implementation.
    //===============================================================

    template<typename TYPE>
    PackedVector2<TYPE>::PackedVector2()
    {}

    template<typename TYPE>
    PackedVector2<TYPE>::PackedVector2(TYPE initValue)
        : m_x(initValue)
        , m_y(initValue)
    {}

    template<typename TYPE>
    PackedVector2<TYPE>::PackedVector2(TYPE x, TYPE y)
        : m_x(x)
        , m_y(y)
    {}

    template<typename TYPE>
    PackedVector2<TYPE>::PackedVector2(const AZ::Vector2& rhs)
        : m_x(rhs.GetX())
        , m_y(rhs.GetY())
    {}

    template<typename TYPE>
    PackedVector2<TYPE>::PackedVector2(const TYPE* initData)
        : m_x(initData[0])
        , m_y(initData[1])
    {}

    template<typename TYPE>
    PackedVector2<TYPE>::operator TYPE* ()
    {
        return &m_x;
    }

    template<typename TYPE>
    PackedVector2<TYPE>::operator const TYPE* () const
    {
        return &m_x;
    }

    template<typename TYPE>
    PackedVector2<TYPE>::operator AZ::Vector2() const
    {
        return AZ::Vector2(m_x, m_y);
    }

    template<typename TYPE>
    void PackedVector2<TYPE>::Set(TYPE x, TYPE y)
    {
        m_x = x;
        m_y = y;
    }

    template<typename TYPE>
    TYPE PackedVector2<TYPE>::GetElement(size_t index) const
    {
        AZ_MATH_ASSERT(index < 2, "access beyond bounds of PackedVector4");
        return reinterpret_cast<const TYPE*>(this)[index];
    }

    template<typename TYPE>
    void PackedVector2<TYPE>::SetElement(size_t index, TYPE val)
    {
        AZ_MATH_ASSERT(index < 2, "access beyond bounds of PackedVector4");
        reinterpret_cast<TYPE*>(this)[index] = val;
    }

    template<typename TYPE>
    TYPE PackedVector2<TYPE>::GetX() const
    {
        return m_x;
    }

    template<typename TYPE>
    void PackedVector2<TYPE>::SetX(TYPE v)
    {
        m_x = v;
    }

    template<typename TYPE>
    TYPE PackedVector2<TYPE>::GetY() const
    {
        return m_y;
    }

    template<typename TYPE>
    void PackedVector2<TYPE>::SetY(TYPE v)
    {
        m_y = v;
    }
}

