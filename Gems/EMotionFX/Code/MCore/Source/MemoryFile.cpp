/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "MemoryFile.h"
#include "DiskFile.h"
#include "LogManager.h"


namespace MCore
{
    // return the unique type identification number
    uint32 MemoryFile::GetType() const
    {
        return TYPE_ID;
    }


    // try to open the memory location
    bool MemoryFile::Open(uint8* memoryStart, size_t length)
    {
        m_memoryStart    = memoryStart;
        m_currentPos     = memoryStart;
        m_length         = length;
        m_usedLength     = length;
        m_preAllocSize   = 1024; // pre-allocate 1 extra KB

        // if we need to create a new memory block
        if (memoryStart == nullptr)
        {
            m_allocate = true;
            if (length > 0)
            {
                m_memoryStart = (uint8*)MCore::Allocate(length, MCORE_MEMCATEGORY_MEMORYFILE);
                m_currentPos  = m_memoryStart;
            }
        }
        else
        {
            m_allocate = false;
        }

        return true;
    }


    // close the file
    void MemoryFile::Close()
    {
        // get rid of the allocated memory
        if (m_allocate)
        {
            MCore::Free(m_memoryStart);
        }

        m_memoryStart    = nullptr;
        m_currentPos     = nullptr;
        m_length         = 0;
        m_usedLength     = 0;
    }


    // flush the file
    void MemoryFile::Flush()
    {
    }


    bool MemoryFile::GetIsOpen() const
    {
        return (m_memoryStart != nullptr);
    }


    // return true when we have reached the end of the file
    bool MemoryFile::GetIsEOF() const
    {
        return (GetPos() >= GetFileSize());
    }


    // returns the next byte in the file
    uint8 MemoryFile::GetNextByte()
    {
        uint8 value = *m_currentPos;
        m_currentPos++;
        return value;
    }


    // returns the position (offset) in the file in bytes
    size_t MemoryFile::GetPos() const
    {
        return (size_t)(m_currentPos - m_memoryStart);
    }


    // write a given byte to the file
    bool MemoryFile::WriteByte(const uint8 value)
    {
        Write(&value, 1);
        return true;
    }


    // seek a given number of bytes ahead from it's current position
    bool MemoryFile::Forward(size_t numBytes)
    {
        uint8* newPos = m_currentPos + numBytes;
        if (newPos > (m_memoryStart + m_length))
        {
            return false;
        }

        m_currentPos = newPos;
        return true;
    }


    // seek to an absolute position in the file (offset in bytes)
    bool MemoryFile::Seek(size_t offset)
    {
        if (offset > m_length)
        {
            return false;
        }

        m_currentPos = m_memoryStart + offset;
        return true;
    }


    // write data to the file
    size_t MemoryFile::Write(const void* data, size_t length)
    {
        // if it won't fit in our allocated buffer, we have to enlarge it
        if ((m_currentPos + length > m_memoryStart + m_length) && m_allocate)
        {
            size_t offset = m_currentPos - m_memoryStart;
            size_t numBytesExtra = m_currentPos + length - m_memoryStart;
            numBytesExtra += m_preAllocSize;
            m_memoryStart = (uint8*)MCore::Realloc((uint8*)m_memoryStart, m_length + numBytesExtra, MCORE_MEMCATEGORY_MEMORYFILE);
            m_length += numBytesExtra;
            m_currentPos = m_memoryStart + offset;
        }

        // memcopy over the data
        MCORE_ASSERT((m_currentPos + length) <= (m_memoryStart + m_length)); // make sure we don't write past the end of our buffer
        MCore::MemCopy(m_currentPos, data, length);
        Forward(length);

        // only overwrite the used length in case we reached the boundary (don't do it in case we modify some data in the middle etc.)
        if (m_usedLength < GetPos())
        {
            m_usedLength = GetPos();
        }

        return length;
    }


    // read data from the file
    size_t MemoryFile::Read(void* data, size_t length)
    {
        if (m_currentPos + length > (uint8*)m_memoryStart + m_length)
        {
            const size_t numRead = length - ((m_currentPos + length) - ((uint8*)m_memoryStart + m_length));
            MCore::MemCopy(data, m_currentPos, numRead);
            Forward(numRead);
            MCore::LogWarning("MCore::MemoryFile::Read() - We can only read %d bytes of the %d bytes requested, as we are reading past the end of the memory file!", numRead, length);
            return numRead;
        }

        MCore::MemCopy(data, m_currentPos, length);
        Forward(length);
        return length;
    }


    // returns the filesize in bytes
    size_t MemoryFile::GetFileSize() const
    {
        return m_usedLength;
    }


    // get the memory start address
    uint8* MemoryFile::GetMemoryStart() const
    {
        return m_memoryStart;
    }


    // get the pre-alloc size
    size_t MemoryFile::GetPreAllocSize() const
    {
        return m_preAllocSize;
    }


    // set the pre-alloc size
    void MemoryFile::SetPreAllocSize(size_t newSizeInBytes)
    {
        m_preAllocSize = newSizeInBytes;
    }


    // load memory file from disk
    bool MemoryFile::LoadFromDiskFile(const char* fileName)
    {
        MCORE_ASSERT(fileName);

        // check if the file name is valid
        if (fileName[0] == 0)
        {
            LogError("Cannot load disk file. File name is empty.");
            return false;
        }

        // create the new disk file
        DiskFile diskFile;
        if (diskFile.Open(fileName, DiskFile::READ) == false)
        {
            LogError("Cannot open file '%s' in read mode. Please check if the file actually exists and try again.", fileName);
            return false;
        }

        // get the length of the disk file
        const size_t fileSize = diskFile.GetFileSize();

        // read from our incoming disk file and copy over the data to a temporary buffer
        uint8* data = (uint8*)Allocate(fileSize, MCORE_MEMCATEGORY_MEMORYFILE, 16);
        diskFile.Read(data, fileSize);
        diskFile.Close();

        // write the disk file data to the memory file
        Write(data, fileSize);

        // get rid of the temporary buffer
        Free(data);

        return true;
    }


    // save memory file to disk
    bool MemoryFile::SaveToDiskFile(const char* fileName)
    {
        MCORE_ASSERT(fileName);

        // check if the file name is valid
        if (fileName[0] == 0)
        {
            LogError("Cannot save to disk file. File name is empty.");
            return false;
        }

        // check if the memory file is opened
        if (GetIsOpen() == false)
        {
            LogError("Memory file not opened.");
            return false;
        }

        // seek to the beginning of the memory file
        Seek(0);

        // create the new disk file
        DiskFile newDiskFile;
        if (newDiskFile.Open(fileName, DiskFile::WRITE) == false)
        {
            LogError("Cannot open file '%s' in write mode. Please check if the file is write protected. It might be in use by another application or it is read-only. Please try again after resolving any possible issues.", fileName);
            return false;
        }

        newDiskFile.Write(GetMemoryStart(), GetFileSize());
        newDiskFile.Close();

        return true;
    }
} // namespace MCore
