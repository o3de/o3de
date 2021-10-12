/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// This file contains only the support definitions for CZipDir class
// implementation. This it to unload the ZipDir.h from secondary stuff.

#pragma once

#include <AzCore/base.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzFramework/Archive/ZipFileFormat.h>

#if AZ_TRAIT_USE_WINDOWS_FILE_API && AZ_TRAIT_OS_IS_HOST_OS_PLATFORM
#define SUPPORT_UNBUFFERED_IO
#endif


struct z_stream_s;

namespace AZ::IO
{
    class FileIOBase;
    struct MemoryBlock;
}

namespace AZ::IO::ZipDir
{
    struct FileEntry;
    struct CZipFile
    {
        AZ::IO::HandleType m_fileHandle;
#ifdef SUPPORT_UNBUFFERED_IO
        AZ::IO::SystemFile m_unbufferedFile;
        size_t m_nSectorSize;
        void* m_pReadTarget;
#endif
        int64_t m_nSize;
        int64_t m_nCursor;
        const char* m_szFilename;

        AZStd::intrusive_ptr<AZ::IO::MemoryBlock> m_pInMemoryData;
        AZ::IO::FileIOBase* m_fileIOBase = nullptr;

        CZipFile();
        CZipFile(const CZipFile& file) = delete;
        CZipFile & operator=(const CZipFile&) = delete;

        void Swap(CZipFile& other);

        bool IsInMemory() const { return m_pInMemoryData != nullptr; }
        void LoadToMemory(AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData = {});
        void UnloadFromMemory();
        void Close(bool bUnloadFromMem = true);

#ifdef SUPPORT_UNBUFFERED_IO
        bool OpenUnbuffered(const char* filename);
        bool EvaluateSectorSize(const char* filename);
#endif
    };

    // possible errors occurring during the method execution
    // to avoid clashing with the global Windows defines, we prefix these with ZD_
    enum ErrorEnum
    {
        ZD_ERROR_SUCCESS = 0,
        ZD_ERROR_IO_FAILED,
        ZD_ERROR_UNEXPECTED,
        ZD_ERROR_UNSUPPORTED,
        ZD_ERROR_INVALID_SIGNATURE,
        ZD_ERROR_ZIP_FILE_IS_CORRUPT,
        ZD_ERROR_DATA_IS_CORRUPT,
        ZD_ERROR_NO_CDR,
        ZD_ERROR_CDR_IS_CORRUPT,
        ZD_ERROR_NO_MEMORY,
        ZD_ERROR_VALIDATION_FAILED,
        ZD_ERROR_CRC32_CHECK,
        ZD_ERROR_ZLIB_FAILED,
        ZD_ERROR_ZLIB_CORRUPTED_DATA,
        ZD_ERROR_ZLIB_NO_MEMORY,
        ZD_ERROR_CORRUPTED_DATA,
        ZD_ERROR_INVALID_CALL,
        ZD_ERROR_NOT_IMPLEMENTED,
        ZD_ERROR_FILE_NOT_FOUND,
        ZD_ERROR_DIR_NOT_FOUND,
        ZD_ERROR_NAME_TOO_LONG,
        ZD_ERROR_INVALID_PATH,
        ZD_ERROR_FILE_ALREADY_EXISTS,
        ZD_ERROR_ARCHIVE_TOO_LARGE,
    };

    // the error describes the reason of the error, as well as the error code, line of code where it happened etc.
    struct Error
    {
        Error(ErrorEnum _nError, const char* _szDescription, const char* _szFunction, const char* _szFile, unsigned _nLine)
            : nError(_nError)
            , m_szDescription(_szDescription)
            , szFunction(_szFunction)
            , szFile(_szFile)
            , nLine(_nLine)
        {
        }

        ErrorEnum nError;
        const char* getError();

        const char* getDescription() {return m_szDescription; }
        const char* szFunction, * szFile;
        unsigned nLine;
    protected:
        // the description of the error; if needed, will be made as a dynamic string
        const char* m_szDescription;
    };

#if defined(_RELEASE)
    inline static constexpr bool IsReleaseConfig{ true };
#else
    inline static constexpr bool IsReleaseConfig{};
#endif // _RELEASE

    // possible initialization methods
    enum class InitMethod
    {
        // initializes without any sort of extra validation steps
        Default,

        // initializes with extra validation steps
        // not available in RELEASE
        // will check CDR and local headers data match
        ValidateHeaders,

        // initializes with extra validation steps
        // not available in RELEASE
        // will check CDR and local headers data match
        // will check file data CRC matches (when file is read)
        FullValidation,
    };

    // Uncompresses raw (without wrapping) data that is compressed with method 8 (deflated) in the Zip file
    // returns one of the Z_* errors (Z_OK upon success)
    int ZipRawUncompress(void* pUncompressed, size_t* pDestSize, const void* pCompressed, size_t nSrcSize);

    // compresses the raw data into raw data. The buffer for compressed data itself with the heap passed. Uses method 8 (deflate)
    // returns one of the Z_* errors (Z_OK upon success), and the size in *pDestSize. the pCompressed buffer must be at least nSrcSize*1.001+12 size
    int ZipRawCompress(const void* pUncompressed, size_t* pDestSize, void* pCompressed, size_t nSrcSize, int nLevel);
    int ZipRawCompressZSTD(const void* pUncompressed, size_t* pDestSize, void* pCompressed, size_t nSrcSize, int nLevel);
    int ZipRawCompressLZ4(const void* pUncompressed, size_t* pDestSize, void* pCompressed, size_t nSrcSize, int nLevel);

    // fseek wrapper with memory in file support.
    int64_t FSeek(CZipFile* zipFile, int64_t origin, int command);

    // fread wrapper with file in memory  support
    int64_t FRead(CZipFile* zipFile, void* data, size_t nElemSize, size_t nCount);

    // ftell wrapper with file in memory support
    int64_t FTell(CZipFile* zipFile);

    int FEof(CZipFile* zipFile);


    //////////////////////////////////////////////////////////////////////////
    struct SExtraZipFileData
    {
        uint64_t nLastModifyTime{};
    };

    struct FileEntryBase
    {
        FileEntryBase() = default;
        FileEntryBase(const ZipFile::CDRFileHeader& header, const SExtraZipFileData& extra);

        inline static constexpr uint32_t INVALID_DATA_OFFSET = 0xFFFFFFFF;
        ZipFile::DataDescriptor desc{};
        uint32_t nFileDataOffset{ INVALID_DATA_OFFSET }; // offset of the packed info inside the file; NOTE: this can be INVALID_DATA_OFFSET, if not calculated yet!
        uint32_t nFileHeaderOffset{ INVALID_DATA_OFFSET }; // offset of the local file header
        uint32_t nNameOffset{};       // offset of the file name in the name pool for the directory

        uint16_t nMethod{};             // the method of compression (0 if no compression/store)
        uint16_t nReserved0{};          // Reserved

        // the file modification times
        uint16_t nLastModTime{};
        uint16_t nLastModDate{};
        uint64_t nNTFS_LastModifyTime{};

        // the offset to the start of the next file's header - this
        // can be used to calculate the available space in zip file
        uint32_t nEOFOffset{};

        // whether to check the CRC upon the next data read
        bool bCheckCRCNextRead{};
    };

    // this is the record about the file in the Zip file.
    struct FileEntry
        : FileEntryBase
    {
        AZ_CLASS_ALLOCATOR(FileEntry, AZ::SystemAllocator, 0);

        // mutex that can be used to product reads for the current file entry
        AZStd::mutex m_readLock;

        using FileEntryBase::FileEntryBase;

        FileEntry(const FileEntry&) = delete;
        FileEntry& operator=(const FileEntry&) = delete;

        bool IsInitialized()
        {
            // structure marked as non-initialized should have nFileHeaderOffset == INVALID_DATA_OFFSET
            return nFileHeaderOffset != INVALID_DATA_OFFSET;
        }
        // returns the name of this file, given the pointer to the name pool
        const char* GetName(const char* pNamePool) const
        {
            return pNamePool + nNameOffset;
        }

        // sets the current time to modification time
        // calculates CRC32 for the new data
        void OnNewFileData(const void* pUncompressed, uint64_t nSize, uint64_t nCompressedSize, uint32_t nCompressionMethod, bool bContinuous);

        uint64_t GetModificationTime();

        bool IsCompressed() const
        {
            return (
                nMethod != ZipFile::METHOD_STORE_AND_STREAMCIPHER_KEYTABLE &&
                nMethod != ZipFile::METHOD_STORE
                );
        }
    };


    // tries to refresh the file entry from the given file (reads from there if needed)
    // returns the error code if the operation was impossible to complete
    ErrorEnum Refresh(CZipFile* f, FileEntryBase* pFileEntry);

    // writes into the file local header (NOT including the name, only the header structure)
    // the file must be opened both for reading and writing
    ErrorEnum UpdateLocalHeader(AZ::IO::HandleType fileHandle, FileEntryBase* pFileEntry);

    // writes into the file local header - without Extra data
    // puts the new offset to the file data to the file entry
    // in case of error can put INVALID_DATA_OFFSET into the data offset field of file entry
    ErrorEnum WriteLocalHeader(AZ::IO::HandleType fileHandle, FileEntryBase* pFileEntry, AZStd::string_view szRelativePath);

    // conversion routines for the date/time fields used in Zip
    uint16_t DOSDate(tm*);
    uint16_t DOSTime(tm*);

    const char* DOSTimeCStr(uint16_t nTime);
    const char* DOSDateCStr(uint16_t nTime);

    struct DirHeader;
    // this structure represents a subdirectory descriptor in the directory record.
    // it points to the actual directory info (list of its subdirs and files), as well
    // as on its name
    struct DirEntry
    {
        AZ_CLASS_ALLOCATOR(DirEntry, AZ::SystemAllocator, 0);
        uint32_t nDirHeaderOffset{}; // offset, in bytes, relative to this object, of the actual directory record header
        uint32_t nNameOffset{}; // offset of the dir name in the name pool of the parent directory

        // returns the name of this directory, given the pointer to the name pool of hte parent directory
        const char* GetName(const char* pNamePool) const
        {
            return pNamePool + nNameOffset;
        }

        // returns the pointer to the actual directory record.
        // call this function only for the actual structure instance contained in a directory record and
        // followed by the other directory records
        const DirHeader* GetDirectory()    const
        {
            return (const DirHeader*)(((const char*)this) + nDirHeaderOffset);
        }
        DirHeader* GetDirectory()
        {
            return (DirHeader*)(((char*)this) + nDirHeaderOffset);
        }
    };

    // this is the head of the directory record
    // the name pool follows straight the directory and file entries.
    struct DirHeader
    {
        uint16_t numDirs{}; // number of directory entries - DirEntry structures
        uint16_t numFiles{}; // number of file entries - FileEntry structures

        // returns the pointer to the name pool that follows this object
        // you can only call this method for the structure instance actually followed by the dir record
        const char* GetNamePool() const
        {
            return ((char*)(this + 1)) + (size_t)this->numDirs * sizeof(DirEntry) + (size_t)this->numFiles * sizeof(FileEntry);
        }
        char* GetNamePool()
        {
            return ((char*)(this + 1)) + (size_t)this->numDirs * sizeof(DirEntry) + (size_t)this->numFiles * sizeof(FileEntry);
        }

        // returns the pointer to the i-th directory
        // call this only for the actual instance of the structure at the head of dir record
        const DirEntry* GetSubdirEntry(uint32_t i) const
        {
            AZ_Assert(i < numDirs, "Index %u is out of bounds to lookup subdirectory entry . Index must be less than %hu", i, numDirs);
            return ((const DirEntry*)(this + 1)) + i;
        }
        DirEntry* GetSubdirEntry(uint32_t i)
        {
            return const_cast<DirEntry*>(const_cast<const DirHeader*>(this)->GetSubdirEntry(i));
        }

        // returns the pointer to the i-th file
        // call this only for the actual instance of the structure at the head of dir record
        const FileEntry* GetFileEntry(uint32_t i) const
        {
            AZ_Assert(i < numFiles, "Index %u is out of range of number of file entries %hu", i, numFiles);
            return (const FileEntry*)(((const DirEntry*)(this + 1)) + numDirs) + i;
        }
        FileEntry* GetFileEntry (uint32_t i)
        {
            return const_cast<FileEntry*>(const_cast<const DirHeader*>(this)->GetFileEntry(i));
        }

        // finds the subdirectory entry by the name, using the names from the name pool
        // assumes: all directories are sorted in alphabetical order.
        // case-sensitive (must be lower-case if case-insensitive search in Win32 is performed)
        DirEntry* FindSubdirEntry(AZStd::string_view szName);

        // finds the file entry by the name, using the names from the name pool
        // assumes: all directories are sorted in alphabetical order.
        // case-sensitive (must be lower-case if case-insensitive search in Win32 is performed)
        FileEntry* FindFileEntry(AZStd::string_view szName);
    };

    // this is the sorting predicate for directory entries
    struct DirEntrySortPred
    {
        DirEntrySortPred(const char* pNamePool)
            : m_pNamePool{ pNamePool }
        {
        }

        bool operator()(const FileEntry& left, const FileEntry& right) const
        {
            return strcmp(left.GetName(m_pNamePool), right.GetName(m_pNamePool)) < 0;
        }

        bool operator()(const FileEntry& left, AZStd::string_view szRight) const
        {
            return left.GetName(m_pNamePool) < szRight;
        }

        bool operator()(AZStd::string_view szLeft, const FileEntry& right) const
        {
            return szLeft < right.GetName(m_pNamePool);
        }

        bool operator()(const DirEntry& left, const DirEntry& right) const
        {
            return strcmp(left.GetName(m_pNamePool), right.GetName(m_pNamePool)) < 0;
        }

        bool operator()(const DirEntry& left, AZStd::string_view szName) const
        {
            return left.GetName(m_pNamePool) < szName;
        }

        bool operator()(const char* szLeft, const DirEntry& right) const
        {
            return szLeft < right.GetName(m_pNamePool);
        }

        const char* m_pNamePool;
    };

    struct UncompressLookahead
    {
        inline static constexpr size_t Capacity = 16384;

        UncompressLookahead()
            : cachedStartIdx(0)
            , cachedEndIdx(0)
        {
        }

        char buffer[Capacity];
        uint32_t cachedStartIdx;
        uint32_t cachedEndIdx;
    };
}

