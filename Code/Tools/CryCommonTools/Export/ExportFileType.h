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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_EXPORTFILETYPE_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_EXPORTFILETYPE_H
#pragma once


enum CryFileType
{
    CRY_FILE_TYPE_NONE = 0x0000,
    CRY_FILE_TYPE_CGF = 0x0001,
    CRY_FILE_TYPE_CGA = 0x0002,
    CRY_FILE_TYPE_CHR = 0x0004,
    CRY_FILE_TYPE_CAF = 0x0008,
    CRY_FILE_TYPE_ANM = 0x0010,
    CRY_FILE_TYPE_SKIN = 0x0020,
    CRY_FILE_TYPE_INTERMEDIATE_CAF = 0x0040,
    //START: Add Skinned Geometry (.CGF) export type (for touch bending vegetation)
    CRY_FILE_TYPE_SKIN_CGF = 0x0080,
    //END: Add Skinned Geometry (.CGF) export type (for touch bending vegetation)
};

namespace ExportFileTypeHelpers
{
    const char* CryFileTypeToString(int cryFileType);
    int StringToCryFileType(const char* str);
};


#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_EXPORTFILETYPE_H

