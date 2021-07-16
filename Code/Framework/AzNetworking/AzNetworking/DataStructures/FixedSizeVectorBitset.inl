/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzNetworking
{
    template <AZStd::size_t CAPACITY, typename ElementType>
    inline FixedSizeVectorBitset<CAPACITY, ElementType>& FixedSizeVectorBitset<CAPACITY, ElementType>::operator =(const SelfType& rhs)
    {
        m_count = rhs.m_count;
        m_bitset = rhs.m_bitset;
        return *this;
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline FixedSizeVectorBitset<CAPACITY, ElementType>& FixedSizeVectorBitset<CAPACITY, ElementType>::operator|=(const SelfType& rhs)
    {
        if (rhs.GetSize() > GetSize())
        {
            Resize(rhs.GetSize());
        }
        uint32_t usedElementSize = (GetSize() + BitsetType::ElementTypeBits - 1) / BitsetType::ElementTypeBits;
        for (uint32_t i = 0; i < usedElementSize; ++i)
        {
            m_bitset.GetContainer()[i] |= rhs.m_bitset.GetContainer()[i];
        }
        return *this;
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline void FixedSizeVectorBitset<CAPACITY, ElementType>::SetBit(uint32_t index, bool value)
    {
        if (index > m_count)
        {
            return;
        }
        m_bitset.SetBit(index, value);
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline bool FixedSizeVectorBitset<CAPACITY, ElementType>::GetBit(uint32_t index) const
    {
        if (index > m_count)
        {
            return false;
        }
        return m_bitset.GetBit(index);
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline bool FixedSizeVectorBitset<CAPACITY, ElementType>::AnySet() const
    {
        uint32_t usedElementSize = (GetSize() + BitsetType::ElementTypeBits - 1) / BitsetType::ElementTypeBits;
        for (uint32_t i = 0; i < usedElementSize; ++i)
        {
            if (m_bitset.GetContainer()[i] != 0)
            {
                return true;
            }
        }
        return false;
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline uint32_t FixedSizeVectorBitset<CAPACITY, ElementType>::GetValidBitCount() const
    {
        return m_count;
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline void FixedSizeVectorBitset<CAPACITY, ElementType>::Subtract(const SelfType& other)
    {
        if (other.GetSize() > GetSize())
        {
            Resize(other.GetSize());
        }
        uint32_t usedElementSize = (GetSize() + BitsetType::ElementTypeBits - 1) / BitsetType::ElementTypeBits;
        for (uint32_t i = 0; i < usedElementSize; ++i)
        {
            m_bitset.GetContainer()[i] &= ~other.m_bitset.GetContainer()[i];
        }
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline uint32_t FixedSizeVectorBitset<CAPACITY, ElementType>::GetSize() const
    {
        return m_count;
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline uint32_t FixedSizeVectorBitset<CAPACITY, ElementType>::GetCapacity() const
    {
        return CAPACITY;
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline bool FixedSizeVectorBitset<CAPACITY, ElementType>::Resize(uint32_t count)
    {
        if (count > CAPACITY)
        {
            return false;
        }
        const bool shouldClear = m_count > count;
        m_count = count;
        if (shouldClear)
        {
            ClearUnusedBits();
        }
        return true;
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline bool FixedSizeVectorBitset<CAPACITY, ElementType>::AddBits(uint32_t count)
    {
        return Resize(m_count + count);
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline void FixedSizeVectorBitset<CAPACITY, ElementType>::Clear()
    {
        m_count = 0;
        m_bitset.InitializeAll(false);
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline bool FixedSizeVectorBitset<CAPACITY, ElementType>::GetBack() const
    {
        return m_bitset.GetBit(m_count - 1); // Asserts internally if out of range
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline bool FixedSizeVectorBitset<CAPACITY, ElementType>::PushBack(bool value)
    {
        if (m_count >= CAPACITY)
        {
            return false;
        }
        m_bitset.SetBit(m_count, value);
        ++m_count;
        return true;
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline bool FixedSizeVectorBitset<CAPACITY, ElementType>::PopBack()
    {
        if (m_count <= 0)
        {
            return false;
        }
        m_bitset.SetBit(m_count - 1, false);
        --m_count;
        return true;
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline bool FixedSizeVectorBitset<CAPACITY, ElementType>::Serialize(ISerializer& serializer)
    {
        static constexpr uint32_t NumBytesRequiredForSize = AZ::RequiredBytesForValue<CAPACITY>();
        using SizeType = typename AZ::SizeType<NumBytesRequiredForSize, false>::Type;

        // m_count is always less than CAPACITY, so should fit safely in SizeType
        SizeType count = static_cast<SizeType>(m_count);
        if (!serializer.Serialize(count, "Count"))
        {
            return false;
        }
        m_count = count;

        const uint32_t numBytesToSerialize = (m_count + 7) / 8; // Round up nearest byte
        uint8_t* bitsetContainer = reinterpret_cast<uint8_t*>(m_bitset.GetContainer().data());
        for (uint32_t i = 0; i < numBytesToSerialize; ++i)
        {
            if (!serializer.Serialize(bitsetContainer[i], "Byte"))
            {
                return false;
            }
        }
        ClearUnusedBits();
        return true;
    }

    template <AZStd::size_t CAPACITY, typename ElementType>
    inline void FixedSizeVectorBitset<CAPACITY, ElementType>::ClearUnusedBits()
    {
        uint32_t usedElementSize = (GetSize() + BitsetType::ElementTypeBits - 1) / BitsetType::ElementTypeBits;
        for (uint32_t i = usedElementSize + 1; i < BitsetType::ElementCount; ++i)
        {
            m_bitset.GetContainer()[i] = 0;
        }
        m_bitset.ClearUnusedBits();
    }
}
