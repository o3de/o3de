/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Description : File wrapper.
#pragma once

#include <CryPath.h>
#include <ISystem.h>
#include <AzFramework/Archive/IArchive.h>
#include <IConsole.h>
#include <AzCore/IO/FileIO.h>

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

    bool Open(const char* filename, const char* mode, int nOpenFlagsEx = 0);
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
    const char* GetFilename() const { return m_filename; };

    // Description:
    //    Retrieves the filename after adjustment to the real relative to engine root path.
    // Example:
    //    Original filename "textures/red.dds" adjusted filename will look like "game/textures/red.dds"
    // Return:
    //    Adjusted filename, this is a pointer to a static string, copy return value if you want to keep it.
    const char* GetAdjustedFilename() const;

    // Summary:
    //   Checks if file is opened from Archive file.
    bool IsInPak() const;

    // Summary:
    //   Gets path of archive this file is in.
    const char* GetPakPath() const;

private:
    char m_filename[CRYFILE_MAX_PATH];
    AZ::IO::HandleType m_fileHandle;
    AZ::IO::IArchive* m_pIArchive;
};

// Summary:
// CCryFile implementation.
inline CCryFile::CCryFile()
{
    m_fileHandle = AZ::IO::InvalidHandle;
    m_pIArchive = gEnv ? gEnv->pCryPak : NULL;
}

//////////////////////////////////////////////////////////////////////////
inline CCryFile::CCryFile(const char* filename, const char* mode)
{
    m_fileHandle = AZ::IO::InvalidHandle;
    m_pIArchive = gEnv ? gEnv->pCryPak : NULL;
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
inline bool CCryFile::Open(const char* filename, const char* mode, int nOpenFlagsEx)
{
    char tempfilename[CRYFILE_MAX_PATH] = "";
    azstrcpy(tempfilename, CRYFILE_MAX_PATH, filename);

#if !defined (_RELEASE)
    if (gEnv && gEnv->IsEditor() && gEnv->pConsole)
    {
        ICVar* const pCvar = gEnv->pConsole->GetCVar("ed_lowercasepaths");
        if (pCvar)
        {
            const int lowercasePaths = pCvar->GetIVal();
            if (lowercasePaths)
            {
                const AZStd::string lowerString = PathUtil::ToLower(tempfilename);
                azstrcpy(tempfilename, CRYFILE_MAX_PATH, lowerString.c_str());
            }
        }
    }
#endif
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        Close();
    }
    azstrcpy(m_filename, CRYFILE_MAX_PATH, tempfilename);

    if (m_pIArchive)
    {
        m_fileHandle = m_pIArchive->FOpen(tempfilename, mode, nOpenFlagsEx);
    }
    else
    {
        AZ::IO::FileIOBase::GetInstance()->Open(tempfilename, AZ::IO::GetOpenModeFromStringMode(mode), m_fileHandle);
    }

    return m_fileHandle != AZ::IO::InvalidHandle;
}

//////////////////////////////////////////////////////////////////////////
inline void CCryFile::Close()
{
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        if (m_pIArchive)
        {
            m_pIArchive->FClose(m_fileHandle);
        }
        else
        {
            AZ::IO::FileIOBase::GetInstance()->Close(m_fileHandle);
        }
        m_fileHandle = AZ::IO::InvalidHandle;
        m_filename[0] = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::Write(const void* lpBuf, size_t nSize)
{
    assert(m_fileHandle != AZ::IO::InvalidHandle);
    if (m_pIArchive)
    {
        return m_pIArchive->FWrite(lpBuf, 1, nSize, m_fileHandle);
    }

    if (AZ::IO::FileIOBase::GetInstance()->Write(m_fileHandle, lpBuf, nSize))
    {
        return nSize;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::ReadRaw(void* lpBuf, size_t nSize)
{
    assert(m_fileHandle != AZ::IO::InvalidHandle);
    if (m_pIArchive)
    {
        return m_pIArchive->FReadRaw(lpBuf, 1, nSize, m_fileHandle);
    }

    AZ::u64 bytesRead = 0;
    AZ::IO::FileIOBase::GetInstance()->Read(m_fileHandle, lpBuf, nSize, false, &bytesRead);

    return static_cast<size_t>(bytesRead);
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::GetLength()
{
    assert(m_fileHandle != AZ::IO::InvalidHandle);
    if (m_pIArchive)
    {
        return m_pIArchive->FGetSize(m_fileHandle);
    }
    //long curr = ftell(m_file);
    AZ::u64 size = 0;
    AZ::IO::FileIOBase::GetInstance()->Size(m_fileHandle, size);
    return static_cast<size_t>(size);
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::Seek(size_t seek, int mode)
{
    assert(m_fileHandle != AZ::IO::InvalidHandle);
    if (m_pIArchive)
    {
        return m_pIArchive->FSeek(m_fileHandle, long(seek), mode);
    }

    if (AZ::IO::FileIOBase::GetInstance()->Seek(m_fileHandle, seek, AZ::IO::GetSeekTypeFromFSeekMode(mode)))
    {
        return 0;
    }
    return 1;
}

//////////////////////////////////////////////////////////////////////////
inline bool CCryFile::IsInPak() const
{
    if (m_fileHandle != AZ::IO::InvalidHandle && m_pIArchive)
    {
        return m_pIArchive->GetFileArchivePath(m_fileHandle) != NULL;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
inline const char* CCryFile::GetPakPath() const
{
    if (m_fileHandle != AZ::IO::InvalidHandle && m_pIArchive)
    {
        const char* sPath = m_pIArchive->GetFileArchivePath(m_fileHandle);
        if (sPath != NULL)
        {
            return sPath;
        }
    }
    return "";
}


//////////////////////////////////////////////////////////////////////////
inline const char* CCryFile::GetAdjustedFilename() const
{
    static char szAdjustedFile[AZ::IO::IArchive::MaxPath];
    assert(m_pIArchive);
    if (!m_pIArchive)
    {
        return "";
    }

    // Gets mod path to file.
    const char* gameUrl = m_pIArchive->AdjustFileName(m_filename, szAdjustedFile, AZ::IO::IArchive::MaxPath, 0);

    // Returns standard path otherwise.
    if (gameUrl != &szAdjustedFile[0])
    {
        azstrcpy(szAdjustedFile, AZ::IO::IArchive::MaxPath, gameUrl);
    }
    return szAdjustedFile;
}
