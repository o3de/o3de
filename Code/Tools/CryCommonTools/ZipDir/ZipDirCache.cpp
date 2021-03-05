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
// Original file Copyright Crytek GMBH or its affiliates, used under license.


#include <smartptr.h>
#include "FileUtil.h"
#include "ZipFileFormat.h"
#include "zipdirstructures.h"
#include "ZipDirTree.h"
#include "ZipDirCache.h"
#include "ZipDirFind.h"
#include "ZipDirCacheFactory.h"
#include <zlib.h>
#include <AzCore/IO/SystemFile.h>
#include "PathHelpers.h"
#include <ZipDir/ZipDir_Traits_Platform.h>

using namespace ZipFile;

// initializes the instance structure
void ZipDir::Cache::Construct(FILE* fNew, size_t nDataSizeIn, const EncryptionKey& key)
{
    m_nRefCount = 0;
    m_pFile = fNew;
    m_nDataSize = nDataSizeIn;
    m_nZipPathOffset = nDataSizeIn;
    m_bEncryptHeaders = false;
    m_encryptionKey = key;
}

// self-destruct when ref count drops to 0
void ZipDir::Cache::Delete()
{
    if (m_pFile)
    {
        fclose (m_pFile);
    }
    free(this);
}

// looks for the given file record in the Central Directory. If there's none, returns NULL.
// if there is some, returns the pointer to it.
// the Path must be the relative path to the file inside the Zip
// if the file handle is passed, it will be used to find the file data offset, if one hasn't been initialized yet
ZipDir::FileEntry* ZipDir::Cache::FindFile (const char* szPath, [[maybe_unused]] bool bRefresh)
{
    ZipDir::FindFile fd (this);
    if (!fd.FindExact(szPath))
    {
        assert (!fd.GetFileEntry());
        return NULL;
    }
    assert (fd.GetFileEntry());
    return fd.GetFileEntry();
}

// loads the given file into the pCompressed buffer (the actual compressed data)
// if the pUncompressed buffer is supplied, uncompresses the data there
// buffers must have enough memory allocated, according to the info in the FileEntry
// NOTE: there's no need to decompress if the method is 0 (store)
// returns 0 if successful or error code if couldn't do something
ZipDir::ErrorEnum ZipDir::Cache::ReadFile (FileEntry* pFileEntry, void* pCompressed, void* pUncompressed)
{
    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_CALL;
    }

    if (pFileEntry->desc.lSizeUncompressed == 0)
    {
        assert (pFileEntry->desc.lSizeCompressed == 0);
        return ZD_ERROR_SUCCESS;
    }

    assert (pFileEntry->desc.lSizeCompressed > 0);

    ErrorEnum nError = Refresh(pFileEntry);
    if (nError != ZD_ERROR_SUCCESS)
    {
        return nError;
    }

    if (AZ_TRAIT_CRYCOMMONTOOLS_FSEEK(m_pFile, pFileEntry->nFileDataOffset, SEEK_SET))
    {
        return ZD_ERROR_IO_FAILED;
    }

    SmartPtr pBufferDestroyer;

    void* pBuffer = pCompressed; // the buffer where the compressed data will go

    if (pFileEntry->nMethod == 0 && pUncompressed)
    {
        // we can directly read into the uncompress buffer
        pBuffer = pUncompressed;
    }

    if (!pBuffer)
    {
        if (!pUncompressed)
        {
            // what's the sense of it - no buffers at all?
            return ZD_ERROR_INVALID_CALL;
        }

        pBuffer = malloc(pFileEntry->desc.lSizeCompressed);
        pBufferDestroyer.Attach(pBuffer); // we want it auto-freed once we return
    }


    if (fread (pBuffer, pFileEntry->desc.lSizeCompressed, 1, m_pFile) != 1)
    {
        return ZD_ERROR_IO_FAILED;
    }

    if (pFileEntry->nMethod == METHOD_DEFLATE_AND_ENCRYPT)
    {
        ZipDir::Decrypt((char*)pBuffer, pFileEntry->desc.lSizeCompressed, m_encryptionKey);
    }

    // if there's a buffer for uncompressed data, uncompress it to that buffer
    if (pUncompressed)
    {
        if (pFileEntry->nMethod == 0)
        {
            assert (pBuffer == pUncompressed);
            //assert (pFileEntry->desc.lSizeCompressed == pFileEntry->nSizeUncompressed);
            //memcpy (pUncompressed, pBuffer, pFileEntry->desc.lSizeCompressed);
        }
        else
        {
            unsigned long nSizeUncompressed = pFileEntry->desc.lSizeUncompressed;
            if (Z_OK != ZipRawUncompress(pUncompressed, &nSizeUncompressed, pBuffer, pFileEntry->desc.lSizeCompressed))
            {
                return ZD_ERROR_CORRUPTED_DATA;
            }
        }
    }

    return ZD_ERROR_SUCCESS;
}

// loads and unpacks the file into a newly created buffer (that must be subsequently freed with
// Free()) Returns NULL if failed
void* ZipDir::Cache::AllocAndReadFile (FileEntry* pFileEntry)
{
    if (!pFileEntry)
    {
        return NULL;
    }

    void* pData = malloc(pFileEntry->desc.lSizeUncompressed);
    if (pData)
    {
        if (ZD_ERROR_SUCCESS != ReadFile (pFileEntry, NULL, pData))
        {
            free(pData);
            pData = NULL;
        }
    }
    return pData;
}

// frees the memory block that was previously allocated by AllocAndReadFile
void ZipDir::Cache::Free (void* pData)
{
    free(pData);
}

// refreshes information about the given file entry into this file entry
ZipDir::ErrorEnum ZipDir::Cache::Refresh (FileEntry* pFileEntry)
{
    if (!pFileEntry)
    {
        return ZD_ERROR_INVALID_CALL;
    }

    if (pFileEntry->nFileDataOffset != pFileEntry->INVALID_DATA_OFFSET)
    {
        return ZD_ERROR_SUCCESS; // the data offset has been successfully read..
    }

    return ZipDir::Refresh(m_pFile, pFileEntry, m_bEncryptHeaders);
}

//////////////////////////////////////////////////////////////////////////
uint32 ZipDir::Cache::GetFileDataOffset(FileEntry* pFileEntry)
{
    if (pFileEntry->nFileDataOffset == pFileEntry->INVALID_DATA_OFFSET)
    {
        ZipDir::Refresh (m_pFile, pFileEntry, m_bEncryptHeaders);
    }
    return pFileEntry->nFileDataOffset;
}

// returns the size of memory occupied by the instance referred to by this cache
// must be exact, because it's used by CacheRW to reallocate this cache
size_t ZipDir::Cache::GetSize() const
{
    return m_nDataSize + sizeof(Cache) + strlen(GetFilePath());
}


// QUICK check to determine whether the file entry belongs to this object
bool ZipDir::Cache::IsOwnerOf (const FileEntry* pFileEntry) const
{
    // just check whether the pointer is within the memory block of this cache instance
    return ((ULONG_PTR)pFileEntry >= (ULONG_PTR)(GetRoot() + 1)
                && (ULONG_PTR)pFileEntry <= ((ULONG_PTR)GetRoot()) + m_nDataSize - sizeof(FileEntry));
}

bool ZipDir::Cache::UnpakToDisk(const string& destFolder)
{
    return UnpakToDiskInternal(GetRoot(), destFolder);
}

bool ZipDir::Cache::UnpakToDiskInternal(ZipDir::DirHeader* folder, const string& destFolder)
{
    if (!folder)
    {
        return false;
    }

    if (!FileUtil::EnsureDirectoryExists(destFolder.c_str()))
    {
        return false;
    }

    bool result = true;
    for (ZipFile::ushort fileNum = 0; fileNum < folder->numFiles; ++fileNum)
    {
        ZipDir::FileEntry* fileEntry = folder->GetFileEntry(fileNum);
        if (!fileEntry)
        {
            result = false;
            continue;
        }

        string filePath = PathHelpers::Join(destFolder, fileEntry->GetName(folder->GetNamePool()));
        AZ::IO::SystemFile file;
        if (!file.Open(filePath.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_WRITE | AZ::IO::SystemFile::SF_OPEN_CREATE))
        {
            result = false;
            continue;
        }

        if (!fileEntry->desc.lSizeUncompressed)
        {
            // Nothing to write. Just close the file.
            file.Close();
            continue;
        }

        AZStd::vector<AZ::u8> buffer(fileEntry->desc.lSizeUncompressed);
        if (ReadFile(fileEntry, nullptr, buffer.data()) == ZD_ERROR_SUCCESS)
        {
            file.Write(buffer.data(), buffer.size());
            file.Close();
        }
        else
        {
            file.Close();
            AZ::IO::SystemFile::Delete(filePath.c_str());
            result = false;
            continue;
        }
    }

    for (ZipFile::ushort dirNum = 0; dirNum < folder->numDirs; ++dirNum)
    {
        ZipDir::DirEntry* entry = folder->GetSubdirEntry(dirNum);
        if (!entry)
        {
            result = false;
            continue;
        }

        string newPath = PathHelpers::Join(destFolder, entry->GetName(folder->GetNamePool()));
        if (!UnpakToDiskInternal(entry->GetDirectory(), newPath))
        {
            result = false;
            continue;
        }
    }

    return result;
}

