/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>

namespace AzNetworking
{
    using BitsetChunk = uint32_t;

    //! @class RingBufferBitset
    //! @brief fixed size data structure optimized for storing and rotating large numbers of bits.
    template <uint32_t SIZE>
    class RingbufferBitset
    {
    public:

        static constexpr uint32_t NumBitsetChunkedBits = AZ::Log2(BitsetChunk(~0));
        static constexpr uint32_t RingbufferContainerSize = SIZE / NumBitsetChunkedBits;
        static_assert((SIZE % NumBitsetChunkedBits) == 0, "RingbufferBitset must be a multiple of NumBitsetChunkedBits");

        using RingbufferContainer = AZStd::array<BitsetChunk, RingbufferContainerSize>;

        RingbufferBitset();
        ~RingbufferBitset() = default;

        //! Resets the bitset to all empty.
        void Reset();

        //! Gets the current value of the specified bit.
        //! @param index index of the bit to retrieve the value of
        //! @return boolean true if the bit is set, false otherwise
        bool GetBit(uint32_t index) const;

        //! Sets the specified bit to the provided value.
        //! @param index index of the bit to set
        //! @param value value to set the bit to
        void SetBit(uint32_t index, bool value);

        //! Returns the number of accessible bits in this ringbuffer bitset.
        //! @return the number of accessible bits in this ringbuffer bitset
        uint32_t GetValidBitCount() const;

        //! Pushes back the specified number of bits padding with zero bits.
        //! @param numBits the number of bits to push into the bitset
        void PushBackBits(uint32_t numBits);

        //! Retrieves a single element from the ringbuffer bitset.
        //! @param elementIndex the index of the ringbuffer element to retrieve
        //! @return the requested bitset element
        const BitsetChunk& GetBitsetElement(uint32_t elementIndex) const;

        //! Returns the number of bits that are unused in the head element.
        //! @return the number of bits that are unused in the head element
        uint32_t GetUnusedHeadBits() const;

    private:

        //! Converts the provided absolute index into a internal ring buffer element index.
        //! @param absoluteIndex the absolute index value to convert to a ring-buffer index
        //! @return the converted ring-buffer index
        uint32_t GetChunkIndexHelper(uint32_t absoluteIndex) const;

        //! Increments the internal head index.
        void IncrementHead();

        uint32_t m_headElementOffset = 0; //< What index is the first element in our ring buffer
        uint32_t m_unusedHeadBits = NumBitsetChunkedBits; //< How many bits are unused in the head RingbufferContainer element
        RingbufferContainer m_container;
    };

    //! Helper method for extracting a bit from a specific element.
    //! @param element  the specific element to get the bit from
    //! @param bitIndex the index of the bit to retrieve
    //! @return boolean true if the bit is set, false otherwise
    template <typename TYPE>
    bool GetBitHelper(TYPE element, uint32_t bitIndex);

    //! A helper method for setting bits.
    //! @param element  the specific element to set the bit within
    //! @param bitIndex the index of the bit to set
    //! @param value    the value to set the bit to
    template <typename TYPE>
    void SetBitHelper(TYPE &element, uint32_t bitIndex, bool value);
}

#include <AzNetworking/DataStructures/RingBufferBitset.inl>
