/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/fixed_string.h>

#include <AzFramework/Archive/ArchiveFindData.h>
#include <AzFramework/Archive/ArchiveVars.h>

enum EStreamSourceMediaType : int32_t;

namespace AZ::IO
{
    enum class FileSearchPriority;
    struct IResourceList;
    struct INestedArchive;
    struct IArchive;

    using PathString = AZ::IO::FixedMaxPathString;
    using StackString = AZStd::fixed_string<512>;

    struct MemoryBlock
        : AZStd::intrusive_base
    {
        AZ_CLASS_ALLOCATOR(MemoryBlock, AZ::SystemAllocator);
        MemoryBlock() = default;

        struct AddressDeleter
        {
            void operator()(uint8_t* ptr) const
            {
                azfree(ptr);
            }
        };
        using AddressPtr = AZStd::unique_ptr<uint8_t[], AddressDeleter>;

        AddressPtr m_address;
        size_t m_size{};
    };

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

        //
        enum ERecordFileOpenList
        {
            RFOM_Disabled,                          // file open are not recorded (fast, no extra memory)
            RFOM_EngineStartup,                 // before a level is loaded
            RFOM_Level,                                 // during level loading till export2game -> resourcelist.txt, used to generate the list for level2level loading
            RFOM_NextLevel                          // used for level2level loading
        };

        enum EInMemoryArchiveLocation
        {
            eInMemoryPakLocale_Unload = 0,
            eInMemoryPakLocale_CPU,
            eInMemoryPakLocale_GPU,
            eInMemoryPakLocale_PAK,
        };


        using SignedFileSize = int64_t;

        virtual ~IArchive() = default;

        // after this call, the archive file will be searched for files when they aren't on the OS file system
        // Arguments:
        //   pName - must not be 0
        virtual bool OpenPack(AZStd::string_view pName, AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData = {},
            AZ::IO::FixedMaxPathString* pFullPath = nullptr, bool addLevels = true) = 0;
        // after this call, the archive file will be searched for files when they aren't on the OS file system
        virtual bool OpenPack(AZStd::string_view pBindingRoot, AZStd::string_view pName,
            AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData = {}, AZ::IO::FixedMaxPathString* pFullPath = nullptr, bool addLevels = true) = 0;
        // after this call, the file will be unlocked and closed, and its contents won't be used to search for files
        virtual bool ClosePack(AZStd::string_view pName) = 0;
        // opens pack files by the path and wildcard
        virtual bool OpenPacks(AZStd::string_view pWildcard, AZStd::vector<AZ::IO::FixedMaxPathString>* pFullPaths = nullptr) = 0;
        // opens pack files by the path and wildcard
        virtual bool OpenPacks(AZStd::string_view pBindingRoot, AZStd::string_view pWildcard,
            AZStd::vector<AZ::IO::FixedMaxPathString>* pFullPaths = nullptr) = 0;
        // closes pack files by the path and wildcard
        virtual bool ClosePacks(AZStd::string_view pWildcard) = 0;
        //returns if a archive exists matching the wildcard
        virtual bool FindPacks(AZStd::string_view pWildcardIn) = 0;

        // Set access status of a archive files with a wildcard
        virtual bool SetPacksAccessible(bool bAccessible, AZStd::string_view pWildcard) = 0;

        // Set access status of a pack file
        virtual bool SetPackAccessible(bool bAccessible, AZStd::string_view pName) = 0;

        // Set and Get the localization folder name (Languages, Localization, ...)
        virtual void SetLocalizationFolder(AZStd::string_view sLocalizationFolder) = 0;
        virtual const char* GetLocalizationFolder() const = 0;
        virtual const char* GetLocalizationRoot() const = 0;

        // Open file handle, file can be on disk or in Archive file.
        // Possible mode is r,b,x
        // ex: AZ::IO::HandleType fileHandle = FOpen( "test.txt","rbx" );
        // mode x is a direct access mode, when used file reads will go directly into the low level file system without any internal data caching.
        // Text mode is not supported for files in Archives.
        virtual AZ::IO::HandleType FOpen(AZStd::string_view pName, const char* mode) = 0;

        // Get pointer to the internally cached, loaded data of the file.
        // WARNING! The returned pointer is only valid while the fileHandle has not been closed.
        virtual void* FGetCachedFileData(AZ::IO::HandleType fileHandle, size_t& nFileSize) = 0;

        // Read raw data from file, no endian conversion.
        virtual size_t FRead(void* data, size_t bytesToRead, AZ::IO::HandleType fileHandle) = 0;

        // Write file data, cannot be used for writing into the Archive.
        // Use INestedArchive interface for writing into the archive files.
        virtual size_t FWrite(const void* data, size_t bytesToWrite, AZ::IO::HandleType fileHandle) = 0;

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

        // Arguments:
        virtual ArchiveFileIterator FindFirst(AZStd::string_view pDir, FileSearchLocation searchType = FileSearchLocation::InPak) = 0;
        virtual ArchiveFileIterator FindNext(AZ::IO::ArchiveFileIterator handle) = 0;
        virtual bool FindClose(AZ::IO::ArchiveFileIterator handle) = 0;
        //returns file modification time
        virtual IArchive::FileTime GetModificationTime(AZ::IO::HandleType fileHandle) = 0;

        // Description:
        //    Checks if specified file exist in filesystem.
        virtual bool IsFileExist(AZStd::string_view sFilename, FileSearchLocation = FileSearchLocation::Any) = 0;

        // Checks if path is a folder
        virtual bool IsFolder(AZStd::string_view sPath) = 0;

        virtual IArchive::SignedFileSize GetFileSizeOnDisk(AZStd::string_view filename) = 0;

        // open the physical archive file - creates if it doesn't exist
        // returns NULL if it's invalid or can't open the file
        // nFlags is a combination of flags from EArchiveFlags enum.
        virtual AZStd::intrusive_ptr<INestedArchive> OpenArchive(AZStd::string_view szPath, AZStd::string_view = {}, uint32_t nFlags = 0,
            AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData = nullptr) = 0;

        // returns the path to the archive in which the file was opened
        // returns empty path view if the file is a physical file
        virtual AZ::IO::PathView GetFileArchivePath(AZ::IO::HandleType fileHandle) = 0;

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

        // useful for gathering file access statistics, assert if it was inserted already but then it does not become inserted
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

        // gets the current pak priority
        virtual FileSearchPriority GetPakPriority() const = 0;

        // Summary:
        // Return offset in archive file (ideally has to return offset on DVD) for streaming requests sorting
        virtual uint64_t GetFileOffsetOnMedia(AZStd::string_view szName) const = 0;

        // Summary:
        // Return media type for the file
        virtual EStreamSourceMediaType GetFileMediaType(AZStd::string_view szName) const = 0;

        // Event sent when a archive file is opened that contains a level.pak
        // @param const AZStd::vector<AZStd::string>& - Array of directories containing level.pak files
        using LevelPackOpenEvent = AZ::Event<const AZStd::vector<AZ::IO::Path>&>;
        virtual auto GetLevelPackOpenEvent()->LevelPackOpenEvent* = 0;
        // Event sent when a archive contains a level.pak is closed
        // @param const AZStd::string_view - Name of the pak file that was closed
        using LevelPackCloseEvent = AZ::Event<AZStd::string_view>;
        virtual auto GetLevelPackCloseEvent()->LevelPackCloseEvent* = 0;

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
