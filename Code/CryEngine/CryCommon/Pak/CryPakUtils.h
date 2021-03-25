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

#include <AzFramework/Archive/IArchive.h>

//! Everybody should use fxopen instead of fopen so it opens on all platforms
inline AZ::IO::HandleType fxopen(const char* file, const char* mode, bool bGameRelativePath = false)
{
    if (gEnv && gEnv->pCryPak)
    {
        gEnv->pCryPak->CheckFileAccessDisabled(file, mode);
}
    bool bWriteAccess = false;
    for (const char* s = mode; *s; s++)
    {
        if (*s == 'w' || *s == 'W' || *s == 'a' || *s == 'A' || *s == '+')
        {
            bWriteAccess = true;
            break;
        }
        ;
    }

    if (gEnv && gEnv->pCryPak)
    {
        int nAdjustFlags = 0;
        if (!bGameRelativePath)
        {
            nAdjustFlags |= AZ::IO::IArchive::FLAGS_PATH_REAL;
        }
        if (bWriteAccess)
        {
            nAdjustFlags |= AZ::IO::IArchive::FLAGS_FOR_WRITING;
        }
        char path[_MAX_PATH];
        const char* szAdjustedPath = gEnv->pCryPak->AdjustFileName(file, path, AZ_ARRAY_SIZE(path), nAdjustFlags);

#if !AZ_TRAIT_LEGACY_CRYPAK_UNIX_LIKE_FILE_SYSTEM
        if (bWriteAccess)
        {
            // Make sure folder is created.
            gEnv->pCryPak->MakeDir(szAdjustedPath);
        }
#endif
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::FileIOBase::GetInstance()->Open(szAdjustedPath, AZ::IO::GetOpenModeFromStringMode(mode), fileHandle);
        return fileHandle;
    }
    else
    {
        return AZ::IO::InvalidHandle;
    }
}

class CDebugAllowFileAccess
{
public:
#if defined(_RELEASE)
    ILINE CDebugAllowFileAccess() { }
    ILINE void End() { }
#else
    CDebugAllowFileAccess()
    {
        m_threadId = AZStd::this_thread::get_id();
        m_oldDisable = gEnv->pCryPak ? gEnv->pCryPak->DisableRuntimeFileAccess(false, m_threadId) : false;
        m_active = true;
    }
    ~CDebugAllowFileAccess()
    {
        End();
    }
    void End()
    {
        if (m_active)
        {
            if (gEnv && gEnv->pCryPak)
            {
                gEnv->pCryPak->DisableRuntimeFileAccess(m_oldDisable, m_threadId);
            }
            m_active = false;
        }
    }
protected:
    AZStd::thread_id m_threadId;
    bool        m_oldDisable;
    bool        m_active;
#endif
};

//////////////////////////////////////////////////////////////////////////


class CInMemoryFileLoader
{
public:
    CInMemoryFileLoader(AZ::IO::IArchive* pCryPak)
        : m_pPak(pCryPak)
        , m_fileHandle(AZ::IO::InvalidHandle)
        , m_pBuffer(0)
        , m_pCursor(0)
        , m_nFileSize(0) {}
    ~CInMemoryFileLoader()
    {
        Close();
    }

    bool IsFileExists() const
    {
        return m_fileHandle != AZ::IO::InvalidHandle;
    }

    AZ::IO::HandleType GetFileHandle() const
    {
        return m_fileHandle;
    }

    bool FOpen(const char* name, const char* mode, bool bImmediateCloseFile = false)
    {
        if (m_pPak)
        {
            assert(m_fileHandle == AZ::IO::InvalidHandle);
            m_fileHandle = m_pPak->FOpen(name, mode);
            if (m_fileHandle == AZ::IO::InvalidHandle)
            {
                return false;
            }

            m_nFileSize = m_pPak->FGetSize(m_fileHandle);
            if (m_nFileSize == 0)
            {
                Close();
                return false;
            }

            m_pCursor = m_pBuffer = (char*)m_pPak->PoolMalloc(m_nFileSize);

            size_t nReaded = m_pPak->FReadRawAll(m_pBuffer, m_nFileSize, m_fileHandle);
            if (nReaded != m_nFileSize)
            {
                Close();
                return false;
            }

            if (bImmediateCloseFile)
            {
                m_pPak->FClose(m_fileHandle);
                m_fileHandle = AZ::IO::InvalidHandle;
            }

            return true;
        }

        return false;
    }

    void FClose()
    {
        Close();
    }

    size_t FReadRaw(void* data, size_t length, size_t elems)
    {
        ptrdiff_t dist = m_pCursor - m_pBuffer;

        size_t count = length;
        if (dist + count * elems > m_nFileSize)
        {
            count = (m_nFileSize - dist) / elems;
        }

        memmove(data, m_pCursor, count * elems);
        m_pCursor += count * elems;

        return count;
    }

    template<class T>
    size_t FRead(T* data, size_t elems, bool bSwapEndian = eLittleEndian)
    {
        ptrdiff_t dist = m_pCursor - m_pBuffer;

        size_t count = elems;
        if (dist + count * sizeof(T) > m_nFileSize)
        {
            count = (m_nFileSize - dist) / sizeof(T);
        }

        memmove(data, m_pCursor, count * sizeof(T));
        m_pCursor += count * sizeof(T);

        SwapEndian(data, count, bSwapEndian);
        return count;
    }

    size_t FTell()
    {
        ptrdiff_t dist = m_pCursor - m_pBuffer;
        return dist;
    }

    int FSeek(int64_t origin, int command)
    {
        int retCode = -1;
        int64_t newPos;
        char* newPosBuf;
        switch (command)
        {
        case SEEK_SET:
            newPos = origin;
            if (newPos <= (int64_t)m_nFileSize)
            {
                m_pCursor = m_pBuffer + newPos;
                retCode = 0;
            }
            break;
        case SEEK_CUR:
            newPosBuf = m_pCursor + origin;
            if (newPosBuf <= m_pBuffer + m_nFileSize)
            {
                m_pCursor = newPosBuf;
                retCode = 0;
            }
            break;
        case SEEK_END:
            newPos = m_nFileSize - origin;
            if (newPos <= (int64_t)m_nFileSize)
            {
                m_pCursor = m_pBuffer + newPos;
                retCode = 0;
            }
            break;
        default:
            // Not valid disk operation!
            AZ_Assert(false, "Invalid disk operation");
        }
        return retCode;
    }


private:

    void Close()
    {
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            m_pPak->FClose(m_fileHandle);
        }

        if (m_pBuffer)
        {
            m_pPak->PoolFree(m_pBuffer);
        }

        m_pBuffer = m_pCursor = 0;
        m_nFileSize = 0;
        m_fileHandle = AZ::IO::InvalidHandle;
    }

private:
    AZ::IO::HandleType m_fileHandle;
    char* m_pBuffer;
    AZ::IO::IArchive* m_pPak;
    char* m_pCursor;
    size_t m_nFileSize;
};


//////////////////////////////////////////////////////////////////////////
// Helper class that can be used to recursively scan the directory.
//////////////////////////////////////////////////////////////////////////
struct SDirectoryEnumeratorHelper
{
public:
    void ScanDirectoryRecursive(AZ::IO::IArchive* pIPak, const AZStd::string& root, const AZStd::string& pathIn, const AZStd::string& fileSpec, AZStd::vector<AZStd::string>& files)
    {
        auto AddSlash = [](AZStd::string_view path) -> AZStd::string
        {
            if (path.ends_with(AZ_CORRECT_DATABASE_SEPARATOR))
            {
                return path;
            }
            else if (path.ends_with(AZ_WRONG_DATABASE_SEPARATOR))
            {
                return AZStd::string{ path.substr(0, path.size() - 1) } + AZ_CORRECT_DATABASE_SEPARATOR;
            }
            return path.empty() ? AZStd::string(path) : AZStd::string(path) + AZ_CORRECT_DATABASE_SEPARATOR;
        };
        AZStd::string dir;
        AZ::StringFunc::Path::Join(root.c_str(), pathIn.c_str(), dir);
        dir = AddSlash(dir);

        ScanDirectoryFiles(pIPak, "", dir, fileSpec, files);

        AZStd::string findFilter;
        AZ::StringFunc::Path::Join(dir.c_str(), "*", findFilter);

        // Add all directories.

        AZ::IO::ArchiveFileIterator pakFileIterator = pIPak->FindFirst(findFilter.c_str());
        if (pakFileIterator)
        {
            do
            {
                // Skip back folders.
                if (pakFileIterator.m_filename[0] == '.')
                {
                    continue;
                }
                if (pakFileIterator.m_filename.empty())
                {
                    AZ_Fatal("Archive", "IArchive FindFirst/FindNext returned empty name while looking for '%s'", findFilter.c_str());
                    continue;
                }
                if ((pakFileIterator.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory) // skip sub directories.
                {
                    AZStd::string scanDir = AZStd::string::format("%s%.*s/", AddSlash(pathIn).c_str(), aznumeric_cast<int>(pakFileIterator.m_filename.size()), pakFileIterator.m_filename.data());
                    scanDir += AZ_CORRECT_DATABASE_SEPARATOR;
                    ScanDirectoryRecursive(pIPak, root, scanDir, fileSpec, files);
                    continue;
                }
            } while (pakFileIterator = pIPak->FindNext(pakFileIterator));
            pIPak->FindClose(pakFileIterator);
        }
    }
private:
    void ScanDirectoryFiles(AZ::IO::IArchive* pIPak, const AZStd::string& root, const AZStd::string& path, const AZStd::string& fileSpec, AZStd::vector<AZStd::string>& files)
    {
        AZStd::string dir;
        AZ::StringFunc::Path::Join(root.c_str(), path.c_str(), dir);

        AZStd::string findFilter;
        AZ::StringFunc::Path::Join(dir.c_str(), fileSpec.c_str(), findFilter);

        AZ::IO::ArchiveFileIterator pakFileIterator = pIPak->FindFirst(findFilter.c_str());
        if (pakFileIterator)
        {
            do
            {
                // Skip back folders and subdirectories.
                if (pakFileIterator.m_filename[0] == '.' || (pakFileIterator.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
                {
                    continue;
                }
                AZStd::string fullPath;
                AZ::StringFunc::Path::Join(path.c_str(), AZStd::string(pakFileIterator.m_filename).c_str(), fullPath);
                files.push_back(fullPath);
            } while (pakFileIterator = pIPak->FindNext(pakFileIterator));
            pIPak->FindClose(pakFileIterator);
        }
    }


};
