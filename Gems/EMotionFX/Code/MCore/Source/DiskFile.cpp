/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        , m_file(nullptr)
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
        if (m_file)
        {
            Close();
        }

        const char* fileMode = [mode]() -> const char*
        {
            switch(mode)
            {
            case READ: // open for reading, file must exist
                return "rb";
            case WRITE: // open for writing, file will be overwritten if it already exists
                return "wb";
            case READWRITE: // open for reading and writing, file must exist
                return "r+b";
            case READWRITECREATE: // open for reading and writing, file will be overwritten when already exists, or created when it doesn't
                return "w+b";
            case APPEND: // open for writing at the end of the file, file will be created when it doesn't exist
                return "ab";
            case READWRITEAPPEND: // open for reading and appending (writing), file will be created if it doesn't exist
                return "a+b";
            default:
                return "";
            }
        }();

        // set the file mode we used
        m_fileMode = mode;

        // set the filename
        m_fileName = fileName;

        // try to open the file
        m_file = nullptr;
        azfopen(&m_file, fileName, fileMode);

        // check on success
        return (m_file != nullptr);
    }


    // close the file
    void DiskFile::Close()
    {
        if (m_file)
        {
            fclose(m_file);
            m_file = nullptr;
        }
    }


    // flush the file
    void DiskFile::Flush()
    {
        MCORE_ASSERT(m_file);
        fflush(m_file);
    }


    bool DiskFile::GetIsOpen() const
    {
        return (m_file != nullptr);
    }


    // return true when we have reached the end of the file
    bool DiskFile::GetIsEOF() const
    {
        MCORE_ASSERT(m_file);
        return (feof(m_file) != 0);
    }


    // returns the next byte in the file
    uint8 DiskFile::GetNextByte()
    {
        MCORE_ASSERT(m_file);
        MCORE_ASSERT((m_fileMode == READ) || (m_fileMode == READWRITE) || (m_fileMode == READWRITEAPPEND) || (m_fileMode == APPEND) || (m_fileMode == READWRITECREATE)); // make sure we opened the file in read mode
        return static_cast<uint8>(fgetc(m_file));
    }


    // write a given byte to the file
    bool DiskFile::WriteByte(uint8 value)
    {
        MCORE_ASSERT(m_file);
        MCORE_ASSERT((m_fileMode == WRITE) || (m_fileMode == READWRITE) || (m_fileMode == READWRITEAPPEND) || (m_fileMode == READWRITECREATE)); // make sure we opened the file in write mode

        if (fputc(value, m_file) == EOF)
        {
            return false;
        }

        return true;
    }


    // write data to the file
    size_t DiskFile::Write(const void* data, size_t length)
    {
        MCORE_ASSERT(m_file);
        MCORE_ASSERT((m_fileMode == WRITE) || (m_fileMode == READWRITE) || (m_fileMode == READWRITEAPPEND) || (m_fileMode == READWRITECREATE)); // make sure we opened the file in write mode

        if (fwrite(data, length, 1, m_file) == 0)
        {
            return 0;
        }

        return length;
    }


    // read data from the file
    size_t DiskFile::Read(void* data, size_t length)
    {
        MCORE_ASSERT(m_file);
        MCORE_ASSERT((m_fileMode == READ) || (m_fileMode == READWRITE) || (m_fileMode == READWRITEAPPEND) || (m_fileMode == APPEND) || (m_fileMode == READWRITECREATE)); // make sure we opened the file in read mode

        if (fread(data, length, 1, m_file) == 0)
        {
            return 0;
        }

        return length;
    }


    // get the file mode
    DiskFile::EMode DiskFile::GetFileMode() const
    {
        return m_fileMode;
    }


    // get the file name
    const AZStd::string& DiskFile::GetFileName() const
    {
        return m_fileName;
    }
} // namespace MCore
