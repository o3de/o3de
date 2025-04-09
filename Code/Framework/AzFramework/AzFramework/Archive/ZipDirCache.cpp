/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Console/Console.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/conversions.h>

#include <AzFramework/Archive/ZipFileFormat.h>
#include <AzFramework/Archive/ZipDirStructures.h>
#include <AzFramework/Archive/ZipDirTree.h>
#include <AzFramework/Archive/ZipDirList.h>
#include <AzFramework/Archive/ZipDirCache.h>
#include <AzFramework/Archive/ZipDirCacheFactory.h>
#include <AzFramework/Archive/ZipDirFind.h>
#include <AzFramework/IO/FileOperations.h>

#include <locale>
#include <random>
#include <cinttypes>
#include <lz4frame.h>
#include <zstd.h>
#include <zlib.h>

namespace AZ::IO::ZipDir
{
    AZ_CVAR(int32_t, az_archive_zip_directory_cache_verbosity, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Sets the verbosity level for zip directory cache operations\n"
        ">=1 - Turns on verbose logging of all operations");

    namespace ZipDirCacheInternal
    {
        [[nodiscard]] static AZStd::intrusive_ptr<AZ::IO::MemoryBlock> CreateMemoryBlock(size_t size)
        {
            AZStd::intrusive_ptr<AZ::IO::MemoryBlock> memoryBlock{ aznew AZ::IO::MemoryBlock{} };

            memoryBlock->m_address.reset(reinterpret_cast<uint8_t*>(azmalloc(size, alignof(uint8_t))));
            memoryBlock->m_size = size;

            return memoryBlock;
        }

        // generates random file name
        static AZStd::fixed_string<8> GetRandomName(int nAttempt)
        {
            if (nAttempt)
            {
                std::random_device rd;  //Will be used to obtain a seed for the random number engine
                std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
                std::uniform_int_distribution<> distrib(0, 10 + 'z' - 'a');
                char szBuf[8];
                int i;
                for (i = 0; i < AZ_ARRAY_SIZE(szBuf) - 1; ++i)
                {
                    int r = distrib(gen);
                    szBuf[i] = static_cast<char>(r > 9 ? (r - 10) + 'a' : '0' + r);
                }
                szBuf[i] = '\0';
                return szBuf;
            }
            else
            {
                return {};
            }
        }
    }

    // creates and if needed automatically destroys the file entry
    class FileEntryTransactionAdd
    {
    public:
        operator FileEntry* ()
        {
            return m_pFileEntry;
        }
        explicit operator bool() const
        {
            return m_pFileEntry != nullptr;
        }
        FileEntry* operator -> () { return m_pFileEntry; }
        FileEntryTransactionAdd(Cache* pCache, AZStd::string_view szRelativePath)
            : m_pCache(pCache)
            , m_szRelativePath(AZ::IO::PosixPathSeparator)
            , m_bCommitted(false)
        {
            // Update the cache string pool with the relative path to the file
            auto pathIt = m_pCache->m_relativePathPool.emplace(AZ::IO::PathView(szRelativePath, AZ::IO::PosixPathSeparator).LexicallyNormal());
            m_szRelativePath = *pathIt.first;
            // this is the name of the directory - create it or find it
            m_pFileEntry = m_pCache->GetRoot()->Add(m_szRelativePath.Native());
            if (m_pFileEntry && az_archive_zip_directory_cache_verbosity)
            {
                AZ_TracePrintf("Archive", R"(File "%s" has been added to archive at root "%s")", pathIt.first->c_str(), pCache->GetFilePath());
            }
        }
        ~FileEntryTransactionAdd()
        {
            if (m_pFileEntry && !m_bCommitted)
            {
                m_pCache->RemoveFile(m_szRelativePath.Native());
                m_pCache->m_relativePathPool.erase(m_szRelativePath);
            }
        }
        void Commit()
        {
            m_bCommitted = true;
        }
        AZStd::string_view GetRelativePath() const
        {
            return m_szRelativePath.Native();
        }
    private:
        Cache* m_pCache;
        AZ::IO::PathView m_szRelativePath;
        FileEntry* m_pFileEntry;
        bool m_bCommitted;
    };

    Cache::Cache() = default;

    void Cache::Close()
    {
        if (m_fileHandle != AZ::IO::InvalidHandle)
        {
            if (!(m_nFlags & FLAGS_READ_ONLY))
            {
                if ((m_nFlags & FLAGS_UNCOMPACTED) && !(m_nFlags & FLAGS_DONT_COMPACT))
                {
                    if (!RelinkZip())
                    {
                        WriteCDR();
                    }
                }
                else
                    if (m_nFlags & FLAGS_CDR_DIRTY)
                    {
                        WriteCDR();
                    }
            }

            if (m_fileHandle != AZ::IO::InvalidHandle)                      // RelinkZip() might have closed the file
            {
                AZ::IO::FileIOBase::GetDirectInstance()->Close(m_fileHandle);
                m_fileHandle = AZ::IO::InvalidHandle;
            }
        }
        m_treeDir.Clear();
    }

    bool Cache::WriteCompressedData(uint8_t* data, size_t size, bool)
    {
        if (size == 0)
        {
            return true;
        }

        // Danny - changed this because writing a single large chunk (more than 6MB?) causes
        // Windows fwrite to (silently?!) fail. If it's big then break up the write into
        // chunks
        const size_t maxChunk = 1 << 20;
        size_t sizeLeft = size;
        size_t  sizeToWrite;
        char* ptr = (char*)data;
        while ((sizeToWrite = (AZStd::min)(sizeLeft, maxChunk)) != 0)
        {
            if (!AZ::IO::FileIOBase::GetDirectInstance()->Write(m_fileHandle, ptr, sizeToWrite))
            {
                return false;
            }

            ptr += sizeToWrite;
            sizeLeft -= sizeToWrite;
        }

        return true;
    }

    bool Cache::WriteNullData(size_t size)
    {
        const size_t maxChunk = 1 << 20;
        size_t sizeLeft = size;
        size_t sizeToWrite;
        AZStd::intrusive_ptr<AZ::IO::MemoryBlock> memoryBlock = ZipDirCacheInternal::CreateMemoryBlock(maxChunk);
        if (memoryBlock)
        {
            return ZD_ERROR_IO_FAILED;
        }

        memset(memoryBlock->m_address.get(), 0, sizeof(char) * maxChunk);

        while ((sizeToWrite = (AZStd::min)(sizeLeft, maxChunk)) != 0)
        {
            if (!AZ::IO::FileIOBase::GetDirectInstance()->Write(m_fileHandle, memoryBlock->m_address.get(), sizeToWrite))
            {
                return false;
            }
            sizeLeft -= sizeToWrite;
        }

        return true;
    }

    size_t Cache::GetCompressedSizeEstimate(size_t uncompressedSize, CompressionCodec::Codec codec)
    {
        switch (codec)
        {
        case CompressionCodec::Codec::ZLIB:
            return (uncompressedSize + (uncompressedSize >> 3) + 32);
        case CompressionCodec::Codec::ZSTD:
            return ZSTD_compressBound(uncompressedSize);
        case CompressionCodec::Codec::LZ4:
            return LZ4F_compressFrameBound(uncompressedSize, nullptr);
        default:
            AZ_Assert(false, "Unknown codec passed in for size estimate");
            break;
        }
        return 0;
    }

    // Adds a new file to the zip or update an existing one
    // adds a directory (creates several nested directories if needed)
    ErrorEnum Cache::UpdateFile(AZStd::string_view szRelativePathSrc, const void* pUncompressed, uint64_t nSize, uint32_t nCompressionMethod, int nCompressionLevel, CompressionCodec::Codec codec)
    {
        AZStd::intrusive_ptr<AZ::IO::MemoryBlock> memoryBlock;

        // we'll need the compressed data
        void* pCompressed = nullptr;
        const void* dataBuffer{};
        size_t nSizeCompressed;
        int nError = Z_ERRNO;

        if (nSize == 0)
        {
            nCompressionMethod = ZipFile::METHOD_STORE;
        }
        switch (nCompressionMethod)
        {
        case ZipFile::METHOD_DEFLATE:
            nSizeCompressed = GetCompressedSizeEstimate(nSize, codec);
            memoryBlock = ZipDirCacheInternal::CreateMemoryBlock(nSizeCompressed);
            pCompressed = memoryBlock->m_address.get();
            dataBuffer = pCompressed;

            switch (codec)
            {
            case CompressionCodec::Codec::ZSTD:
                nError = ZipRawCompressZSTD(pUncompressed, &nSizeCompressed, pCompressed, nSize, nCompressionLevel);
                break;

            case CompressionCodec::Codec::ZLIB:
                nError = ZipRawCompress(pUncompressed, &nSizeCompressed, pCompressed, nSize, nCompressionLevel);
                break;

            case CompressionCodec::Codec::LZ4:
                nError = ZipRawCompressLZ4(pUncompressed, &nSizeCompressed, pCompressed, nSize, nCompressionLevel);
                break;
            }
            if (Z_OK != nError)
            {
                return ZD_ERROR_ZLIB_FAILED;
            }
            break;

        case ZipFile::METHOD_STORE:
            dataBuffer = pUncompressed;
            nSizeCompressed = nSize;
            break;

        default:
            return ZD_ERROR_UNSUPPORTED;
        }

        // create or find the file entry.. this object will rollback (delete the object
        // if the operation fails) if needed.
        FileEntryTransactionAdd pFileEntry(this, szRelativePathSrc);

        if (!pFileEntry)
        {
            return ZD_ERROR_INVALID_PATH;
        }

        pFileEntry->OnNewFileData(pUncompressed, nSize, aznumeric_cast<uint32_t>(nSizeCompressed), nCompressionMethod, false);
        // since we changed the time, we'll have to update CDR
        m_nFlags |= FLAGS_CDR_DIRTY;

        // the new CDR position, if the operation completes successfully
        uint32_t lNewCDROffset = m_lCDROffset;

        if (pFileEntry->IsInitialized())
        {
            // this file entry is already allocated in CDR

            // check if the new compressed data fits into the old place
            size_t nFreeSpace = pFileEntry->nEOFOffset - pFileEntry->nFileHeaderOffset - sizeof(ZipFile::LocalFileHeader) - pFileEntry.GetRelativePath().size();

            if (nFreeSpace != nSizeCompressed)
            {
                m_nFlags |= FLAGS_UNCOMPACTED;
            }

            if (nFreeSpace >= nSizeCompressed)
            {
                // and we can just override the compressed data in the file
                ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, pFileEntry.GetRelativePath());
                if (e != ZD_ERROR_SUCCESS)
                {
                    return e;
                }
            }
            else
            {
                // we need to write the file anew - in place of current CDR
                pFileEntry->nFileHeaderOffset = m_lCDROffset;
                ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, pFileEntry.GetRelativePath());
                lNewCDROffset = pFileEntry->nEOFOffset;
                if (e != ZD_ERROR_SUCCESS)
                {
                    return e;
                }
            }
        }
        else
        {
            pFileEntry->nFileHeaderOffset = m_lCDROffset;
            ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, pFileEntry.GetRelativePath());
            if (e != ZD_ERROR_SUCCESS)
            {
                return e;
            }

            lNewCDROffset = aznumeric_cast<uint32_t>(pFileEntry->nFileDataOffset + nSizeCompressed);

            m_nFlags |= FLAGS_CDR_DIRTY;
        }

        // now we have the fresh local header and data offset
        if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, pFileEntry->nFileDataOffset, AZ::IO::SeekType::SeekFromStart))
        {
            return ZD_ERROR_IO_FAILED;
        }

        const size_t maxChunk = 1 << 20;
        size_t sizeLeft = nSizeCompressed;
        size_t sizeToWrite;
        auto ptr = reinterpret_cast<const uint8_t*>(dataBuffer);
        while ((sizeToWrite = (AZStd::min)(sizeLeft, maxChunk)) != 0)
        {
            if (!AZ::IO::FileIOBase::GetDirectInstance()->Write(m_fileHandle, ptr, sizeToWrite))
            {
                char error[1024];
                [[maybe_unused]] auto azStrErrorResult = azstrerror_s(error, AZ_ARRAY_SIZE(error), errno);
                AZ_Warning("Archive", false, "Cannot write to zip file!! error = (%d): %s", errno, error);
                return ZD_ERROR_IO_FAILED;
            }
            ptr += sizeToWrite;
            sizeLeft -= sizeToWrite;
        }

        // since we wrote the file successfully, update the new CDR position
        m_lCDROffset = lNewCDROffset;
        pFileEntry.Commit();

        return ZD_ERROR_SUCCESS;
    }

    //   Adds a new file to the zip or update an existing one if it is not compressed - just stored  - start a big file
    ErrorEnum Cache::StartContinuousFileUpdate(AZStd::string_view szRelativePathSrc, uint64_t nSize)
    {
        AZ::IO::MemoryBlock memoryBlock;

        // create or find the file entry.. this object will rollback (delete the object
        // if the operation fails) if needed.
        FileEntryTransactionAdd pFileEntry(this, szRelativePathSrc);

        if (!pFileEntry)
        {
            return ZD_ERROR_INVALID_PATH;
        }

        pFileEntry->OnNewFileData(nullptr, nSize, nSize, ZipFile::METHOD_STORE, false);
        // since we changed the time, we'll have to update CDR
        m_nFlags |= FLAGS_CDR_DIRTY;

        // the new CDR position, if the operation completes successfully
        size_t lNewCDROffset = m_lCDROffset;
        if (pFileEntry->IsInitialized())
        {
            // check if the new compressed data fits into the old place
            size_t nFreeSpace = pFileEntry->nEOFOffset - pFileEntry->nFileHeaderOffset - sizeof(ZipFile::LocalFileHeader) - pFileEntry.GetRelativePath().size();

            if (nFreeSpace != nSize)
            {
                m_nFlags |= FLAGS_UNCOMPACTED;
            }

            if (nFreeSpace >= nSize)
            {
                // and we can just override the compressed data in the file
                ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, pFileEntry.GetRelativePath());
                if (e != ZD_ERROR_SUCCESS)
                {
                    return e;
                }
            }
            else
            {
                // we need to write the file anew - in place of current CDR
                pFileEntry->nFileHeaderOffset = m_lCDROffset;
                ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, pFileEntry.GetRelativePath());
                lNewCDROffset = pFileEntry->nEOFOffset;
                if (e != ZD_ERROR_SUCCESS)
                {
                    return e;
                }
            }
        }
        else
        {
            pFileEntry->nFileHeaderOffset = m_lCDROffset;
            ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, pFileEntry.GetRelativePath());
            if (e != ZD_ERROR_SUCCESS)
            {
                return e;
            }

            lNewCDROffset = pFileEntry->nFileDataOffset + nSize;

            m_nFlags |= FLAGS_CDR_DIRTY;
        }

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, pFileEntry->nFileDataOffset, AZ::IO::SeekType::SeekFromStart))
        {
            return ZD_ERROR_IO_FAILED;
        }

        if (!WriteNullData(nSize))
        {
            return ZD_ERROR_IO_FAILED;
        }

        pFileEntry->nEOFOffset = pFileEntry->nFileDataOffset;

        // since we wrote the file successfully, update the new CDR position
        m_lCDROffset = aznumeric_cast<uint32_t>(lNewCDROffset);
        pFileEntry.Commit();

        return ZD_ERROR_SUCCESS;
    }

    // Adds a new file to the zip or update an existing's segment if it is not compressed - just stored
    // adds a directory (creates several nested directories if needed)
    ErrorEnum Cache::UpdateFileContinuousSegment(AZStd::string_view szRelativePathSrc, [[maybe_unused]] uint64_t nSize, const void* pUncompressed, uint64_t nSegmentSize, uint64_t nOverwriteSeekPos)
    {
        const bool shouldOverwriteSeekOffset = nOverwriteSeekPos != (std::numeric_limits<uint64_t>::max)();
        AZ::IO::MemoryBlock memoryBlock;

        // create or find the file entry.. this object will rollback (delete the object
        // if the operation fails) if needed.
        FileEntryTransactionAdd pFileEntry(this, szRelativePathSrc);

        if (!pFileEntry)
        {
            return ZD_ERROR_INVALID_PATH;
        }

        pFileEntry->OnNewFileData(pUncompressed, nSegmentSize, nSegmentSize, ZipFile::METHOD_STORE, true);
        // since we changed the time, we'll have to update CDR
        m_nFlags |= FLAGS_CDR_DIRTY;

        // this file entry is already allocated in CDR
        uint64_t lSegmentOffset = pFileEntry->nEOFOffset;

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, pFileEntry->nFileHeaderOffset, AZ::IO::SeekType::SeekFromStart))
        {
            return ZD_ERROR_IO_FAILED;
        }

        // and we can just override the compressed data in the file
        ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, pFileEntry.GetRelativePath());
        if (e != ZD_ERROR_SUCCESS)
        {
            return e;
        }

        if (shouldOverwriteSeekOffset)
        {
            lSegmentOffset = pFileEntry->nFileDataOffset + nOverwriteSeekPos;
        }

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, lSegmentOffset, AZ::IO::SeekType::SeekFromStart))
        {
            return ZD_ERROR_IO_FAILED;
        }

        constexpr bool encrypt = false; // we do not support encryption for continuous file update
        if (!WriteCompressedData((uint8_t*)pUncompressed, nSegmentSize, encrypt))
        {
            char error[1024];
            [[maybe_unused]] auto azStrErrorResult = azstrerror_s(error, AZ_ARRAY_SIZE(error), errno);
            AZ_Warning("Archive", false, "Cannot write to zip file!! error = (%d): %s", errno, error);
            return ZD_ERROR_IO_FAILED;
        }

        if (!shouldOverwriteSeekOffset)
        {
            pFileEntry->nEOFOffset = aznumeric_cast<uint32_t>(lSegmentOffset + nSegmentSize);
        }

        // since we wrote the file successfully, update CDR
        pFileEntry.Commit();
        return ZD_ERROR_SUCCESS;
    }


    ErrorEnum Cache::UpdateFileCRC(AZStd::string_view szRelativePathSrc, AZ::Crc32 dwCRC32)
    {
        // create or find the file entry.. this object will rollback (delete the object
        // if the operation fails) if needed.
        FileEntryTransactionAdd pFileEntry(this, szRelativePathSrc);

        if (!pFileEntry)
        {
            return ZD_ERROR_INVALID_PATH;
        }

        // since we changed the time, we'll have to update CDR
        m_nFlags |= FLAGS_CDR_DIRTY;

        pFileEntry->desc.lCRC32 = dwCRC32;

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, pFileEntry->nFileHeaderOffset, AZ::IO::SeekType::SeekFromStart))
        {
            return ZD_ERROR_IO_FAILED;
        }

        // and we can just override the compressed data in the file
        ErrorEnum e = WriteLocalHeader(m_fileHandle, pFileEntry, pFileEntry.GetRelativePath());
        if (e != ZD_ERROR_SUCCESS)
        {
            return e;
        }

        // since we wrote the file successfully, update
        pFileEntry.Commit();
        return ZD_ERROR_SUCCESS;
    }


    // deletes the file from the archive
    ErrorEnum Cache::RemoveFile(AZStd::string_view szRelativePathSrc)
    {
        AZ::IO::PathView szRelativePath{ szRelativePathSrc };

        AZStd::string_view fileName; // the name of the file to delete

        FileEntryTree* pDir; // the dir from which the subdir will be deleted

        if (szRelativePath.HasParentPath())
        {
            FindDir fd(GetRoot());
            // the directory to remove
            pDir = fd.FindExact(szRelativePath.ParentPath());
            if (!pDir)
            {
                return ZD_ERROR_DIR_NOT_FOUND;// there is no such directory
            }
            fileName = szRelativePath.Filename().Native();
        }
        else
        {
            pDir = GetRoot();
            fileName = szRelativePath.Native();
        }

        ErrorEnum e = pDir->RemoveFile(fileName);
        if (e == ZD_ERROR_SUCCESS)
        {
            m_nFlags |= FLAGS_UNCOMPACTED | FLAGS_CDR_DIRTY;

            if (az_archive_zip_directory_cache_verbosity)
            {
                AZ_TracePrintf("Archive", R"(File "%.*s" has been remove from archive at root "%s")",
                    AZ_STRING_ARG(szRelativePath.Native()), GetFilePath());
            }
        }
        return e;
    }


    // deletes the directory, with all its descendants (files and subdirs)
    ErrorEnum Cache::RemoveDir(AZStd::string_view szRelativePathSrc)
    {
        AZ::IO::PathView szRelativePath{ szRelativePathSrc };

        AZStd::string_view dirName; // the name of the dir to delete

        FileEntryTree* pDir; // the dir from which the subdir will be deleted

        if (szRelativePath.HasParentPath())
        {
            FindDir fd(GetRoot());
            // the directory to remove
            pDir = fd.FindExact(szRelativePath.ParentPath());
            if (!pDir)
            {
                return ZD_ERROR_DIR_NOT_FOUND;// there is no such directory
            }
            dirName = szRelativePath.Filename().Native();
        }
        else
        {
            pDir = GetRoot();
            dirName = szRelativePath.Native();
        }

        ErrorEnum e = pDir->RemoveDir(dirName);
        if (e == ZD_ERROR_SUCCESS)
        {
            m_nFlags |= FLAGS_UNCOMPACTED | FLAGS_CDR_DIRTY;

            if (az_archive_zip_directory_cache_verbosity)
            {
                AZ_TracePrintf("Archive", R"(Directory "%.*s" has been remove from archive at root "%s")",
                    AZ_STRING_ARG(szRelativePath.Native()), GetFilePath());
            }
        }
        return e;
    }

    // deletes all files and directories in this archive
    ErrorEnum Cache::RemoveAll()
    {
        ErrorEnum e = m_treeDir.RemoveAll();
        if (e == ZD_ERROR_SUCCESS)
        {
            m_nFlags |= FLAGS_UNCOMPACTED | FLAGS_CDR_DIRTY;
        }
        return e;
    }

    ErrorEnum Cache::ReadFile(FileEntry* pFileEntry, void* pCompressed, void* pUncompressed)
    {
        if (!pFileEntry)
        {
            return ZD_ERROR_INVALID_CALL;
        }

        if (pFileEntry->desc.lSizeUncompressed == 0)
        {
            // note that even in a compressed zip file, 0 bytes MUST be stored using "STORE" compression and thus will remain 0
            AZ_Assert(pFileEntry->desc.lSizeCompressed == 0, "Compressed file has uncompressed size of 0, but compressed size of %" PRIu32, pFileEntry->desc.lSizeCompressed);
            return ZD_ERROR_SUCCESS;
        }

        AZ_Assert(pFileEntry->desc.lSizeCompressed > 0, "Compressed file has compressed size of 0. It cannot be read");

        ErrorEnum nError = Refresh(pFileEntry);
        if (nError != ZD_ERROR_SUCCESS)
        {
            return nError;
        }

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(m_fileHandle, pFileEntry->nFileDataOffset, AZ::IO::SeekType::SeekFromStart))
        {
            return ZD_ERROR_IO_FAILED;
        }

        AZStd::intrusive_ptr<AZ::IO::MemoryBlock> memoryBlock;

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

            memoryBlock = ZipDirCacheInternal::CreateMemoryBlock(pFileEntry->desc.lSizeCompressed);
            pBuffer = memoryBlock->m_address.get();
        }

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Read(m_fileHandle, pBuffer, pFileEntry->desc.lSizeCompressed, true))
        {
            return ZD_ERROR_IO_FAILED;
        }

        // if there's a buffer for uncompressed data, uncompress it to that buffer
        if (pUncompressed)
        {
            if (pFileEntry->nMethod == 0)
            {
                AZ_Assert(pBuffer == pUncompressed, "When the file entry is uncompressed the buffer should point to uncompressed buffer");
            }
            else
            {
                size_t nSizeUncompressed = pFileEntry->desc.lSizeUncompressed;
                if (Z_OK != ZipRawUncompress(pUncompressed, &nSizeUncompressed, pBuffer, pFileEntry->desc.lSizeCompressed))
                {
                    return ZD_ERROR_CORRUPTED_DATA;
                }
                if (pFileEntry->bCheckCRCNextRead)
                {
                    pFileEntry->bCheckCRCNextRead = false;
                    uLong uCRC32 = AZ::Crc32((Bytef*)pUncompressed, nSizeUncompressed);
                    if (uCRC32 != pFileEntry->desc.lCRC32)
                    {
                        AZ_Warning("Archive", false, "ZD_ERROR_CRC32_CHECK: Uncompressed stream CRC32 check failed");
                        return ZD_ERROR_CRC32_CHECK;
                    }
                }
            }
        }

        return ZD_ERROR_SUCCESS;
    }


    //////////////////////////////////////////////////////////////////////////
    // finds the file by exact path
    FileEntry* Cache::FindFile(AZStd::string_view szPathSrc, [[maybe_unused]] bool bFullInfo)
    {
        AZ::IO::PathView szPath{ szPathSrc };

        ZipDir::FindFile fd(GetRoot());
        FileEntry* fileEntry = fd.FindExact(szPath);
        if (!fileEntry)
        {
            if (az_archive_zip_directory_cache_verbosity)
            {
                AZ_TracePrintf("Archive", "FindExact failed to find file %.*s at root %s", AZ_STRING_ARG(szPath.Native()), GetFilePath());
            }
            return {};
        }
        return fileEntry;
    }

    // refreshes information about the given file entry into this file entry
    ErrorEnum Cache::Refresh(FileEntryBase* pFileEntry)
    {
        if (!pFileEntry)
        {
            return ZD_ERROR_INVALID_CALL;
        }

        if (pFileEntry->nFileDataOffset != FileEntryBase::INVALID_DATA_OFFSET)
        {
            return ZD_ERROR_SUCCESS; // the data offset has been successfully read..
        }
        CZipFile tmp;
        tmp.m_fileHandle = m_fileHandle;
        return ZipDir::Refresh(&tmp, pFileEntry);
    }


    // writes the CDR to the disk
    bool Cache::WriteCDR(AZ::IO::HandleType fTarget)
    {
        if (fTarget == AZ::IO::InvalidHandle)
        {
            return false;
        }

        if (!AZ::IO::FileIOBase::GetDirectInstance()->Seek(fTarget, m_lCDROffset, AZ::IO::SeekType::SeekFromStart))
        {
            return false;
        }

        FileRecordList arrFiles(GetRoot());
        //arrFiles.SortByFileOffset();
        size_t nSizeCDR = arrFiles.GetStats().nSizeCDR;
        void* pCDR = azmalloc(nSizeCDR, alignof(uint8_t));
        [[maybe_unused]] size_t nSizeCDRSerialized = arrFiles.MakeZipCDR(m_lCDROffset, pCDR);
        AZ_Assert(nSizeCDRSerialized == nSizeCDR, "Serialized CDR size %zu does not match size in memory %zu", nSizeCDRSerialized, nSizeCDR);
        if (m_encryptedHeaders == ZipFile::HEADERS_ENCRYPTED_TEA)
        {
            AZ_Warning("Archive", false, "Attempt to use XTEA pak encryption when it is disabled.");
            Free(pCDR);
            return false;
        }

        bool success = AZ::IO::FileIOBase::GetDirectInstance()->Write(fTarget, pCDR, nSizeCDR);
        Free(pCDR);
        return success;
    }

    bool Cache::RelinkZip()
    {
        AZ::IO::FileIOBase* fileSystem = AZ::IO::FileIOBase::GetDirectInstance();

        for (int nAttempt = 0; nAttempt < 32; ++nAttempt)
        {
            auto strNewFilePath = AZ::IO::PathString::format("%s$%s", m_strFilePath.c_str(), ZipDirCacheInternal::GetRandomName(nAttempt).c_str());
            AZ::IO::HandleType fileHandle;
            if (fileSystem->Open(strNewFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, fileHandle))
            {
                bool bOk = RelinkZip(fileHandle);
                fileSystem->Close(fileHandle);

                if (!bOk)
                {
                    // we don't need the temporary file
                    fileSystem->Remove(strNewFilePath.c_str());
                    return false;
                }

                // we successfully relinked, now copy the temporary file to the original file
                fileSystem->Close(m_fileHandle);
                m_fileHandle = AZ::IO::InvalidHandle;

                fileSystem->Remove(m_strFilePath.c_str());
                if (AZ::IO::Move(strNewFilePath.c_str(), m_strFilePath.c_str()))
                {
                    // successfully renamed - reopen
                    return fileSystem->Open(m_strFilePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeUpdate | AZ::IO::OpenMode::ModeBinary, m_fileHandle);
                }
                else
                {
                    // could not rename
                    return false;
                }
            }
        }

        // couldn't open temp file
        return false;
    }

    bool Cache::RelinkZip(AZ::IO::HandleType fTmp)
    {
        FileRecordList arrFiles(GetRoot());
        arrFiles.SortByFileOffset();

        // we back up our file entries, because we'll need to restore them
        // in case the operation fails
        AZStd::vector<FileEntryBase> arrFileEntryBackup;
        arrFiles.Backup(arrFileEntryBackup);

        // this is the set of files that are to be written out - compressed data and the file record iterator
        AZStd::vector<FileDataRecordPtr> queFiles;
        queFiles.reserve(g_nMaxItemsRelinkBuffer);

        AZ::IO::FileIOBase* fileSystem = AZ::IO::FileIOBase::GetDirectInstance();

        // the total size of data in the queue
        uint32_t nQueueSize = 0;

        for (FileRecordList::iterator it = arrFiles.begin(); it != arrFiles.end(); ++it)
        {
            // find the file data offset
            if (ZD_ERROR_SUCCESS != Refresh(it->pFileEntryBase))
            {
                return false;
            }

            // go to the file data
            if (!fileSystem->Seek(m_fileHandle, it->pFileEntryBase->nFileDataOffset, AZ::IO::SeekType::SeekFromStart))
            {
                return false;
            }

            // allocate memory for the file compressed data
            FileDataRecordPtr pFile = aznew FileDataRecord(*it);

            if (!pFile)
            {
                return false;
            }

            // read the compressed data
            if (it->pFileEntryBase->desc.lSizeCompressed && !fileSystem->Read(m_fileHandle, pFile->GetData(), it->pFileEntryBase->desc.lSizeCompressed, true))
            {
                return false;
            }

            // put the file into the queue for copying (writing)
            queFiles.push_back(pFile);
            nQueueSize += it->pFileEntryBase->desc.lSizeCompressed;

            // if the queue is big enough, write it out
            if (nQueueSize > g_nSizeRelinkBuffer || queFiles.size() >= g_nMaxItemsRelinkBuffer)
            {
                nQueueSize = 0;
                if (!WriteZipFiles(queFiles, fTmp))
                {
                    return false;
                }
            }
        }

        if (!WriteZipFiles(queFiles, fTmp))
        {
            return false;
        }

        uint32_t lOldCDROffset = m_lCDROffset;
        // the file data has now been written out. Now write the CDR
        AZ::u64 tellOffset = 0;
        fileSystem->Tell(fTmp, tellOffset);
        m_lCDROffset = static_cast<uint32_t>(tellOffset);

        if (WriteCDR(fTmp) && AZ::IO::FileIOBase::GetDirectInstance()->Flush(fTmp))
        {
            // the new file positions are already there - just discard the backup and return
            return true;
        }
        // recover from backup
        arrFiles.Restore(arrFileEntryBackup);
        m_lCDROffset = lOldCDROffset;
        return false;
    }

    // writes out the file data in the queue into the given file. Empties the queue
    bool Cache::WriteZipFiles(AZStd::vector<FileDataRecordPtr>& queFiles, AZ::IO::HandleType fTmp)
    {
        AZ::IO::FileIOBase* fileSystem = AZ::IO::FileIOBase::GetDirectInstance();
        for (auto it = queFiles.begin(); it != queFiles.end(); ++it)
        {
            // set the new header offset to the file entry - we won't need it
            AZ::u64 tellOffset = 0;
            fileSystem->Tell(fTmp, tellOffset);
            (*it)->pFileEntryBase->nFileHeaderOffset = static_cast<uint32_t>(tellOffset);

            // while writing the local header, the data offset will also be calculated
            if (ZD_ERROR_SUCCESS != WriteLocalHeader(fTmp, (*it)->pFileEntryBase, (*it)->strPath.c_str()))
            {
                return false;
            }

            // write the compressed file data
            if ((*it)->pFileEntryBase->desc.lSizeCompressed && !fileSystem->Write(fTmp, (*it)->GetData(), (*it)->pFileEntryBase->desc.lSizeCompressed))
            {
                return false;
            }

            tellOffset = 0;
            fileSystem->Tell(fTmp, tellOffset);
            AZ_Assert((*it)->pFileEntryBase->nEOFOffset == tellOffset, "File offset %" PRIx64 " does not match file entry EOF offset of %" PRIx32,
                tellOffset, (*it)->pFileEntryBase->nEOFOffset);
        }
        queFiles.clear();
        queFiles.reserve(g_nMaxItemsRelinkBuffer);
        return true;
    }
}
