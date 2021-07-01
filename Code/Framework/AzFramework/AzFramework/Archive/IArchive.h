/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/Archive/ArchiveFindData.h>


enum EStreamSourceMediaType : int32_t;

namespace AZ::IO
{
    enum class ArchiveLocationPriority;
    struct IResourceList;
    struct INestedArchive;
    struct IArchive;

    using PathString = AZStd::fixed_string<AZ::IO::MaxPathLength>;
    using StackString = AZStd::fixed_string<512>;

    struct MemoryBlock;
    struct MemoryBlockDeleter
    {
        void operator()(const AZStd::intrusive_refcount<AZStd::atomic_uint, MemoryBlockDeleter>* ptr) const;
        AZ::IAllocatorAllocate* m_allocator{};
    };
    struct MemoryBlock
        : AZStd::intrusive_refcount<AZStd::atomic_uint, MemoryBlockDeleter>
    {
        MemoryBlock() = default;
        MemoryBlock(MemoryBlockDeleter deleter)
            : AZStd::intrusive_refcount<AZStd::atomic_uint, MemoryBlockDeleter>{ deleter }
        {}
        struct AddressDeleter
        {
            using DeleteFunc = void(*)(uint8_t*);
            AddressDeleter()
                : m_deleteFunc{ nullptr }
            {}
            AddressDeleter(DeleteFunc deleteFunc)
                : m_deleteFunc{ deleteFunc }
            {}
            void operator()(uint8_t* ptr) const
            {
                if (m_deleteFunc)
                {
                    m_deleteFunc(ptr);
                }
                else
                {
                    delete[] ptr;
                }
            }
            DeleteFunc m_deleteFunc;
        };
        using AddressPtr = AZStd::unique_ptr<uint8_t[], AddressDeleter>;

        AddressPtr m_address;
        size_t m_size{};
    };

    inline void MemoryBlockDeleter::operator()(const AZStd::intrusive_refcount<AZStd::atomic_uint, MemoryBlockDeleter>* ptr) const
    {
        auto address = const_cast<MemoryBlock*>(static_cast<const MemoryBlock*>(ptr));
        if (m_allocator)
        {
            address ->~MemoryBlock();
            m_allocator->DeAllocate(address);
        }
        else
        {
            delete address;
        }
    }

    struct IArchiveFileAccessSink
    {
        virtual ~IArchiveFileAccessSink() {}
        // Arguments:
        //   in - 0 if asynchronous read
        //   szFullPath - must not be 0
        virtual void ReportFileOpen(AZ::IO::HandleType inFileHandle, AZStd::string_view szFullPath) = 0;
    };

    // Summary
    //   Interface to the Archive file system
    // See Also
    //   Archive
    struct IArchive
    {
        AZ_RTTI(IArchive, "{764A2260-FF8A-4C86-B958-EBB0B69D9DFA}");
        using FileTime = uint64_t;
        // Flags used in file path resolution rules
        enum EPathResolutionRules
        {
            // If used, the source path will be treated as the destination path
            // and no transformations will be done. Pass this flag when the path is to be the actual
            // path on the disk/in the packs and doesn't need adjustment (or after it has come through adjustments already)
            // if this is set, AdjustFileName will not map the input path into the folder (Ex: Shaders will not be converted to Game\Shaders)
            FLAGS_PATH_REAL = 1 << 16,

            // AdjustFileName will always copy the file path to the destination path:
            // regardless of the returned value, szDestpath can be used
            FLAGS_COPY_DEST_ALWAYS = 1 << 17,

            // Adds trailing slash to the path
            FLAGS_ADD_TRAILING_SLASH = 1L << 18,

            // if this is set, AdjustFileName will not make relative paths into full paths
            FLAGS_NO_FULL_PATH = 1 << 21,

            // if this is set, AdjustFileName will redirect path to disc
            FLAGS_REDIRECT_TO_DISC = 1 << 22,

            // if this is set, AdjustFileName will not adjust path for writing files
            FLAGS_FOR_WRITING = 1 << 23,

            // if this is set, the archive would be stored in memory (gpu)
            FLAGS_PAK_IN_MEMORY = 1 << 25,

            // Store all file names as crc32 in a flat directory structure.
            FLAGS_FILENAMES_AS_CRC32 = 1 << 26,

            // if this is set, AdjustFileName will try to find the file under any mod paths we know about
            FLAGS_CHECK_MOD_PATHS = 1 << 27,

            // if this is set, AdjustFileName will always check the filesystem/disk and not check inside open archives
            FLAGS_NEVER_IN_PAK = 1 << 28,

            // returns existing file name from the local data or existing cache file name
            // used by the resource compiler to pass the real file name
            FLAGS_RESOLVE_TO_CACHE = 1 << 29,

            // if this is set, the archive would be stored in memory (cpu)
            FLAGS_PAK_IN_MEMORY_CPU = 1 << 30,

            // if this is set, the level pak is inside another archive
            FLAGS_LEVEL_PAK_INSIDE_PAK = 1 << 31,
        };

        // Used for widening FOpen functionality. They're ignored for the regular File System files.
        enum EFOpenFlags
        {
            // If possible, will prevent the file from being read from memory.
            FOPEN_HINT_DIRECT_OPERATION = 1,
            // Will prevent a "missing file" warnings to be created.
            FOPEN_HINT_QUIET = 1 << 1,
            // File should be on disk
            FOPEN_ONDISK = 1 << 2,
            // Open is done by the streaming thread.
            FOPEN_FORSTREAMING = 1 << 3,
        };

        //
        enum ERecordFileOpenList
        {
            RFOM_Disabled,                          // file open are not recorded (fast, no extra memory)
            RFOM_EngineStartup,                 // before a level is loaded
            RFOM_Level,                                 // during level loading till export2game -> resourcelist.txt, used to generate the list for level2level loading
            RFOM_NextLevel                          // used for level2level loading
        };
        // the size of the buffer that receives the full path to the file
        inline static constexpr size_t MaxPath = 1024;

        //file location enum used in isFileExist to control where the archive system looks for the file.
        enum EFileSearchLocation
        {
            eFileLocation_Any = 0,
            eFileLocation_OnDisk,
            eFileLocation_InPak,
        };

        enum EInMemoryArchiveLocation
        {
            eInMemoryPakLocale_Unload = 0,
            eInMemoryPakLocale_CPU,
            eInMemoryPakLocale_GPU,
            eInMemoryPakLocale_PAK,
        };

        enum EFileSearchType
        {
            eFileSearchType_AllowInZipsOnly = 0,
            eFileSearchType_AllowOnDiskAndInZips,
            eFileSearchType_AllowOnDiskOnly
        };

        using SignedFileSize = int64_t;

        virtual ~IArchive() = default;

        /**
        * Deprecated: Use the AZ::IO::FileIOBase::ResolvePath function below that doesn't accept the nFlags or skipMods parameters
        * given the source relative path, constructs the full path to the file according to the flags
        * returns the pointer to the constructed path (can be either szSourcePath, or szDestPath, or NULL in case of error
        */
        // 
        virtual const char* AdjustFileName(AZStd::string_view src, char* dst, size_t dstSize, uint32_t nFlags, bool skipMods = false) = 0;

        virtual bool Init(AZStd::string_view szBasePath) = 0;
        virtual void Release() = 0;

        // Summary:
        //   Returns true if given archivepath is installed to HDD
        //   If no file path is given it will return true if whole application is installed to HDD
        virtual bool IsInstalledToHDD(AZStd::string_view acFilePath = 0) const = 0;

        // after this call, the archive file will be searched for files when they aren't on the OS file system
        // Arguments:
        //   pName - must not be 0
        virtual bool OpenPack(AZStd::string_view pName, uint32_t nFlags = FLAGS_PATH_REAL, AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData = {},
            AZ::IO::FixedMaxPathString* pFullPath = nullptr, bool addLevels = true) = 0;
        // after this call, the archive file will be searched for files when they aren't on the OS file system
        virtual bool OpenPack(AZStd::string_view pBindingRoot, AZStd::string_view pName, uint32_t nFlags = FLAGS_PATH_REAL,
            AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData = {}, AZ::IO::FixedMaxPathString* pFullPath = nullptr, bool addLevels = true) = 0;
        // after this call, the file will be unlocked and closed, and its contents won't be used to search for files
        virtual bool ClosePack(AZStd::string_view pName, uint32_t nFlags = FLAGS_PATH_REAL) = 0;
        // opens pack files by the path and wildcard
        virtual bool OpenPacks(AZStd::string_view pWildcard, uint32_t nFlags = FLAGS_PATH_REAL, AZStd::vector<AZ::IO::FixedMaxPathString>* pFullPaths = nullptr) = 0;
        // opens pack files by the path and wildcard
        virtual bool OpenPacks(AZStd::string_view pBindingRoot, AZStd::string_view pWildcard, uint32_t nFlags = FLAGS_PATH_REAL,
            AZStd::vector<AZ::IO::FixedMaxPathString>* pFullPaths = nullptr) = 0;
        // closes pack files by the path and wildcard
        virtual bool ClosePacks(AZStd::string_view pWildcard, uint32_t nFlags = FLAGS_PATH_REAL) = 0;
        //returns if a archive exists matching the wildcard
        virtual bool FindPacks(AZStd::string_view pWildcardIn) = 0;

        // Set access status of a archive files with a wildcard
        virtual bool SetPacksAccessible(bool bAccessible, AZStd::string_view pWildcard, uint32_t nFlags = FLAGS_PATH_REAL) = 0;

        // Set access status of a pack file
        virtual bool SetPackAccessible(bool bAccessible, AZStd::string_view pName, uint32_t nFlags = FLAGS_PATH_REAL) = 0;

        // Load or unload archive file completely to memory.
        virtual bool LoadPakToMemory(AZStd::string_view pName, EInMemoryArchiveLocation eLoadToMemory, AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pMemoryBlock = nullptr) = 0;
        virtual void LoadPaksToMemory(int nMaxArchiveSize, bool bLoadToMemory) = 0;

        // Processes an alias command line containing multiple aliases.
        virtual void ParseAliases(AZStd::string_view szCommandLine) = 0;
        // adds or removes an alias from the list
        virtual void SetAlias(AZStd::string_view szName, AZStd::string_view szAlias, bool bAdd) = 0;
        // gets an alias from the list, if any exist.
        // if bReturnSame==true, it will return the input name if an alias doesn't exist. Otherwise returns NULL
        virtual const char* GetAlias(AZStd::string_view szName, bool bReturnSame = true) = 0;

        // lock all the operations
        virtual void Lock() = 0;
        virtual void Unlock() = 0;

        // Set and Get the localization folder name (Languages, Localization, ...)
        virtual void SetLocalizationFolder(AZStd::string_view sLocalizationFolder) = 0;
        virtual const char* GetLocalizationFolder() const = 0;
        virtual const char* GetLocalizationRoot() const = 0;

        // Open file handle, file can be on disk or in Archive file.
        // Possible mode is r,b,x
        // ex: AZ::IO::HandleType fileHandle = FOpen( "test.txt","rbx" );
        // mode x is a direct access mode, when used file reads will go directly into the low level file system without any internal data caching.
        // Text mode is not supported for files in Archives.
        // for nFlags @see IArchive::EFOpenFlags
        virtual AZ::IO::HandleType FOpen(AZStd::string_view pName, const char* mode, uint32_t nFlags = 0) = 0;

        // Read raw data from file, no endian conversion.
        virtual size_t FReadRaw(void* data, size_t length, size_t elems, AZ::IO::HandleType fileHandle) = 0;

        // Read all file contents into the provided memory, nSizeOfFile must be the same as returned by GetFileSize(handle)
        // Current seek pointer is ignored and reseted to 0.
        // no endian conversion.
        virtual size_t FReadRawAll(void* data, size_t nFileSize, AZ::IO::HandleType fileHandle) = 0;

        // Get pointer to the internally cached, loaded data of the file.
        // WARNING! The returned pointer is only valid while the fileHandle has not been closed.
        virtual void* FGetCachedFileData(AZ::IO::HandleType fileHandle, size_t& nFileSize) = 0;

        // Write file data, cannot be used for writing into the Archive.
        // Use INestedArchive interface for writing into the archivefiles.
        virtual size_t FWrite(const void* data, size_t length, size_t elems, AZ::IO::HandleType fileHandle) = 0;

        virtual int FPrintf(AZ::IO::HandleType fileHandle, const char* format, ...) = 0;
        virtual char* FGets(char*, int, AZ::IO::HandleType) = 0;
        virtual int Getc(AZ::IO::HandleType) = 0;
        virtual size_t FGetSize(AZ::IO::HandleType fileHandle) = 0;
        virtual size_t FGetSize(AZStd::string_view pName, bool bAllowUseFileSystem = false) = 0;
        virtual bool IsInPak(AZ::IO::HandleType fileHandle) = 0;
        virtual bool RemoveFile(AZStd::string_view pName) = 0; // remove file from FS (if supported)
        virtual bool RemoveDir(AZStd::string_view pName) = 0;  // remove directory from FS (if supported)
        virtual bool IsAbsPath(AZStd::string_view pPath) = 0; // determines if pPath is an absolute or relative path

        virtual size_t FSeek(AZ::IO::HandleType fileHandle, uint64_t seek, int mode) = 0;
        virtual uint64_t FTell(AZ::IO::HandleType fileHandle) = 0;
        virtual int FClose(AZ::IO::HandleType fileHandle) = 0;
        virtual int FEof(AZ::IO::HandleType fileHandle) = 0;
        virtual int FFlush(AZ::IO::HandleType fileHandle) = 0;

        //! Return pointer to pool if available
        virtual void* PoolMalloc(size_t size) = 0;
        //! Free pool
        virtual void PoolFree(void* p) = 0;

        // Return an interface to the Memory Block allocated on the File Pool memory.
        // sUsage indicates for what usage this memory was requested.
        virtual AZStd::intrusive_ptr<AZ::IO::MemoryBlock> PoolAllocMemoryBlock(size_t nSize, const char* sUsage, size_t nAlign = 1) = 0;

        // Arguments:
        //   nFlags is a combination of EPathResolutionRules flags.
        virtual ArchiveFileIterator FindFirst(AZStd::string_view pDir, EFileSearchType searchType = eFileSearchType_AllowInZipsOnly) = 0;
        virtual ArchiveFileIterator FindNext(AZ::IO::ArchiveFileIterator handle) = 0;
        virtual bool FindClose(AZ::IO::ArchiveFileIterator handle) = 0;
        //returns file modification time
        virtual IArchive::FileTime GetModificationTime(AZ::IO::HandleType fileHandle) = 0;

        // Description:
        //    Checks if specified file exist in filesystem.
        virtual bool IsFileExist(AZStd::string_view sFilename, EFileSearchLocation = eFileLocation_Any) = 0;

        // Checks if path is a folder
        virtual bool IsFolder(AZStd::string_view sPath) = 0;

        virtual IArchive::SignedFileSize GetFileSizeOnDisk(AZStd::string_view filename) = 0;

        // creates a directory
        virtual bool MakeDir(AZStd::string_view szPath, bool bGamePathMapping = false) = 0;

        // open the physical archive file - creates if it doesn't exist
        // returns NULL if it's invalid or can't open the file
        // nFlags is a combination of flags from EArchiveFlags enum.
        virtual AZStd::intrusive_ptr<INestedArchive> OpenArchive(AZStd::string_view szPath, AZStd::string_view = {}, uint32_t nFlags = 0,
            AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData = nullptr) = 0;

        // returns the path to the archive in which the file was opened
        // returns NULL if the file is a physical file, and "" if the path to archive is unknown (shouldn't ever happen)
        virtual const char* GetFileArchivePath(AZ::IO::HandleType fileHandle) = 0;

        // compresses the raw data into raw data. The buffer for compressed data itself with the heap passed. Uses method 8 (deflate)
        // returns one of the Z_* errors (Z_OK upon success)
        // MT-safe
        virtual int RawCompress(const void* pUncompressed, size_t* pDestSize, void* pCompressed, size_t nSrcSize, int nLevel = -1) = 0;

        // Uncompresses raw (without wrapping) data that is compressed with method 8 (deflated) in the Zip file
        // returns one of the Z_* errors (Z_OK upon success)
        // This function just mimics the standard uncompress (with modification taken from unzReadCurrentFile)
        // with 2 differences: there are no 16-bit checks, and
        // it initializes the inflation to start without waiting for compression method byte, as this is the
        // way it's stored into zip file
        virtual int RawUncompress(void* pUncompressed, size_t* pDestSize, const void* pCompressed, size_t nSrcSize) = 0;

        //////////////////////////////////////////////////////////////////////////
        // Files collector.
        //////////////////////////////////////////////////////////////////////////

        // Turn on/off recording of filenames of opened files.
        virtual void RecordFileOpen(ERecordFileOpenList eList) = 0;
        // Record this file if recording is enabled.
        // Arguments:
        //   in - 0 if asynchronous read
        virtual void RecordFile(AZ::IO::HandleType infileHandle, AZStd::string_view szFilename) = 0;
        // Summary:
        //    Get resource list of all recorded files, the next level, ...
        virtual IResourceList* GetResourceList(ERecordFileOpenList eList) = 0;
        virtual void SetResourceList(ERecordFileOpenList eList, IResourceList* pResourceList) = 0;

        // get the current mode, can be set by RecordFileOpen()
        virtual IArchive::ERecordFileOpenList GetRecordFileOpenList() = 0;

        // computes CRC (zip compatible) for a file
        // useful if a huge uncompressed file is generation in non continuous way
        // good for big files - low memory overhead (1MB)
        // Arguments:
        //   szPath - must not be 0
        // Returns:
        //   error code
        virtual uint32_t ComputeCRC(AZStd::string_view szPath, uint32_t nFileOpenFlags = 0) = 0;

        // computes MD5 checksum for a file
        // good for big files - low memory overhead (1MB)
        // Arguments:
        //   szPath - must not be 0
        //   md5 - destination array of uint8_t [16]
        // Returns:
        //   true if success, false on failure
        virtual bool ComputeMD5(AZStd::string_view szPath, uint8_t* md5, uint32_t nFileOpenFlags = 0, bool useDirectFileAccess = false) = 0;

        // useful for gathering file access statistics, assert if it was inserted already but then it does not become insersted
        // Arguments:
        //   pSink - must not be 0
        virtual void RegisterFileAccessSink(IArchiveFileAccessSink* pSink) = 0;
        // assert if it was not registered already
        // Arguments:
        //   pSink - must not be 0
        virtual void UnregisterFileAccessSink(IArchiveFileAccessSink* pSink) = 0;

        // When enabled, files accessed at runtime will be tracked
        virtual void DisableRuntimeFileAccess(bool status) = 0;
        virtual bool DisableRuntimeFileAccess(bool status, AZStd::thread_id threadId) = 0;
        virtual bool CheckFileAccessDisabled(AZStd::string_view name, const char* mode) = 0;
        virtual void SetRenderThreadId(AZStd::thread_id renderThreadId) = 0;

        // gets the current pak priority
        virtual ArchiveLocationPriority GetPakPriority() const = 0;

        // Summary:
        // Return offset in archive file (ideally has to return offset on DVD) for streaming requests sorting
        virtual uint64_t GetFileOffsetOnMedia(AZStd::string_view szName) const = 0;

        // Summary:
        // Return media type for the file
        virtual EStreamSourceMediaType GetFileMediaType(AZStd::string_view szName) const = 0;

        // Event sent when a archive file is opened that contains a level.pak
        // @param const AZStd::vector<AZStd::string>& - Array of directories containing level.pak files
        using LevelPackOpenEvent = AZ::Event<const AZStd::vector<AZStd::string>&>;
        virtual auto GetLevelPackOpenEvent()->LevelPackOpenEvent* = 0;
        // Event sent when a archive contains a level.pak is closed
        // @param const AZStd::string_view - Name of the pak file that was closed
        using LevelPackCloseEvent = AZ::Event<AZStd::string_view>;
        virtual auto GetLevelPackCloseEvent()->LevelPackCloseEvent* = 0;

        // Type-safe endian conversion read.
        template<class T>
        size_t FRead(T* data, size_t elems, AZ::IO::HandleType fileHandle, bool bSwapEndian = false)
        {
            size_t count = FReadRaw(data, sizeof(T), elems, fileHandle);
            SwapEndian(data, count, bSwapEndian);
            return count;
        }
        // Type-independent Write.
        template<class T>
        void FWrite(T* data, size_t elems, AZ::IO::HandleType fileHandle)
        {
            FWrite((void*)data, sizeof(T), elems, fileHandle);
        }

        inline static constexpr IArchive::SignedFileSize FILE_NOT_PRESENT = -1;
    };

    class ScopedFileHandle
    {
    public:
        ScopedFileHandle(IArchive& archiveInstance, AZStd::string_view fileName, const char* mode)
            : m_archive(archiveInstance)
        {
            m_fileHandle = m_archive.FOpen(fileName, mode);
        }

        ~ScopedFileHandle()
        {
            if (m_fileHandle != AZ::IO::InvalidHandle)
            {
                m_archive.FClose(m_fileHandle);
            }
        }

        operator AZ::IO::HandleType()
        {
            return m_fileHandle;
        }

        bool IsValid() const
        {
            return m_fileHandle != AZ::IO::InvalidHandle;
        }

    private:
        AZ::IO::HandleType m_fileHandle;
        IArchive& m_archive;
    };


    // The IResourceList provides an access to the collection of the resource`s file names.
    // Client can add a new file names to the resource list and check if resource already in the list.
    struct IResourceList
        : public AZStd::intrusive_base
    {
        // <interfuscator:shuffle>
        // Description:
        //    Adds a new resource to the list.
        virtual void Add(AZStd::string_view sResourceFile) = 0;

        // Description:
        //    Clears resource list.
        virtual void Clear() = 0;

        // Description:
        //    Checks if specified resource exist in the list.
        virtual bool IsExist(AZStd::string_view sResourceFile) = 0;

        // Description:
        //    Loads a resource list from the resource list file.
        virtual bool Load(AZStd::string_view sResourceListFilename) = 0;

        //////////////////////////////////////////////////////////////////////////
        // Enumeration.
        //////////////////////////////////////////////////////////////////////////

        // Description:
        //    Returns the file name of the first resource or NULL if resource list is empty.
        virtual const char* GetFirst() = 0;
        // Description:
        //    Returns the file name of the next resource or NULL if reached the end.
        //    Client must call GetFirst before calling GetNext.
        virtual const char* GetNext() = 0;

    };
}
