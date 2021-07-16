/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
