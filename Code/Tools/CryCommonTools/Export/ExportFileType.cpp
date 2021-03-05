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
#include "ExportFileType.h"
#include "StringHelpers.h"


struct SFileTypeInfo
{
    int type;
    const char* name;
};

SFileTypeInfo s_fileTypes[] =
{
    { CRY_FILE_TYPE_CGF, "cgf" },
    { CRY_FILE_TYPE_CGA, "cga" },
    { CRY_FILE_TYPE_CHR, "chr" },
    { CRY_FILE_TYPE_CAF, "caf" },
    { CRY_FILE_TYPE_ANM, "anm" },
    { CRY_FILE_TYPE_CHR | CRY_FILE_TYPE_CAF, "chrcaf" },
    { CRY_FILE_TYPE_CGA | CRY_FILE_TYPE_ANM, "cgaanm" },
    { CRY_FILE_TYPE_SKIN, "skin" },
    { CRY_FILE_TYPE_INTERMEDIATE_CAF, "i_caf" },
};

static const int s_fileTypeCount = (sizeof(s_fileTypes) / sizeof(s_fileTypes[0]));


const char* ExportFileTypeHelpers::CryFileTypeToString(int const cryFileType)
{
    for (int i = 0; i < s_fileTypeCount; ++i)
    {
        if (s_fileTypes[i].type == cryFileType)
        {
            return s_fileTypes[i].name;
        }
    }
    return "unknown";
}

int ExportFileTypeHelpers::StringToCryFileType(const char* str)
{
    if (str)
    {
        for (int i = 0; i < s_fileTypeCount; ++i)
        {
            if (_stricmp(str, s_fileTypes[i].name) == 0)
            {
                return s_fileTypes[i].type;
            }
        }
    }
    return CRY_FILE_TYPE_NONE;
}
