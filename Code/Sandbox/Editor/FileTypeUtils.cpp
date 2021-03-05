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

#include "FileTypeUtils.h"

// the supported file types
static const char* szSupportedExt[] =
{
    "chr",
    "chr(c)",
    "skin",
    "skin(c)",
    "cdf",
    "cdf(c)",
    "cgf",
    "cgf(c)",
    "cga",
    "cga(c)",
    "cid",
    "caf"
};

bool IsPreviewableFileType (const char* szPath)
{
    QString szExt = Path::GetExt(szPath);

    for (unsigned int i = 0; i < sizeof(szSupportedExt) / sizeof(szSupportedExt[0]); ++i)
    {
        if (QString::compare(szExt, szSupportedExt[i], Qt::CaseInsensitive) == 0)
        {
            return true;
        }
    }

    return false;
}
