/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AzNetworking
{
    template <AZStd::size_t SIZE>
    inline constexpr AZStd::size_t ByteBuffer<SIZE>::GetCapacity()
    {
        return SIZE;
    }

    template <AZStd::size_t SIZE>
    inline AZStd::size_t ByteBuffer<SIZE>::GetSize() const
    {
        return m_buffer.size();
    }

    template <AZStd::size_t SIZE>
    inline bool ByteBuffer<SIZE>::Resize(AZStd::size_t newSize)
    {
        if (newSize > GetCapacity())
        {
            return false;
        }
        m_buffer.resize_no_construct(newSize);
        return true;
    }

    template <AZStd::size_t SIZE>
    inline const uint8_t* ByteBuffer<SIZE>::GetBuffer() const
    {
        return m_buffer.data();
    }

    template <AZStd::size_t SIZE>
    inline uint8_t* ByteBuffer<SIZE>::GetBuffer()
    {
        return m_buffer.data();
    }

    template <AZStd::size_t SIZE>
    inline const uint8_t* ByteBuffer<SIZE>::GetBufferEnd() const
    {
        return GetBuffer() + GetSize();
    }

    template <AZStd::size_t SIZE>
    inline uint8_t* ByteBuffer<SIZE>::GetBufferEnd()
    {
        return GetBuffer() + GetSize();
    }

    template <AZStd::size_t SIZE>
    inline bool ByteBuffer<SIZE>::CopyValues(const uint8_t* buffer, AZStd::size_t bufferSize)
    {
        if (Resize(bufferSize))
        {
            memcpy(m_buffer.data(), buffer, bufferSize);
            return true;
        }
        return false;
    }

    template <AZStd::size_t SIZE>
    bool ByteBuffer<SIZE>::IsSame(const uint8_t* buffer, AZStd::size_t bufferSize) const
    {
        return (m_buffer.size() == bufferSize)
            && (memcmp(m_buffer.data(), buffer, bufferSize) == 0);
    }

    template <AZStd::size_t SIZE>
    inline bool ByteBuffer<SIZE>::operator==(const ByteBuffer& rhs) const
    {
        return (m_buffer.size() == rhs.m_buffer.size())
            && (memcmp(m_buffer.data(), rhs.m_buffer.data(), m_buffer.size()) == 0);
    }

    template <AZStd::size_t SIZE>
    inline bool ByteBuffer<SIZE>::operator!=(const ByteBuffer& rhs) const
    {
        return (m_buffer.size() != rhs.m_buffer.size())
            || (memcmp(m_buffer.data(), rhs.m_buffer.data(), m_buffer.size()) != 0);
    }

    template <AZStd::size_t SIZE>
    inline bool ByteBuffer<SIZE>::Serialize(ISerializer& serializer)
    {
        static constexpr AZStd::size_t RequiredBytes = AZ::RequiredBytesForValue<SIZE>();
        using SizeType = typename AZ::SizeType<RequiredBytes, false>::Type;

        // Important since we need to know the original and new sizes for vector resize
        SizeType size = static_cast<SizeType>(GetSize());
        uint32_t outSize = size;

        return serializer.Serialize(size, "Size")
            && Resize(size)
            && serializer.SerializeBytes(m_buffer.data(), static_cast<uint32_t>(GetCapacity()), false, outSize, "Buffer")
            && (outSize == size);
    }
}
