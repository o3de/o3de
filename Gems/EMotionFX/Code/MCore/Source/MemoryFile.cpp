/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mMemoryStart    = memoryStart;
        mCurrentPos     = memoryStart;
        mLength         = length;
        mUsedLength     = length;
        mPreAllocSize   = 1024; // pre-allocate 1 extra KB

        // if we need to create a new memory block
        if (memoryStart == nullptr)
        {
            mAllocate = true;
            if (length > 0)
            {
                mMemoryStart = (uint8*)MCore::Allocate(length, MCORE_MEMCATEGORY_MEMORYFILE);
                mCurrentPos  = mMemoryStart;
            }
        }
        else
        {
            mAllocate = false;
        }

        return true;
    }


    // close the file
    void MemoryFile::Close()
    {
        // get rid of the allocated memory
        if (mAllocate)
        {
            MCore::Free(mMemoryStart);
        }

        mMemoryStart    = nullptr;
        mCurrentPos     = nullptr;
        mLength         = 0;
        mUsedLength     = 0;
    }


    // flush the file
    void MemoryFile::Flush()
    {
    }


    bool MemoryFile::GetIsOpen() const
    {
        return (mMemoryStart != nullptr);
    }


    // return true when we have reached the end of the file
    bool MemoryFile::GetIsEOF() const
    {
        return (GetPos() >= GetFileSize());
    }


    // returns the next byte in the file
    uint8 MemoryFile::GetNextByte()
    {
        uint8 value = *mCurrentPos;
        mCurrentPos++;
        return value;
    }


    // returns the position (offset) in the file in bytes
    size_t MemoryFile::GetPos() const
    {
        return (size_t)(mCurrentPos - mMemoryStart);
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
        uint8* newPos = mCurrentPos + numBytes;
        if (newPos > (mMemoryStart + mLength))
        {
            return false;
        }

        mCurrentPos = newPos;
        return true;
    }


    // seek to an absolute position in the file (offset in bytes)
    bool MemoryFile::Seek(size_t offset)
    {
        if (offset > mLength)
        {
            return false;
        }

        mCurrentPos = mMemoryStart + offset;
        return true;
    }


    // write data to the file
    size_t MemoryFile::Write(const void* data, size_t length)
    {
        // if it won't fit in our allocated buffer, we have to enlarge it
        if ((mCurrentPos + length > mMemoryStart + mLength) && mAllocate)
        {
            size_t offset = mCurrentPos - mMemoryStart;
            size_t numBytesExtra = mCurrentPos + length - mMemoryStart;
            numBytesExtra += mPreAllocSize;
            mMemoryStart = (uint8*)MCore::Realloc((uint8*)mMemoryStart, mLength + numBytesExtra, MCORE_MEMCATEGORY_MEMORYFILE);
            mLength += numBytesExtra;
            mCurrentPos = mMemoryStart + offset;
        }

        // memcopy over the data
        MCORE_ASSERT((mCurrentPos + length) <= (mMemoryStart + mLength)); // make sure we don't write past the end of our buffer
        MCore::MemCopy(mCurrentPos, data, length);
        Forward(length);

        // only overwrite the used length in case we reached the boundary (don't do it in case we modify some data in the middle etc.)
        if (mUsedLength < GetPos())
        {
            mUsedLength = GetPos();
        }

        return length;
    }


    // read data from the file
    size_t MemoryFile::Read(void* data, size_t length)
    {
        //  MCORE_ASSERT(mCurrentPos + length <= (uint8*)mMemoryStart + mLength);   // make sure we don't read past the end of the memory block
        if (mCurrentPos + length > (uint8*)mMemoryStart + mLength)
        {
            const size_t numRead = length - ((mCurrentPos + length) - ((uint8*)mMemoryStart + mLength));
            MCore::MemCopy(data, mCurrentPos, numRead);
            Forward(static_cast<uint32>(numRead));
            MCore::LogWarning("MCore::MemoryFile::Read() - We can only read %d bytes of the %d bytes requested, as we are reading past the end of the memory file!", numRead, length);
            return static_cast<uint32>(numRead);
        }

        MCore::MemCopy(data, mCurrentPos, length);
        Forward(length);
        return length;
    }


    // returns the filesize in bytes
    size_t MemoryFile::GetFileSize() const
    {
        return static_cast<uint32>(mUsedLength); // TODO: convert to size_t later
    }


    // get the memory start address
    uint8* MemoryFile::GetMemoryStart() const
    {
        return mMemoryStart;
    }


    // get the pre-alloc size
    size_t MemoryFile::GetPreAllocSize() const
    {
        return mPreAllocSize;
    }


    // set the pre-alloc size
    void MemoryFile::SetPreAllocSize(size_t newSizeInBytes)
    {
        mPreAllocSize = newSizeInBytes;
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
