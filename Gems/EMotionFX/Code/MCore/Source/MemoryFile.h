/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

// include the Core headers
#include "StandardHeaders.h"
#include "File.h"


namespace MCore
{
    // forward declaration
    class DiskFile;

    /**
     * A file stored in memory, located at a given position with a known length in bytes.
     * It is also possible to write to the file. In this case, when writing past the end of the file, new memory will be allocated.
     */
    class MCORE_API MemoryFile
        : public File
    {
        MCORE_MEMORYOBJECTCATEGORY(MemoryFile, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_MEMORYFILE)

    public:
        // the type returned by GetType()
        enum
        {
            TYPE_ID = 0x0000002
        };

        /**
         * Constructor.
         */
        MemoryFile()
            : File()
            , mMemoryStart(nullptr)
            , mCurrentPos(nullptr)
            , mLength(0)
            , mUsedLength(0)
            , mPreAllocSize(1024)
            , mAllocate(false) {}

        /**
         * Destructor. Automatically closes the file.
         */
        ~MemoryFile()   { Close(); }

        /**
         * Get the unique type ID.
         * @result The type identification number.
         */
        uint32 GetType() const override;

        /**
         * Open the file from a given memory location, with a given length in bytes.
         * If you want to create a new block of memory that can grow, like creating a new file on disk, you
         * can pass nullptr as memory start address, and as length the initial memory block size you want to be allocated, or 0 if it should start empty.
         * @param memoryStart The memory position of the first byte in the file, or nullptr when you want to automatically allocate memory.
         * @param length The length in bytes of the file. So the size in bytes of the memory block.
         * @result Returns true when the file could be opened, otherwise false.
         */
        bool Open(uint8* memoryStart = nullptr, size_t length = 0);

        /**
         * Close the file.
         */
        void Close() override;

        /**
         * Flush the file. All cached (not yet written) data will be forced to written when calling this method.
         */
        void Flush() override;

        /**
         * Check if we reached the end of the file or not.
         * @result Returns true when we have returned the end of the file. Otherwise false is returned.
         */
        bool GetIsEOF() const override;

        /**
         * Reads and returns the next byte/character in the file. So this will increase the position in the file with one.
         * @result The character or byte read.
         */
        uint8 GetNextByte() override;

        /**
         * Returns the position in the file.
         * @result The offset in the file in bytes.
         */
        size_t GetPos() const override;

        /**
         * Write a character or byte in the file.
         * @param value The character or byte to write.
         * @result Returns true when successfully written, otherwise false is returned.
         */
        bool WriteByte(uint8 value) override;

        /**
         * Seek ahead a given number of bytes. This can be used to skip a given number of upcoming bytes in the file.
         * @param numBytes The number of bytes to seek ahead.
         * @result Returns true when succeeded or false when an error occured (for example when we where trying to read past the end of the file).
         */
        bool Forward(size_t numBytes) override;

        /**
         * Seek to a given byte position in the file, where 0 would be the beginning of the file.
         * So we're talking about the absolute position in the file. After successfully executing this method
         * the method GetPos will return the given offset.
         * @param offset The offset in bytes (the position) in the file to seek to.
         * @result Returns true when successfully moved to the given position in the file, otherwise false.
         */
        bool Seek(size_t offset) override;

        /**
         * Writes a given amount of data to the file.
         * @param data The pointer to the data to write.
         * @param length The size in bytes of the data to write.
         * @result Returns the number of written bytes.
         */
        size_t Write(const void* data, size_t length) override;

        /**
         * Read a given amount of data from the file.
         * @param data The pointer where to store the read data.
         * @param length The size in bytes of the data to read.
         * @result The number of bytes read.
         */
        size_t Read(void* data, size_t length) override;

        /**
         * Check if the file has been opened already.
         * @result Returns true when the file has been opened, otherwise false.
         */
        bool GetIsOpen() const override;

        /**
         * Returns the size of this file in bytes.
         * @result The filesize in bytes.
         */
        size_t GetFileSize() const override;

        /**
         * Get the memory start address, where the data is stored.
         * @result The memory start address that points to the start of the file.
         */
        uint8* GetMemoryStart() const;

        /**
         * Get the pre-allocation size, which is the number of bytes that are allocated extra when
         * writing to the file and the data wouldn't fit in the file anymore. On default this value is 1024.
         * This can reduce the number of reallocations being performed significantly.
         * @result The pre-allocation size, in bytes.
         */
        size_t GetPreAllocSize() const;

        /**
         * Set the pre-allocation size, which is the number of bytes that are allocated extra when
         * writing to the file and the data wouldn't fit in the file anymore. On default this value is 1024.
         * This can reduce the number of reallocations being performed significantly.
         * @param newSizeInBytes The size in bytes to allocate on top of the required allocation size.
         */
        void SetPreAllocSize(size_t newSizeInBytes);

        /**
         * Load memory file from disk.
         * The memory file object should be newly created or not containing any data yet when calling this function. Also please make sure
         * that the memory file is opened.
         * @param fileName The full or relative path/file name of the disk file to load into this memory file.
         * @return True when loading worked fine, false if any error occurred. All errors that happen will be logged.
         */
        bool LoadFromDiskFile(const char* fileName);

        /**
         * Save the memory file to disk.
         * The memory file object should contain data yet when calling this function else the resulting disk file will be empty. Also please make sure
         * that the memory file is opened.
         * @param fileName The full or relative path/file name of the disk file to be created.
         * @return True when saving worked fine, false if any error occurred. All errors that happen will be logged.
         */
        bool SaveToDiskFile(const char* fileName);

    private:
        uint8*  mMemoryStart;   /**< The location of the file */
        uint8*  mCurrentPos;    /**< The current location */
        size_t  mLength;        /**< The total length of the file. */
        size_t  mUsedLength;    /**< The actual used length of the memory file. */
        size_t  mPreAllocSize;  /**< The pre-allocation size (in bytes) when we have to reallocate memory. This prevents many allocations. The default=1024, which is 1kb.*/
        bool    mAllocate;      /**< Can we reallocate or not? */
    };
} // namespace MCore
