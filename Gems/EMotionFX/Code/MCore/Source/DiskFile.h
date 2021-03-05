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
#include "MemoryManager.h"


namespace MCore
{
    /**
     * A standard file as we normally think of. In other words, a file stored on the harddisk or a CD or any other comparable medium.
     * This is for binary files only. If you plan to read text files, please use the MCore::DiskTextFile class.
     */
    class MCORE_API DiskFile
        : public File
    {
        MCORE_MEMORYOBJECTCATEGORY(DiskFile, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_DISKFILE)

    public:
        // the type returned by GetType()
        enum
        {
            TYPE_ID = 0x0000001
        };

        /**
         * File opening modes.
         * Do not use a combination of these modes. Just pick one.
         */
        enum EMode
        {
            READ            = 0,    /**< open for reading, if the file doesn't exist the Open method will fail. */
            WRITE           = 1,    /**< open for writing, if the file already exists it will be overwritten. */
            READWRITE       = 2,    /**< opens the file for both reading and writing, the file must already exist else the Open method will fail. */
            READWRITECREATE = 3,    /**< opens the file for both reading and writing, if the file exists already it will be overwritten. */
            APPEND          = 4,    /**< opens for writing at the end of the file, will create a new file if it doesn't yet exist. */
            READWRITEAPPEND = 5     /**< opens for reading and appending (writing), creates the file when it doesn't exist. */
        };

        /**
         * The constructor.
         */
        DiskFile();

        /**
         * The destructor. Automatically closes the file.
         */
        virtual ~DiskFile();

        /**
         * Get the unique type ID.
         * @result The type identification number.
         */
        uint32 GetType() const override;

        /**
         * Try to open the file, given a filename and open mode.
         * The file is always opened in binary mode. If you want to load text files, use MCore::DiskTextFile.
         * @param fileName The filename on disk.
         * @param mode The file open mode. See enum EMode for more information. Do NOT use a combination of these modes.
         * @result Returns true when successfully opened the file, otherwise false is returned.
         */
        virtual bool Open(const char* fileName, EMode mode);

        /**
         * Close the file.
         */
        void Close() override;

        /**
         * Flush the file. All cached (not yet written) data will be forced to written when calling this method.
         */
        void Flush() override;

        /**
         * Returns true if we reached the end of this file, otherwise false is returned.
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
         * @result Returns the number of bytes read.
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
         * Returns the mode the file has been opened with.
         * @result The mode the file has been opened with. See enum EMode.
         */
        EMode GetFileMode() const;

        /**
         * Returns the name of the file as it has been opened.
         * @result The string containing the filename as it has been passed to the method Open.
         */
        const AZStd::string& GetFileName() const;

    protected:
        AZStd::string  mFileName;  /**< The filename */
        FILE*          mFile;      /**< The file handle. */
        EMode          mFileMode;  /**< The mode we opened the file with. */
    };
} // namespace MCore
