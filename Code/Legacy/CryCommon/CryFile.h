/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Description : File wrapper.
#pragma once

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzFramework/Archive/IArchive.h>

//////////////////////////////////////////////////////////////////////////
#define CRYFILE_MAX_PATH                         260
//////////////////////////////////////////////////////////////////////////

// Summary:
//   Wrapper on file system.
class CCryFile
{
public:
    CCryFile();
    CCryFile(const char* filename, const char* mode);
    ~CCryFile();

    bool Open(const char* filename, const char* mode);
    void Close();

    // Summary:
    //   Writes data in a file to the current file position.
    size_t Write(const void* lpBuf, size_t nSize);
    // Summary:
    //   Reads data from a file at the current file position.
    size_t ReadRaw(void* lpBuf, size_t nSize);

    // Summary:
    //   Automatic endian-swapping version.
    template<class T>
    inline size_t ReadType(T* pDest, size_t nCount = 1)
    {
        size_t nRead = ReadRaw(pDest, sizeof(T) * nCount);
        SwapEndian(pDest, nCount);
        return nRead;
    }

    // Summary:
    //   Retrieves the length of the file.
    size_t GetLength();

    // Summary:
    //   Moves the current file pointer to the specified position.
    size_t Seek(size_t seek, int mode);

    // Description:
    //    Retrieves the filename of the selected file.
    const char* GetFilename() const { return m_filename.c_str(); };

    // Summary:
    //   Checks if file is opened from Archive file.
    bool IsInPak() const;

    // Summary:
    //   Gets path of archive this file is in.
    AZ::IO::PathView GetPakPath() const;

private:
    AZ::IO::FixedMaxPath m_filename;
    AZ::IO::HandleType m_fileHandle;
};

// Summary:
// CCryFile implementation.
inline CCryFile::CCryFile()
{
    m_fileHandle = AZ::IO::InvalidHandle;
}

//////////////////////////////////////////////////////////////////////////
inline CCryFile::CCryFile(const char* filename, const char* mode)
{
    m_fileHandle = AZ::IO::InvalidHandle;
    Open(filename, mode);
}

//////////////////////////////////////////////////////////////////////////
inline CCryFile::~CCryFile()
{
    Close();
}

//////////////////////////////////////////////////////////////////////////
// Notes:
//   For nOpenFlagsEx see IArchive::EFOpenFlags
// See also:
//   IArchive::EFOpenFlags
inline bool CCryFile::Open(const char* filename, const char* mode)
{
    m_filename = filename;
#if !defined (_RELEASE)
    if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
    {
        if (bool lowercasePaths{}; console->GetCvarValue("ed_lowercasepaths", lowercasePaths) == AZ::GetValueResult::Success)
        {
            if (lowercasePaths)
            {
                AZStd::to_lower(m_filename.Native().begin(), m_filename.Native().end());
            }
        }
    }
#endif
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        Close();
    }

    AZ::IO::FileIOBase::GetInstance()->Open(m_filename.c_str(), AZ::IO::GetOpenModeFromStringMode(mode), m_fileHandle);

    return m_fileHandle != AZ::IO::InvalidHandle;
}

//////////////////////////////////////////////////////////////////////////
inline void CCryFile::Close()
{
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        AZ::IO::FileIOBase::GetInstance()->Close(m_fileHandle);

        m_fileHandle = AZ::IO::InvalidHandle;
        m_filename.clear();
    }
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::Write(const void* lpBuf, size_t nSize)
{
    AZ_Assert(m_fileHandle != AZ::IO::InvalidHandle, "File Handle is invalid. Cannot Write");

    if (AZ::IO::FileIOBase::GetInstance()->Write(m_fileHandle, lpBuf, nSize))
    {
        return nSize;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::ReadRaw(void* lpBuf, size_t nSize)
{
    AZ_Assert(m_fileHandle != AZ::IO::InvalidHandle, "File Handle is invalid. Cannot Read");

    AZ::u64 bytesRead = 0;
    AZ::IO::FileIOBase::GetInstance()->Read(m_fileHandle, lpBuf, nSize, false, &bytesRead);

    return static_cast<size_t>(bytesRead);
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::GetLength()
{
    AZ_Assert(m_fileHandle != AZ::IO::InvalidHandle, "File Handle is invalid. Cannot query file length");
    //long curr = ftell(m_file);
    AZ::u64 size = 0;
    AZ::IO::FileIOBase::GetInstance()->Size(m_fileHandle, size);
    return static_cast<size_t>(size);
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::Seek(size_t seek, int mode)
{
    AZ_Assert(m_fileHandle != AZ::IO::InvalidHandle, "File Handle is invalid. Cannot seek in unopen file");

    if (AZ::IO::FileIOBase::GetInstance()->Seek(m_fileHandle, seek, AZ::IO::GetSeekTypeFromFSeekMode(mode)))
    {
        return 0;
    }
    return 1;
}

//////////////////////////////////////////////////////////////////////////
inline bool CCryFile::IsInPak() const
{
    if (auto archive = AZ::Interface<AZ::IO::IArchive>::Get();
        m_fileHandle != AZ::IO::InvalidHandle && archive != nullptr)
    {
        return !archive->GetFileArchivePath(m_fileHandle).empty();
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
inline AZ::IO::PathView CCryFile::GetPakPath() const
{
    if (auto archive = AZ::Interface<AZ::IO::IArchive>::Get();
        m_fileHandle != AZ::IO::InvalidHandle && archive != nullptr)
    {
        if (AZ::IO::PathView sPath(archive->GetFileArchivePath(m_fileHandle)); sPath.empty())
        {
            return sPath;
        }
    }
    return {};
}

