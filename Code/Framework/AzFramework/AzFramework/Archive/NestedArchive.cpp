/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzFramework/Archive/NestedArchive.h>
#include <AzFramework/Archive/ZipDirStructures.h>
#include <AzFramework/Archive/ZipDirTree.h>
#include <AzFramework/Archive/Archive.h>

namespace AZ::IO
{
    NestedArchive::NestedArchive(IArchive* pArchive, AZStd::string_view strBindRoot, ZipDir::CachePtr pCache, uint32_t nFlags)
        : m_archive{ pArchive }
        , m_pCache{ pCache }
        , m_strBindRoot{ strBindRoot }
        , m_nFlags{ nFlags }
    {
        static_cast<Archive*>(m_archive)->Register(this);
    }

    NestedArchive::~NestedArchive()
    {
        static_cast<Archive*>(m_archive)->Unregister(this);
    }


    auto NestedArchive::GetRootFolderHandle() -> Handle
    {
        return m_pCache->GetRoot();
    }

    int NestedArchive::UpdateFileCRC(AZStd::string_view szRelativePath, AZ::Crc32 dwCRC)
    {
        if (m_nFlags & FLAGS_READ_ONLY)
        {
            return ZipDir::ZD_ERROR_INVALID_CALL;
        }

        AZStd::fixed_string<AZ::IO::MaxPathLength> fullPath = AdjustPath(szRelativePath);
        if (fullPath.empty())
        {
            return ZipDir::ZD_ERROR_INVALID_PATH;
        }

        m_pCache->UpdateFileCRC(fullPath, dwCRC);

        return ZipDir::ZD_ERROR_SUCCESS;
    }



    //////////////////////////////////////////////////////////////////////////
    // deletes the file from the archive
    int NestedArchive::RemoveFile(AZStd::string_view szRelativePath)
    {
        if (m_nFlags & FLAGS_READ_ONLY)
        {
            return ZipDir::ZD_ERROR_INVALID_CALL;
        }

        AZStd::fixed_string<AZ::IO::MaxPathLength> fullPath = AdjustPath(szRelativePath);
        if (fullPath.empty())
        {
            return ZipDir::ZD_ERROR_INVALID_PATH;
        }
        return m_pCache->RemoveFile(fullPath);
    }


    //////////////////////////////////////////////////////////////////////////
    // deletes the directory, with all its descendants (files and subdirs)
    int NestedArchive::RemoveDir(AZStd::string_view szRelativePath)
    {
        if (m_nFlags & FLAGS_READ_ONLY)
        {
            return ZipDir::ZD_ERROR_INVALID_CALL;
        }

        AZStd::fixed_string<AZ::IO::MaxPathLength> fullPath = AdjustPath(szRelativePath);
        if (fullPath.empty())
        {
            return ZipDir::ZD_ERROR_INVALID_PATH;
        }
        return m_pCache->RemoveDir(fullPath);
    }

    int NestedArchive::RemoveAll()
    {
        return m_pCache->RemoveAll();
    }

    //////////////////////////////////////////////////////////////////////////
    // Adds a new file to the zip or update an existing one
    // adds a directory (creates several nested directories if needed)
    // compression methods supported are 0 (store) and 8 (deflate) , compression level is 0..9 or -1 for default (like in zlib)
    int NestedArchive::UpdateFile(AZStd::string_view szRelativePath, const void* pUncompressed, uint64_t nSize, uint32_t nCompressionMethod, int nCompressionLevel, CompressionCodec::Codec codec)
    {
        if (m_nFlags & FLAGS_READ_ONLY)
        {
            return ZipDir::ZD_ERROR_INVALID_CALL;
        }

        AZStd::fixed_string<AZ::IO::MaxPathLength> fullPath = AdjustPath(szRelativePath);
        if (fullPath.empty())
        {
            return ZipDir::ZD_ERROR_INVALID_PATH;
        }
        return m_pCache->UpdateFile(fullPath, pUncompressed, nSize, nCompressionMethod, nCompressionLevel, codec);
    }

    //////////////////////////////////////////////////////////////////////////
    //   Adds a new file to the zip or update an existing one if it is not compressed - just stored  - start a big file
    int NestedArchive::StartContinuousFileUpdate(AZStd::string_view szRelativePath, uint64_t nSize)
    {
        if (m_nFlags & FLAGS_READ_ONLY)
        {
            return ZipDir::ZD_ERROR_INVALID_CALL;
        }

        AZStd::fixed_string<AZ::IO::MaxPathLength> fullPath = AdjustPath(szRelativePath);
        if (fullPath.empty())
        {
            return ZipDir::ZD_ERROR_INVALID_PATH;
        }
        return m_pCache->StartContinuousFileUpdate(fullPath, nSize);
    }


    //////////////////////////////////////////////////////////////////////////
    // Adds a new file to the zip or update an existing segment if it is not compressed - just stored
    // adds a directory (creates several nested directories if needed)
    int NestedArchive::UpdateFileContinuousSegment(AZStd::string_view szRelativePath, uint64_t nSize, const void* pUncompressed, uint64_t nSegmentSize, uint64_t nOverwriteSeekPos)
    {
        if (m_nFlags & FLAGS_READ_ONLY)
        {
            return ZipDir::ZD_ERROR_INVALID_CALL;
        }

        AZStd::fixed_string<AZ::IO::MaxPathLength> fullPath = AdjustPath(szRelativePath);
        if (fullPath.empty())
        {
            return ZipDir::ZD_ERROR_INVALID_PATH;
        }
        return m_pCache->UpdateFileContinuousSegment(fullPath, nSize, pUncompressed, nSegmentSize, nOverwriteSeekPos);
    }

    // finds the file; you don't have to close the returned handle
    auto NestedArchive::FindFile(AZStd::string_view szRelativePath) -> Handle
    {
        AZStd::fixed_string<AZ::IO::MaxPathLength> fullPath = AdjustPath(szRelativePath);
        if (fullPath.empty())
        {
            return nullptr;
        }
        return m_pCache->FindFile(fullPath);
    }

    // returns the size of the file (unpacked) by the handle
    uint64_t NestedArchive::GetFileSize(Handle fileHandle)
    {
        AZ_Assert(m_pCache->IsOwnerOf(reinterpret_cast<ZipDir::FileEntry*>(fileHandle)), "File handle is not owned by archive");
        return reinterpret_cast<ZipDir::FileEntry*>(fileHandle)->desc.lSizeUncompressed;
    }

    // reads the file into the preallocated buffer (must be at least the size of GetFileSize())
    int NestedArchive::ReadFile(Handle fileHandle, void* pBuffer)
    {
        AZ_Assert(m_pCache->IsOwnerOf(reinterpret_cast<ZipDir::FileEntry*>(fileHandle)), "File Handle is not owned by archive");
        return m_pCache->ReadFile(reinterpret_cast<ZipDir::FileEntry*>(fileHandle), nullptr, pBuffer);
    }

    const char* NestedArchive::GetFullPath() const
    {
        return m_pCache->GetFilePath();
    }

    ZipDir::Cache* NestedArchive::GetCache()
    {
        return m_pCache.get();
    }

    uint32_t NestedArchive::GetFlags() const
    {
        return m_nFlags;
    }
    bool NestedArchive::SetFlags(uint32_t nFlagsToSet)
    {
        if (nFlagsToSet & FLAGS_RELATIVE_PATHS_ONLY)
        {
            m_nFlags |= FLAGS_RELATIVE_PATHS_ONLY;
        }

        if (nFlagsToSet & FLAGS_ON_HDD)
        {
            m_nFlags |= FLAGS_ON_HDD;
        }

        if (nFlagsToSet & FLAGS_RELATIVE_PATHS_ONLY ||
            nFlagsToSet & FLAGS_ON_HDD)
        {
            // we don't support changing of any other flags
            return true;
        }
        return false;
    }

    bool NestedArchive::ResetFlags(uint32_t nFlagsToReset)
    {
        if (nFlagsToReset & FLAGS_RELATIVE_PATHS_ONLY)
        {
            m_nFlags &= ~FLAGS_RELATIVE_PATHS_ONLY;
        }

        if (nFlagsToReset & ~(FLAGS_RELATIVE_PATHS_ONLY))
        {
            // we don't support changing of any other flags
            return false;
        }
        return true;
    }

    bool NestedArchive::SetPackAccessible(bool bAccessible)
    {
        if (bAccessible)
        {
            bool bResult = (m_nFlags & INestedArchive::FLAGS_DISABLE_PAK) != 0;
            m_nFlags &= ~INestedArchive::FLAGS_DISABLE_PAK;
            return bResult;
        }
        else
        {
            bool bResult = (m_nFlags & INestedArchive::FLAGS_DISABLE_PAK) == 0;
            m_nFlags |= INestedArchive::FLAGS_DISABLE_PAK;
            return bResult;
        }
    }

    AZ::IO::FixedMaxPathString NestedArchive::AdjustPath(AZStd::string_view szRelativePath)
    {
        if (szRelativePath.empty())
        {
            return {};
        }

        if (m_nFlags & FLAGS_RELATIVE_PATHS_ONLY)
        {
            return AZStd::fixed_string<AZ::IO::MaxPathLength>{ szRelativePath };
        }

        if ((szRelativePath.size() > 1 && szRelativePath[1] == ':') || (m_nFlags & FLAGS_ABSOLUTE_PATHS))
        {
            // make the normalized full path and try to match it against the binding root of this object
            auto resolvedPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(szRelativePath);

            // Make sure the resolve path is longer than the bind root and that it starts with the bind root
            if (!resolvedPath || resolvedPath->Native().size() <= m_strBindRoot.size() || azstrnicmp(resolvedPath->c_str(), m_strBindRoot.c_str(), m_strBindRoot.size()) != 0)
            {
                return {};
            }

            // Remove the bind root prefix from the resolved path
            resolvedPath->Native().erase(0, m_strBindRoot.size() + 1);
            return resolvedPath->Native();
        }

        return AZ::IO::FixedMaxPathString{ szRelativePath };
    }
}
