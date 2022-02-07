/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzFramework/Archive/INestedArchive.h>
#include <AzFramework/Archive/IArchive.h>
#include <AzFramework/Archive/ZipDirCache.h>

namespace AZ::IO
{
    struct NestedArchiveSortByName
    {
        bool operator()(const INestedArchive* left, const INestedArchive* right) const
        {
            return left->GetFullPath() < right->GetFullPath();
        }
        bool operator()(AZStd::string_view left, const INestedArchive* right) const
        {
            return AZ::IO::PathView(left) < right->GetFullPath();
        }
        bool operator()(const INestedArchive* left, AZStd::string_view right) const
        {
            return left->GetFullPath() < AZ::IO::PathView(right);
        }
    };

    class NestedArchive
        : public INestedArchive
    {
    public:
        AZ_CLASS_ALLOCATOR(NestedArchive, AZ::SystemAllocator, 0);

        NestedArchive(IArchive* pArchive, AZStd::string_view strBindRoot, ZipDir::CachePtr pCache, uint32_t nFlags = 0);
        ~NestedArchive() override;

        auto GetRootFolderHandle() -> Handle override;

        // Adds a new file to the zip or update an existing one
        // adds a directory (creates several nested directories if needed)
        // compression methods supported are 0 (store) and 8 (deflate) , compression level is 0..9 or -1 for default (like in zlib)
        int UpdateFile(AZStd::string_view szRelativePath, const void* pUncompressed, uint64_t nSize, uint32_t nCompressionMethod = ZipFile::METHOD_STORE,
            int nCompressionLevel = -1, CompressionCodec::Codec codec = CompressionCodec::Codec::ZLIB) override;

        // Adds a new file to the zip or update an existing one if it is not compressed - just stored  - start a big file
        int StartContinuousFileUpdate(AZStd::string_view szRelativePath, uint64_t nSize) override;

        // Adds a new file to the zip or update an existing segment if it is not compressed - just stored
        // adds a directory (creates several nested directories if needed)
        // Arguments:
        // nOverwriteSeekPos - std::numeric_limits<uint64_t>::max(i.e uint64_t(-1)) means the seek pos should not be overwritten
        int UpdateFileContinuousSegment(AZStd::string_view szRelativePath, uint64_t nSize, const void* pUncompressed, uint64_t nSegmentSize, uint64_t nOverwriteSeekPos) override;

        int UpdateFileCRC(AZStd::string_view szRelativePath, AZ::Crc32 dwCRC) override;

        // deletes the file from the archive
        int RemoveFile(AZStd::string_view szRelativePath) override;

        // deletes the directory, with all its descendants (files and subdirectories)
        int RemoveDir(AZStd::string_view szRelativePath) override;
        
        // deletes all files from the archive
        int RemoveAll() override;

        // lists all the files in the archive
        int ListAllFiles(AZStd::vector<AZ::IO::Path>& outFileEntries) override;

        // finds the file; you don't have to close the returned handle
        Handle FindFile(AZStd::string_view szRelativePath) override;

        // returns the size of the file (unpacked) by the handle
        uint64_t GetFileSize(Handle fileHandle) override;

        // reads the file into the preallocated buffer (must be at least the size of GetFileSize())
        int ReadFile(Handle fileHandle, void* pBuffer) override;

        // returns the full path to the archive file
        AZ::IO::PathView GetFullPath() const override;

        uint32_t GetFlags() const override;
        bool SetFlags(uint32_t nFlagsToSet) override;
        bool ResetFlags(uint32_t nFlagsToReset) override;

        bool SetPackAccessible(bool bAccessible) override;

        ZipDir::Cache* GetCache();

    protected:
        // returns the pointer to the relative file path to be passed
        // to the underlying Cache pointer. Uses the given buffer to construct the path.
        // returns nullptr if the file path is invalid
        AZ::IO::FixedMaxPathString AdjustPath(AZStd::string_view szRelativePath);


        ZipDir::CachePtr m_pCache;
        // the binding root may be empty string - in this case, the absolute path binding won't work
        AZ::IO::Path m_strBindRoot;
        IArchive* m_archive{};
        uint32_t m_nFlags{};
    };

}
