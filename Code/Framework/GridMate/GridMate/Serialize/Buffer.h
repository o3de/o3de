/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_UTILS_BUFFER
#define GM_UTILS_BUFFER

#include <AzCore/std/utils.h>

#include <GridMate/Types.h>
#include <GridMate/Serialize/MarshalerTypes.h>
#include <GridMate/Serialize/PackedSize.h>

namespace GridMate
{
    /**
    * Generic read buffer.
    */
    class ReadBuffer
    {
        const char* m_data;

        PackedSize  m_startOffset; // A buffer might begin on some non-zero bit offset from the first byte
        PackedSize  m_read;        // where the current marker is, in other words how much was read so far
        PackedSize  m_length;      // the total length of the buffer

        bool m_overrun;
        EndianType m_endianType;

        // Note, do take the current bits offset as well
        AZ_FORCE_INLINE const char* GetRawBytePtr(AZStd::size_t offsetInBytes = 0) const
        {
            return m_data + (m_startOffset + m_read).GetBytes() + offsetInBytes;
        }

        AZ_FORCE_INLINE AZ::u8 GetRawByte() const
        {
            return static_cast<AZ::u8>(*GetRawBytePtr());
        }

        AZ_FORCE_INLINE AZ::u8 GetNextRawByte() const
        {
            return static_cast<AZ::u8>(*GetRawBytePtr(1));
        }

        AZ_FORCE_INLINE AZ::u8 GetBitOffset() const
        {
            return (m_startOffset.GetAdditionalBits() + m_read.GetAdditionalBits()) % CHAR_BIT;
        }

    public:
        /**
        * @endianType   Endian type of the buffer
        * @data         the starting byte of the buffer, note the actual start might begin in the middle of the byte somewhere
        * @size         the length of the buffer, note the buffer might be in bytes and a non-8 number of bits
        * @offset       the start of the buffer, usually, it is either zero bytes from @data or some non-zero number of bits from @data
        */
        ReadBuffer(EndianType endianType, const char* data = nullptr, PackedSize size = 0, PackedSize offset = 0);
        ReadBuffer(const ReadBuffer& rhs);

        // Is empty or at the end of the buffer, ignoring trailing bits if any
        AZ_FORCE_INLINE bool IsEmptyIgnoreTrailingBits() const
        {
            return m_read.GetSizeInBytesRoundUp() == m_length.GetSizeInBytesRoundUp();
        }

        // Is empty or at the end of the buffer
        AZ_FORCE_INLINE bool IsEmpty() const
        {
            return m_read == m_length;
        }

        AZ_FORCE_INLINE PackedSize Left() const { return (m_length - m_read); }
        AZ_FORCE_INLINE PackedSize Read() const { return m_read; }
        AZ_FORCE_INLINE PackedSize Size() const { return m_length; }
        AZ_FORCE_INLINE const char* Get() const { return m_data; }
        AZ_FORCE_INLINE const char* GetCurrent() const { return GetRawBytePtr(); }

        AZ_FORCE_INLINE bool IsOverrun() const { return m_overrun; }
        AZ_FORCE_INLINE bool IsValid() const { return (m_data != nullptr) && (m_length >= m_read); }
        AZ_FORCE_INLINE EndianType GetEndianType() const { return m_endianType; }
        AZ_FORCE_INLINE void SetEndianType(EndianType endianType) { m_endianType = endianType; }

        template<typename Type>
        AZ_FORCE_INLINE bool Read(Type& pod)
        {
            return Read(pod, Marshaler<Type>());
        }

        template<typename Type, typename MarshalerType>
        AZ_FORCE_INLINE bool Read(Type& pod, MarshalerType&& marshaler)
        {
            marshaler.Unmarshal(pod, *this);
            return !m_overrun;
        }

        bool ReadRaw(void* data, PackedSize dataSize);
        bool ReadRawBit(bool& data);

        bool Skip(PackedSize bytes);

        ReadBuffer ReadInnerBuffer(PackedSize size)
        {
            if (!IsValid() || Left() < size)
            {
                AZ_Assert(false, "Reading past the end of the buffer");
                return ReadBuffer(EndianType::IgnoreEndian);
            }

            ReadBuffer buffer(m_endianType, m_data, size + m_read, m_startOffset);
            buffer.Skip(m_read);

            Skip(size);
            return buffer;
        }
    };

    /**
    * Base class for write buffers. It's a base class because we implement different memory allocators as children.
    *
    * Assuming that write buffers always start on the byte boundary, i.e. they aren't asked to write with a starting bit offset.
    */
    class WriteBuffer
    {
    public:
        /**
            Marker is used to safely write to a existing section of WriteBuffer.
            Call WriteBuffer::InsertMarker() to retrieve a valid marker.

            For a Marker to write data, the Marshaler must be a fixed size. This is designated by the
            presence of a size_t MarshalSize member in the Marshaler itself.
        **/
        template<typename Type, typename MarshalerType = Marshaler<Type> >
        class Marker
        {
            friend class WriteBuffer;
            typedef Type DataType;

            PackedSize m_offset; ///< Offset of the marker data into the buffer stream.
            WriteBuffer* m_buffer; ///< Pointer to the buffer stream.
            MarshalerType m_marshaler;

            AZ_FORCE_INLINE Marker(PackedSize offset, WriteBuffer* buffer)
                : m_offset(offset)
                , m_buffer(buffer)
            {
                static_assert(IsFixedMarshaler<MarshalerType>::Value, "Markers require a fixed size marshaler to write data");
            }
        public:
            ///< Default constructor (invalid marker)
            AZ_FORCE_INLINE Marker()
                : m_buffer(nullptr)
            {
                static_assert(IsFixedMarshaler<MarshalerType>::Value, "Markers require a fixed size marshaler to write data");
            }

            AZ_FORCE_INLINE bool IsValid() const { return m_buffer != nullptr; }
            AZ_FORCE_INLINE PackedSize GetOffsetAfterMarker() const { return m_offset + MarshalerType::MarshalSize; }
            AZ_FORCE_INLINE void SetData(const Type& pod) { *this = pod; }

            Marker& operator=(const Type& value);

            AZ_FORCE_INLINE operator Type()
            {
                static_assert(IsFixedMarshaler<MarshalerType>::Value, "Markers require a fixed size marshaler to write data");

                ReadBuffer buffer(m_buffer->GetEndianType(), m_buffer->m_data + m_offset.GetBytes(), MarshalerType::MarshalSize);
                Type value;
                m_marshaler.Unmarshal(value, buffer);
                return value;
            }
        };

        explicit WriteBuffer(EndianType endianType);
        virtual ~WriteBuffer();

        AZ_FORCE_INLINE const char* Get() const { return m_data; }
        AZ_FORCE_INLINE void Clear() { m_size = 0; }
        AZ_FORCE_INLINE AZStd::size_t Size() const
        {
            return m_size.GetSizeInBytesRoundUp();
        }
        AZ_FORCE_INLINE PackedSize GetExactSize() const
        {
            return m_size;
        }

        AZ_FORCE_INLINE EndianType GetEndianType() const { return m_endianType; }
        AZ_FORCE_INLINE void SetEndianType(EndianType endianType) { m_endianType = endianType; }

        virtual void WriteRaw(const void* data, PackedSize size);
        virtual void WriteRawBit(bool data);
        virtual void WriteFromBuffer(ReadBuffer& rb, PackedSize size);

        /// Insert a marker in the stream, so you can later overwrite this value conveniently.
        template<typename Type>
        AZ_FORCE_INLINE Marker<Type, Marshaler<Type> > InsertMarker()
        {
            return InsertMarker<Type, Marshaler<Type> >();
        }

        /// Insert a marker in the stream, so you can later overwrite this value conveniently.
        template<typename Type, typename MarshalerType>
        AZ_FORCE_INLINE Marker<Type, MarshalerType> InsertMarker()
        {
            static_assert(IsFixedMarshaler<MarshalerType>::Value, "Markers require a fixed size marshaler to write data");

            auto offset = m_size;
            if (m_capacity - m_size < PackedSize(MarshalerType::MarshalSize))
            {
                Grow(MarshalerType::MarshalSize);
            }
            m_size.IncrementBytes(MarshalerType::MarshalSize);
            return Marker<Type, MarshalerType>(offset, this);
        }

        /// Insert a marker in the stream, and write in an initial value.
        template<typename Type>
        AZ_FORCE_INLINE Marker<Type, Marshaler<Type> > InsertMarker(const Type& val)
        {
            return InsertMarker<Type, Marshaler<Type> >(val);
        }

        /// Insert a marker in the stream, and write in an initial value.
        template<typename Type, typename MarshalerType>
        AZ_FORCE_INLINE Marker<Type, MarshalerType> InsertMarker(const Type& val)
        {
            Marker<Type, MarshalerType> marker = InsertMarker<Type, MarshalerType>();
            marker.SetData(val);
            return marker;
        }

        /// Writes the data that starts at the beginning of a byte.
        template<typename Type>
        AZ_FORCE_INLINE void WriteWithByteAlignment(const Type& pod)
        {
            FillUpByte();
            Write(pod);
        }

        /// Write data to the stream. Data must be copy constructible (to perform endian swap).
        template<typename Type>
        AZ_FORCE_INLINE void Write(const Type& pod)
        {
            Write(pod, Marshaler<Type>());
        }

        /// Write data to the stream. Data must be copy constructible (to perform endian swap).
        template<typename Type, typename MarshalerType>
        AZ_FORCE_INLINE void Write(const Type& pod, MarshalerType&& marshaler)
        {
            marshaler.Marshal(*this, pod);
        }

    protected:
        //////////////////////////////////////////////////////////////////////////
        // Allocation hooks
        // Required to implement
        virtual void* Allocate(size_t byteSize, size_t alignment) = 0;
        virtual void DeAllocate(void* ptr, size_t byteSize, size_t alignment) = 0;
        //////////////////////////////////////////////////////////////////////////

        void Destroy();
        void Grow(size_t growSize);

        char* m_data;

        PackedSize m_size;

        PackedSize m_capacity;
        EndianType m_endianType;

    private:
        WriteBuffer(const WriteBuffer&) = delete;

        // Note, do take the current bits offset as well
        AZ_FORCE_INLINE char* GetRawBytePtr(AZStd::size_t offsetInBytes = 0) const
        {
            return m_data + m_size.GetBytes() + offsetInBytes;
        }

        AZ_FORCE_INLINE AZ::u8 GetRawByte() const
        {
            return static_cast<AZ::u8>(*GetRawBytePtr());
        }

        AZ_FORCE_INLINE AZ::u8 GetNextRawByte() const
        {
            return static_cast<AZ::u8>(*GetRawBytePtr(1));
        }

        AZ_FORCE_INLINE AZ::u8 GetBitOffset() const
        {
            return m_size.GetAdditionalBits();
        }

        /*
        * Skip to the next byte boundary.
        */
        AZ_FORCE_INLINE void FillUpByte()
        {
            if (m_size.GetAdditionalBits() > 0)
            {
                m_size = PackedSize(m_size.GetBytes() + 1);
            }
        }
    };

    /**
    * Write buffer using dynamic allocations, flexible
    */
    class WriteBufferDynamic
        : public WriteBuffer
    {
        typedef WriteBuffer BaseType;
    public:

        WriteBufferDynamic(EndianType endianType, size_t initialCapacity = 2048);
        WriteBufferDynamic(const WriteBufferDynamic& rhs);
        WriteBufferDynamic(const BaseType& rhs);

        WriteBufferDynamic(WriteBufferDynamic&& rhs);
        WriteBufferDynamic& operator=(WriteBufferDynamic&& rhs);

        virtual ~WriteBufferDynamic();

        void Init(PackedSize capacity);

        void Swap(WriteBufferDynamic&& rhs);

        WriteBufferDynamic& operator+=(const BaseType& rhs);
        WriteBufferDynamic operator+(const BaseType& rhs);

    protected:
        void* Allocate(size_t byteSize, size_t alignment) override;
        void DeAllocate(void* ptr, size_t byteSize, size_t alignment) override;
    };

    /**
    * Write buffer with static internal buffer crafted for write buffers
    * on the stack.
    */
    template<AZStd::size_t BufferSize = 2048>
    class WriteBufferStatic
        : public WriteBuffer
    {
    public:
        WriteBufferStatic(EndianType endianType)
            : WriteBuffer(endianType)
        {
            m_data = reinterpret_cast<char*>(&m_storage);
            m_capacity = BufferSize;
        }

        WriteBufferStatic(const WriteBufferStatic& rhs)
            : WriteBuffer(rhs.GetEndianType())
        {
            m_data = reinterpret_cast<char*>(&m_storage);
            m_capacity = BufferSize;
            WriteRaw(rhs.Get(), rhs.Size());
        }

        WriteBufferStatic(const WriteBuffer& rhs)
            : WriteBuffer(rhs.GetEndianType())
        {
            m_data = reinterpret_cast<char*>(&m_storage);
            m_capacity = BufferSize;
            WriteRaw(rhs.Get(), rhs.Size());
        }

        virtual ~WriteBufferStatic()
        {
            m_data = nullptr;
        }

        WriteBufferStatic& operator+=(const WriteBuffer& rhs) { WriteRaw(rhs.Get(), rhs.Size()); return *this; }

    protected:
        void* Allocate(size_t byteSize, size_t alignment) override
        {
            (void) byteSize;
            (void) alignment;
            AZ_Assert(false, "We requested more memory %d that the static buffer holds %d!", byteSize, BufferSize);
            return nullptr;
        }
        void DeAllocate(void* ptr, size_t byteSize, size_t alignment) override
        {
            (void) ptr;
            (void) byteSize;
            (void) alignment;
            AZ_Assert(false, "No need to call deallocate for this buffer!");
        }

        typename AZStd::aligned_storage<BufferSize, 8>::type m_storage;
    };

    /**
    * Write buffer in place memory location
    */
    class WriteBufferStaticInPlace
        : public WriteBuffer
    {
    public:
        WriteBufferStaticInPlace(EndianType endianType, void* data, size_t capacity);
        WriteBufferStaticInPlace(const WriteBufferStaticInPlace& rhs);
        virtual ~WriteBufferStaticInPlace();

        WriteBufferStaticInPlace& operator+=(const WriteBuffer& rhs);

    protected:
        void* Allocate(size_t byteSize, size_t alignment) override;
        void DeAllocate(void* ptr, size_t byteSize, size_t alignment) override;
    };

    template<typename T, typename MarshalerType>
    WriteBuffer::Marker<T, MarshalerType>& WriteBuffer::Marker<T, MarshalerType>::operator=(const T& value)
    {
        static_assert(IsFixedMarshaler<MarshalerType>::Value, "Markers require a fixed size marshaler to write data");

        WriteBufferStaticInPlace buffer(m_buffer->GetEndianType(), m_buffer->m_data + m_offset.GetBytes(), MarshalerType::MarshalSize);
        m_marshaler.Marshal(buffer, value);

        AZ_Assert(buffer.Size() == MarshalerType::MarshalSize, "Must have written the correct amount to the buffer");

        return *this;
    }
}

#endif // GM_UTILS_BUFFER
