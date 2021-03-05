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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_PAKMANAGER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_PAKMANAGER_H
#pragma once


#include "ZipDir/ZipDir.h"
#include "PakSystem.h"
#include "PakHelpers.h"
#include "RcFile.h"
#include "IProgress.h"

struct IResourceCompiler;
class IConfig;

class PakManager
{
public:
    PakManager(IProgress* pProgress);
    ~PakManager();

    void RegisterKeys(IResourceCompiler* pRC);
    IPakSystem* GetPakSystem();
    bool HasPakFiles() const;

    unsigned GetMaxThreads() const;
    // -----------------------------------------------
    enum ECallResult
    {
        eCallResult_Skipped,    // functionality didn't apply and has been skipped
        eCallResult_Succeeded,  // call has been successfull
        eCallResult_Erroneous,  // call has run and ended, but with minor errors (duplicate CRC etc.)
        eCallResult_Failed,     // call has failed - pak files are in inconsistent state
        eCallResult_BadArgs,    // arguments are illformed - pak files have not been touched or changed
    };

    ECallResult CompileFilesIntoPaks(
        const IConfig* config,
        const std::vector<RcFile>& m_allFiles);

    ECallResult DeleteFilesFromPaks(
        const IConfig* config,
        const std::vector<string>& deletedTargetFiles);

private:
    ECallResult SplitListFileToPaks(
        const IConfig* config,
        const std::vector<string>& sourceRootsReversed,
        const std::vector<RcFile>& files,
        const string& pakFilePath);

    ECallResult CreatePakFile(
        const IConfig* config,
        const std::vector<RcFile>& sourceFiles,
        const string& folderInPak,
        const string& requestedPakFilename,
        bool bUpdate);

    ECallResult SynchronizePaks(
        const IConfig* config,
        const std::vector<string>& deletedTargetFiles);

    ECallResult UnzipPakFile(
        const IConfig* config,
        const std::vector<RcFile>& sourceFiles,
        const string& unzipFolder); 

private:
    // All output zip files.
    std::vector<string> m_zipFiles;

    PakSystem m_pPakSystem;
    IProgress* m_pProgress;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_PAKMANAGER_H
