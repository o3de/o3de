/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Declaration of the class that will keep the ZipDir Cache object
// and will provide all its services to access Zip file, plus it will
// provide services to write to the zip file efficiently
// Time to time, the contained Cache object will be recreated during
// an archive add operation
//
#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzFramework/Archive/Codec.h>
#include <AzFramework/Archive/ZipDirStructures.h>
#include <AzFramework/Archive/ZipDirTree.h>

namespace AZ::IO::ZipDir
{
    struct FileDataRecord;

    class Cache
        : public AZStd::intrusive_base
    {
    public:
        AZ_CLASS_ALLOCATOR(Cache, AZ::SystemAllocator, 0);
        // the size of the buffer that's using during re-linking the zip file
        inline static constexpr size_t g_nSizeRelinkBuffer = 1024 * 1024;
        inline static constexpr size_t g_nMaxItemsRelinkBuffer = 128; // max number of files to read before (without) writing

        inline static constexpr int compressedBlockHeaderSizeInBytes = 4; //number of bytes we need in front of the compressed block to indicate which compressor was used

        Cache();
        explicit Cache(AZ::IAllocatorAllocate* allocator);

        ~Cache()
        {
            Close();
        }

        bool IsValid() const
        {
            return m_fileHandle != AZ::IO::InvalidHandle;
        }

        // Adds a new file to the zip or update an existing one
        // adds a directory (creates several nested directories if needed)
        ErrorEnum UpdateFile(AZStd::string_view szRelativePath, const void* pUncompressed, uint64_t nSize, uint32_t nCompressionMethod = ZipFile::METHOD_STORE, int nCompressionLevel = -1, CompressionCodec::Codec codec = CompressionCodec::Codec::ZLIB);

        //   Adds a new file to the zip or update an existing one if it is not compressed - just stored  - start a big file
        ErrorEnum StartContinuousFileUpdate(AZStd::string_view szRelativePath, uint64_t nSize);

        // Adds a new file to the zip or update an existing segment if it is not compressed - just stored
        // adds a directory (creates several nested directories if needed)
        // Arguments:
        // nOverwriteSeekPos - std::numeric_limits<uint64_t>::max(i.e uint64_t(-1)) means the seek pos should not be overwritten
        ErrorEnum UpdateFileContinuousSegment(AZStd::string_view szRelativePath, uint64_t nSize, const void* pUncompressed, uint64_t nSegmentSize, uint64_t nOverwriteSeekPos);

        ErrorEnum UpdateFileCRC(AZStd::string_view szRelativePath, AZ::Crc32 dwCRC32);

        // deletes the file from the archive
        ErrorEnum RemoveFile(AZStd::string_view szRelativePath);

        // deletes the directory, with all its descendants (files and subdirs)
        ErrorEnum RemoveDir(AZStd::string_view szRelativePath);

        // deletes all files and directories in this archive
        ErrorEnum RemoveAll();

        // closes the current zip file
        void Close();

        FileEntry* FindFile(AZStd::string_view szPath, bool bFullInfo = false);

        ErrorEnum ReadFile(FileEntry* pFileEntry, void* pCompressed, void* pUncompressed);

        void Free(void* ptr)
        {
            m_allocator->DeAllocate(ptr);
        }

        // refreshes information about the given file entry into this file entry
        ErrorEnum Refresh(FileEntryBase* pFileEntry);

        // QUICK check to determine whether the file entry belongs to this object
        bool IsOwnerOf(const FileEntry* pFileEntry) const
        {
            return m_treeDir.IsOwnerOf(pFileEntry);
        }

        // returns the string - path to the zip file from which this object was constructed.
        // this will be "" if the object was constructed with a factory that wasn't created with FLAGS_MEMORIZE_ZIP_PATH
        AZ::IO::PathView GetFilePath() const
        {
            return m_strFilePath;
        }

        FileEntryTree* GetRoot()
        {
            return &m_treeDir;
        }

        // writes the CDR to the disk
        bool WriteCDR() { return WriteCDR(m_fileHandle); }
        bool WriteCDR(AZ::IO::HandleType fTarget);

        bool RelinkZip();
    protected:
        bool RelinkZip(AZ::IO::HandleType fTmp);
        // writes out the file data in the queue into the given file. Empties the queue
        bool WriteZipFiles(AZStd::vector<AZStd::intrusive_ptr<FileDataRecord>>& queFiles, AZ::IO::HandleType fTmp);

        bool WriteCompressedData(uint8_t* data, size_t size, bool encrypt);
        bool WriteNullData(size_t size);

        ZipFile::CryCustomEncryptionHeader& GetEncryptionHeader() { return m_headerEncryption; }
        ZipFile::CrySignedCDRHeader& GetSignedHeader() { return m_headerSignature; }
        ZipFile::CryCustomExtendedHeader& GetExtendedHeader() { return m_headerExtended; }

        size_t GetCompressedSizeEstimate(size_t uncompressedSize, CompressionCodec::Codec codec);

    protected:
        friend class CacheFactory;
        friend class FileEntryTransactionAdd;
        FileEntryTree m_treeDir;
        AZ::IO::HandleType m_fileHandle;
        AZ::IAllocatorAllocate* m_allocator;
        AZ::IO::Path m_strFilePath;

        // String Pool for persistently storing paths as long as they reside in the cache
        AZStd::unordered_set<AZ::IO::Path> m_relativePathPool;

        // offset to the start of CDR in the file,even if there's no CDR there currently
        // when a new file is added, it can start from here, but this value will need to be updated then
        uint32_t m_lCDROffset;

        enum
        {
            // if this is set, the file needs to be compacted before it can be used by
            // all standard zip tools, because gaps between file datas can be present
            FLAGS_UNCOMPACTED = 1 << 0,
            // if this is set, the CDR needs to be written to the file
            FLAGS_CDR_DIRTY = 1 << 1,
            // if this is set, the file is opened in read-only mode. no write operations are to be performed
            FLAGS_READ_ONLY = 1 << 2,
            // when this is set, compact operation is not performed
            FLAGS_DONT_COMPACT = 1 << 3
        };
        uint32_t m_nFlags;

        // CDR buffer.
        AZStd::vector<uint8_t> m_CDR_buffer;

        ZipFile::EHeaderEncryptionType m_encryptedHeaders;
        ZipFile::EHeaderSignatureType m_signedHeaders;

        // Zip Headers
        ZipFile::CryCustomEncryptionHeader m_headerEncryption;
        ZipFile::CrySignedCDRHeader m_headerSignature;
        ZipFile::CryCustomExtendedHeader m_headerExtended;
    };

    using CachePtr = AZStd::intrusive_ptr<Cache>;
}

