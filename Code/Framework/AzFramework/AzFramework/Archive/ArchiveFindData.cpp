/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/wildcard.h>
#include <AzFramework/Archive/Archive.h>
#include <AzFramework/Archive/ArchiveFindData.h>
#include <AzFramework/Archive/ArchiveVars.h>
#include <AzFramework/Archive/ZipDirFind.h>

namespace AZ::IO
{
    FileDesc::FileDesc(Attribute fileAttribute, uint64_t fileSize, time_t accessTime, time_t creationTime, time_t writeTime)
        : nAttrib{ fileAttribute }
        , nSize{ fileSize }
        , tAccess{ accessTime }
        , tCreate{ creationTime }
        , tWrite{ writeTime }
    {
    }

    // ArchiveFileIterator
    ArchiveFileIterator::ArchiveFileIterator(FindData* findData)
        : m_findData{ findData }
    {
    }

    ArchiveFileIterator ArchiveFileIterator::operator++()
    {
        ArchiveFileIterator resultIter;
        if (m_findData)
        {
            resultIter = m_findData->Fetch();
        }
        return resultIter;
    }
    ArchiveFileIterator ArchiveFileIterator::operator++(int)
    {
        return operator++();
    }


    ArchiveFileIterator::operator bool() const
    {
        return m_findData && m_lastFetchValid;
    }

    // FindData::ArchiveFile
    FindData::ArchiveFile::ArchiveFile() = default;
    FindData::ArchiveFile::ArchiveFile(AZStd::string_view filename, const FileDesc& fileDesc)
        : m_filename(filename)
        , m_fileDesc(fileDesc)
    {
    }
    size_t FindData::ArchiveFile::GetHash() const
    {
        return AZStd::hash<AZ::IO::PathView>{}(m_filename.c_str());
    }

    bool FindData::ArchiveFile::operator==(const ArchiveFile& rhs) const
    {
        return GetHash() == rhs.GetHash();
    }

    // FindData::ArchiveFilehash
    size_t FindData::ArchiveFileHash::operator()(const ArchiveFile& archiveFile) const
    {
        return archiveFile.GetHash();
    }

    // FindData
    void FindData::Scan(IArchive* archive, AZStd::string_view szDir, bool bAllowUseFS, bool bScanZips)
    {
        // get the priority into local variable to avoid it changing in the course of
        // this function execution
        ArchiveLocationPriority nVarPakPriority = archive->GetPakPriority();

        if (nVarPakPriority == ArchiveLocationPriority::ePakPriorityFileFirst)
        {
            // first, find the file system files
            ScanFS(archive, szDir);
            if (bScanZips)
            {
                ScanZips(archive, szDir);
            }
        }
        else
        {
            // first, find the zip files
            if (bScanZips)
            {
                ScanZips(archive, szDir);
            }
            if (bAllowUseFS || nVarPakPriority != ArchiveLocationPriority::ePakPriorityPakOnly)
            {
                ScanFS(archive, szDir);
            }
        }
    }

    void FindData::ScanFS([[maybe_unused]] IArchive* archive, AZStd::string_view szDirIn)
    {
        AZ::IO::PathView directory{ szDirIn };
        AZ::IO::FixedMaxPath searchDirectory = directory.ParentPath();
        AZ::IO::FixedMaxPath pattern = directory.Filename();
        auto ScanFileSystem = [this](const char* filePath) -> bool
        {
            ArchiveFile archiveFile{ AZ::IO::PathView(filePath).Filename().Native(), {} };

            if (AZ::IO::FileIOBase::GetDirectInstance()->IsDirectory(filePath))
            {
                archiveFile.m_fileDesc.nAttrib = archiveFile.m_fileDesc.nAttrib | AZ::IO::FileDesc::Attribute::Subdirectory;
                m_fileSet.emplace(AZStd::move(archiveFile));
            }
            else
            {
                if (AZ::IO::FileIOBase::GetDirectInstance()->IsReadOnly(filePath))
                {
                    archiveFile.m_fileDesc.nAttrib = archiveFile.m_fileDesc.nAttrib | AZ::IO::FileDesc::Attribute::ReadOnly;
                }
                AZ::u64 fileSize = 0;
                AZ::IO::FileIOBase::GetDirectInstance()->Size(filePath, fileSize);
                archiveFile.m_fileDesc.nSize = fileSize;
                archiveFile.m_fileDesc.tWrite = AZ::IO::FileIOBase::GetDirectInstance()->ModificationTime(filePath);

                // These times are not supported by our file interface
                archiveFile.m_fileDesc.tAccess = archiveFile.m_fileDesc.tWrite;
                archiveFile.m_fileDesc.tCreate = archiveFile.m_fileDesc.tWrite;
                m_fileSet.emplace(AZStd::move(archiveFile));
            }
            return true;
        };
        AZ::IO::FileIOBase::GetDirectInstance()->FindFiles(searchDirectory.c_str(), pattern.c_str(), ScanFileSystem);
    }

    //////////////////////////////////////////////////////////////////////////
    void FindData::ScanZips(IArchive* archive, AZStd::string_view szDir)
    {
        AZ::IO::FixedMaxPath sourcePath;
        if (!AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(sourcePath, szDir))
        {
            AZ_Assert(false, "Unable to resolve Path for file path %.*s", aznumeric_cast<int>(szDir.size()), szDir.data());
            return;
        }

        auto ScanInZip = [this](ZipDir::Cache* zipCache, AZStd::string_view relativePath)
        {
            ZipDir::FindFile findFileEntry(zipCache);
            for (findFileEntry.FindFirst(relativePath); findFileEntry.GetFileEntry(); findFileEntry.FindNext())
            {
                ZipDir::FileEntry* fileEntry = findFileEntry.GetFileEntry();
                AZStd::string_view fname = findFileEntry.GetFileName();
                if (fname.empty())
                {
                    AZ_Fatal("Archive", "Empty filename within zip file: '%s'", zipCache->GetFilePath());
                }
                AZ::IO::FileDesc fileDesc;
                fileDesc.nAttrib = AZ::IO::FileDesc::Attribute::ReadOnly | AZ::IO::FileDesc::Attribute::Archive;
                fileDesc.nSize = fileEntry->desc.lSizeUncompressed;
                fileDesc.tWrite = fileEntry->GetModificationTime();
                m_fileSet.emplace(fname, fileDesc);
            }

            ZipDir::FindDir findDirectoryEntry(zipCache);
            for (findDirectoryEntry.FindFirst(relativePath); findDirectoryEntry.GetDirEntry(); findDirectoryEntry.FindNext())
            {
                AZStd::string_view fname = findDirectoryEntry.GetDirName();
                if (fname.empty())
                {
                    AZ_Fatal("Archive", "Empty directory name within zip file: '%s'", zipCache->GetFilePath());
                }
                AZ::IO::FileDesc fileDesc;
                fileDesc.nAttrib = AZ::IO::FileDesc::Attribute::ReadOnly | AZ::IO::FileDesc::Attribute::Archive | AZ::IO::FileDesc::Attribute::Subdirectory;
                m_fileSet.emplace(fname, fileDesc);
            }
        };

        auto archiveInst = static_cast<Archive*>(archive);
        AZStd::shared_lock lock(archiveInst->m_csZips);
        for (auto it = archiveInst->m_arrZips.begin(); it != archiveInst->m_arrZips.end(); ++it)
        {
            // filter out the stuff which does not match.

            // the problem here is that szDir might be something like "@products@/levels/*"
            // but our archive might be mounted at the root, or at some other folder at like "@products@" or "@products@/levels/mylevel"
            // so there's really no way to filter out opening the pack and looking at the files inside.
            // however, the bind root is not part of the inner zip entry name either
            // and the ZipDir::FindFile actually expects just the chopped off piece.
            // we have to find the common path segments between them and check that:

            AZ::IO::FixedMaxPath bindRoot;
            if (!AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(bindRoot, it->m_pathBindRoot))
            {
                AZ_Assert(false, "Unable to resolve Path for archive %s bind root %s", it->GetFullPath(), it->m_pathBindRoot.c_str());
                return;
            }


            // Example:
            // "@products@\\levels\\*" <--- szDir
            // "@products@\\" <--- mount point
            // ~~~~~~~~~~~ Common part
            // "levels\\*" <---- remainder that is not in common
            // "" <--- mount point remainder.  In this case, we should scan the contents of the pak for the remainder

            // Example:
            // "@products@\\levels\\*" <--- szDir
            // "@products@\\levels\\mylevel\\" <--- mount point (its level.pak)
            //  ~~~~~~~~~~~~~~~~~~ common part
            // "*" <----  remainder that is not in common
            // "mylevel\\" <--- mount point remainder.

            // example:
            // "@products@\\levels\\otherlevel\\*" <--- szDir
            // "@products@\\levels\\mylevel\\" <--- mount point (its level.pak)
            // "otherlevel\\*" <----  remainder
            // "mylevel\\" <--- mount point remainder.

            // the general strategy here is that IF there is a mount point remainder
            // then it means that the pack's mount point itself might be a return value, not the files inside the pack
            // in that case, we compare the mount point remainder itself with the search filter

            auto [bindRootIter, sourcePathIter] = AZStd::mismatch(bindRoot.begin(), bindRoot.end(),
                sourcePath.begin(), sourcePath.end());
            if (bindRootIter != bindRoot.end())
            {
                AZ::IO::FixedMaxPath sourcePathRemainder;
                for (; sourcePathIter != sourcePath.end(); ++sourcePathIter)
                {
                    sourcePathRemainder /= *sourcePathIter;
                }

                // Retrieve next path component of the mount point remainder
                if (!bindRootIter->empty() && bindRootIter->Match(sourcePathRemainder.Native()))
                {
                    AZ::IO::FileDesc fileDesc{ AZ::IO::FileDesc::Attribute::ReadOnly | AZ::IO::FileDesc::Attribute::Archive | AZ::IO::FileDesc::Attribute::Subdirectory };
                    m_fileSet.emplace(AZStd::move(bindRootIter->Native()), fileDesc);
                }
            }
            else
            {
                AZ::IO::FixedMaxPath sourcePathRemainder = sourcePath.LexicallyRelative(bindRoot);
                // if we get here, it means that the search pattern's root and the mount point for this pack are identical
                // which means we may search inside the pack.
                ScanInZip(it->pZip.get(), sourcePathRemainder.Native());
            }

        }
    }

    AZ::IO::ArchiveFileIterator FindData::Fetch()
    {
        if (m_fileSet.empty())
        {
            return {};
        }

        // Remove Fetched item from the FindData map so that the iteration continues
        AZ::IO::ArchiveFileIterator fileIterator;
        auto archiveFileIt = m_fileSet.begin();
        fileIterator.m_filename = archiveFileIt->m_filename;
        fileIterator.m_fileDesc = archiveFileIt->m_fileDesc;
        fileIterator.m_lastFetchValid = true;
        fileIterator.m_findData = this;
        m_fileSet.erase(archiveFileIt);
        return fileIterator;
    }
}
