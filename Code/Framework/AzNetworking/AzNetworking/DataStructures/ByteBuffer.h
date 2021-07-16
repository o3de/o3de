/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/fixed_vector.h>
#include <AzNetworking/Serialization/ISerializer.h>

namespace AzNetworking
{
    //! @class ByteBuffer
    //! @brief serializeable byte buffer with efficient serialization.
    template <AZStd::size_t SIZE>
    class ByteBuffer
    {
    public:

        ByteBuffer() = default;
        ~ByteBuffer() = default;

        //! Returns the maximum number of elements this vector can reserve for use.
        //! @return the maximum number of elements this vector can reserve for use
        static constexpr AZStd::size_t GetCapacity();

        //! Returns the number of elements reserved for usage in this vector.
        //! @return the number of elements reserved for usage in this vector
        AZStd::size_t GetSize() const;

        //! Resizes the vector to the requested number of elements, does not initialize new elements.
        //! @param newSize the number of elements to size the vector to
        //! @return boolean true on success
        bool Resize(AZStd::size_t newSize);

        //! Const raw buffer access.
        //! @return const pointer to the array's internal memory buffer
        const uint8_t* GetBuffer() const;

        //! Non-const raw buffer access.
        //! @return non-const pointer to the array's internal memory buffer
        uint8_t* GetBuffer();

        //! Const raw end-of-buffer access.
        //! @return const pointer to the first unused byte in the array's internal memory buffer
        const uint8_t* GetBufferEnd() const;

        //! Raw end-of-buffer access.
        //! @return non-const pointer to the first unused byte in the array's internal memory buffer
        uint8_t* GetBufferEnd();

        //! Overwrites the data in this ByteBuffer with the data in the provided buffer.
        //! @param buffer     pointer to the buffer data to copy
        //! @param bufferSize the number of bytes in the buffer to copy
        //! @return boolean true on success, false for failure
        bool CopyValues(const uint8_t* buffer, AZStd::size_t bufferSize);

        //! Tests for equality to a raw byte buffer.
        //! @param buffer     pointer to the buffer data to copy
        //! @param bufferSize the number of bytes in the buffer to copy
        //! @return boolean true if rhs and buffer are identical, false otherwise
        bool IsSame(const uint8_t* buffer, AZStd::size_t bufferSize) const;

        //! Equality operator.
        //! @param rhs the byte buffer to compare against
        //! @return boolean true if rhs and lhs are identical, false otherwise
        bool operator==(const ByteBuffer& rhs) const;

        //! Inequality operator.
        //! @param rhs the byte buffer to compare against
        //! @return boolean true if rhs and lhs are not identical, false otherwise
        bool operator!=(const ByteBuffer& rhs) const;

        //! Base serialize method for all serializable structures or classes to implement.
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(ISerializer& serializer);

    private:

        AZStd::fixed_vector<uint8_t, SIZE> m_buffer;
    };

    // The maximum allowable packet size
    static constexpr uint32_t MaxPacketSize = 16384;
    static_assert((MaxPacketSize & (MaxPacketSize - 1)) == 0, "Maximum packet size is not aligned/a power of 2");
    using TcpPacketEncodingBuffer = ByteBuffer<MaxPacketSize>;

    // This is the max possible MTU for UDP packets, the actual transmitted packet size depends on connection MTU
    static constexpr uint32_t MaxUdpTransmissionUnit = 1024;
    static_assert(MaxPacketSize > MaxUdpTransmissionUnit, "UDP MTU must be smaller than maximum packet size");
    static_assert((MaxUdpTransmissionUnit & (MaxUdpTransmissionUnit - 1)) == 0, "Maximum UPD MTU is not aligned/a power of 2");
    using UdpPacketEncodingBuffer = ByteBuffer<MaxPacketSize>;

    // This is a general packet encoding buffer that will fit within the constraints of either the TCP or UDP transport layers
    using PacketEncodingBuffer = ByteBuffer<MaxPacketSize>;

    // This is a packet encoding buffer specifically for MTU fragmentation
    using ChunkBuffer = ByteBuffer<MaxUdpTransmissionUnit>;
}

#include <AzNetworking/DataStructures/ByteBuffer.inl>
