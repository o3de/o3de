/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "native/utilities/ByteArrayStream.h"


namespace AssetProcessor
{
    using namespace AZ::IO;

    ByteArrayStream::ByteArrayStream()
    {
        m_usingOwnArray = true;
        m_activeArray = &m_ownArray;
        m_currentPos = 0;
        m_readOnly = false;
    }

    ByteArrayStream::ByteArrayStream(QByteArray* other)
    {
        m_activeArray = other;
        m_usingOwnArray = false;
        m_currentPos = m_activeArray->size();
        m_readOnly = false;
    }

    ByteArrayStream::ByteArrayStream(const char* data, unsigned int length)
    {
        m_activeArray = &m_ownArray;
        m_ownArray.setRawData(data, length);
        m_usingOwnArray = true;
        m_currentPos = 0;
        m_readOnly = true;
    }

    void ByteArrayStream::Reserve(int amount)
    {
        if (m_readOnly)
        {
            return;
        }
        m_activeArray->reserve(amount);
    }

    void ByteArrayStream::Seek(OffsetType bytes, SeekMode mode)
    {
        SizeType finalPosition = GenericStream::ComputeSeekPosition(bytes, mode);

        AZ_Assert(finalPosition < INT_MAX, "Overflow of SizeType to int in ByteArrayStream.");
        AZ_Assert(finalPosition <= m_activeArray->size(), "You cant seek beyond end of file");

        // safety clamp!
        finalPosition = AZ::GetClamp<SizeType>(finalPosition, 0, static_cast<SizeType>(m_activeArray->size()));
        m_currentPos = static_cast<int>(finalPosition);
    }

    SizeType ByteArrayStream::Read(SizeType bytes, void* oBuffer)
    {
        // eg
        // xxxxx <-- data, size() 5
        //   ^   <-- pos, currently 2
        // we have 3 bytes available .. 5 - 2.

        int actualAvailableBytes = m_activeArray->size() - m_currentPos;
        if (actualAvailableBytes <= 0)
        {
            return static_cast<SizeType>(0);
        }

        const char* data = m_activeArray->constData();
        data += m_currentPos;

        AZ_Assert(bytes < std::numeric_limits<int>::max(), "Overflow in ByteArrayStream::Read.");
        // safety cast.
        bytes = AZ::GetMin(static_cast<SizeType>(std::numeric_limits<int>::max()), bytes);

        int bytesToRead = AZ::GetMin<int>(static_cast<int>(bytes), actualAvailableBytes);

        memcpy(oBuffer, data, bytesToRead);
        m_currentPos += bytesToRead;
        return bytesToRead;
    }

    SizeType ByteArrayStream::PrepareForWrite(SizeType bytes)
    {
        // how much bigger does our array have to grow?
        // example
        // oooooooo <---- capacity = 8
        // xxxxx <--- size = 5
        //   ^   <--- currentpos = 2 (3rd byte)
        // if we're asking for a 10 byte write the final picture will be
        // xxyyyyyyyyyy  <--- size() = 12.

        if (m_readOnly)
        {
            return 0;
        }

        SizeType intMaxSize = std::numeric_limits<int>::max();
        SizeType finalSize = bytes + static_cast<SizeType>(m_currentPos);
        AZ_Assert(finalSize < intMaxSize, "Overflow in ByteArrayStream::Write");
        if (finalSize > intMaxSize)
        {
            SizeType delta = finalSize - intMaxSize;
            finalSize -= delta;
            bytes -= delta;
        }
        int intSize = static_cast<int>(finalSize);
        if (intSize > m_activeArray->capacity())
        {
            // grow the array, but let's be smart about it.
            // assume there'll be another write the same size pretty soon.
            // we'd like to grow it by about a quarter of its current size
            // thus if we're making one LARGE write,  it grows 0
            int growthAmount = intSize / 4;

            // don't allow overflow here either.
            if (static_cast<SizeType>(growthAmount) + static_cast<SizeType>(intSize) >= intMaxSize)
            {
                growthAmount = 0;
            }

            m_activeArray->reserve(intSize + growthAmount);
        }

        if (intSize > m_activeArray->size())
        {
            m_activeArray->resize(intSize);
        }

        return bytes;
    }

    SizeType ByteArrayStream::Write(SizeType bytes, const void* iBuffer)
    {
        bytes = PrepareForWrite(bytes);

        if (bytes > 0)
        {
            char* data = m_activeArray->data();
            data += m_currentPos;
            memcpy(data, iBuffer, bytes);
            m_currentPos += static_cast<int>(bytes);
        }

        return bytes;
    }

    SizeType ByteArrayStream::WriteFromStream(SizeType bytes, AZ::IO::GenericStream* inputStream)
    {
        AZ_Assert(inputStream, "Cannot copy from a null input stream.");
        AZ_Assert(inputStream != this, "Can't write and read from the same stream.");

        bytes = PrepareForWrite(bytes);

        if (bytes > 0)
        {
            char* data = m_activeArray->data();
            data += m_currentPos;
            bytes = inputStream->Read(bytes, data);
            m_currentPos += static_cast<int>(bytes);
        }

        return bytes;
    }

    SizeType    ByteArrayStream::GetCurPos() const
    {
        return static_cast<SizeType>(m_currentPos);
    }

    SizeType    ByteArrayStream::GetLength() const
    {
        return static_cast<SizeType>(m_activeArray->size());
    }

    QByteArray ByteArrayStream::GetArray() const
    {
        if (m_usingOwnArray)
        {
            return m_ownArray;
        }

        return *m_activeArray;
    }
}
