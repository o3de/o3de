/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Console/Console.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Archive/ZipFileFormat.h>
#include <AzFramework/Archive/ZipDirStructures.h>
#include <AzFramework/Archive/ZipDirTree.h>

namespace AZ::IO::ZipDir
{
    // Adds or finds the file. Returns non-initialized structure if it was added,
    // or an IsInitialized() structure if it was found
    FileEntry* FileEntryTree::Add(AZ::IO::PathView inputPathView)
    {
        if (inputPathView.empty())
        {
            AZ_Assert(false, "An empty file path cannot be added to the zip file entry tree");
            return nullptr;
        }

        // If a path separator was found, add a subdirectory
        auto inputPathIter = inputPathView.begin();
        AZ::IO::PathView firstPathSegment(*inputPathIter);
        auto inputPathNextIter = inputPathIter == inputPathView.end() ? inputPathView.end() : AZStd::next(inputPathIter, 1);
        AZ::IO::PathView remainingPath = inputPathNextIter != inputPathView.end() ?
            AZStd::string_view(inputPathNextIter->Native().begin(), inputPathView.Native().end())
            : AZStd::string_view{};
        if (!remainingPath.empty())
        {
            auto dirEntryIter = m_mapDirs.find(firstPathSegment);
            // we have a subdirectory here - create the file in it
            if (dirEntryIter == m_mapDirs.end())
            {
                dirEntryIter = m_mapDirs.emplace(firstPathSegment, AZStd::make_unique<FileEntryTree>()).first;
            }
            return dirEntryIter->second->Add(remainingPath);
        }
        // Add the filename
        auto fileEntryIter = m_mapFiles.find(firstPathSegment);
        if (fileEntryIter == m_mapFiles.end())
        {
            fileEntryIter = m_mapFiles.emplace(firstPathSegment, AZStd::make_unique<FileEntry>()).first;
        }
        return fileEntryIter->second.get();
    }

    // adds a file to this directory
    ErrorEnum FileEntryTree::Add(AZ::IO::PathView szPath, const FileEntryBase& file)
    {
        FileEntry* pFile = Add(szPath);
        if (!pFile)
        {
            return ZD_ERROR_INVALID_PATH;
        }
        if (pFile->IsInitialized())
        {
            return ZD_ERROR_FILE_ALREADY_EXISTS;
        }
        static_cast<FileEntryBase&>(*pFile) = file;
        return ZD_ERROR_SUCCESS;
    }

    // returns the number of files in this tree, including this and subdirectories
    uint32_t FileEntryTree::NumFilesTotal() const
    {
        uint32_t numFiles = aznumeric_cast<uint32_t>(m_mapFiles.size());
        for (const auto& [dirname, fileEntryTree] : m_mapDirs)
        {
            numFiles += fileEntryTree->NumFilesTotal();
        }
        return numFiles;
    }

    //////////////////////////////////////////////////////////////////////////
    uint32_t FileEntryTree::NumDirsTotal() const
    {
        uint32_t numDirs = 1;
        for (const auto& [dirname, fileEntryTree] : m_mapDirs)
        {
            numDirs += fileEntryTree->NumDirsTotal();
        }
        return numDirs;
    }

    void FileEntryTree::Clear()
    {
        m_mapDirs.clear();
        m_mapFiles.clear();
    }

    bool FileEntryTree::IsOwnerOf(const FileEntry* pFileEntry) const
    {
        for (const auto& [path, fileEntry] : m_mapFiles)
        {
            if (pFileEntry == fileEntry.get())
            {
                return true;
            }
        }

        for (const auto& [path, fileEntryTree] : m_mapDirs)
        {
            if (fileEntryTree->IsOwnerOf(pFileEntry))
            {
                return true;
            }
        }

        return false;
    }

    FileEntryTree* FileEntryTree::FindDir(AZ::IO::PathView szDirName)
    {
        if (auto it = m_mapDirs.find(szDirName); it != m_mapDirs.end())
        {
            return it->second.get();
        }

        return nullptr;
    }

    FileEntryTree::FileMap::iterator FileEntryTree::FindFile(AZ::IO::PathView szFileName)
    {
        return m_mapFiles.find(szFileName);
    }

    FileEntry* FileEntryTree::GetFileEntry(FileMap::iterator it)
    {
        return it == GetFileEnd() ? nullptr : it->second.get();
    }

    FileEntryTree* FileEntryTree::GetDirEntry(SubdirMap::iterator it)
    {
        return it == GetDirEnd() ? nullptr : it->second.get();
    }

    ErrorEnum FileEntryTree::RemoveDir(AZ::IO::PathView szDirName)
    {
        SubdirMap::iterator itRemove = m_mapDirs.find(szDirName);
        if (itRemove == m_mapDirs.end())
        {
            return ZD_ERROR_FILE_NOT_FOUND;
        }

        m_mapDirs.erase(itRemove);
        return ZD_ERROR_SUCCESS;
    }

    ErrorEnum FileEntryTree::RemoveAll()
    {
        Clear();
        return ZD_ERROR_SUCCESS;
    }

    ErrorEnum FileEntryTree::RemoveFile(AZ::IO::PathView szFileName)
    {
        FileMap::iterator itRemove = m_mapFiles.find(szFileName);
        if (itRemove == m_mapFiles.end())
        {
            return ZD_ERROR_FILE_NOT_FOUND;
        }

        m_mapFiles.erase(itRemove);
        return ZD_ERROR_SUCCESS;
    }
}
