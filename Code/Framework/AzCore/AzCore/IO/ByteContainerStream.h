/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/typetraits/is_const.h>
#include <AzCore/std/typetraits/has_member_function.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ::IO
{
    /*
    * ByteContainerStream
    * A stream that wraps around a sequence container
    * Currently only AZStd::vector and AZStd::fixed_vector are supported as
    * we use direct memcpy, so underlying storage has to be guaranteed continuous.
    * It can be generalized if necessary, but for now this should do.
    */
    template<bool V>
    struct WritableStreamType  {};
    template<>
    struct WritableStreamType<true>
    {
        typedef int type;
    };

    template<typename ContainerType>
    class ByteContainerStream
        : public GenericStream
    {
    public:
        ByteContainerStream() = default;
        ByteContainerStream(ContainerType* buffer, size_t maxGrowSize = 5 * 1024 * 1024);
        ~ByteContainerStream() override {}

        bool Open(ContainerType& buffer, size_t maxGrowSize = 5 * 1024 * 1024);
        bool        IsOpen() const override                              { return m_opened; }
        bool        CanSeek() const override                             { return true; }
        bool        CanRead() const override                    { return true; }
        bool        CanWrite() const override                   { return true; }
        SizeType GetCurPos() const override
        {
            if (!IsOpen())
            {
                AZ_Error("ByteContainerStream", false, "Stream is closed, cannot query current position");
                return 0;
            }
            return m_pos;
        }
        SizeType GetLength() const override
        {
            if (!IsOpen())
            {
                AZ_Error("ByteContainerStream", false, "Cannot query Length on a close stream");
                return 0;
            }
            return m_buffer->size();
        }
        void        Seek(OffsetType bytes, SeekMode mode) override;
        SizeType    Read(SizeType bytes, void* oBuffer) override;
        SizeType    Write(SizeType bytes, const void* iBuffer) override;
        SizeType    WriteFromStream(SizeType bytes, GenericStream* inputStream) override;
        template<typename T>
        inline SizeType Write(const T* iBuffer)
        {
            return Write(sizeof(T), iBuffer);
        }

        // Truncate the stream to the current position.
        void                Truncate();

        ContainerType* GetData() const
        {
            return IsOpen() ? m_buffer : nullptr;
        }

        bool ReOpen() override
        {
            AZ_Warning("ByteContainerStream", IsOpen(), "The stream is already open."
                " This operation will reset the seek offset");
            m_pos = 0;
            m_opened = m_buffer != nullptr;
            return m_opened;
        }
        void Close() override
        {
            m_opened = false;
            m_pos = 0;
        }

    protected:
        SizeType PrepareToWrite(SizeType bytes);
        template<typename T>
        SizeType Write(SizeType bytes, const void* iBuffer, typename T::type);
        template<typename T>
        SizeType WriteFromStream(SizeType bytes, GenericStream* inputStream, typename T::type);
        template<typename T>
        SizeType Write(SizeType, const void*, unsigned int);
        template<typename T>
        SizeType WriteFromStream(SizeType, GenericStream*, unsigned int);

        ContainerType*  m_buffer{};
        size_t          m_pos{};
        size_t          m_maxGrowSize{}; //< Maximum grow size to avoid the buffer growing too fast when it's working on big files
        //! Used to track if the ByteContainerStream has received a Close() call
        bool            m_opened{ false };
    };

    namespace Internal
    {
        AZ_HAS_MEMBER(ResizeNoConstruct, resize_no_construct, void, (size_t));

        // As we are about to write bytes to a container, use the fast resize (without constructing elements) if available.
        template<class ContainerType>
        inline void ResizeContainerBuffer(ContainerType* buffer, size_t newLength, const AZStd::true_type& /* HasResizeNoConstruct<ContainerType> */)
        {
            buffer->resize_no_construct(newLength);
        }

        // If resize_no_construct if not supported by the container, just call resize.
        template<class ContainerType>
        inline void ResizeContainerBuffer(ContainerType* buffer, size_t newLength, const AZStd::false_type& /* HasResizeNoConstruct<ContainerType> */)
        {
            buffer->resize(newLength);
        }

        // Set the container capacity to manage better number of allocations (especially when doing small allocation)
        AZ_HAS_MEMBER(SetCapacity, set_capacity, void, (size_t));

        template<class ContainerType>
        inline void AdjustContainerBufferCapacity(ContainerType* buffer, size_t requiredSize, size_t maxCapacityGrowSize, const AZStd::true_type& /* HasSetCapacity<ContainerType> */)
        {
            if (requiredSize > buffer->capacity())
            {
                // grow by 50%, if we can.
                size_t newCapacityGrowSize = AZ::GetClamp<size_t>(requiredSize / 2, 4, maxCapacityGrowSize);
                size_t newCapacity = requiredSize + newCapacityGrowSize;
                buffer->set_capacity(newCapacity);
            }
        }

        template<class ContainerType>
        inline void AdjustContainerBufferCapacity(ContainerType* buffer, size_t requiredSize, size_t maxCapacityGrowSize, const AZStd::false_type& /* HasSetCapacityt<ContainerType> */)
        {
            (void)buffer; (void)requiredSize; (void)maxCapacityGrowSize;
            // no set capacity support we will need to rely on resize alone
        }
    }

    template<typename ContainerType>
    ByteContainerStream<ContainerType>::ByteContainerStream(ContainerType* buffer, size_t maxGrowSize)
        : m_buffer(buffer)
        , m_pos(0)
        , m_maxGrowSize(maxGrowSize)
        , m_opened(m_buffer != nullptr)
    {
        static_assert(sizeof(typename ContainerType::value_type) == 1, "The buffer is not a container of bytes!");
        AZ_Assert(m_buffer, "You cannot make a ByteContainerStream with buffer = null!");
    }

    template<typename ContainerType>
    bool ByteContainerStream<ContainerType>::Open(ContainerType& buffer, size_t maxGrowSize)
    {
        if (IsOpen())
        {
            AZ_Error("ByteContainerStream", false, "Attempting to open new Byte Container stream"
                " while an existing stream is open. Please invoke Close(), before calling Open()");
            return false;
        }

        m_buffer = &buffer;
        m_maxGrowSize = maxGrowSize;
        m_pos = 0;
        m_opened = true;
        return m_opened;
    }

    template<typename ContainerType>
    void ByteContainerStream<ContainerType>::Seek(OffsetType bytes, SeekMode mode)
    {
        if (!IsOpen())
        {
            AZ_Error("ByteContainerStream", false, "The stream is closed, cannot seek");
            return;
        }
        m_pos = static_cast<size_t>(ComputeSeekPosition(bytes, mode));
    }

    template<typename ContainerType>
    SizeType ByteContainerStream<ContainerType>::Read(SizeType bytes, void* oBuffer)
    {
        if (!IsOpen())
        {
            AZ_Error("ByteContainerStream", false, "The stream is closed, the read call will return 0 bytes");
            return 0;
        }

        size_t len = m_buffer->size();
        size_t bytesToRead = 0;
        if (m_pos + static_cast<size_t>(bytes) < len)
        {
            bytesToRead = static_cast<size_t>(bytes);
        }
        else if (len > m_pos)
        {
            bytesToRead = len - m_pos;
        }
        if (bytesToRead > 0)
        {
            AZ_Assert(oBuffer, "Output buffer can't be null!");
            memcpy(oBuffer, m_buffer->data() + m_pos, bytesToRead);
            m_pos += bytesToRead;
        }
        return bytesToRead;
    }

    template<typename ContainerType>
    SizeType ByteContainerStream<ContainerType>::Write(SizeType bytes, const void* iBuffer)
    {
        if (!IsOpen())
        {
            AZ_Error("ByteContainerStream", false, "The stream is closed, write operattion will not occur");
            return 0;
        }

        return Write<WritableStreamType<!AZStd::is_const<ContainerType>::value> >(bytes, iBuffer, 0);
    }

    template<typename ContainerType>
    SizeType ByteContainerStream<ContainerType>::WriteFromStream(SizeType bytes, GenericStream* inputStream)
    {
        if (!IsOpen())
        {
            AZ_Error("ByteContainerStream", false, "The stream is closed, copy operation from other stream will not occur");
            return 0;
        }

        return WriteFromStream<WritableStreamType<!AZStd::is_const<ContainerType>::value> >(bytes, inputStream, 0);
    }

    template<typename ContainerType>
    SizeType ByteContainerStream<ContainerType>::PrepareToWrite(SizeType bytes)
    {
        size_t len = m_buffer->size();
        size_t requiredLen = static_cast<size_t>(m_pos + bytes);

        if (requiredLen > len)
        {
            // Make sure we grow the write buffer in a reasonable manner to avoid too many allocations
            Internal::AdjustContainerBufferCapacity(m_buffer, requiredLen, m_maxGrowSize, typename Internal::HasSetCapacity<ContainerType>::type());

            Internal::ResizeContainerBuffer(m_buffer, requiredLen, typename Internal::HasResizeNoConstruct<ContainerType>::type());
        }

        // For now, just return back the value that was handed in.  This could also be used to return 0 if error checking gets added
        // to the buffer adjustments above.
        return bytes;
    }

    template<typename ContainerType>
    template<typename T>
    SizeType ByteContainerStream<ContainerType>::Write(SizeType bytes, const void* iBuffer, typename T::type)
    {
        size_t bytesToCopy = aznumeric_cast<size_t>(PrepareToWrite(bytes));
        memcpy(m_buffer->data() + m_pos, iBuffer, bytesToCopy);
        m_pos += bytesToCopy;
        return bytes;
    }

    template<typename ContainerType>
    template<typename T>
    SizeType ByteContainerStream<ContainerType>::WriteFromStream(SizeType bytes, GenericStream* inputStream, typename T::type)
    {
        AZ_Assert(inputStream, "Input stream is null!");
        AZ_Assert(inputStream != this, "Can't write and read from the same stream.");
        size_t bytesToCopy = aznumeric_cast<size_t>(PrepareToWrite(bytes));
        bytesToCopy = inputStream->Read(bytesToCopy, m_buffer->data() + m_pos);
        m_pos += bytesToCopy;
        return bytesToCopy;
    }

    template<typename ContainerType>
    template<typename T>
    SizeType ByteContainerStream<ContainerType>::Write(SizeType, const void*, unsigned int)
    {
        AZ_Assert(false, "This stream is read-only!");
        return 0;
    }

    template<typename ContainerType>
    template<typename T>
    SizeType ByteContainerStream<ContainerType>::WriteFromStream(SizeType, GenericStream*, unsigned int)
    {
        AZ_Assert(false, "This stream is read-only!");
        return 0;
    }


    template<typename ContainerType>
    void ByteContainerStream<ContainerType>::Truncate()
    {
        if (!IsOpen())
        {
            AZ_Error("ByteContainerStream", false, "The stream is closed, cannot truncate buffer");
            return;
        }
        Internal::ResizeContainerBuffer(m_buffer, m_pos, typename Internal::HasResizeNoConstruct<ContainerType>::type());
    }
}   // namespace AZ::IO
