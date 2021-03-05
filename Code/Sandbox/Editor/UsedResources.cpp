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

#include "EditorDefs.h"

#include "UsedResources.h"
#include "ErrorReport.h"

CUsedResources::CUsedResources()
{
}

void CUsedResources::Add(const char* pResourceFileName)
{
    if (pResourceFileName && strcmp(pResourceFileName, ""))
    {
        files.insert(pResourceFileName);
    }
}

void CUsedResources::Validate(IErrorReport* pReport)
{
    auto pPak = gEnv->pCryPak;

    for (TResourceFiles::iterator it = files.begin(); it != files.end(); ++it)
    {
        const QString& filename = *it;

        bool fileExists = pPak->IsFileExist(filename.toUtf8().data());

        if (!fileExists)
        {
            for (int i = 0; !fileExists && i < IResourceCompilerHelper::GetNumEngineImageFormats(); ++i)
            {
                fileExists = gEnv->pCryPak->IsFileExist(PathUtil::ReplaceExtension(filename.toUtf8().data(), IResourceCompilerHelper::GetEngineImageFormat(i, true)).c_str());
            }
            for (int i = 0; !fileExists && i < IResourceCompilerHelper::GetNumSourceImageFormats(); ++i)
            {
                fileExists = gEnv->pCryPak->IsFileExist(PathUtil::ReplaceExtension(filename.toUtf8().data(), IResourceCompilerHelper::GetSourceImageFormat(i, true)).c_str());
            }
        }


        if (!fileExists)
        {
            CErrorRecord err;

            err.error = QObject::tr("Resource File %1 not found,").arg(filename);
            err.severity = CErrorRecord::ESEVERITY_ERROR;
            err.flags |= CErrorRecord::FLAG_NOFILE;
            pReport->ReportError(err);
        }
    }
}
