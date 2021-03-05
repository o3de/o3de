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

#include "StdAfx.h"
#include "CBAHelpers.h"
#include "../PathHelpers.h"
#include "StringHelpers.h"

static string FindRootContainingFileGoingUpwards(const char* filePath, const char* filePathToLookFor, IPakSystem* pakSystem)
{
    // Here we just search upwards from the current directory, looking for a directory that
    // contains a file at the relative path "Animations/Animations.cba". This is designed to
    // handle root Game paths that differ from the default "Game".
    string rootDirCandidate = PathHelpers::GetDirectory(filePath);
    string rootDir;
    while (!rootDirCandidate.empty())
    {
        string cbaCandidatePath = PathHelpers::Join(rootDirCandidate, filePathToLookFor);
        if (PakSystemFile* file = pakSystem->Open(cbaCandidatePath.c_str(), "r"))
        {
            // File exists, we have found the correct root path.
            pakSystem->Close(file);

            rootDir = rootDirCandidate;
            break;
        }

        string previousCandidate = rootDirCandidate;
        rootDirCandidate = PathHelpers::GetDirectory(rootDirCandidate);
        if (rootDirCandidate == previousCandidate)
        {
            break;
        }
    }

    return (rootDir.empty() ? rootDir : PathHelpers::Join(rootDir, filePathToLookFor));
}

string CBAHelpers::FindCBAFileForFile(const char* filePath, IPakSystem* pakSystem)
{
    return FindRootContainingFileGoingUpwards(filePath, "Animations/Animations.cba", pakSystem);
}

string CBAHelpers::FindSkeletonListForFile(const char* filePath, IPakSystem* pakSystem)
{
    return FindRootContainingFileGoingUpwards(filePath, "Animations/SkeletonList.xml", pakSystem);
}
