/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <AzCore/base.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/std/sort.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetBundleManifest.h>
#include <AzFramework/Asset/AssetRegistry.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/Archive/Archive.h>
#include <AzFramework/Archive/NestedArchive.h>
#include <AzFramework/Archive/ArchiveBus.h>
#include <AzFramework/Archive/ArchiveVars.h>
#include <AzFramework/Archive/MissingFileReport.h>
#include <AzFramework/Archive/ZipDirFind.h>
#include <AzFramework/Archive/ZipDirCacheFactory.h>

#include <cinttypes>

namespace AZ::IO
{
    AZ_CVAR(int, sys_PakPriority, aznumeric_cast<int>(ArchiveVars{}.nPriority), nullptr, AZ::ConsoleFunctorFlags::Null,
        "If set to 1, tells Archive to try to open the file in pak first, then go to file system");
    AZ_CVAR(int, sys_PakMessageInvalidFileAccess, ArchiveVars{}.nMessageInvalidFileAccess, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Message Box synchronous file access when in game");

    AZ_CVAR(int, sys_PakWarnOnPakAccessFailures, ArchiveVars{}.nWarnOnPakAccessFails, nullptr, AZ::ConsoleFunctorFlags::Null,
        "If 1, access failure for Paks is treated as a warning, if zero it is only a log message.");
    AZ_CVAR(int, sys_report_files_not_found_in_paks, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Reports when files are searched for in paks and not found. 1 = log, 2 = warning, 3 = error");
    AZ_CVAR(int32_t, az_archive_verbosity, 0, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Sets the verbosity level for logging Archive operations\n"
        ">=1 - Turns on verbose logging of all operations");
}

namespace AZ::IO::ArchiveInternal
{
    // this is the start of indexation of pseudofiles:
    // to the actual index , this offset is added to get the valid handle
    static constexpr size_t PseudoFileIdxOffset = 1;

    struct CCachedFileRawData
    {
        void* m_pCachedData;

        CCachedFileRawData(size_t nAlloc);
        ~CCachedFileRawData();
    };

    CCachedFileRawData::CCachedFileRawData(size_t nAlloc)
    {
        m_pCachedData = AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(nAlloc, alignof(uint8_t), 0, "CCachedFileRawData::CCachedFileRawData");
    }

    CCachedFileRawData::~CCachedFileRawData()
    {
        if (m_pCachedData)
        {
            AZ::AllocatorInstance<AZ::OSAllocator>::Get().DeAllocate(m_pCachedData);
        }
        m_pCachedData = nullptr;
    }

    // an (inside zip) emulated open file
    struct CZipPseudoFile
    {
        AZ_CLASS_ALLOCATOR(CZipPseudoFile, AZ::SystemAllocator, 0);
        CZipPseudoFile()
        {
            Construct();
        }
        ~CZipPseudoFile()
        {
        }


        // this object must be constructed before usage
        // nFlags is a combination of _O_... flags
        void Construct(CCachedFileData* pFileData = nullptr);
        // this object needs to be freed manually when the Archive shuts down..
        void Destruct();

        CCachedFileDataPtr GetFile() { return m_pFileData; }

        uint64_t FTell() { return m_nCurSeek; }

        uint32_t GetFileSize() { return GetFile() ? GetFile()->GetFileEntry()->desc.lSizeUncompressed : 0; }

        int FSeek(uint64_t nOffset, int nMode);
        size_t FRead(void* pDest, size_t bytesToRead, AZ::IO::HandleType fileHandle);
        void* GetFileData(size_t& nFileSize, AZ::IO::HandleType fileHandle);
        int FEof();

        uint64_t GetModificationTime() { return m_pFileData->GetFileEntry()->GetModificationTime(); }
        AZ::IO::PathView GetArchivePath() { return m_pFileData->GetZip()->GetFilePath(); }
    protected:
        uint64_t m_nCurSeek;
        CCachedFileDataPtr m_pFileData;
    };

    //////////////////////////////////////////////////////////////////////////
    // this object must be constructed before usage
    // nFlags is a combination of _O_... flags
    void ArchiveInternal::CZipPseudoFile::Construct(CCachedFileData* pFileData)
    {
        m_pFileData = pFileData;
        m_nCurSeek = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // this object needs to be freed manually when the Archive shuts down..
    void ArchiveInternal::CZipPseudoFile::Destruct()
    {
        AZ_Assert(m_pFileData, "Destruct was invoked on a nullptr m_pFileData");
        // mark it free, and deallocate the pseudo file memory
        m_pFileData.reset();
    }

    //////////////////////////////////////////////////////////////////////////
    int ArchiveInternal::CZipPseudoFile::FSeek(uint64_t nOffset, int nMode)
    {
        if (!m_pFileData)
        {
            return -1;
        }

        switch (nMode)
        {
        case SEEK_SET:
            m_nCurSeek = nOffset;
            break;
        case SEEK_CUR:
            m_nCurSeek += nOffset;
            break;
        case SEEK_END:
            m_nCurSeek = GetFileSize() + nOffset;
            break;
        default:
            AZ_Assert(false, "Invalid seek option has been supplied to FSeek");
            return -1;
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    size_t ArchiveInternal::CZipPseudoFile::FRead(void* pDest, size_t bytesToRead, [[maybe_unused]] AZ::IO::HandleType fileHandle)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!GetFile())
        {
            return 0;
        }

        size_t nTotal = bytesToRead;
        if (!nTotal || (uint32_t)m_nCurSeek >= GetFileSize())
        {
            return 0;
        }

        nTotal = AZStd::min<size_t>(nTotal, GetFileSize() - m_nCurSeek);

        int64_t nReadBytes = GetFile()->ReadData(pDest, m_nCurSeek, nTotal);
        if (nReadBytes == -1)
        {
            return 0;
        }

        if (static_cast<size_t>(nReadBytes) != nTotal)
        {
            AZ_Warning("Archive", false, "FRead did not read expected number of byte from file, only %zu of %lld bytes read", nTotal, nReadBytes);
            nTotal = (size_t)nReadBytes;
        }
        m_nCurSeek += nTotal;
        return nTotal;
    }


    //////////////////////////////////////////////////////////////////////////
    void* ArchiveInternal::CZipPseudoFile::GetFileData(size_t& nFileSize, [[maybe_unused]] AZ::IO::HandleType fileHandle)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!GetFile())
        {
            return nullptr;
        }

        nFileSize = GetFileSize();

        void* pData = GetFile()->GetData();
        m_nCurSeek = nFileSize;
        return pData;
    }

    //////////////////////////////////////////////////////////////////////////
    int ArchiveInternal::CZipPseudoFile::FEof()
    {
        return (uint32_t)m_nCurSeek >= GetFileSize();
    }

}

namespace AZ::IO
{
    struct Archive::CachedRawDataEntry
    {
        AZStd::unique_ptr<ArchiveInternal::CCachedFileRawData> m_data;
        size_t m_fileSize = 0;
    };

    //////////////////////////////////////////////////////////////////////////
    // IResourceList implementation class.
    //////////////////////////////////////////////////////////////////////////
    class CResourceList
        : public IResourceList
    {
    public:
        AZ_CLASS_ALLOCATOR(CResourceList, AZ::SystemAllocator, 0);
        CResourceList() { m_iter = m_set.end(); }
        ~CResourceList() override {}

        void Add(AZStd::string_view sResourceFile) override
        {
            if (sResourceFile.empty())
            {
                return;
            }
            AZ::IO::FixedMaxPath convertedFilename;
            if (!AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(convertedFilename, sResourceFile))
            {
                AZ_Error("Archive", false, "Path %.*s cannot be resolved. It is longer than MaxPathLength %zu",
                    AZ_STRING_ARG(sResourceFile), AZ::IO::MaxPathLength);
                return;
            }

            AZStd::scoped_lock lock(m_lock);
            m_set.emplace(convertedFilename);
        }
        void Clear() override
        {
            AZStd::scoped_lock lock(m_lock);
            m_set.clear();
            m_iter = m_set.end();
        }
        bool IsExist(AZStd::string_view sResourceFile) override
        {
            AZ::IO::FixedMaxPath convertedFilename;
            if (!AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(convertedFilename, sResourceFile))
            {
                AZ_Error("Archive", false, "Path %.*s cannot be resolved. It is longer than MaxPathLength %zu",
                    AZ_STRING_ARG(sResourceFile), AZ::IO::MaxPathLength);
                return false;
            }

            AZStd::scoped_lock lock(m_lock);
            return m_set.contains(AZ::IO::PathView{ convertedFilename });
        }
        bool Load(AZStd::string_view sResourceListFilename) override
        {
            Clear();
            AZ::IO::SystemFile file;
            if (AZ::IO::PathString resourcePath{ sResourceListFilename }; file.Open(resourcePath.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
            {
                AZStd::scoped_lock lock(m_lock);

                AZ::IO::SizeType nLen = file.Length();
                AZStd::string pMemBlock;
                pMemBlock.resize_no_construct(nLen);;
                file.Read(pMemBlock.size(), pMemBlock.data());

                // Parse file, every line in a file represents a resource filename.
                AZ::StringFunc::TokenizeVisitor(pMemBlock,
                    [this](AZStd::string_view token)
                    {
                        Add(token);
                    }, "\r\n");
                return true;
            }
            return false;
        }
        const char* GetFirst() override
        {
            AZStd::scoped_lock lock(m_lock);
            if (!m_set.empty())
            {
                m_iter = m_set.begin();
                return m_iter->c_str();
            }
            return nullptr;
        }
        const char* GetNext() override
        {
            AZStd::scoped_lock lock(m_lock);
            if (m_iter != m_set.end())
            {
                ++m_iter;
                if (m_iter != m_set.end())
                {
                    return m_iter->c_str();
                }
            }
            return nullptr;
        }

    private:
        using ResourceSet = AZStd::set<AZ::IO::Path, AZStd::less<>>;
        AZStd::recursive_mutex m_lock;
        ResourceSet m_set;
        ResourceSet::iterator m_iter;
    };

    //////////////////////////////////////////////////////////////////////////
    // Automatically calculate time taken by file operations.
    //////////////////////////////////////////////////////////////////////////
    struct SAutoCollectFileAccessTime
    {
        SAutoCollectFileAccessTime(Archive* pArchive)
            : m_pArchive{ pArchive }
            , m_startTime{ AZStd::chrono::system_clock::now() }
        {
        }
        ~SAutoCollectFileAccessTime()
        {
            m_pArchive->m_fFileAccessTime += aznumeric_cast<float>(AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::system_clock::now() - m_startTime).count());
        }
    private:
        Archive* m_pArchive;
        AZStd::chrono::system_clock::time_point m_startTime;
    };

    /////////////////////////////////////////////////////
    // Initializes the archive system;
    //   pVarPakPriority points to the variable, which is, when set to 1,
    //   signals that the files from archive should have higher priority than filesystem files
    Archive::Archive()
        : m_pEngineStartupResourceList{ new CResourceList{} }
        , m_pLevelResourceList{ new CResourceList{} }
        , m_pNextLevelResourceList{ new CResourceList{} }
        , m_mainThreadId{ AZStd::this_thread::get_id() }
    {
        CompressionBus::Handler::BusConnect();
    }

    //////////////////////////////////////////////////////////////////////////
    Archive::~Archive()
    {
        CompressionBus::Handler::BusDisconnect();

        m_arrZips = {};

        uint32_t numFilesForcedToClose = 0;
        // scan through all open files and close them
        {
            AZStd::unique_lock lock(m_csOpenFiles);
            for (ZipPseudoFileArray::iterator itFile = m_arrOpenFiles.begin(); itFile != m_arrOpenFiles.end(); ++itFile)
            {
                if ((*itFile)->GetFile())
                {
                    (*itFile)->Destruct();
                    ++numFilesForcedToClose;
                }
            }
            m_arrOpenFiles = {};
        }

        AZ_Warning("Archive", numFilesForcedToClose == 0, "%u files were forced to close", numFilesForcedToClose);

        AZ_Error("Archive", m_arrArchives.empty(), "There are %zu external references to archive objects: they have dangling pointers and will either lead to memory leaks or crashes", m_arrArchives.size());

        AZ_Assert(m_cachedFileRawDataSet.empty(), "All Archive file cached raw data instances not closed");
    }

    void Archive::LogFileAccessCallStack([[maybe_unused]] AZStd::string_view name, [[maybe_unused]] AZStd::string_view nameFull, [[maybe_unused]] const char* mode)
    {
        // Print call stack for each find.
        AZ_TracePrintf("Archive", "LogFileAccessCallStack() - name=%.*s; nameFull=%.*s; mode=%s\n", AZ_STRING_ARG(name), AZ_STRING_ARG(nameFull), mode);
        AZ::Debug::Trace::PrintCallstack("Archive", 32);
    }

    //////////////////////////////////////////////////////////////////////////
    void Archive::SetLocalizationFolder(AZStd::string_view sLocalizationFolder)
    {
        if (m_sLocalizationFolder.empty())
        {
            m_sLocalizationRoot = sLocalizationFolder;
            m_sLocalizationRoot += AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING;
            m_sLocalizationFolder = m_sLocalizationRoot;
            return;
        }

        // Get the localization folder
        m_sLocalizationFolder = sLocalizationFolder;
        m_sLocalizationFolder += AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING;
    }


    //////////////////////////////////////////////////////////////////////////
    bool Archive::IsFileExist(AZStd::string_view sFilename, EFileSearchLocation fileLocation)
    {
        const AZ::IO::ArchiveLocationPriority nVarPakPriority = GetPakPriority();

        auto szFullPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(sFilename);
        if (!szFullPath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(sFilename.size()), sFilename.data());
            return false;
        }

        switch(fileLocation)
        {
        case IArchive::eFileLocation_Any:
            // Search for file based on pak priority
            switch (nVarPakPriority)
            {
            case ArchiveLocationPriority::ePakPriorityFileFirst:
                return FileIOBase::GetDirectInstance()->Exists(szFullPath->c_str()) || FindPakFileEntry(szFullPath->Native());
            case ArchiveLocationPriority::ePakPriorityPakFirst:
                return FindPakFileEntry(szFullPath->Native()) || IO::FileIOBase::GetDirectInstance()->Exists(szFullPath->c_str());
            case ArchiveLocationPriority::ePakPriorityPakOnly:
                return FindPakFileEntry(szFullPath->Native());
            default:
                AZ_Assert(false, "PakPriority %d doesn't match a value in the ArchiveLocationPriority enum",
                    aznumeric_cast<int>(nVarPakPriority));
            }
            break;
        case IArchive::eFileLocation_InPak:
            return FindPakFileEntry(szFullPath->Native());
        case IArchive::eFileLocation_OnDisk:
            if (nVarPakPriority != ArchiveLocationPriority::ePakPriorityPakOnly)
            {
                return FileIOBase::GetDirectInstance()->Exists(szFullPath->c_str());
            }
            break;
        default:
            AZ_Assert(false, "File Search Location %d didn't match either eFileLocation_Any, eFileLocation_InPak, or eFileLocation_OnDisk",
                aznumeric_cast<int>(fileLocation));
            break;
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    bool Archive::IsFolder(AZStd::string_view sPath)
    {
        AZStd::fixed_string<AZ::IO::MaxPathLength > filePath{ sPath };
        return AZ::IO::FileIOBase::GetDirectInstance()->IsDirectory(filePath.c_str());
    }


    IArchive::SignedFileSize Archive::GetFileSizeOnDisk(AZStd::string_view filename)
    {
        AZ::u64 fileSize = 0;
        if (AZ::IO::PathString filepath{ filename }; AZ::IO::FileIOBase::GetDirectInstance()->Size(filepath.c_str(), fileSize))
        {
            return static_cast<IArchive::SignedFileSize>(fileSize);
        }
        return IArchive::FILE_NOT_PRESENT;
    }

    //////////////////////////////////////////////////////////////////////////
    AZ::IO::HandleType Archive::FOpen(AZStd::string_view pName, const char* szMode)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        const size_t pathLen = pName.size();
        if (pathLen == 0 || pathLen >= AZ::IO::MaxPathLength)
        {
            return AZ::IO::InvalidHandle;
        }

        SAutoCollectFileAccessTime accessTime(this);

        // get the priority into local variable to avoid it changing in the course of
        // this function execution (?)
        const ArchiveLocationPriority nVarPakPriority = GetPakPriority();

        AZ::IO::OpenMode nOSFlags = AZ::IO::GetOpenModeFromStringMode(szMode);

        AZ::IO::FixedMaxPath szFullPath;
        if (!AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(szFullPath, pName))
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(pName.size()), pName.data());
            return false;
        }

        const bool fileWritable = (nOSFlags & (AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeAppend | AZ::IO::OpenMode::ModeUpdate)) != AZ::IO::OpenMode::Invalid;
        AZ_PROFILE_SCOPE(Game, "File: %s Archive: %p", szFullPath.c_str(), this);
        if (fileWritable)
        {
            // we need to open the file for writing, but we failed to do so.
            // the only reason that can be is that there are no directories for that file.
            // now create those dirs
            if (AZ::IO::FixedMaxPath parentPath = szFullPath.ParentPath();
                !AZ::IO::FileIOBase::GetDirectInstance()->CreatePath(parentPath.c_str()))
            {
                return AZ::IO::InvalidHandle;
            }

            if (AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                AZ::IO::FileIOBase::GetDirectInstance()->Open(szFullPath.c_str(), nOSFlags, fileHandle))
            {
                if (az_archive_verbosity)
                {
                    AZ_TracePrintf("Archive", "<Archive LOG FILE ACCESS> Archive::FOpen() has directly opened requested file %s for writing", szFullPath.c_str());
                }
                return fileHandle;
            }

            return AZ::IO::InvalidHandle;
        }

        auto OpenFromFileSystem = [this, &szFullPath, pName, nOSFlags]() -> HandleType
        {
            if (AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
                AZ::IO::FileIOBase::GetDirectInstance()->Open(szFullPath.c_str(), nOSFlags, fileHandle))
            {
                if (az_archive_verbosity)
                {
                    AZ_TracePrintf("Archive", "<Archive LOG FILE ACCESS> Archive::FOpen() has directly opened requested file %s on for reading", szFullPath.c_str());
                }
                RecordFile(fileHandle, pName);

                return fileHandle;
            }

            return AZ::IO::InvalidHandle;
        };
        auto OpenFromArchive = [this, &szFullPath, pName]() -> HandleType
        {
            uint32_t archiveFlags = 0;
            CCachedFileDataPtr pFileData = GetFileData(szFullPath.Native(), archiveFlags);
            if (pFileData == nullptr)
            {
                return AZ::IO::InvalidHandle;
            }

            bool logged = false;
            if (ZipDir::Cache* pZip = pFileData->GetZip(); pZip != nullptr)
            {
                AZ::IO::PathView pZipFilePath = pZip->GetFilePath();
                if (!pZipFilePath.empty())
                {
                    if (az_archive_verbosity)
                    {
                        AZ_TracePrintf("Archive", "<Archive LOG FILE ACCESS> Archive::FOpen() has opened requested file %s from archive %.*s, disk offset %u",
                            szFullPath.c_str(), AZ_STRING_ARG(pZipFilePath.Native()), pFileData->GetFileEntry()->nFileDataOffset);
                        logged = true;
                    }
                }
            }

            if (!logged)
            {
                if (az_archive_verbosity)
                {
                    AZ_TracePrintf("Archive", "<Archive LOG FILE ACCESS> Archive::FOpen() has opened requested file %s from an archive file who's path isn't known",
                        szFullPath.c_str());
                }
            }


            size_t nFile;
            // find the empty slot and open the file there; return the handle
            {
                // try to open the pseudofile from one of the zips, make sure there is no user alias
                AZStd::unique_lock lock(m_csOpenFiles);
                for (nFile = 0; nFile < m_arrOpenFiles.size() && m_arrOpenFiles[nFile]->GetFile(); ++nFile)
                {
                    continue;
                }
                if (nFile == m_arrOpenFiles.size())
                {
                    m_arrOpenFiles.emplace_back(AZStd::make_unique<ArchiveInternal::CZipPseudoFile>());
                }
                AZStd::unique_ptr<ArchiveInternal::CZipPseudoFile>& rZipFile = m_arrOpenFiles[nFile];
                rZipFile->Construct(pFileData.get());
            }

            AZ::IO::HandleType handle = (AZ::IO::HandleType)(nFile + ArchiveInternal::PseudoFileIdxOffset);

            RecordFile(handle, pName);

            return handle; // the handle to the file
        };

        switch (nVarPakPriority)
        {
        case ArchiveLocationPriority::ePakPriorityFileFirst:
        {
            AZ::IO::HandleType fileHandle = OpenFromFileSystem();
            return fileHandle != AZ::IO::InvalidHandle ? fileHandle : OpenFromArchive();
        }
        case ArchiveLocationPriority::ePakPriorityPakFirst:
        {
            AZ::IO::HandleType fileHandle = OpenFromArchive();
            return fileHandle != AZ::IO::InvalidHandle ? fileHandle : OpenFromFileSystem();
        }
        case ArchiveLocationPriority::ePakPriorityPakOnly:
        {
            return OpenFromArchive();
        }
        default:
            return AZ::IO::InvalidHandle;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // given the file name, searches for the file entry among the zip files.
    // if there's such file in one of the zips, then creates (or used cached)
    // CCachedFileData instance and returns it.
    // The file data object may be created in this function,
    // and it's important that the intrusive is returned: another thread may release the existing
    // cached data before the function returns
    // the path must be absolute normalized lower-case with forward-slashes
    CCachedFileDataPtr Archive::GetFileData(AZStd::string_view szName, uint32_t& nArchiveFlags, ZipDir::CachePtr* pZip)
    {
        CCachedFileData* pResult{};

        ZipDir::CachePtr archive;
        ZipDir::FileEntry* pFileEntry = FindPakFileEntry(szName, nArchiveFlags, &archive);
        if (pFileEntry)
        {
            pResult = new CCachedFileData(archive, nArchiveFlags, pFileEntry, szName);
        }
        else
        {
            AZ::IO::PathString missingFilepath{ szName };
            AZ::IO::Internal::ReportFileMissingFromArchive(missingFilepath.c_str());
        }

        if (pZip)
        {
            *pZip = archive;
        }

        return pResult;
    }

    CCachedFileDataPtr Archive::GetFileData(ZipDir::CachePtr zipFile, AZStd::string_view fileName)
    {
        ZipDir::FileEntry* pFileEntry = zipFile->FindFile(fileName);
        return pFileEntry ? new CCachedFileData(zipFile, 0, pFileEntry, fileName) : nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    CCachedFileDataPtr Archive::GetOpenedFileDataInZip(AZ::IO::HandleType fileHandle)
    {
        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        return pseudoFile ? pseudoFile->GetFile() : nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    // tests if the given file path refers to an existing file inside registered (opened) packs
    // the path must be absolute normalized lower-case with forward-slashes
    ZipDir::FileEntry* Archive::FindPakFileEntry(AZStd::string_view szPath, uint32_t& nArchiveFlags, ZipDir::CachePtr* pZip) const
    {
        AZ::IO::FixedMaxPath resolvedPath;
        if (!AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(resolvedPath, szPath))
        {
            AZ_Error("Archive", false, "Path %s cannot be converted to @alias@ form. It is longer than MaxPathLength %zu", aznumeric_cast<int>(szPath.size()),
                szPath.data(), AZ::IO::MaxPathLength);
            return nullptr;
        }


        AZStd::shared_lock lock(m_csZips);
        // scan through registered archive files and try to find this file
        for (auto itZip = m_arrZips.rbegin(); itZip != m_arrZips.rend(); ++itZip)
        {
            if (itZip->pArchive->GetFlags() & INestedArchive::FLAGS_DISABLE_PAK)
            {
                continue;
            }


            // If the bindRootIter is at the end then it is a prefix of the source path
            if (resolvedPath.IsRelativeTo(itZip->m_pathBindRoot))
            {
                // unaliasedIter is past the bind root, so append the rest of it to a new relative path object
                AZ::IO::FixedMaxPath relativePathInZip = resolvedPath.LexicallyRelative(itZip->m_pathBindRoot);

                ZipDir::FileEntry* pFileEntry = itZip->pZip->FindFile(relativePathInZip.Native());
                if (pFileEntry)
                {
                    if (pZip)
                    {
                        *pZip = itZip->pZip;
                    }

                    nArchiveFlags = itZip->pArchive->GetFlags();
                    return pFileEntry;
                }
            }
        }
        nArchiveFlags = 0;

        return nullptr;
    }

    ZipDir::FileEntry* Archive::FindPakFileEntry(AZStd::string_view szPath) const
    {
        uint32_t flags;
        return FindPakFileEntry(szPath, flags);
    }

    uint64_t Archive::FTell(AZ::IO::HandleType fileHandle)
    {
        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        if (pseudoFile)
        {
            return pseudoFile->FTell();
        }
        else
        {
            AZ::u64 returnValue = 0;
            AZ::IO::FileIOBase::GetDirectInstance()->Tell(fileHandle, returnValue);
            return returnValue;
        }
    }

    // returns the path to the archive in which the file was opened
    AZ::IO::PathView Archive::GetFileArchivePath(AZ::IO::HandleType fileHandle)
    {
        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        if (pseudoFile)
        {
            return pseudoFile->GetArchivePath();
        }
        else
        {
            return {};
        }
    }

    // returns the file modification time
    uint64_t Archive::GetModificationTime(AZ::IO::HandleType fileHandle)
    {
        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        if (pseudoFile)
        {
            return pseudoFile->GetModificationTime();
        }

        return AZ::IO::FileIOBase::GetDirectInstance()->ModificationTime(fileHandle);
    }

    size_t Archive::FGetSize(AZ::IO::HandleType fileHandle)
    {
        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        if (pseudoFile)
        {
            return pseudoFile->GetFileSize();
        }

        AZ::u64 fileSize = 0;
        AZ::IO::FileIOBase::GetDirectInstance()->Size(fileHandle, fileSize);
        return fileSize;
    }

    //////////////////////////////////////////////////////////////////////////
    size_t Archive::FGetSize(AZStd::string_view sFilename, bool bAllowUseFileSystem)
    {
        auto fullPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(sFilename);
        if (!fullPath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(sFilename.size()), sFilename.data());
            return 0;
        }

        if (GetPakPriority() == ArchiveLocationPriority::ePakPriorityFileFirst) // if the file system files have priority now..
        {
            IArchive::SignedFileSize nFileSize = GetFileSizeOnDisk(fullPath->Native());
            if (nFileSize != IArchive::FILE_NOT_PRESENT)
            {
                return aznumeric_cast<size_t>(nFileSize);
            }
        }

        ZipDir::FileEntry* pFileEntry = FindPakFileEntry(fullPath->Native());
        if (pFileEntry) // try to find the pseudo-file in one of the zips
        {
            return pFileEntry->desc.lSizeUncompressed;
        }

        if (bAllowUseFileSystem || GetPakPriority() == ArchiveLocationPriority::ePakPriorityPakFirst) // if the archive files had more priority, we didn't attempt fopen before- try it now
        {
            IArchive::SignedFileSize nFileSize = GetFileSizeOnDisk(fullPath->Native());
            if (nFileSize != IArchive::FILE_NOT_PRESENT)
            {
                return aznumeric_cast<size_t>(nFileSize);
            }
        }

        return 0;
    }

    int Archive::FFlush(AZ::IO::HandleType fileHandle)
    {
        SAutoCollectFileAccessTime accessTime(this);

        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        if (pseudoFile)
        {
            return 0;
        }


        if (AZ::IO::FileIOBase::GetDirectInstance()->Flush(fileHandle))
        {
            return 0;
        }
        return 1;
    }

    size_t Archive::FSeek(AZ::IO::HandleType fileHandle, uint64_t seek, int mode)
    {
        SAutoCollectFileAccessTime accessTime(this);

        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        if (pseudoFile)
        {
            return pseudoFile->FSeek(seek, mode);
        }

        if (AZ::IO::FileIOBase::GetDirectInstance()->Seek(fileHandle, static_cast<AZ::s64>(seek), AZ::IO::GetSeekTypeFromFSeekMode(mode)))
        {
            return 0;
        }

        return 1;
    }

    size_t Archive::FWrite(const void* data, size_t bytesToWrite, AZ::IO::HandleType fileHandle)
    {
        SAutoCollectFileAccessTime accessTime(this);

        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        if (pseudoFile)
        {
            return 0;
        }

        AZ_Assert(fileHandle != AZ::IO::InvalidHandle, "Invalid file has been passed to FWrite");
        if (AZ::u64 bytesWritten{}; AZ::IO::FileIOBase::GetDirectInstance()->Write(fileHandle, data, bytesToWrite, &bytesWritten))
        {
            return bytesWritten;
        }
        return 0;
    }

    //////////////////////////////////////////////////////////////////////////
    size_t Archive::FRead(void* pData, size_t bytesToRead, AZ::IO::HandleType fileHandle)
    {
        AZ_PROFILE_FUNCTION(AzCore);
        SAutoCollectFileAccessTime accessTime(this);

        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        if (pseudoFile)
        {
            return pseudoFile->FRead(pData, bytesToRead, fileHandle);
        }

        AZ::u64 bytesRead = 0;
        AZ::IO::FileIOBase::GetDirectInstance()->Read(fileHandle, pData, bytesToRead, false, &bytesRead);
        return bytesRead;
    }

    //////////////////////////////////////////////////////////////////////////
    void* Archive::FGetCachedFileData(AZ::IO::HandleType fileHandle, size_t& nFileSize)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        SAutoCollectFileAccessTime accessTime(this);
        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        if (pseudoFile)
        {
            return pseudoFile->GetFileData(nFileSize, fileHandle);
        }

        // Cached lookup
        {
            RawDataCacheLockGuard lock(m_cachedFileRawDataMutex);
            auto itr = m_cachedFileRawDataSet.find(fileHandle);
            if (itr != m_cachedFileRawDataSet.end())
            {
                CachedRawDataEntry& entry = itr->second;
                nFileSize = entry.m_fileSize;
                return entry.m_data->m_pCachedData;
            }
        }

        // Cache miss, now read the file
        nFileSize = FGetSize(fileHandle);

        auto pCachedFileRawData{ AZStd::make_unique<ArchiveInternal::CCachedFileRawData>(nFileSize) };

        AZ::IO::FileIOBase::GetDirectInstance()->Seek(fileHandle, 0, AZ::IO::SeekType::SeekFromStart);
        if (!AZ::IO::FileIOBase::GetDirectInstance()->Read(fileHandle, pCachedFileRawData->m_pCachedData, nFileSize))
        {
            char fileNameBuffer[AZ_MAX_PATH_LEN];
            AZ::IO::FileIOBase::GetDirectInstance()->GetFilename(fileHandle, fileNameBuffer, AZ_ARRAY_SIZE(fileNameBuffer));
            AZ_Warning("Archive", false, "Failed to read %zu bytes when attempting to read raw filedata for file %s",
                nFileSize, fileNameBuffer);
            return nullptr;
        }

        // Add to the cache
        void* pCachedData = nullptr;
        {
            RawDataCacheLockGuard lock(m_cachedFileRawDataMutex);

            CachedRawDataEntry& entry = m_cachedFileRawDataSet[fileHandle];
            if (!entry.m_data)
            {
                entry.m_data = AZStd::move(pCachedFileRawData);
                entry.m_fileSize = nFileSize;
            }
            else
            {
                if (az_archive_verbosity)
                {
                    char fileNameBuffer[AZ_MAX_PATH_LEN];
                    [[maybe_unused]] const char* fileName = AZ::IO::FileIOBase::GetDirectInstance()->GetFilename(fileHandle, fileNameBuffer,
                            AZ_ARRAY_SIZE(fileNameBuffer)) ? fileNameBuffer : "unknown";
                    AZ_TracePrintf("Archive", R"(Perf Warning: First call to read file "%s" made from multiple threads concurrently)" "\n",
                        fileName);
                }
                AZ_Assert(entry.m_fileSize == nFileSize, "Cached data size(%zu) does not match filesize(%zu)", entry.m_fileSize, nFileSize);
            }

            pCachedData = entry.m_data->m_pCachedData;
        }

        return pCachedData;
    }

    //////////////////////////////////////////////////////////////////////////
    int Archive::FClose(AZ::IO::HandleType fileHandle)
    {
        // Free cached data (not all files have raw cached data)
        {
            RawDataCacheLockGuard lock(m_cachedFileRawDataMutex);
            m_cachedFileRawDataSet.erase(fileHandle);
        }

        SAutoCollectFileAccessTime accessTime(this);
        auto nPseudoFile = static_cast<size_t>(static_cast<uintptr_t>(fileHandle) - ArchiveInternal::PseudoFileIdxOffset);
        AZStd::unique_lock lock(m_csOpenFiles);
        if (nPseudoFile < m_arrOpenFiles.size())
        {
            m_arrOpenFiles[nPseudoFile]->Destruct();
            return 0;
        }
        else
        {
            if (AZ::IO::FileIOBase::GetDirectInstance()->Close(fileHandle))
            {
                return 0;
            }
            return 1;
        }
    }

    bool Archive::IsInPak(AZ::IO::HandleType fileHandle)
    {
        return GetPseudoFile(fileHandle) != nullptr;
    }

    int Archive::FEof(AZ::IO::HandleType fileHandle)
    {
        SAutoCollectFileAccessTime accessTime(this);
        ArchiveInternal::CZipPseudoFile* pseudoFile = GetPseudoFile(fileHandle);
        if (pseudoFile)
        {
            return pseudoFile->FEof();
        }

        return AZ::IO::FileIOBase::GetDirectInstance()->Eof(fileHandle);
    }


    //////////////////////////////////////////////////////////////////////////
    AZ::IO::ArchiveFileIterator Archive::FindFirst(AZStd::string_view pDir, EFileSearchType searchType)
    {
        auto szFullPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pDir);
        if (!szFullPath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(pDir.size()), pDir.data());
            return {};
        }

        bool bScanZips{};
        bool bAllowUseFileSystem{};
        switch (searchType)
        {
            case IArchive::eFileSearchType_AllowInZipsOnly:
                bAllowUseFileSystem = false;
                bScanZips = true;
                break;
            case IArchive::eFileSearchType_AllowOnDiskAndInZips:
                bAllowUseFileSystem = true;
                bScanZips = true;
                break;
            case IArchive::eFileSearchType_AllowOnDiskOnly:
                bAllowUseFileSystem = true;
                bScanZips = false;
                break;
        }

        AZStd::intrusive_ptr pFindData = aznew AZ::IO::FindData();
        pFindData->Scan(this, szFullPath->Native(), bAllowUseFileSystem, bScanZips);

        return pFindData->Fetch();
    }

    //////////////////////////////////////////////////////////////////////////
    AZ::IO::ArchiveFileIterator Archive::FindNext(AZ::IO::ArchiveFileIterator fileIterator)
    {
        return ++fileIterator;
    }

    bool Archive::FindClose(AZ::IO::ArchiveFileIterator fileIterator)
    {
        fileIterator.m_findData.reset();
        return true;
    }

    auto Archive::GetLevelPackOpenEvent() -> LevelPackOpenEvent*
    {
        return &m_levelOpenEvent;
    }

    auto Archive::GetLevelPackCloseEvent()->LevelPackCloseEvent*
    {
        return &m_levelCloseEvent;
    }
    //======================================================================
    bool Archive::OpenPack(AZStd::string_view szBindRootIn, AZStd::string_view szPath,
        AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData, AZ::IO::FixedMaxPathString* pFullPath, bool addLevels)
    {
        AZ_Assert(!szBindRootIn.empty(), "Bind Root should not be empty");

        auto szFullPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(szPath);
        if (!szFullPath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(szPath.size()), szPath.data());
            return false;
        }

        auto szBindRoot = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(szBindRootIn);
        if (!szBindRoot)
        {
            AZ_Assert(false, "Unable to resolve path for bindroot %.*s", aznumeric_cast<int>(szBindRootIn.size()), szBindRootIn.data());
            return false;
        }

        bool result = OpenPackCommon(szBindRoot->Native(), szFullPath->Native(), pData, addLevels);

        if (pFullPath)
        {
            *pFullPath = AZStd::move(szFullPath->Native());
        }

        return result;
    }

    bool Archive::OpenPack(AZStd::string_view szPath, AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData,
        AZ::IO::FixedMaxPathString* pFullPath, bool addLevels)
    {
        auto szFullPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(szPath);
        if (!szFullPath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(szPath.size()), szPath.data());
            return false;
        }

        AZStd::string_view bindRoot = szFullPath->ParentPath().Native();

        bool result = OpenPackCommon(bindRoot, szFullPath->Native(), pData, addLevels);

        if (pFullPath)
        {
            *pFullPath = AZStd::move(szFullPath->Native());
        }

        return result;
    }


    bool Archive::OpenPackCommon(AZStd::string_view szBindRoot, AZStd::string_view szFullPath,
        AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData, bool addLevels)
    {
        // setup PackDesc before the duplicate test
        PackDesc desc;
        desc.m_strFileName = szFullPath;

        if (AZ::IO::FixedMaxPath pathBindRoot; !AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pathBindRoot, szBindRoot))
        {
            AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pathBindRoot, "@products@");
            desc.m_pathBindRoot = pathBindRoot.LexicallyNormal().String();
        }
        else
        {
            // Create a bind root
            desc.m_pathBindRoot = pathBindRoot.LexicallyNormal().String();
        }

        // hold the lock from the point we query the zip array,
        // so we don't end up adding a given archive twice
        {
            AZStd::unique_lock lock(m_csZips);
            // try to find this - maybe the pack has already been opened
            for (auto it = m_arrZips.begin(); it != m_arrZips.end(); ++it)
            {
                if (AZ::IO::PathView archiveFilePath = it->pZip->GetFilePath();
                    archiveFilePath == desc.m_strFileName && it->m_pathBindRoot == desc.m_pathBindRoot)
                {
                    return true; // already opened
                }
            }
        }

        const int flags = INestedArchive::FLAGS_OPTIMIZED_READ_ONLY | INestedArchive::FLAGS_ABSOLUTE_PATHS;

        desc.pArchive = OpenArchive(szFullPath, szBindRoot, flags, pData);
        if (!desc.pArchive)
        {
            return false; // couldn't open the archive
        }

        AZ_TracePrintf("Archive", "Opening archive file %.*s\n", AZ_STRING_ARG(szFullPath));
        desc.pZip = static_cast<NestedArchive*>(desc.pArchive.get())->GetCache();

        AZStd::unique_lock lock(m_csZips);
        // Insert the archive lexically but before any override archives
        // This allows us to order the archives allowing the later archives
        // that have priority for same name files. This supports the
        // patching of the base program underneath the mods/override archives
        // All we have to do is name the archive appropriately to make
        // sure later archives added to the current set of archives sort higher
        // and therefore get used instead of lower sorted archives
        AZ::IO::PathView nextBundle;
        ZipArray::reverse_iterator revItZip = m_arrZips.rbegin();
        for (; revItZip != m_arrZips.rend(); ++revItZip)
        {
            nextBundle = revItZip->GetFullPath();
            if (desc.GetFullPath() > revItZip->GetFullPath())
            {
                break;
            }
        }

        auto bundleManifest = GetBundleManifest(desc.pZip);
        AZStd::shared_ptr<AzFramework::AssetRegistry> bundleCatalog;
        if (bundleManifest)
        {
            bundleCatalog = GetBundleCatalog(desc.pZip, bundleManifest->GetCatalogName());
        }

        bool usePrefabSystemForLevels = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

        if (usePrefabSystemForLevels)
        {
            m_arrZips.insert(revItZip.base(), desc);
        }
        else
        {
            // [LYN-2376] Remove once legacy slice support is removed
            AZStd::vector<AZStd::string> levelDirs;

            if (addLevels)
            {
                // Note that manifest version two and above will contain level directory information inside them
                // otherwise we will fallback to scanning the archive for levels.
                if (bundleManifest && bundleManifest->GetBundleVersion() >= 2)
                {
                    levelDirs = bundleManifest->GetLevelDirectories();
                }
                else
                {
                    levelDirs = ScanForLevels(desc.pZip);
                }
            }

            if (!levelDirs.empty())
            {
                desc.m_containsLevelPak = true;
            }

            m_arrZips.insert(revItZip.base(), desc);

            m_levelOpenEvent.Signal(levelDirs);
        }

        AZ::IO::ArchiveNotificationBus::Broadcast([](AZ::IO::ArchiveNotifications* archiveNotifications, const char* bundleName,
            AZStd::shared_ptr<AzFramework::AssetBundleManifest> bundleManifest, const AZ::IO::FixedMaxPath& nextBundle, AZStd::shared_ptr<AzFramework::AssetRegistry> bundleCatalog)
        {
            archiveNotifications->BundleOpened(bundleName, bundleManifest, nextBundle.c_str(), bundleCatalog);
        }, desc.m_strFileName.c_str(), bundleManifest, nextBundle, bundleCatalog);

        return true;
    }


    // after this call, the file will be unlocked and closed, and its contents won't be used to search for files
    bool Archive::ClosePack(AZStd::string_view pName)
    {
        auto szZipPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pName);
        if (!szZipPath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(pName.size()), pName.data());
            return false;
        }

        bool usePrefabSystemForLevels = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

        AZStd::unique_lock lock(m_csZips);
        for (auto it = m_arrZips.begin(); it != m_arrZips.end();)
        {
            if (szZipPath == it->GetFullPath())
            {
                // this is the pack with the given name - remove it, and if possible it will be deleted
                // the zip is referenced from the archive and *it; the archive is referenced only from *it
                //
                // the pZip (cache) can be referenced from stream engine and pseudo-files.
                // the archive can be referenced from outside
                AZ::IO::ArchiveNotificationBus::Broadcast([](AZ::IO::ArchiveNotifications* archiveNotifications, const AZ::IO::FixedMaxPath& bundleName)
                {
                    archiveNotifications->BundleClosed(bundleName.c_str());
                }, it->GetFullPath());

                if (usePrefabSystemForLevels)
                {
                    it = m_arrZips.erase(it);
                }
                else
                {
                    // [LYN-2376] Remove once legacy slice support is removed
                    bool needRescan = false;
                    if (it->m_containsLevelPak)
                    {
                        needRescan = true;
                    }

                    it = m_arrZips.erase(it);

                    if (needRescan)
                    {
                        m_levelCloseEvent.Signal(szZipPath->Native());
                    }
                }
            }
            else
            {
                ++it;
            }
        }
        return true;
    }

    bool Archive::FindPacks(AZStd::string_view pWildcardIn)
    {
        auto filePath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pWildcardIn);
        if (!filePath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(pWildcardIn.size()), pWildcardIn.data());
            return false;
        }

        bool foundMatchingPackFile = false;
        AZ::IO::FileIOBase::GetDirectInstance()->FindFiles(AZ::IO::FixedMaxPath(filePath->ParentPath()).c_str(),
            AZ::IO::FixedMaxPath(filePath->Filename()).c_str(), [&foundMatchingPackFile]([[maybe_unused]] const char* filePath) -> bool
        {
            // Even one invocation here means we found a matching file
            foundMatchingPackFile = true;
            // Don't bother getting any more
            return false;
        });
        return foundMatchingPackFile;
    }

    bool Archive::OpenPacks(AZStd::string_view pWildcardIn, AZStd::vector<AZ::IO::FixedMaxPathString>* pFullPaths)
    {
        auto strBindRoot{ AZ::IO::PathView(pWildcardIn).ParentPath() };
        AZ::IO::FixedMaxPath bindRoot;
        if (!strBindRoot.empty())
        {
            bindRoot = strBindRoot;
        }
        return OpenPacksCommon(bindRoot.Native(), pWildcardIn, pFullPaths);
    }

    bool Archive::OpenPacks(AZStd::string_view szBindRoot, AZStd::string_view pWildcardIn, AZStd::vector<AZ::IO::FixedMaxPathString>* pFullPaths)
    {
        auto bindRoot = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(szBindRoot);
        if (!bindRoot)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(szBindRoot.size()), szBindRoot.data());
            return false;
        }
        return OpenPacksCommon(bindRoot->Native(), pWildcardIn, pFullPaths);
    }

    bool Archive::OpenPacksCommon(AZStd::string_view szDir, AZStd::string_view pWildcardIn, AZStd::vector<AZ::IO::FixedMaxPathString>* pFullPaths, bool addLevels)
    {
        constexpr AZStd::string_view wildcards{ "*?" };
        if (wildcards.find_first_of(pWildcardIn) == AZStd::string_view::npos)
        {
            // No wildcards, just open pack
            if (OpenPackCommon(szDir, pWildcardIn, nullptr, addLevels))
            {
                if (pFullPaths)
                {
                    pFullPaths->emplace_back(pWildcardIn);
                }
            }
            return true;
        }

        if (AZ::IO::ArchiveFileIterator fileIterator = FindFirst(pWildcardIn, IArchive::eFileSearchType_AllowOnDiskOnly); fileIterator)
        {
            AZStd::vector<AZ::IO::FixedMaxPath> files;
            do
            {
                auto& foundFilename = files.emplace_back(fileIterator.m_filename);
                AZStd::to_lower(foundFilename.Native().begin(), foundFilename.Native().end());
            }
            while (fileIterator = FindNext(fileIterator));

            // Open files in alphabetical order.
            AZStd::sort(files.begin(), files.end());
            bool bAllOk = true;
            for (const AZ::IO::FixedMaxPath& file : files)
            {
                bAllOk = OpenPackCommon(szDir, file.Native(), nullptr, addLevels) && bAllOk;

                if (pFullPaths)
                {
                    pFullPaths->emplace_back(AZStd::move(file.Native()));
                }
            }

            FindClose(fileIterator);
            return bAllOk;
        }

        return false;
    }


    bool Archive::ClosePacks(AZStd::string_view pWildcardIn)
    {
        auto path = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pWildcardIn);
        if (!path)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(pWildcardIn.size()), pWildcardIn.data());
            return false;
        }

        return AZ::IO::FileIOBase::GetDirectInstance()->FindFiles(AZ::IO::FixedMaxPath(path->ParentPath()).c_str(),
            AZ::IO::FixedMaxPath(path->Filename()).c_str(), [this](const char* filePath) -> bool
        {
            ClosePack(filePath);
            return true;
        });
    }

    //////////////////////////////////////////////////////////////////////////
    ArchiveInternal::CZipPseudoFile* Archive::GetPseudoFile(AZ::IO::HandleType fileHandle) const
    {
        AZStd::shared_lock lock(m_csOpenFiles);
        auto nPseudoFile = static_cast<size_t>(static_cast<uintptr_t>(fileHandle) - ArchiveInternal::PseudoFileIdxOffset);
        if (nPseudoFile < m_arrOpenFiles.size())
        {
            return m_arrOpenFiles[nPseudoFile].get();
        }
        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////

    CCachedFileData::CCachedFileData(ZipDir::CachePtr pZip, uint32_t nArchiveFlags, ZipDir::FileEntry* pFileEntry, [[maybe_unused]] AZStd::string_view szFilename)
    {
        m_nArchiveFlags = nArchiveFlags;
        m_pFileData = nullptr;
        m_pZip = pZip;
        m_pFileEntry = pFileEntry;
    }

    CCachedFileData::~CCachedFileData()
    {
        // forced destruction
        if (m_pFileData)
        {
            AZ::AllocatorInstance<AZ::OSAllocator>::Get().DeAllocate(m_pFileData);
            m_pFileData = nullptr;
        }

        m_pZip = nullptr;
        m_pFileEntry = nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    bool CCachedFileData::GetDataTo(void* pFileData, int nDataSize, bool bDecompress)
    {
        AZ_Assert(m_pZip, "ZipFile is nullptr");
        AZ_Assert(m_pFileEntry && m_pZip->IsOwnerOf(m_pFileEntry), "ZipFile is not owner of m_pFileEntry");

        if (static_cast<uint32_t>(nDataSize) != m_pFileEntry->desc.lSizeUncompressed && bDecompress)
        {
            return false;
        }
        else if (static_cast<uint32_t>(nDataSize) != m_pFileEntry->desc.lSizeCompressed && !bDecompress)
        {
            return false;
        }

        if (!m_pFileData)
        {
            AZStd::scoped_lock lock(m_pFileEntry->m_readLock);
            if (!m_pFileData)
            {
                if (ZipDir::ZD_ERROR_SUCCESS != m_pZip->ReadFile(m_pFileEntry, nullptr, pFileData))
                {
                    return false;
                }
            }
            else
            {
                memcpy(pFileData, m_pFileData, nDataSize);
            }
        }
        else
        {
            memcpy(pFileData, m_pFileData, nDataSize);
        }
        return true;
    }

    // return the data in the file, or nullptr if error
    void* CCachedFileData::GetData(bool bRefreshCache, bool decompress)
    {
        // first, do a "dirty" fast check without locking the critical section
        // in most cases, the data's going to be already there, and if it's there,
        // nobody's going to release it until this object is destructed.
        if (bRefreshCache && !m_pFileData)
        {
            AZ_Assert(m_pZip, "ZipFile is nullptr");
            AZ_Assert(m_pFileEntry && m_pZip->IsOwnerOf(m_pFileEntry), "ZipFile is not the owner of m_pFileEntry");
            // Then, lock it and check whether the data is still not there.
            // if it's not, allocate memory and unpack the file
            AZStd::scoped_lock lock(m_pFileEntry->m_readLock);
            if (!m_pFileData)
            {
                // don't try to decompress if its not actually compressed
                decompress = decompress && m_pFileEntry->IsCompressed();

                // if we are going to decompress into the buffer, we MUST allocate enough for it!
                // if we are either requesting decompressed data, or we are already decompressed, then we will need enough room for the
                // decompressed data
                bool allocateForDecompressed = decompress || (!m_pFileEntry->IsCompressed());
                uint32_t nTempBufferSize = (allocateForDecompressed) ? m_pFileEntry->desc.lSizeUncompressed : m_pFileEntry->desc.lSizeCompressed;
                void* fileData = AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(nTempBufferSize, 1, 0, "CCachedFileData::GetData");

                ZipDir::ErrorEnum result = m_pZip->ReadFile(m_pFileEntry, nullptr, fileData);

                if (result != ZipDir::ZD_ERROR_SUCCESS)
                {
                    AZ_Warning("Archive", false, "[ERROR] ReadFile returned %d\n", result);
                    AZ::AllocatorInstance<AZ::OSAllocator>::Get().DeAllocate(fileData);
                }
                else
                {
                    m_pFileData = fileData;
                }
            }
        }
        return m_pFileData;
    }

    //////////////////////////////////////////////////////////////////////////
    int64_t CCachedFileData::ReadData(void* pBuffer, int64_t nFileOffset, int64_t nReadSize)
    {
        if (!m_pFileEntry)
        {
            return -1;
        }

        int64_t nFileSize = m_pFileEntry->desc.lSizeUncompressed;
        if (nFileOffset + nReadSize > nFileSize)
        {
            nReadSize = nFileSize - nFileOffset;
        }
        if (nReadSize < 0)
        {
            return -1;
        }

        if (nReadSize == 0)
        {
            return 0;
        }

        if (m_pFileEntry->nMethod == ZipFile::METHOD_STORE) //Can't use this technique for METHOD_STORE_AND_STREAMCIPHER_KEYTABLE as seeking with encryption performs poorly
        {
            AZStd::scoped_lock lock(m_pFileEntry->m_readLock);
            // Uncompressed read.
            if (ZipDir::ZD_ERROR_SUCCESS != m_pZip->ReadFile(m_pFileEntry, nullptr, pBuffer))
            {
                return -1;
            }
        }
        else
        {
            uint8_t* pSrcBuffer = (uint8_t*)GetData();

            if (pSrcBuffer)
            {
                pSrcBuffer += nFileOffset;
                memcpy(pBuffer, pSrcBuffer, (size_t)nReadSize);
            }
            else
            {
                return -1;
            }
        }

        return nReadSize;
    }

    uint32_t CCachedFileData::GetFileDataOffset()
    {
        m_pZip->Refresh(m_pFileEntry);
        return m_pFileEntry->nFileDataOffset;
    }

    //////////////////////////////////////////////////////////////////////////
    // open the physical archive file - creates if it doesn't exist
    // returns nullptr if it's invalid or can't open the file
    AZStd::intrusive_ptr<INestedArchive> Archive::OpenArchive(AZStd::string_view szPath, AZStd::string_view bindRoot, uint32_t nFlags, AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData)
    {
        auto szFullPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(szPath);
        if (!szFullPath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(szPath.size()), szPath.data());
            return {};
        }

        // if it's simple and read-only, it's assumed it's read-only
        if (nFlags & INestedArchive::FLAGS_OPTIMIZED_READ_ONLY)
        {
            nFlags |= INestedArchive::FLAGS_READ_ONLY;
        }

        uint32_t nFactoryFlags = 0;


        if (nFlags & INestedArchive::FLAGS_DONT_COMPACT)
        {
            nFactoryFlags |= ZipDir::CacheFactory::FLAGS_DONT_COMPACT;
        }

        if (nFlags & INestedArchive::FLAGS_READ_ONLY)
        {
            nFactoryFlags |= ZipDir::CacheFactory::FLAGS_READ_ONLY;
        }


        INestedArchive* pArchive = FindArchive(szFullPath->Native());
        if (pArchive)
        {
            // check for compatibility
            if (!(nFlags & INestedArchive::FLAGS_RELATIVE_PATHS_ONLY) && (pArchive->GetFlags() & INestedArchive::FLAGS_RELATIVE_PATHS_ONLY))
            {
                pArchive->ResetFlags(INestedArchive::FLAGS_RELATIVE_PATHS_ONLY);
            }

            // we found one
            if (!(nFlags & INestedArchive::FLAGS_READ_ONLY) && (pArchive->GetFlags() & INestedArchive::FLAGS_READ_ONLY))
            {
                // we don't support upgrading from ReadOnly to ReadWrite
                return nullptr;
            }

            return pArchive;
        }

        AZStd::string strBindRoot;

        // if no bind root is specified, compute one:
        strBindRoot = !bindRoot.empty() ? bindRoot : szFullPath->ParentPath().Native();

        // Check if archive file disk exist on disk.
        const bool pakOnDisk = FileIOBase::GetDirectInstance()->Exists(szFullPath->c_str());
        if (!pakOnDisk && (nFactoryFlags & ZipDir::CacheFactory::FLAGS_READ_ONLY))
        {
            // Archive file not found.
            if (az_archive_verbosity)
            {
                AZ_TracePrintf("Archive", "Archive file %s does not exist\n", szFullPath->c_str());
            }
            return nullptr;
        }

        ZipDir::InitMethod initType = ZipDir::InitMethod::Default;
        if (!ZipDir::IsReleaseConfig)
        {
            if ((nFlags & INestedArchive::FLAGS_FULL_VALIDATE) != 0)
            {
                initType = ZipDir::InitMethod::FullValidation;
            }
            else if ((nFlags & INestedArchive::FLAGS_VALIDATE_HEADERS) != 0)
            {
                initType = ZipDir::InitMethod::ValidateHeaders;
            }
        }

        ZipDir::CacheFactory factory(initType, nFactoryFlags);

        ZipDir::CachePtr cache = factory.New(szFullPath->c_str());
        if (cache)
        {
            return new NestedArchive(this, strBindRoot, cache, nFlags);
        }

        return nullptr;
    }

    void Archive::Register(INestedArchive* pArchive)
    {
        AZStd::unique_lock lock(m_archiveMutex);
        ArchiveArray::iterator it = AZStd::lower_bound(m_arrArchives.begin(), m_arrArchives.end(), pArchive, NestedArchiveSortByName());
        m_arrArchives.insert(it, pArchive);
    }

    void Archive::Unregister(INestedArchive* pArchive)
    {
        AZStd::unique_lock lock(m_archiveMutex);
        if (pArchive)
        {
            AZ_TracePrintf("Archive", "Closing Archive file: %.*s\n", AZ_STRING_ARG(pArchive->GetFullPath().Native()));
        }
        ArchiveArray::iterator it;
        if (m_arrArchives.size() < 16)
        {
            // for small array sizes, we'll use linear search
            it = AZStd::find(m_arrArchives.begin(), m_arrArchives.end(), pArchive);
        }
        else
        {
            it = AZStd::lower_bound(m_arrArchives.begin(), m_arrArchives.end(), pArchive, NestedArchiveSortByName());
        }

        if (it != m_arrArchives.end() && *it == pArchive)
        {
            m_arrArchives.erase(it);
        }
        else
        {
            AZ_Assert(false, "Cannot unregister an archive that has not been registered");
        }
    }

    INestedArchive* Archive::FindArchive(AZStd::string_view szFullPath) const
    {
        AZStd::shared_lock lock(m_archiveMutex);
        auto it = AZStd::lower_bound(m_arrArchives.begin(), m_arrArchives.end(), szFullPath, NestedArchiveSortByName());
        if (it != m_arrArchives.end() && szFullPath == (*it)->GetFullPath())
        {
            return *it;
        }
        else
        {
            return nullptr;
        }
    }

    // compresses the raw data into raw data. The buffer for compressed data itself with the heap passed. Uses method 8 (deflate)
    // returns one of the Z_* errors (Z_OK upon success)
    // MT-safe
    int Archive::RawCompress(const void* pUncompressed, size_t* pDestSize, void* pCompressed, size_t nSrcSize, int nLevel)
    {
        return ZipDir::ZipRawCompress(pUncompressed, pDestSize, pCompressed, nSrcSize, nLevel);
    }

    // Uncompresses raw (without wrapping) data that is compressed with method 8 (deflated) in the Zip file
    // returns one of the Z_* errors (Z_OK upon success)
    // This function just mimics the standard uncompress (with modification taken from unzReadCurrentFile)
    // with 2 differences: there are no 16-bit checks, and
    // it initializes the inflation to start without waiting for compression method byte, as this is the
    // way it's stored into zip file
    int Archive::RawUncompress(void* pUncompressed, size_t* pDestSize, const void* pCompressed, size_t nSrcSize)
    {
        return ZipDir::ZipRawUncompress(pUncompressed, pDestSize, pCompressed, nSrcSize);
    }

    //////////////////////////////////////////////////////////////////////////
    void Archive::RecordFileOpen(ERecordFileOpenList eList)
    {
        m_eRecordFileOpenList = eList;

        switch (m_eRecordFileOpenList)
        {
        case RFOM_Disabled:
        case RFOM_EngineStartup:
        case RFOM_Level:
            break;

        case RFOM_NextLevel:
        default:
            AZ_Assert(false, "File Record %d option is not supported", aznumeric_cast<int>(eList));
            break;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    IArchive::ERecordFileOpenList Archive::GetRecordFileOpenList()
    {
        return m_eRecordFileOpenList;
    }

    //////////////////////////////////////////////////////////////////////////
    IResourceList* Archive::GetResourceList(ERecordFileOpenList eList)
    {
        switch (eList)
        {
        case RFOM_EngineStartup:
            return m_pEngineStartupResourceList.get();
        case RFOM_Level:
            return m_pLevelResourceList.get();
        case RFOM_NextLevel:
            return m_pNextLevelResourceList.get();

        case RFOM_Disabled:
        default:
            AZ_Assert(false, "File record option %d", aznumeric_cast<int>(eList));
        }
        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    void Archive::SetResourceList(ERecordFileOpenList eList, IResourceList* pResourceList)
    {
        switch (eList)
        {
        case RFOM_EngineStartup:
            m_pEngineStartupResourceList = pResourceList;
            break;
        case RFOM_Level:
            m_pLevelResourceList = pResourceList;
            break;
        case RFOM_NextLevel:
            m_pNextLevelResourceList = pResourceList;
            break;

        case RFOM_Disabled:
        default:
            AZ_Assert(false, "File record option %d is not supported by SetResourceList", aznumeric_cast<int>(eList));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void Archive::RecordFile([[maybe_unused]] AZ::IO::HandleType inFileHandle, [[maybe_unused]] AZStd::string_view szFilename)
    {
#if !defined(_RELEASE)
        CheckFileAccess(szFilename);

        for (IArchiveFileAccessSink* sink : m_FileAccessSinks)
        {
            sink->ReportFileOpen(inFileHandle, szFilename);
        }
#endif
    }

    void Archive::CheckFileAccess(AZStd::string_view szFilename)
    {
        bool shouldCheckFileAccess = false;
        if (m_eRecordFileOpenList != IArchive::RFOM_Disabled)
        {
            // we only want to record ASSET access
            // assets are identified as files that are relative to the resolved @products@ alias path
            auto fileIoBase = AZ::IO::FileIOBase::GetInstance();
            const char* aliasValue = fileIoBase->GetAlias("@products@");

            if (AZ::IO::FixedMaxPath resolvedFilePath;
                fileIoBase->ResolvePath(resolvedFilePath, szFilename)
                && aliasValue != nullptr
                && resolvedFilePath.IsRelativeTo(aliasValue))
            {
                IResourceList* pList = GetResourceList(m_eRecordFileOpenList);

                if (pList)
                {
                    pList->Add(szFilename);
                }

                shouldCheckFileAccess = true;
            }
        }

        if (shouldCheckFileAccess)
        {
#if !defined(_RELEASE)
            AZ::IO::ArchiveNotificationBus::Broadcast([](AZ::IO::ArchiveNotifications* archiveNotifications, AZStd::string_view filenameView)
            {
                AZ::IO::PathString filePath{ filenameView };
                archiveNotifications->FileAccess(filePath.c_str());
            }, szFilename);
#endif
        }
    }


    bool Archive::DisableRuntimeFileAccess(bool status, AZStd::thread_id threadId)
    {
        bool prev = false;
        if (threadId == m_mainThreadId)
        {
            prev = m_disableRuntimeFileAccess;
            m_disableRuntimeFileAccess = status;
        }
        return prev;
    }

    //////////////////////////////////////////////////////////////////////////
    void Archive::RegisterFileAccessSink(IArchiveFileAccessSink* pSink)
    {
        AZ_Assert(pSink, "cannot register nullptr sink");

        if (AZStd::find(m_FileAccessSinks.begin(), m_FileAccessSinks.end(), pSink) != m_FileAccessSinks.end())
        {
            // was already registered
            AZ_Assert(false, "ArchiveFileAccessSink has already been registered");
            return;
        }

        m_FileAccessSinks.push_back(pSink);
    }


    //////////////////////////////////////////////////////////////////////////
    void Archive::UnregisterFileAccessSink(IArchiveFileAccessSink* pSink)
    {
        AZ_Assert(pSink, "cannot unregister nullptr sink");

        if (auto it = AZStd::find(m_FileAccessSinks.begin(), m_FileAccessSinks.end(), pSink); it != m_FileAccessSinks.end())
        {
            m_FileAccessSinks.erase(it);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    bool Archive::RemoveFile(AZStd::string_view pName)
    {
        AZ::IO::FixedMaxPathString szFullPath{ pName };
        return AZ::IO::FileIOBase::GetDirectInstance()->Remove(szFullPath.c_str()) == AZ::IO::ResultCode::Success;
    }

    //////////////////////////////////////////////////////////////////////////
    bool Archive::RemoveDir(AZStd::string_view pName)
    {
        AZ::IO::FixedMaxPathString szFullPath{ pName };

        if (AZ::IO::FileIOBase::GetDirectInstance()->IsDirectory(szFullPath.c_str()))
        {
            AZ::IO::FileIOBase::GetDirectInstance()->DestroyPath(szFullPath.c_str());
            return true;
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    bool Archive::IsAbsPath(AZStd::string_view path)
    {
#if AZ_TRAIT_IS_ABS_PATH_IF_COLON_FOUND_ANYWHERE
        return path.find_first_of(':') != AZStd::string_view::npos;
#else
        constexpr AZStd::string_view pathSeparators{ AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR };
        return (!path.empty() && pathSeparators.find_first_of(path[0]) != AZStd::string_view::npos)
            || (path.size() > 2 && path[1] == ':' && pathSeparators.find_first_of(path[2]) != AZStd::string_view::npos);
#endif
    }

    void* Archive::PoolMalloc(size_t size)
    {
        return AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(size, 1, 0, "Archive::Malloc");
    }

    void Archive::PoolFree(void* p)
    {
        return AZ::AllocatorInstance<AZ::OSAllocator>::Get().DeAllocate(p);
    }

    // gets the current archive priority
    ArchiveLocationPriority Archive::GetPakPriority() const
    {
        int pakPriority = aznumeric_cast<int>(ArchiveVars{}.nPriority);
        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            [[maybe_unused]] AZ::GetValueResult getCvarResult = console->GetCvarValue("sys_PakPriority", pakPriority);
            AZ_Error("Archive", getCvarResult == AZ::GetValueResult::Success, "Lookup of 'sys_PakPriority console variable failed with error %s", AZ::GetEnumString(getCvarResult));
        }
        return static_cast<ArchiveLocationPriority>(pakPriority);
    }

    //////////////////////////////////////////////////////////////////////////


    AZStd::intrusive_ptr<AZ::IO::MemoryBlock> Archive::PoolAllocMemoryBlock(size_t size, const char* usage, size_t alignment)
    {
        if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
        {
            AZ_Error("Archive", false, "OSAllocator is not ready. It cannot be used to allocate a MemoryBlock");
            return {};
        }
        AZ::IAllocatorAllocate* allocator = &AZ::AllocatorInstance<AZ::OSAllocator>::Get();
        AZStd::intrusive_ptr<AZ::IO::MemoryBlock> memoryBlock{ new (allocator->Allocate(sizeof(AZ::IO::MemoryBlock), alignof(AZ::IO::MemoryBlock))) AZ::IO::MemoryBlock{AZ::IO::MemoryBlockDeleter{ &AZ::AllocatorInstance<AZ::OSAllocator>::Get() }} };
        auto CreateFunc = [](size_t byteSize, size_t byteAlignment, const char* name)
        {
            return reinterpret_cast<uint8_t*>(AZ::AllocatorInstance<AZ::OSAllocator>::Get().Allocate(byteSize, byteAlignment, 0, name));
        };
        auto DeleterFunc = [](uint8_t* ptrArray)
        {
            if (ptrArray)
            {
                AZ::AllocatorInstance<AZ::OSAllocator>::Get().DeAllocate(ptrArray);
            }
        };
        memoryBlock->m_address = AZ::IO::MemoryBlock::AddressPtr{ CreateFunc(size, alignment, usage), AZ::IO::MemoryBlock::AddressDeleter{DeleterFunc} };
        memoryBlock->m_size = size;

        return memoryBlock;
    }

    void Archive::FindCompressionInfo(bool& found, AZ::IO::CompressionInfo& info, const AZStd::string_view filename)
    {
        if (!found)
        {
            auto correctedFilename = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(filename);
            if (!correctedFilename)
            {
                AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(filename.size()), filename.data());
                return;
            }

            CheckFileAccess(correctedFilename->Native());

            uint32_t archiveFlags = 0;
            ZipDir::CachePtr archive;
            CCachedFileDataPtr pFileData = GetFileData(correctedFilename->Native(), archiveFlags, &archive);
            if (!pFileData)
            {
                return;
            }

            ZipDir::FileEntry* entry = pFileData->GetFileEntry();
            if (entry && entry->IsInitialized() && archive)
            {
                found = true;

                info.m_archiveFilename.InitFromRelativePath(archive->GetFilePath().Native());
                info.m_offset = pFileData->GetFileDataOffset();
                info.m_compressedSize = entry->desc.lSizeCompressed;
                info.m_uncompressedSize = entry->desc.lSizeUncompressed;
                info.m_isCompressed = entry->IsCompressed();
                info.m_isSharedPak = true;

                switch (GetPakPriority())
                {
                case ArchiveLocationPriority::ePakPriorityFileFirst:
                    info.m_conflictResolution = AZ::IO::ConflictResolution::PreferFile;
                    break;
                case ArchiveLocationPriority::ePakPriorityPakFirst:
                    info.m_conflictResolution = AZ::IO::ConflictResolution::PreferArchive;
                    break;
                case ArchiveLocationPriority::ePakPriorityPakOnly:
                    info.m_conflictResolution = AZ::IO::ConflictResolution::UseArchiveOnly;
                    break;
                }

                info.m_decompressor = []([[maybe_unused]] const AZ::IO::CompressionInfo& info, const void* compressed, size_t compressedSize, void* uncompressed, size_t uncompressedBufferSize)->bool
                {
                    size_t nSizeUncompressed = uncompressedBufferSize;
                    return ZipDir::ZipRawUncompress(uncompressed, &nSizeUncompressed, compressed, compressedSize) == 0;
                };
            }
        }
    }

    // return offset in archive file (ideally has to return offset on DVD)
    uint64_t Archive::GetFileOffsetOnMedia(AZStd::string_view sFilename) const
    {
        auto szFullPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(sFilename);

        if (!szFullPath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(sFilename.size()), sFilename.data());
            return 0;
        }

        ZipDir::CachePtr pZip;
        uint32_t nArchiveFlags;
        ZipDir::FileEntry* pFileEntry = FindPakFileEntry(szFullPath->Native(), nArchiveFlags, &pZip);
        if (!pFileEntry)
        {
            return 0;
        }
        pZip->Refresh(pFileEntry);
        return aznumeric_cast<uint64_t>(pFileEntry->nFileDataOffset);
    }

    EStreamSourceMediaType Archive::GetFileMediaType(AZStd::string_view szName) const
    {
        auto szFullPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(szName);
        if (!szFullPath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(szName.size()), szName.data());
            return static_cast<EStreamSourceMediaType>(0);
        }

        enum StreamMediaType : int32_t
        {
            TypeUnknown = 0,
            TypeHDD,
            TypeDisc,
            TypeMemory,
        };
        return static_cast<EStreamSourceMediaType>(StreamMediaType::TypeHDD);
    }

    bool Archive::SetPacksAccessible(bool bAccessible, AZStd::string_view pWildcard)
    {
        auto filePath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pWildcard);
        if (!filePath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", aznumeric_cast<int>(pWildcard.size()), pWildcard.data());
            return false;
        }

        return AZ::IO::FileIOBase::GetDirectInstance()->FindFiles(AZ::IO::FixedMaxPath(filePath->ParentPath()).c_str(),
            AZ::IO::FixedMaxPath(filePath->Filename()).c_str(), [&](const char* filePath) -> bool
        {
            SetPackAccessible(bAccessible, filePath);
            return true;
        });
    }

    bool Archive::SetPackAccessible(bool bAccessible, AZStd::string_view pName)
    {
        auto szZipPath = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pName);
        if (!szZipPath)
        {
            AZ_Assert(false, "Unable to resolve path for filepath %.*s", AZ_STRING_ARG(pName));
            return false;
        }

        AZStd::unique_lock lock(m_csZips);
        for (auto it = m_arrZips.begin(); it != m_arrZips.end(); ++it)
        {
            if (szZipPath == it->GetFullPath())
            {
                return it->pArchive->SetPackAccessible(bAccessible);
            }
        }

        return true;
    }

    AZStd::shared_ptr<AzFramework::AssetBundleManifest> Archive::GetBundleManifest(ZipDir::CachePtr pZip)
    {
        CCachedFileDataPtr fileData = GetFileData(pZip, AzFramework::AssetBundleManifest::s_manifestFileName);

        // Legacy bundles will not have manifests
        if (!fileData)
        {
            return {};
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve serialize context.");
        auto manifestInfo = AZStd::shared_ptr<AzFramework::AssetBundleManifest>(AZ::Utils::LoadObjectFromBuffer<AzFramework::AssetBundleManifest>(fileData->GetData(), fileData->GetFileEntry()->desc.lSizeUncompressed));

        return manifestInfo;
    }

    AZStd::vector<AZStd::string> Archive::ScanForLevels(ZipDir::CachePtr pZip)
    {
        AZStd::queue<AZStd::string> scanDirs;
        AZStd::vector<AZStd::string> levelDirs;
        AZStd::string currentDir = "levels";
        AZStd::string currentDirPattern;
        AZStd::string currentFilePattern;
        ZipDir::FindDir findDir(pZip);

        findDir.FindFirst(currentDir.c_str());
        if (!findDir.GetDirEntry())
        {
            // if levels folder does not exists at the root, return
            return {};
        }
        ZipDir::FindFile findFile(pZip);
        do
        {
            if (!scanDirs.empty())
            {
                currentDir = scanDirs.front();
                scanDirs.pop();
            }

            currentDirPattern = currentDir + AZ_FILESYSTEM_SEPARATOR_WILDCARD;
            currentFilePattern = currentDir + AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING + "levels.pak";

            ZipDir::FileEntry* fileEntry = findFile.FindExact(currentFilePattern.c_str());
            if (fileEntry)
            {
                levelDirs.emplace_back(currentDir);
                continue;
            }

            for (findDir.FindFirst(currentDirPattern.c_str()); findDir.GetDirEntry(); findDir.FindNext())
            {
                AZStd::string_view dirName = findDir.GetDirName();
                AZStd::string dirToAdd = AZStd::string::format("%s/%.*s", currentDir.data(), aznumeric_cast<int>(dirName.size()), dirName.data());
                scanDirs.push(dirToAdd);
            }
        } while (!scanDirs.empty());

        return levelDirs;
    }

    AZStd::shared_ptr<AzFramework::AssetRegistry> Archive::GetBundleCatalog(ZipDir::CachePtr pZip, const AZStd::string& catalogName)
    {
        CCachedFileDataPtr fileData = GetFileData(pZip, catalogName.c_str());

        // Legacy bundles will not have manifests
        if (!fileData)
        {
            return {};
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve serialize context.");
        auto catalogInfo = AZStd::shared_ptr<AzFramework::AssetRegistry>(AZ::Utils::LoadObjectFromBuffer<AzFramework::AssetRegistry>(fileData->GetData(), fileData->GetFileEntry()->desc.lSizeUncompressed));

        return catalogInfo;
    }
}
