/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/Archive/ZipFileFormat.h>
#include <AzFramework/Archive/ZipDirStructures.h>
#include <AzFramework/Archive/ZipDirCache.h>
#include <AzFramework/Archive/ZipDirFind.h>

namespace AZ::IO::ZipDir
{
    bool FindFile::FindFirst(AZ::IO::PathView szWildcard)
    {
        if (!PreFind(szWildcard))
        {
            return false;
        }

        // finally, this is the name of the file
        m_itFile = m_pDirHeader->GetFileBegin();
        return SkipNonMatchingFiles();
    }

    bool FindDir::FindFirst(AZ::IO::PathView szWildcard)
    {
        if (!PreFind(szWildcard))
        {
            return false;
        }

        // finally, this is the name of the file
        m_itDir = m_pDirHeader->GetDirBegin();
        return SkipNonMatchingDirs();
    }

    // matches the file wildcard in the m_szWildcard to the given file/dir name
    bool FindData::MatchWildcard(AZ::IO::PathView szName)
    {
        return szName.Match(m_szWildcard.Native());
    }


    FileEntry* FindFile::FindExact(AZ::IO::PathView szPath)
    {
        if (!PreFind(szPath))
        {
            return nullptr;
        }

        FileEntryTree::FileMap::iterator itFile = m_pDirHeader->FindFile(m_szWildcard);
        if (itFile == m_pDirHeader->GetFileEnd())
        {
            m_pDirHeader = nullptr; // we didn't find it, fail the search
            return nullptr;

        }

        m_itFile = itFile;
        return m_pDirHeader->GetFileEntry(m_itFile);
    }

    FileEntryTree* FindDir::FindExact(AZ::IO::PathView szPath)
    {
        if (!PreFind(szPath))
        {
            return nullptr;
        }

        // the wild card will contain the target directory name
        return m_pDirHeader->FindDir(m_szWildcard);
    }

    //////////////////////////////////////////////////////////////////////////
    // after this call returns successfully (with true returned), the m_szWildcard
    // contains the file name/glob and m_pDirHeader contains the directory where
    // the file (s) are to be found
    bool FindData::PreFind(AZ::IO::PathView pathGlob)
    {
        if (!m_pRoot)
        {
            return false;
        }

        FileEntryTree* entryTreeHeader = m_pRoot;
        // If there is a root path in the glob path, attempt to locate it from the root
        if (AZ::IO::PathView rootPath = m_szWildcard.RootPath(); !rootPath.empty())
        {
            FileEntryTree* dirEntry = entryTreeHeader->FindDir(rootPath);
            if (dirEntry == nullptr)
            {
                return false;
            }

            entryTreeHeader = dirEntry->GetDirectory();
            pathGlob = pathGlob.RelativePath();
        }


        AZ::IO::PathView filenameSegment = pathGlob;
        // Recurse through the directories within the file tree for each remaining parent path segment
        // of pathGlob parameter
        auto parentPathIter = pathGlob.begin();
        for (auto filenamePathIter = parentPathIter == pathGlob.end() ? pathGlob.end() : AZStd::next(parentPathIter, 1);
            filenamePathIter != pathGlob.end(); ++parentPathIter, ++filenamePathIter)
        {
            FileEntryTree* dirEntry = entryTreeHeader->FindDir(*parentPathIter);
            if (dirEntry == nullptr)
            {
                return false;
            }
            entryTreeHeader = dirEntry->GetDirectory();
            filenameSegment = *filenamePathIter;
        }
       
        // At this point the all the intermediate directories have been found
        // so update the directory header to point at the last file entry tree
        m_pDirHeader = entryTreeHeader;
        m_szWildcard = filenameSegment;
        return true;
    }

    // goes on to the next entry
    bool FindFile::FindNext()
    {
        if (m_pDirHeader && m_itFile != m_pDirHeader->GetFileEnd())
        {
            ++m_itFile;
            return SkipNonMatchingFiles();
        }
        else
        {
            return false;
        }
    }

    // goes on to the next entry
    bool FindDir::FindNext()
    {
        if (m_pDirHeader && m_itDir != m_pDirHeader->GetDirEnd())
        {
            ++m_itDir;
            return SkipNonMatchingDirs();
        }
        else
        {
            return false;
        }
    }

    bool FindFile::SkipNonMatchingFiles()
    {
        AZ_Assert(m_pDirHeader, "Directory header cannot be nullptr when skipping files.");

        for (; m_itFile != m_pDirHeader->GetFileEnd(); ++m_itFile)
        {
            if (MatchWildcard(GetFileName()))
            {
                return true;
            }
        }
        // we didn't find anything other file else
        return false;
    }

    bool FindDir::SkipNonMatchingDirs()
    {
        AZ_Assert(m_pDirHeader, "Directory header is nullptr. Directories cannot be skipped");

        for (; m_itDir != m_pDirHeader->GetDirEnd(); ++m_itDir)
        {
            if (MatchWildcard(GetDirName()))
            {
                return true;
            }
        }
        // we didn't find anything other file else
        return false;
    }


    FileEntry* FindFile::GetFileEntry()
    {
        return m_pDirHeader && m_itFile != m_pDirHeader->GetFileEnd() ? m_pDirHeader->GetFileEntry(m_itFile) : nullptr;
    }
    FileEntryTree* FindDir::GetDirEntry()
    {
        return m_pDirHeader && m_itDir != m_pDirHeader->GetDirEnd() ? m_pDirHeader->GetDirEntry(m_itDir) : nullptr;
    }

    AZStd::string_view FindFile::GetFileName()
    {
        if (m_pDirHeader && m_itFile != m_pDirHeader->GetFileEnd())
        {
            return m_pDirHeader->GetFileName(m_itFile);
        }
        else
        {
            return ""; // default name
        }
    }

    AZStd::string_view FindDir::GetDirName()
    {
        if (m_pDirHeader && m_itDir != m_pDirHeader->GetDirEnd())
        {
            return m_pDirHeader->GetDirName(m_itDir);
        }
        else
        {
            return ""; // default name
        }
    }
}
