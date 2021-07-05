/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    bool AZStdStringLessCaseInsensitive::operator()(AZStd::string_view left, AZStd::string_view right) const
    {
        // If one or both strings are 0-length, return true if the left side is smaller, false if they're equal or left is larger.
        size_t compareLength = (AZStd::min)(left.size(), right.size());
        if (compareLength == 0)
        {
            return left.size() < right.size();
        }

        // They're both non-zero, so compare the strings up until the length of the shorter string.
        int compareResult = azstrnicmp(left.data(), right.data(), compareLength);

        // If both strings are equal for the number of characters compared, return true if the left side is shorter, false if
        // they're equal or left is longer.
        if (compareResult == 0)
        {
            return left.size() < right.size();
        }

        // Return true if the left side should come first alphabetically, false if the right side should.
        return compareResult < 0;
    }

    FileDesc::FileDesc(Attribute fileAttribute, uint64_t fileSize, time_t accessTime, time_t creationTime, time_t writeTime)
        : nAttrib{ fileAttribute }
        , nSize{ fileSize }
        , tAccess{ accessTime }
        , tCreate{ creationTime }
        , tWrite{ writeTime }
    {
    }

    ArchiveFileIterator::ArchiveFileIterator(FindData* findData, AZStd::string_view filename, const FileDesc& fileDesc)
        : m_findData{ findData }
        , m_filename{ filename }
        , m_fileDesc{ fileDesc }
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
        AZStd::string searchDirectory;
        AZStd::string pattern;
        {
            AZ::IO::PathString directory{ szDirIn };
            AZ::StringFunc::Path::GetFullPath(directory.c_str(), searchDirectory);
            AZ::StringFunc::Path::GetFullFileName(directory.c_str(), pattern);
        }
        AZ::IO::FileIOBase::GetDirectInstance()->FindFiles(searchDirectory.c_str(), pattern.c_str(), [&](const char* filePath) -> bool
        {
            AZ::IO::ArchiveFileIterator fileIterator;
            fileIterator.m_filename = AZ::IO::PathView(filePath).Filename().Native();
            fileIterator.m_fileDesc.nAttrib = {};

            if (AZ::IO::FileIOBase::GetDirectInstance()->IsDirectory(filePath))
            {
                fileIterator.m_fileDesc.nAttrib = fileIterator.m_fileDesc.nAttrib | AZ::IO::FileDesc::Attribute::Subdirectory;
                m_fileStack.emplace_back(AZStd::move(fileIterator));
            }
            else
            {
                if (AZ::IO::FileIOBase::GetDirectInstance()->IsReadOnly(filePath))
                {
                    fileIterator.m_fileDesc.nAttrib = fileIterator.m_fileDesc.nAttrib | AZ::IO::FileDesc::Attribute::ReadOnly;
                }
                AZ::u64 fileSize = 0;
                AZ::IO::FileIOBase::GetDirectInstance()->Size(filePath, fileSize);
                fileIterator.m_fileDesc.nSize = fileSize;
                fileIterator.m_fileDesc.tWrite = AZ::IO::FileIOBase::GetDirectInstance()->ModificationTime(filePath);

                // These times are not supported by our file interface
                fileIterator.m_fileDesc.tAccess = fileIterator.m_fileDesc.tWrite;
                fileIterator.m_fileDesc.tCreate = fileIterator.m_fileDesc.tWrite;
                m_fileStack.emplace_back(AZStd::move(fileIterator));
            }
            return true;
        });
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
                m_fileStack.emplace_back(AZ::IO::ArchiveFileIterator{ this, fname, fileDesc });
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
                m_fileStack.emplace_back(AZ::IO::ArchiveFileIterator{ this, fname, fileDesc });
            }
        };

        auto archiveInst = static_cast<Archive*>(archive);
        AZStd::shared_lock lock(archiveInst->m_csZips);
        for (auto it = archiveInst->m_arrZips.begin(); it != archiveInst->m_arrZips.end(); ++it)
        {
            // filter out the stuff which does not match.

            // the problem here is that szDir might be something like "@assets@/levels/*"
            // but our archive might be mounted at the root, or at some other folder at like "@assets@" or "@assets@/levels/mylevel"
            // so there's really no way to filter out opening the pack and looking at the files inside.
            // however, the bind root is not part of the inner zip entry name either
            // and the ZipDir::FindFile actually expects just the chopped off piece.
            // we have to find whats in common between them and check that:

            auto resolvedBindRoot = AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(it->m_pathBindRoot);
            if (!resolvedBindRoot)
            {
                AZ_Assert(false, "Unable to resolve Path for archive %s bind root %s", it->GetFullPath(), it->m_pathBindRoot.c_str());
                return;
            }

            AZ::IO::FixedMaxPath bindRoot{ *resolvedBindRoot };
            auto [bindRootIter, sourcePathIter] = AZStd::mismatch(AZStd::begin(bindRoot), AZStd::end(bindRoot),
                AZStd::begin(sourcePath), AZStd::end(sourcePath));

            if (sourcePathIter == AZStd::begin(sourcePath))
            {
                // The path has no characters in common , early out the search as filepath is not part of the iterated zip
                continue;
            }

            AZ::IO::FixedMaxPath sourcePathRemainder;
            for (; sourcePathIter != AZStd::end(sourcePath); ++sourcePathIter)
            {
                sourcePathRemainder /= *sourcePathIter;
            }
            // Example:
            // "@assets@\\levels\\*" <--- szDir
            // "@assets@\\" <--- mount point
            // ~~~~~~~~~~~ Common part
            // "levels\\*" <---- remainder that is not in common
            // "" <--- mount point remainder.  In this case, we should scan the contents of the pak for the remainder

            // Example:
            // "@assets@\\levels\\*" <--- szDir
            // "@assets@\\levels\\mylevel\\" <--- mount point (its level.pak)
            //  ~~~~~~~~~~~~~~~~~~ common part
            // "*" <----  remainder that is not in common
            // "mylevel\\" <--- mount point remainder.

            // example:
            // "@assets@\\levels\\otherlevel\\*" <--- szDir
            // "@assets@\\levels\\mylevel\\" <--- mount point (its level.pak)
            // "otherlevel\\*" <----  remainder
            // "mylevel\\" <--- mount point remainder.

            // the general strategy here is that IF there is a mount point remainder
            // then it means that the pack's mount point itself might be a return value, not the files inside the pack
            // in that case, we compare the mount point remainder itself with the search filter

            if (bindRootIter != bindRoot.end())
            {
                // Retrieve next path component of the mount point remainder
                if (!bindRootIter->empty() && AZStd::wildcard_match(sourcePathRemainder.Native(), bindRootIter->Native()))
                {
                    AZ::IO::FileDesc fileDesc{ AZ::IO::FileDesc::Attribute::ReadOnly | AZ::IO::FileDesc::Attribute::Archive | AZ::IO::FileDesc::Attribute::Subdirectory };
                    m_fileStack.emplace_back(AZ::IO::ArchiveFileIterator{ this, bindRootIter->Native(), fileDesc });
                }
            }
            else
            {
                
                // if we get here, it means that the search pattern's root and the mount point for this pack are identical
                // which means we may search inside the pack.
                ScanInZip(it->pZip.get(), sourcePathRemainder.Native());
            }
          
        }
    }

    AZ::IO::ArchiveFileIterator FindData::Fetch()
    {
        if (m_fileStack.empty())
        {
            AZ::IO::ArchiveFileIterator emptyFileIterator;
            emptyFileIterator.m_lastFetchValid = false;
            emptyFileIterator.m_findData = this;
            return emptyFileIterator;
        }

        // Remove Fetched item from the FindData map so that the iteration continues
        AZ::IO::ArchiveFileIterator fileIterator{ m_fileStack.back() };
        fileIterator.m_lastFetchValid = true;
        fileIterator.m_findData = this;
        m_fileStack.pop_back();
        return fileIterator;
    }
}
