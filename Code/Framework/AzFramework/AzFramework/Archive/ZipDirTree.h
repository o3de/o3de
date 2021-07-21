/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::IO::ZipDir
{
    class FileEntryTree
    {
    public:
        AZ_CLASS_ALLOCATOR(FileEntryTree, AZ::SystemAllocator, 0);
        ~FileEntryTree () {Clear(); }

        // adds a file to this directory
        // Function can modify szPath input
        ErrorEnum Add(AZStd::string_view szPath, const FileEntryBase& file);

        // Adds or finds the file. Returns non-initialized structure if it was added,
        // or an IsInitialized() structure if it was found
        // Function can modify szPath input
        FileEntry* Add(AZStd::string_view szPath);

        // returns the number of files in this tree, including this and sublevels
        uint32_t NumFilesTotal() const;

        // Total number of directories
        uint32_t NumDirsTotal() const;

        void Clear();

        void Swap(FileEntryTree& rThat)
        {
            m_mapDirs.swap(rThat.m_mapDirs);
            m_mapFiles.swap(rThat.m_mapFiles);
        }

        size_t GetSize() const;

        bool IsOwnerOf(const FileEntry* pFileEntry) const;

        // subdirectories
        using SubdirMap = AZStd::map<AZStd::string_view, AZStd::unique_ptr<FileEntryTree>>;
        // file entries
        using FileMap = AZStd::map<AZStd::string_view, AZStd::unique_ptr<FileEntry>>;

        FileEntryTree* FindDir(AZStd::string_view szDirName);
        ErrorEnum RemoveDir (AZStd::string_view szDirName);
        ErrorEnum RemoveAll ()
        {
            Clear();
            return ZD_ERROR_SUCCESS;
        }
        FileMap::iterator FindFile(AZStd::string_view szFileName);
        ErrorEnum RemoveFile(AZStd::string_view szFileName);
        // the FileEntryTree is simultaneously an entry in the dir list AND the directory header
        FileEntryTree* GetDirectory()
        {
            return this;
        } 

        FileMap::iterator GetFileBegin() { return m_mapFiles.begin(); }
        FileMap::iterator GetFileEnd() { return m_mapFiles.end(); }
        uint32_t NumFiles() const { return aznumeric_cast<uint32_t>(m_mapFiles.size()); }
        SubdirMap::iterator GetDirBegin() { return m_mapDirs.begin(); }
        SubdirMap::iterator GetDirEnd() { return m_mapDirs.end(); }
        uint32_t NumDirs() const { return aznumeric_cast<uint32_t>(m_mapDirs.size()); }
        AZStd::string_view GetFileName(FileMap::iterator it) { return it->first; }
        AZStd::string_view GetDirName(SubdirMap::iterator it) { return it->first; }

        FileEntry* GetFileEntry(FileMap::iterator it);
        FileEntryTree* GetDirEntry(SubdirMap::iterator it);

    protected:
        SubdirMap m_mapDirs;
        FileMap m_mapFiles;
    };
}
