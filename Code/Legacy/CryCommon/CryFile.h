/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : File wrapper.


#ifndef CRYINCLUDE_CRYCOMMON_CRYFILE_H
#define CRYINCLUDE_CRYCOMMON_CRYFILE_H
#pragma once

#include <CryPath.h>
#include <ISystem.h>
#include <AzFramework/Archive/IArchive.h>
#include <IConsole.h>
#include "StringUtils.h"
#include <AzCore/IO/FileIO.h>

//////////////////////////////////////////////////////////////////////////
// Defines for CryEngine filetypes extensions.
//////////////////////////////////////////////////////////////////////////
#define CRY_GEOMETRY_FILE_EXT                    "cgf"
#define CRY_SKEL_FILE_EXT                        "chr" //will be a SKEL soon
#define CRY_SKIN_FILE_EXT                        "skin"
#define CRY_CHARACTER_ANIMATION_FILE_EXT         "caf"
#define CRY_CHARACTER_DEFINITION_FILE_EXT        "cdf"
#define CRY_CHARACTER_LIST_FILE_EXT              "cid"
#define CRY_ANIM_GEOMETRY_FILE_EXT               "cga"
#define CRY_ANIM_GEOMETRY_ANIMATION_FILE_EXT     "anm"
#define CRY_COMPILED_FILE_EXT                    "(c)"
#define CRY_BINARY_XML_FILE_EXT                  "binxml"
#define CRY_XML_FILE_EXT                         "xml"
#define CRY_CHARACTER_PARAM_FILE_EXT             "chrparams"
#define CRY_GEOM_CACHE_FILE_EXT                  "cax"
//////////////////////////////////////////////////////////////////////////
#define CRYFILE_MAX_PATH                         260
//////////////////////////////////////////////////////////////////////////

inline const char* CryGetExt(const char* filepath)
{
    const char* str = filepath;
    size_t len = strlen(filepath);
    for (const char* p = str + len - 1; p >= str; --p)
    {
        switch (*p)
        {
        case ':':
        case '/':
        case '\\':
            // we've reached a path separator - it means there's no extension in this name
            return "";
        case '.':
            // there's an extension in this file name
            return p + 1;
        }
    }
    return "";
}

// Summary:
//   Checks if specified file name is a character file.
// Summary:
//   Checks if specified file name is a character file.
inline bool IsCharacterFile(const char* filename)
{
    const char* ext = CryGetExt(filename);
    if (_stricmp(ext, CRY_SKEL_FILE_EXT) == 0 || _stricmp(ext, CRY_SKIN_FILE_EXT) == 0 || _stricmp(ext, CRY_CHARACTER_DEFINITION_FILE_EXT) == 0 || _stricmp(ext, CRY_ANIM_GEOMETRY_FILE_EXT) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Description:
//   Checks if specified file name is a static geometry file.
inline bool IsStatObjFile(const char* filename)
{
    const char* ext = CryGetExt(filename);
    if (_stricmp(ext, CRY_GEOMETRY_FILE_EXT) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Summary:
//   Wrapper on file system.
class CCryFile
{
public:
    CCryFile();
    CCryFile(AZ::IO::IArchive* pIArchive);    // allow an alternative IArchiveinterface
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
    //   Template version, for automatic size support.
    template<class T>
    inline size_t ReadTypeRaw(T* pDest, size_t nCount = 1)
    {
        return ReadRaw(pDest, sizeof(T) * nCount);
    }

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
    // Summary:
    //   Moves the current file pointer at the beginning of the file.
    void SeekToBegin();
    // Summary:
    //   Moves the current file pointer at the end of the file.
    size_t SeekToEnd();
    // Summary:
    //   Retrieves the current file pointer.
    size_t GetPosition();

    // Summary:
    //   Tests for end-of-file on a selected file.
    bool IsEof();

    // Summary:
    //   Flushes any data yet to be written.
    void Flush();

    // Summary:
    //   Gets a handle to a pack object.
    AZ::IO::HandleType GetHandle() const { return m_fileHandle; };

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

inline CCryFile::CCryFile(AZ::IO::IArchive* pIArchive)
{
    m_fileHandle = AZ::IO::InvalidHandle;
    m_pIArchive = pIArchive;
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
    cry_strcpy(tempfilename, filename);

#if !defined (_RELEASE)
    if (gEnv && gEnv->IsEditor() && gEnv->pConsole)
    {
        ICVar* const pCvar = gEnv->pConsole->GetCVar("ed_lowercasepaths");
        if (pCvar)
        {
            const int lowercasePaths = pCvar->GetIVal();
            if (lowercasePaths)
            {
                const string lowerString = PathUtil::ToLower(tempfilename);
                cry_strcpy(tempfilename, lowerString.c_str());
            }
        }
    }
#endif
    if (m_fileHandle != AZ::IO::InvalidHandle)
    {
        Close();
    }
    cry_strcpy(m_filename, tempfilename);

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
inline void CCryFile::SeekToBegin()
{
    Seek(0, SEEK_SET);
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::SeekToEnd()
{
    return Seek(0, SEEK_END);
}

//////////////////////////////////////////////////////////////////////////
inline size_t CCryFile::GetPosition()
{
    assert(m_fileHandle != AZ::IO::InvalidHandle);
    if (m_pIArchive)
    {
        return m_pIArchive->FTell(m_fileHandle);
    }

    AZ::u64 tellOffset = 0;
    AZ::IO::FileIOBase::GetInstance()->Tell(m_fileHandle, tellOffset);

    return static_cast<size_t>(tellOffset);
}

//////////////////////////////////////////////////////////////////////////
inline bool CCryFile::IsEof()
{
    assert(m_fileHandle != AZ::IO::InvalidHandle);
    if (m_pIArchive)
    {
        return m_pIArchive->FEof(m_fileHandle) != 0;
    }
    
    return AZ::IO::FileIOBase::GetInstance()->Eof(m_fileHandle);
}

//////////////////////////////////////////////////////////////////////////
inline void CCryFile::Flush()
{
    assert(m_fileHandle != AZ::IO::InvalidHandle);

    if (m_pIArchive)
    {
        m_pIArchive->FFlush(m_fileHandle);
    }
    
    AZ::IO::FileIOBase::GetInstance()->Flush(m_fileHandle);
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
        cry_strcpy(szAdjustedFile, gameUrl);
    }
    return szAdjustedFile;
}

#endif // CRYINCLUDE_CRYCOMMON_CRYFILE_H
