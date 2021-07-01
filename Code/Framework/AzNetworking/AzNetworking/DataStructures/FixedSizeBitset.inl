/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    template <AZStd::size_t SIZE, typename ElementType>
    inline FixedSizeBitset<SIZE, ElementType>::FixedSizeBitset()
    {
        InitializeAll(false);
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline FixedSizeBitset<SIZE, ElementType>::FixedSizeBitset(bool value)
    {
        InitializeAll(value);
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline FixedSizeBitset<SIZE, ElementType>::FixedSizeBitset(const ElementType *values)
    {
        static const AZStd::size_t TotalBytes = m_container.size() * sizeof(ElementType);
        memcpy(m_container.data(), values, TotalBytes);
        ClearUnusedBits();
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline FixedSizeBitset<SIZE, ElementType>::FixedSizeBitset(const SelfType& rhs)
    {
        *this = rhs;
    }

    template <AZStd::size_t SIZE, typename ElementType>
    FixedSizeBitset<SIZE, ElementType>& FixedSizeBitset<SIZE, ElementType>::operator =(const SelfType& rhs)
    {
        m_container = rhs.m_container;
        return *this;
    }

    template <AZStd::size_t SIZE, typename ElementType>
    FixedSizeBitset<SIZE, ElementType>& FixedSizeBitset<SIZE, ElementType>::operator |=(const SelfType& rhs)
    {
        for (AZStd::size_t i = 0; i < m_container.size(); ++i)
        {
            m_container[i] |= rhs.m_container[i];
        }
        ClearUnusedBits();
        return *this;
    }

    template <AZStd::size_t SIZE, typename ElementType>
    bool FixedSizeBitset<SIZE, ElementType>::operator ==(const SelfType& rhs) const
    {
        for (AZStd::size_t i = 0; i < m_container.size(); ++i)
        {
            if (m_container[i] != rhs.m_container[i])
            {
                return false;
            }
        }
        return true;
    }

    template <AZStd::size_t SIZE, typename ElementType>
    bool FixedSizeBitset<SIZE, ElementType>::operator !=(const SelfType& rhs) const
    {
        return !(*this == rhs);
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline void FixedSizeBitset<SIZE, ElementType>::InitializeAll(bool value)
    {
        const AZStd::size_t TotalBytes = m_container.size() * sizeof(ElementType);
        memset(m_container.data(), value ? 0xFF : 0x00, TotalBytes);
        ClearUnusedBits();
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline void FixedSizeBitset<SIZE, ElementType>::SetBit(uint32_t index, bool value)
    {
        AZ_Assert(index < SIZE, "Out of bounds access (requested %u, size %u)", index, SIZE);
        constexpr uint32_t ElementTypeBitsLogTwo = AZ::Log2(ElementTypeBits - 1);
        const uint32_t    element = index >> ElementTypeBitsLogTwo;
        const ElementType offset  = index &  (ElementTypeBits - 1);
        const ElementType mask    = static_cast<ElementType>(0x01) << offset;
        const ElementType current = m_container[element];
        m_container[element] = (value) ? current | mask : current & static_cast<ElementType>(~mask);
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline bool FixedSizeBitset<SIZE, ElementType>::GetBit(uint32_t index) const
    {
        AZ_Assert(index < SIZE, "Out of bounds access (requested %u, size %u)", index, SIZE);
        constexpr uint32_t ElementTypeBitsLogTwo = AZ::Log2(ElementTypeBits - 1);
        const uint32_t    element = index >> ElementTypeBitsLogTwo;
        const ElementType offset  = index &  (ElementTypeBits - 1);
        return (static_cast<ElementType>(m_container[element] >> offset) & static_cast<ElementType>(0x01)) ? true : false;
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline bool FixedSizeBitset<SIZE, ElementType>::AnySet() const
    {
        for (uint32_t i = 0; i < m_container.size(); ++i)
        {
            if (m_container[i] != 0)
            {
                return true;
            }
        }
        return false;
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline uint32_t FixedSizeBitset<SIZE, ElementType>::GetValidBitCount() const
    {
        return SIZE;
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline void FixedSizeBitset<SIZE, ElementType>::Subtract(const SelfType& rhs)
    {
        for (uint32_t i = 0; i < m_container.size(); ++i)
        {
            m_container[i] &= ~rhs.m_container[i];
        }
        ClearUnusedBits();
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline const typename FixedSizeBitset<SIZE, ElementType>::ContainerType& FixedSizeBitset<SIZE, ElementType>::GetContainer() const
    {
        return m_container;
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline typename FixedSizeBitset<SIZE, ElementType>::ContainerType& FixedSizeBitset<SIZE, ElementType>::GetContainer()
    {
        return m_container;
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline bool FixedSizeBitset<SIZE, ElementType>::Serialize(ISerializer& serializer)
    {
        constexpr uint32_t numBytesToSerialize = ElementCount * ElementTypeBits / 8;
        uint8_t* bitsetContainer = reinterpret_cast<uint8_t*>(m_container.data());
        bool success = true;
        for (uint32_t i = 0; i < numBytesToSerialize; ++i)
        {
            if (!serializer.Serialize(bitsetContainer[i], GenerateIndexLabel<SIZE>(i).c_str()))
            {
                success = false;
                break;
            }
        }
        ClearUnusedBits();
        return success;
    }

    template <>
    inline void FixedSizeBitset<0>::ClearUnusedBits()
    {
        ;
    }

    template <AZStd::size_t SIZE, typename ElementType>
    inline void FixedSizeBitset<SIZE, ElementType>::ClearUnusedBits()
    {
        constexpr ElementType AllOnes      = static_cast<ElementType>(~0);
        constexpr ElementType LastUsedBits = (SIZE % ElementTypeBits);
        constexpr ElementType ShiftAmount  = (LastUsedBits == 0) ? 0 : ElementTypeBits - LastUsedBits;
        constexpr ElementType ClearBitMask = AllOnes >> ShiftAmount;
        m_container[m_container.size() - 1] &= ClearBitMask;
    }
}
