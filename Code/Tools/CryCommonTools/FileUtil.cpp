/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include <platform.h>
#include "FileUtil.h"
#include "PathHelpers.h"
#include "StringHelpers.h"

#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/std/functional.h>


//////////////////////////////////////////////////////////////////////////
// returns true if 'dir' is a subdirectory of 'baseDir' or same directory as 'baseDir'
// note: returns false in case of wrong names passed
static bool IsSubdirOrSameDir(const char* dir, const char* baseDir)
{

    AZ::IO::LocalFileIO localFileIO;

    char szFullPathDir[AZ_MAX_PATH_LEN];
    if(!localFileIO.ConvertToAbsolutePath(dir, szFullPathDir, sizeof(szFullPathDir)))
    {
        return false;
    }

    char szFullPathBaseDir[2 * 1024];
    if(!localFileIO.ConvertToAbsolutePath(baseDir, szFullPathBaseDir, sizeof(szFullPathBaseDir)))
    {
        return false;
    }

    const char* p = szFullPathDir;
    const char* q = szFullPathBaseDir;
    for (;; ++p, ++q)
    {
        if (tolower(*p) == tolower(*q))
        {
            if (*p == 0)
            {
                // dir is exactly same as baseDir
                return true;
            }
            continue;
        }

        if ((*p == '/' || *p == '\\') && (*q == '/' || *q == '\\'))
        {
            continue;
        }

        if (*p == 0)
        {
            // dir length is shorter than baseDir length. so it's not a subdir
            return false;
        }

        if (*q == 0)
        {
            // baseDir is shorter than dir. so may be it's a subdir.
            const bool isSubdir = (*p == '/' || *p == '\\');
            return isSubdir;
        }

        return false;
    }
}

//////////////////////////////////////////////////////////////////////////
// the paths must have trailing slash
static bool ScanDirectoryRecursive(const string& root, const string& path, const string& file, std::vector<string>& files, bool recursive, const string& dirToIgnore)
{
    bool anyFound = false;
    if (!dirToIgnore.empty())
    {
        if (IsSubdirOrSameDir(root.c_str(), dirToIgnore.c_str()))
        {
            return anyFound;
        }
    }

    AZ::IO::LocalFileIO localFileIO;
    localFileIO.FindFiles(root.c_str(), file.c_str(), [&](const char* filePath) -> bool
    {
        bool isDir = localFileIO.IsDirectory(filePath);
        if (!isDir)
        {
            const string foundFilename(filePath);
            if (StringHelpers::MatchesWildcardsIgnoreCase(foundFilename, file))
            {
                anyFound = true;
                files.push_back(PathHelpers::Join(path, PathHelpers::GetFilename(filePath)));
            }
        }

        return true; // Keep iterating
    });

    if (recursive)
    {
        localFileIO.FindFiles(root.c_str(), "*", [&](const char* filePath) -> bool
        {
            bool isDir = localFileIO.IsDirectory(filePath);
            // If recursive.
            if (isDir && strcmp(filePath, ".") && strcmp(filePath, ".."))
            {
                if (ScanDirectoryRecursive(filePath, PathHelpers::Join(path, PathHelpers::GetFilename(filePath)), file, files, recursive, dirToIgnore))
                {
                    anyFound = true;
                }
            }
            return true; // Keep iterating
        });
    }

    return anyFound;
}

//////////////////////////////////////////////////////////////////////////

bool FileUtil::ScanDirectory(const string& path, const string& file, std::vector<string>& files, bool recursive, const string& dirToIgnore)
{
    return ScanDirectoryRecursive(path, "", file, files, recursive, dirToIgnore);
}


bool FileUtil::EnsureDirectoryExists(const char* szPathIn)
{
    if (!szPathIn || !szPathIn[0])
    {
        return true;
    }

    if (DirectoryExists(szPathIn))
    {
        return true;
    }

    std::vector<char> path(szPathIn, szPathIn + strlen(szPathIn) + 1);
    char* p = &path[0];

    // Skip '/' and '//' in the beginning
    while (*p == '/' || *p == '\\')
    {
        ++p;
    }

    for (;; )
    {
        while (*p != '/' && *p != '\\' && *p)
        {
            ++p;
        }
        const char saved = *p;
        *p = 0;
        AZ::IO::LocalFileIO().CreatePath(&path[0]);
        *p++ = saved;
        if (saved == 0)
        {
            break;
        }
    }

    return DirectoryExists(szPathIn);
}
