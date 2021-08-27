/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the core headers
#include "StandardHeaders.h"
#include "Stream.h"


namespace MCore
{
    /**
     * The file abstract base class.
     * This class represents some file from which the user can read, write, etc.
     * Do not just think of this as only a file on disk. A file could be virtually anything from which can be read and
     * to which can be written. Like a stream, but with more functionalities. For example the file could also be a
     * specified section in your systems main memory. Another example could be the file being some sort of network stream.
     */
    class MCORE_API File
        : public Stream
    {
    public:
        /**
         * The constructor.
         */
        File()
            : Stream() {}

        /**
         * The destructor.
         */
        virtual ~File() {}

        /**
         * Close the file.
         */
        virtual void Close() = 0;

        /**
         * Flush the file. All cached (not yet written) data will be forced to written when calling this method.
         */
        virtual void Flush() = 0;

        /**
         * Check if we reached the end of the file or not.
         * @result Returns true when we have returned the end of the file. Otherwise false is returned.
         */
        virtual bool GetIsEOF() const = 0;

        /**
         * Reads and returns the next byte/character in the file. So this will increase the position in the file with one.
         * @result The character or byte read.
         */
        virtual uint8 GetNextByte() = 0;

        /**
         * Returns the position in the file.
         * @result The offset in the file in bytes.
         */
        virtual size_t GetPos() const = 0;

        /**
         * Returns the size of this file in bytes.
         * @result The filesize in bytes.
         */
        virtual size_t GetFileSize() const = 0;

        /**
         * Write a character or byte in the file.
         * @param value The character or byte to write.
         * @result Returns true when successfully written, otherwise false is returned.
         */
        virtual bool WriteByte(uint8 value) = 0;

        /**
         * Seek ahead a given number of bytes. This can be used to skip a given number of upcoming bytes in the file.
         * @param numBytes The number of bytes to seek ahead.
         * @result Returns true when succeeded or false when an error occured (for example when we where trying to read past the end of the file).
         */
        virtual bool Forward(size_t numBytes) = 0;

        /**
         * Seek to a given byte position in the file, where 0 would be the beginning of the file.
         * So we're talking about the absolute position in the file. After successfully executing this method
         * the method GetPos will return the given offset.
         * @param offset The offset in bytes (the position) in the file to seek to.
         * @result Returns true when successfully moved to the given position in the file, otherwise false.
         */
        virtual bool Seek(size_t offset) = 0;

        /**
         * Read a given amount of data from the file.
         * @param data The pointer where to store the read data.
         * @param length The size in bytes of the data to read.
         * @result Returns the number of read bytes.
         */
        virtual size_t Read(void* data, size_t length) = 0;

        /**
         * Writes a given amount of data to the file.
         * @param data The pointer to the data to write.
         * @param length The size in bytes of the data to write.
         * @result Returns the number of written bytes.
         */
        virtual size_t Write(const void* data, size_t length) = 0;

        /**
         * Check if the file has been opened already.
         * @result Returns true when the file has been opened, otherwise false.
         */
        virtual bool GetIsOpen() const = 0;
    };
} // namespace MCore
