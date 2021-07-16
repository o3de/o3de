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
    template <uint32_t SIZE>
    inline RingbufferBitset<SIZE>::RingbufferBitset()
    {
        Reset();
    }

    template <uint32_t SIZE>
    inline void RingbufferBitset<SIZE>::Reset()
    {
        m_headElementOffset = 0;
        m_unusedHeadBits    = NumBitsetChunkedBits;
        for (uint32_t i = 0; i < m_container.size(); ++i)
        {
            m_container[i] = 0;
        }
    }

    template <uint32_t SIZE>
    inline bool RingbufferBitset<SIZE>::GetBit(uint32_t index) const
    {
        const uint32_t usedHeadBits = NumBitsetChunkedBits - m_unusedHeadBits;

        if (index < usedHeadBits)
        {
            return GetBitHelper(m_container[m_headElementOffset], index);
        }

        index -= usedHeadBits;

        const uint32_t element  = (index >> 5) + 1;
        const uint32_t bitIndex = index & (NumBitsetChunkedBits - 1);

        return GetBitHelper(m_container[GetChunkIndexHelper(element)], bitIndex);
    }

    template <uint32_t SIZE>
    inline void RingbufferBitset<SIZE>::SetBit(uint32_t index, bool value)
    {
        const uint32_t usedHeadBits = NumBitsetChunkedBits - m_unusedHeadBits;

        if (index < usedHeadBits)
        {
            return SetBitHelper(m_container[m_headElementOffset], index, value);
        }

        index -= usedHeadBits;

        const uint32_t element  = (index >> 5) + 1;
        const uint32_t bitIndex = index & (NumBitsetChunkedBits - 1);

        return SetBitHelper(m_container[GetChunkIndexHelper(element)], bitIndex, value);
    }

    template <uint32_t SIZE>
    inline uint32_t RingbufferBitset<SIZE>::GetValidBitCount() const
    {
        return SIZE - NumBitsetChunkedBits;
    }

    template <uint32_t SIZE>
    inline void RingbufferBitset<SIZE>::PushBackBits(uint32_t count)
    {
        const uint32_t headBitsToShift = std::min<uint32_t>(count, m_unusedHeadBits);

        m_container[m_headElementOffset] <<= headBitsToShift;
        m_unusedHeadBits -= headBitsToShift;
        count -= headBitsToShift;

        if (m_unusedHeadBits <= 0)
        {
            IncrementHead();
        }

        while (count > NumBitsetChunkedBits)
        {
            count -= NumBitsetChunkedBits;
            IncrementHead();
        }

        m_unusedHeadBits -= count;
    }

    template <uint32_t SIZE>
    inline const BitsetChunk& RingbufferBitset<SIZE>::GetBitsetElement(uint32_t elementIndex) const
    {
        return m_container[GetChunkIndexHelper(elementIndex)];
    }

    template <uint32_t SIZE>
    inline uint32_t RingbufferBitset<SIZE>::GetUnusedHeadBits() const
    {
        return m_unusedHeadBits;
    }

    template <uint32_t SIZE>
    inline uint32_t RingbufferBitset<SIZE>::GetChunkIndexHelper(uint32_t absoluteIndex) const
    {
        AZ_Assert(absoluteIndex < RingbufferContainerSize, "Out of bounds chunk index requested");
        return ((m_headElementOffset + RingbufferContainerSize) - absoluteIndex) % RingbufferContainerSize;
    }

    template <uint32_t SIZE>
    inline void RingbufferBitset<SIZE>::IncrementHead()
    {
        m_headElementOffset = (m_headElementOffset + 1) % RingbufferContainerSize;
        m_unusedHeadBits = NumBitsetChunkedBits;
        m_container[m_headElementOffset] = 0;
    }

    template <typename TYPE>
    inline bool GetBitHelper(TYPE element, uint32_t bitIndex)
    {
        AZ_Assert(bitIndex < AZ::Log2(TYPE(~0)), "Out of bounds access (requested %u, size %u)", bitIndex, AZ::Log2(TYPE(~0)));
        return ((element >> bitIndex) & 0x01) ? true : false;
    }

    template <typename TYPE>
    inline void SetBitHelper(TYPE &element, uint32_t bitIndex, bool value)
    {
        AZ_Assert(bitIndex < AZ::Log2(TYPE(~0)), "Out of bounds access (requested %u, size %u)", bitIndex, AZ::Log2(TYPE(~0)));
        const TYPE mask = TYPE(0x01 << bitIndex);
        element = (value) ? (element | mask) : (element & (TYPE)(~mask));
    }
}
