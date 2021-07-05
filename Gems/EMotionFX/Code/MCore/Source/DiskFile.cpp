/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <stdio.h>
#include "DiskFile.h"


namespace MCore
{
    // constructor
    DiskFile::DiskFile()
        : File()
        , mFile(nullptr)
    {
    }


    // destructor
    DiskFile::~DiskFile()
    {
        Close();
    }


    // return the unique type identification number
    uint32 DiskFile::GetType() const
    {
        return TYPE_ID;
    }


    // try to open the file
    bool DiskFile::Open(const char* fileName, EMode mode)
    {
        // if the file already is open, close it first
        if (mFile)
        {
            Close();
        }

        //String fileMode;
        char fileMode[4];
        uint32 numChars = 0;

        if (mode == READ)
        {
            fileMode[0] = 'r';
            numChars = 1;
        }                                                                                   // open for reading, file must exist
        if (mode == WRITE)
        {
            fileMode[0] = 'w';
            numChars = 1;
        }                                                                                   // open for writing, file will be overwritten if it already exists
        if (mode == READWRITE)
        {
            fileMode[0] = 'r';
            fileMode[1] = '+';
            numChars = 2;
        }                                                                                   // open for reading and writing, file must exist
        if (mode == READWRITECREATE)
        {
            fileMode[0] = 'w';
            fileMode[1] = '+';
            numChars = 2;
        }                                                                                   // open for reading and writing, file will be overwritten when already exists, or created when it doesn't
        if (mode == APPEND)
        {
            fileMode[0] = 'a';
            numChars = 1;
        }                                                                                   // open for writing at the end of the file, file will be created when it doesn't exist
        if (mode == READWRITEAPPEND)
        {
            fileMode[0] = 'a';
            fileMode[1] = '+';
            numChars = 2;
        }                                                                                   // open for reading and appending (writing), file will be created if it doesn't exist

        // construct the filemode string
        fileMode[numChars++] = 'b'; // open in binary mode
        fileMode[numChars++] = '\0';

        // set the file mode we used
        mFileMode = mode;

        // set the filename
        mFileName = fileName;

        // try to open the file
        mFile = nullptr;
        azfopen(&mFile, fileName, fileMode);

        // check on success
        return (mFile != nullptr);
    }


    // close the file
    void DiskFile::Close()
    {
        if (mFile)
        {
            fclose(mFile);
            mFile = nullptr;
        }
    }


    // flush the file
    void DiskFile::Flush()
    {
        MCORE_ASSERT(mFile);
        fflush(mFile);
    }


    bool DiskFile::GetIsOpen() const
    {
        return (mFile != nullptr);
    }


    // return true when we have reached the end of the file
    bool DiskFile::GetIsEOF() const
    {
        MCORE_ASSERT(mFile);
        return (feof(mFile) != 0);
    }


    // returns the next byte in the file
    uint8 DiskFile::GetNextByte()
    {
        MCORE_ASSERT(mFile);
        MCORE_ASSERT((mFileMode == READ) || (mFileMode == READWRITE) || (mFileMode == READWRITEAPPEND) || (mFileMode == APPEND) || (mFileMode == READWRITECREATE)); // make sure we opened the file in read mode
        return static_cast<uint8>(fgetc(mFile));
    }


    // write a given byte to the file
    bool DiskFile::WriteByte(uint8 value)
    {
        MCORE_ASSERT(mFile);
        MCORE_ASSERT((mFileMode == WRITE) || (mFileMode == READWRITE) || (mFileMode == READWRITEAPPEND) || (mFileMode == READWRITECREATE)); // make sure we opened the file in write mode

        if (fputc(value, mFile) == EOF)
        {
            return false;
        }

        return true;
    }


    // write data to the file
    size_t DiskFile::Write(const void* data, size_t length)
    {
        MCORE_ASSERT(mFile);
        MCORE_ASSERT((mFileMode == WRITE) || (mFileMode == READWRITE) || (mFileMode == READWRITEAPPEND) || (mFileMode == READWRITECREATE)); // make sure we opened the file in write mode

        if (fwrite(data, length, 1, mFile) == 0)
        {
            return 0;
        }

        return length;
    }


    // read data from the file
    size_t DiskFile::Read(void* data, size_t length)
    {
        MCORE_ASSERT(mFile);
        MCORE_ASSERT((mFileMode == READ) || (mFileMode == READWRITE) || (mFileMode == READWRITEAPPEND) || (mFileMode == APPEND) || (mFileMode == READWRITECREATE)); // make sure we opened the file in read mode

        if (fread(data, length, 1, mFile) == 0)
        {
            return 0;
        }

        return length;
    }


    // get the file mode
    DiskFile::EMode DiskFile::GetFileMode() const
    {
        return mFileMode;
    }


    // get the file name
    const AZStd::string& DiskFile::GetFileName() const
    {
        return mFileName;
    }
} // namespace MCore
