/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    bool FindFile::FindFirst(AZStd::string_view szWildcard)
    {
        if (!PreFind(szWildcard))
        {
            return false;
        }

        // finally, this is the name of the file
        m_itFile = m_pDirHeader->GetFileBegin();
        return SkipNonMatchingFiles();
    }

    bool FindDir::FindFirst(AZStd::string_view szWildcard)
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
    // this takes into account the fact that xxx. is the alias name for xxx
    bool FindData::MatchWildcard(AZStd::string_view szName)
    {
        if (AZStd::wildcard_match(m_szWildcard, szName))
        {
            return true;
        }

        // check if the file object name contains extension sign (.)
        size_t extensionOffset = szName.find('.');
        if (extensionOffset != AZStd::string_view::npos)
        {
            return false;
        }

        // no extension sign - add it
        AZStd::fixed_string<AZ_MAX_PATH_LEN> szAlias{ szName };
        szAlias.push_back('.');

        return AZStd::wildcard_match(m_szWildcard, szAlias);
    }


    FileEntry* FindFile::FindExact(AZStd::string_view szPath)
    {
        if (!PreFind(szPath))
        {
            return nullptr;
        }

        FileEntryTree::FileMap::iterator itFile = m_pDirHeader->FindFile(m_szWildcard.c_str());
        if (itFile == m_pDirHeader->GetFileEnd())
        {
            m_pDirHeader = nullptr; // we didn't find it, fail the search
            return nullptr;

        }

        m_itFile = itFile;
        return m_pDirHeader->GetFileEntry(m_itFile);
    }

    FileEntryTree* FindDir::FindExact(AZStd::string_view szPath)
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
    // contains the file name/wildcard and m_pDirHeader contains the directory where
    // the file (s) are to be found
    bool FindData::PreFind(AZStd::string_view szWildcard)
    {
        if (!m_pRoot)
        {
            return false;
        }

        // start the search from the root
        m_pDirHeader = m_pRoot;
        m_szWildcard = szWildcard;

        // for each path directory, copy it into the wildcard buffer and try to find the subdirectory
        for (AZStd::optional<AZStd::string_view> pathEntry = AZ::StringFunc::TokenizeNext(szWildcard, AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR); pathEntry;
            pathEntry = AZ::StringFunc::TokenizeNext(szWildcard, AZ_CORRECT_AND_WRONG_FILESYSTEM_SEPARATOR))
        {
            // Update wildcard to new path entry
            m_szWildcard = *pathEntry; 

            // If the wildcard parameter that has been passed to TokenizeNext is empty
            // Then pathEntry is the final portion of the path
            if (!szWildcard.empty())
            {
                FileEntryTree* dirEntry = m_pDirHeader->FindDir(*pathEntry);
                if (!dirEntry)
                {
                    m_pDirHeader = nullptr; // an intermediate directory has not been found continue the search
                    return false;
                }
                m_pDirHeader = dirEntry->GetDirectory();
            }
        }
       
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
