/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    template<typename TYPE>
    class PackedVector3
    {
    public:
        //===============================================================
        // Constructors.
        //===============================================================
        PackedVector3();
        explicit PackedVector3(TYPE initValue);
        PackedVector3(TYPE x, TYPE y, TYPE z);
        explicit PackedVector3(const AZ::Vector3& rhs);
        explicit PackedVector3(const TYPE* initData);

        //===============================================================
        // Conversions.
        //===============================================================
        explicit operator TYPE* ();
        explicit operator const TYPE* () const;
        explicit operator AZ::Vector3() const;

        //===============================================================
        // Interface.
        //===============================================================
        void Set(TYPE x, TYPE y, TYPE z);

        TYPE GetElement(size_t index) const;
        void SetElement(size_t index, TYPE val);

        TYPE GetX() const;
        TYPE GetY() const;
        TYPE GetZ() const;
        void SetX(TYPE v);
        void SetY(TYPE v);
        void SetZ(TYPE v);

    private:
        TYPE m_x;
        TYPE m_y;
        TYPE m_z;
    };

    AZ_TYPE_INFO_TEMPLATE(PackedVector3, "{AE80F5E2-F809-4E2A-AE63-39F171C62819}", AZ_TYPE_INFO_TYPENAME);

    //===============================================================
    // Standard type variations.
    //===============================================================
    using PackedVector3f = PackedVector3<float>;
    using PackedVector3i = PackedVector3<int32_t>;


    //===============================================================
    // Implementation.
    //===============================================================

    template<typename TYPE>
    PackedVector3<TYPE>::PackedVector3()
    {}

    template<typename TYPE>
    PackedVector3<TYPE>::PackedVector3(TYPE initValue)
        : m_x(initValue)
        , m_y(initValue)
        , m_z(initValue)
    {}

    template<typename TYPE>
    PackedVector3<TYPE>::PackedVector3(TYPE x, TYPE y, TYPE z)
        : m_x(x)
        , m_y(y)
        , m_z(z)
    {}

    template<typename TYPE>
    PackedVector3<TYPE>::PackedVector3(const AZ::Vector3& rhs)
        : m_x(rhs.GetX())
        , m_y(rhs.GetY())
        , m_z(rhs.GetZ())
    {}

    template<typename TYPE>
    PackedVector3<TYPE>::PackedVector3(const TYPE* initData)
        : m_x(initData[0])
        , m_y(initData[1])
        , m_z(initData[2])
    {}

    template<typename TYPE>
    PackedVector3<TYPE>::operator TYPE* ()
    {
        return &m_x;
    }

    template<typename TYPE>
    PackedVector3<TYPE>::operator const TYPE* () const
    {
        return &m_x;
    }

    template<typename TYPE>
    PackedVector3<TYPE>::operator AZ::Vector3() const
    {
        return AZ::Vector3(m_x, m_y, m_z);
    }

    template<typename TYPE>
    void PackedVector3<TYPE>::Set(TYPE x, TYPE y, TYPE z)
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    template<typename TYPE>
    TYPE PackedVector3<TYPE>::GetElement(size_t index) const
    {
        AZ_MATH_ASSERT(index < 3, "access beyond bounds of PackedVector3");
        return reinterpret_cast<const TYPE*>(this)[index];
    }

    template<typename TYPE>
    void PackedVector3<TYPE>::SetElement(size_t index, TYPE val)
    {
        AZ_MATH_ASSERT(index < 3, "access beyond bounds of PackedVector3");
        reinterpret_cast<TYPE*>(this)[index] = val;
    }

    template<typename TYPE>
    TYPE PackedVector3<TYPE>::GetX() const
    {
        return m_x;
    }

    template<typename TYPE>
    void PackedVector3<TYPE>::SetX(TYPE v)
    {
        m_x = v;
    }

    template<typename TYPE>
    TYPE PackedVector3<TYPE>::GetY() const
    {
        return m_y;
    }

    template<typename TYPE>
    void PackedVector3<TYPE>::SetY(TYPE v)
    {
        m_y = v;
    }

    template<typename TYPE>
    TYPE PackedVector3<TYPE>::GetZ() const
    {
        return m_z;
    }

    template<typename TYPE>
    void PackedVector3<TYPE>::SetZ(TYPE v)
    {
        m_z = v;
    }
}
