/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Serialize/Buffer.h>

namespace GridMate
{
    //-----------------------------------------------------------------------------
    // ReadBuffer
    //-----------------------------------------------------------------------------
    ReadBuffer::ReadBuffer(EndianType endianType, const char* data, PackedSize size, PackedSize offset)
        : m_data(data)
        , m_startOffset(offset)
        , m_read()
        , m_length(size)
        , m_overrun(false)
        , m_endianType(endianType)
    { }
    //-----------------------------------------------------------------------------
    ReadBuffer::ReadBuffer(const ReadBuffer& rhs)
        : m_data(rhs.m_data)
        , m_startOffset(rhs.m_startOffset)
        , m_read(rhs.m_read)
        , m_length(rhs.m_length)
        , m_overrun(rhs.m_overrun)
        , m_endianType(rhs.m_endianType)
    {
    }
    //-----------------------------------------------------------------------------
    bool ReadBuffer::ReadRaw(void* data, PackedSize dataSize)
    {
        if (m_overrun || dataSize > (m_length - m_read))
        {
            m_overrun = true;
            return false;
        }

        if ((m_startOffset.GetAdditionalBits() + m_read.GetAdditionalBits()) % CHAR_BIT == 0)
        {
            // The easy case - no bit shifting needed to read stored data.
            memcpy(data, GetRawBytePtr(), dataSize.GetSizeInBytesRoundUp());
            m_read += dataSize;
        }
        else
        {
            // The hard case - bit shift each byte of the stored data.
            for (AZ::u64 i = 0; i < dataSize; ++i, m_read.IncrementBytes(1))
            {
                /*
                * Given the first byte is D1[1234 5678] and next byte is D2[1234 5678] and the current bit offset is 3,
                * and our byte of data to be read is A[1234 5678] then:
                *
                *                     A[  45678]              A[123  ]
                *                         |||||                 |||
                * then we need to read D1[12345678] and D2[12345678].
                */
                AZ::u8 firstPart = GetRawByte() >> GetBitOffset();

                AZ::u8 next_byte = GetNextRawByte();
                next_byte = next_byte & ((1 << GetBitOffset()) - 1);

                AZ::u8 lastPart = next_byte << (CHAR_BIT - GetBitOffset());

                AZ::u8 combinedValue = (firstPart | lastPart);
                AZ::u8* outLocation = reinterpret_cast<AZ::u8*>(data) + i;
                memcpy(outLocation, &combinedValue, 1);
            }
        }

        if (dataSize.GetAdditionalBits() > 0)
        {
            // initialize the left over bits with zeroes
            AZ::u8* lastByte = reinterpret_cast<AZ::u8*>(data) + dataSize.GetBytes();
            *lastByte &= (1U << dataSize.GetAdditionalBits()) - 1U;
        }

        return true;
    }
    //-----------------------------------------------------------------------------
    bool ReadBuffer::ReadRawBit(bool& data)
    {
        AZ_Assert(m_read < m_length, "Attempting to read beyond buffer length!");
        if (m_overrun || 0 >= Left())
        {
            m_overrun = true;
            return false;
        }

        data = !!((GetRawByte() & (1 << GetBitOffset())) >> GetBitOffset());

        m_read.IncrementBit();

        return true;
    }
    //-----------------------------------------------------------------------------
    bool ReadBuffer::Skip(PackedSize skipSize)
    {
        AZ_Assert((m_read + skipSize) <= m_length, "Attempting to skip beyond buffer length!");
        if (m_overrun || skipSize > Left())
        {
            m_overrun = true;
            return false;
        }

        m_read += skipSize;
        return true;
    }

    //-----------------------------------------------------------------------------
    // WriteBuffer
    //-----------------------------------------------------------------------------
    WriteBuffer::WriteBuffer(EndianType endianType)
        : m_data(nullptr)
        , m_size()
        , m_capacity(0)
        , m_endianType(endianType)
    { }
    //-----------------------------------------------------------------------------
    WriteBuffer::~WriteBuffer()
    {
        AZ_Assert(m_data == nullptr, "Derived class you call Destroy prior to destruction!");
    }
    //-----------------------------------------------------------------------------
    void WriteBuffer::WriteRaw(const void* data, PackedSize dataSize)
    {
        if (dataSize > 0)
        {
            // check remaining capacity
            if (m_capacity - m_size.GetSizeInBytesRoundUp() < dataSize)
            {
                Grow(dataSize.GetSizeInBytesRoundUp());
            }

            if (m_size.GetAdditionalBits() == 0)
            {
                // The easy case - no shifting of each byte is necessary
                memcpy(m_data + m_size.GetBytes(), data, dataSize.GetSizeInBytesRoundUp());
                m_size += dataSize;
            }
            else
            {
                // The hard case - we need to shift each byte before writing it
                for (AZ::u64 i = 0; i < dataSize.GetBytes(); ++i, m_size.IncrementBytes(1))
                {
                    /*
                    * Given the input byte is A[1234 5678], the current bit offset is (for example) 3,
                    * meaning D1[---- -XXX] is already written in the byte.
                    * current byte D1[1234 5678] and next byte D2[1234 5678] then:
                    *
                    *                 A[  45678]          A[123  ]
                    *                     |||||             |||
                    * we need to write D1[12345678] D2[12345678].
                    */
                    AZ::u8 inputByte = *(reinterpret_cast<const AZ::u8*>(data) + i);

                    AZ::u8 firstPart = GetRawByte() | (inputByte << GetBitOffset()); // Do take into account the current value in the byte.
                    AZ::u8 lastPart = (inputByte >> (CHAR_BIT - GetBitOffset())); // Grab the remaining most significant digits

                    memcpy(GetRawBytePtr(), &firstPart, 1);
                    memcpy(GetRawBytePtr(1), &lastPart, 1);
                }

                if(dataSize.GetAdditionalBits() > 0)
                {
                    AZ::u8 inputByte = *(reinterpret_cast<const AZ::u8*>(data) + dataSize.GetBytes());

                    AZ::u8 part[2];
                    part[0] = GetRawByte() | (inputByte << GetBitOffset()); // Do take into account the current value in the byte.
                    part[1] = (inputByte >> (CHAR_BIT - GetBitOffset())); // Grab the remaining most significant digits

                    memcpy(GetRawBytePtr(), &part[0], sizeof(part) / sizeof(part[0]));

                    m_size.IncrementBits(dataSize.GetAdditionalBits());
                }
            }
        }
    }

    void WriteBuffer::WriteFromBuffer(ReadBuffer& rb, PackedSize size)
    {
        AZ_Assert(rb.Left() >= size, "Not enough available data in the input buffer!");

        for (AZStd::size_t i = 0; i < size.GetBytes(); ++i)
        {
            AZ::u8 tmp;
            rb.ReadRaw(&tmp, 1);
            WriteRaw(&tmp, 1);
        }

        if (size.GetAdditionalBits() > 0)
        {
            auto bitsLeft = size.GetAdditionalBits();
            for (AZStd::size_t i = 0; i < bitsLeft; ++i)
            {
                bool bit = false;
                rb.ReadRawBit(bit);
                WriteRawBit(bit);
            }
        }
    }

    //-----------------------------------------------------------------------------
    void WriteBuffer::WriteRawBit(bool data)
    {
        if (m_capacity <= m_size)
        {
            Grow(1);
        }

        // zero out bits we are about to write
        *GetRawBytePtr() = (GetRawByte() & ((1 << GetBitOffset()) - 1));

        unsigned char beforeValue = GetRawByte();
        unsigned char newValue = data ?
            (beforeValue | (1 << GetBitOffset())) :
            (beforeValue & (((1 << GetBitOffset()) - 1)));

        *GetRawBytePtr() = newValue;

        m_size.IncrementBit();
    }
    //-----------------------------------------------------------------------------
    void WriteBuffer::Destroy()
    {
        if (m_data)
        {
            DeAllocate(m_data, m_capacity.GetBytes(), 1);
            m_data = nullptr;
            m_capacity = 0;
        }
    }
    //-----------------------------------------------------------------------------
    void WriteBuffer::Grow(size_t growSize)
    {
        size_t newCapacity = m_size.GetSizeInBytesRoundUp() + growSize;
        newCapacity += newCapacity / 2; // preallocate 50% more as the AZStd::vector does.
        char* newData = reinterpret_cast<char*>(Allocate(newCapacity, 1));
        if (m_data)
        {
            memcpy(newData, m_data, m_size.GetSizeInBytesRoundUp());
            DeAllocate(m_data, m_capacity.GetBytes(), 1);
        }
        m_data = newData;
        m_capacity = newCapacity;
    }

    //-----------------------------------------------------------------------------
    // WriteBufferDynamic
    //-----------------------------------------------------------------------------
    WriteBufferDynamic::WriteBufferDynamic(EndianType endianType, size_t initialCapacity)
        : WriteBuffer(endianType)
    {
        Init(initialCapacity);
    }
    //-----------------------------------------------------------------------------
    WriteBufferDynamic::WriteBufferDynamic(const WriteBufferDynamic& rhs)
        : WriteBuffer(rhs.GetEndianType())
    {
        Init(rhs.Size());
        WriteRaw(rhs.Get(), rhs.Size());
    }
    //-----------------------------------------------------------------------------
    WriteBufferDynamic::WriteBufferDynamic(const BaseType& rhs)
        : WriteBuffer(rhs.GetEndianType())
    {
        Init(rhs.Size());
        WriteRaw(rhs.Get(), rhs.Size());
    }
    //-----------------------------------------------------------------------------
    WriteBufferDynamic::WriteBufferDynamic(WriteBufferDynamic&& rhs)
        : WriteBuffer(rhs.GetEndianType())
    {
        Swap(AZStd::forward<WriteBufferDynamic>(rhs));
    }
    //-----------------------------------------------------------------------------
    WriteBufferDynamic& WriteBufferDynamic::operator=(WriteBufferDynamic&& rhs)
    {
        Swap(AZStd::forward<WriteBufferDynamic>(rhs));
        return *this;
    }
    //-----------------------------------------------------------------------------
    void WriteBufferDynamic::Swap(WriteBufferDynamic&& rhs)
    {
        m_data = rhs.m_data;
        m_size = rhs.m_size;
        m_capacity = rhs.m_capacity;
        m_endianType = rhs.m_endianType;

        rhs.m_data = nullptr;
        rhs.m_size = 0;
        rhs.m_capacity = 0;
        rhs.m_endianType = EndianType::IgnoreEndian;
    }
    //-----------------------------------------------------------------------------
    WriteBufferDynamic::~WriteBufferDynamic()
    {
        Destroy();
    }
    //-----------------------------------------------------------------------------
    WriteBufferDynamic& WriteBufferDynamic::operator+=(const BaseType& rhs)
    {
        WriteRaw(rhs.Get(), rhs.Size());
        return *this;
    }
    //-----------------------------------------------------------------------------
    WriteBufferDynamic WriteBufferDynamic::operator+(const BaseType& rhs)
    {
        WriteBufferDynamic wb(rhs.GetEndianType());
        wb += *this;
        wb += rhs;
        return wb;
    }
    //-----------------------------------------------------------------------------
    void WriteBufferDynamic::Init(PackedSize capacity)
    {
        AZ_Assert(m_capacity == 0, "This WriteBufferDynamic has already been initialized!");
        if (capacity > 0)
        {
            m_data = reinterpret_cast<char*>(Allocate(capacity.GetSizeInBytesRoundUp(), 1));
            m_capacity = capacity;
        }
    }
    //-----------------------------------------------------------------------------
    void* WriteBufferDynamic::Allocate(size_t byteSize, size_t alignment)
    {
        return azmalloc(byteSize, alignment, GridMateAllocatorMP, "WriteBuffer");
    }
    //-----------------------------------------------------------------------------
    void WriteBufferDynamic::DeAllocate(void* ptr, size_t byteSize, size_t alignment)
    {
        azfree(ptr, GridMateAllocatorMP, byteSize, alignment);
    }

    //-----------------------------------------------------------------------------
    // WriteBufferStaticInPlace
    //-----------------------------------------------------------------------------
    WriteBufferStaticInPlace::WriteBufferStaticInPlace(EndianType endianType, void* data, size_t capacity)
        : WriteBuffer(endianType)
    {
        m_data = reinterpret_cast<char*>(data);
        m_capacity = capacity;
    }
    //-----------------------------------------------------------------------------
    WriteBufferStaticInPlace::~WriteBufferStaticInPlace()
    {
        m_data = nullptr;
    }
    //-----------------------------------------------------------------------------
    WriteBufferStaticInPlace& WriteBufferStaticInPlace::operator+=(const WriteBuffer& rhs)
    {
        WriteRaw(rhs.Get(), rhs.Size());
        return *this;
    }
    //-----------------------------------------------------------------------------
    void* WriteBufferStaticInPlace::Allocate(size_t byteSize, size_t alignment)
    {
        (void) byteSize;
        (void) alignment;
        AZ_Assert(false, "Requesting %d memory for WriteBufferStaticInPlace is invalid", byteSize);
        return nullptr;
    }
    //-----------------------------------------------------------------------------
    void WriteBufferStaticInPlace::DeAllocate(void* ptr, size_t byteSize, size_t alignment)
    {
        (void) ptr;
        (void) byteSize;
        (void) alignment;
        AZ_Assert(false, "No need to call deallocate for this buffer!");
    }
    //-----------------------------------------------------------------------------
}
