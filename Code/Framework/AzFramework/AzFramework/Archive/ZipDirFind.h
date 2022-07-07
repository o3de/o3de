/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Declaration of the class that can be used to search for the entries
// in a zip dir cache

#pragma once

#include <AzCore/std/string/fixed_string.h>
#include <AzFramework/Archive/ZipDirTree.h>

namespace AZ::IO::ZipDir
{
    class FileEntryTree;
    // create this structure and loop:
    //  FindData fd (pZip);
    //  for (fd.FindFirst("*.cgf"); fd.GetFileEntry(); fd.FindNext())
    //  {} // inside the loop, use GetFileEntry() and GetFileName() to get the file entry and name records
    class FindData
    {
    public:

        FindData(FileEntryTree* pRoot)
            : m_pRoot(pRoot)
        {
        }

        // returns the directory to which the current object belongs
        FileEntryTree* GetParentDir() {return m_pDirHeader; }
    protected:
        // initializes everything until the point where the file must be searched for
        // after this call returns successfully (with true returned), the m_szWildcard
        // contains the file name/wildcard and m_pDirHeader contains the directory where
        // the file (s) are to be found
        bool PreFind(AZ::IO::PathView szWildcard);

        // matches the file wildcard in the m_szWildcard to the given file/dir name
        bool MatchWildcard(AZ::IO::PathView szName);

        // the directory inside which the current object (file or directory) is being searched
        FileEntryTree* m_pDirHeader{};

        FileEntryTree* m_pRoot{}; // the root of the zip file in which to search

        // the actual wildcard being used in the current scan - the file name wildcard only!
        AZ::IO::FixedMaxPath m_szWildcard;
    };

    class FindFile
        : public FindData
    {
    public:
        FindFile(CachePtr pCache)
            : FindData(pCache->GetRoot())
        {
        }
        FindFile(FileEntryTree* pRoot)
            : FindData(pRoot)
        {
        }
        // if bExactFile is passed, only the file is searched, and besides with the exact name as passed (no wildcards)
        bool FindFirst(AZ::IO::PathView szWildcard);

        FileEntry* FindExact(AZ::IO::PathView szPath);

        // goes on to the next file entry
        bool FindNext();

        FileEntry* GetFileEntry();
        AZStd::string_view GetFileName();

    protected:
        bool SkipNonMatchingFiles();
        FileEntryTree::FileMap::iterator m_itFile; // the current file iterator inside the parent directory
    };

    class FindDir
        : public FindData
    {
    public:
        FindDir(CachePtr pCache)
            : FindData(pCache->GetRoot())
        {
        }
        FindDir(FileEntryTree* pRoot)
            : FindData(pRoot)
        {
        }
        // if bExactFile is passed, only the file is searched, and besides with the exact name as passed (no wildcards)
        bool FindFirst(AZ::IO::PathView szWildcard);

        FileEntryTree* FindExact(AZ::IO::PathView szPath);

        // goes on to the next file entry
        bool FindNext();

        FileEntryTree* GetDirEntry();
        AZStd::string_view GetDirName();

    protected:
        bool SkipNonMatchingDirs();
        FileEntryTree::SubdirMap::iterator m_itDir; // the current sub-directory iterator
    };
}
