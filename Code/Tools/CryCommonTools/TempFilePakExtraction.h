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

// Description : Opens a temporary file for read only access, where the file could be
//               located in a zip or pak file. Note that if the file specified
//               already exists it does not delete it when finished.


#ifndef CRYINCLUDE_CRYCOMMONTOOLS_TEMPFILEPAKEXTRACTION_H
#define CRYINCLUDE_CRYCOMMONTOOLS_TEMPFILEPAKEXTRACTION_H
#pragma once

#include <platform.h>

struct IPakSystem;

class TempFilePakExtraction
{
public:
    TempFilePakExtraction(const char* filename, const char* tempPath, IPakSystem* pPakSystem);
    ~TempFilePakExtraction();

    const string& GetTempName() const
    {
        return m_strTempFileName;
    }

    const string& GetOriginalName() const
    {
        return m_strOriginalFileName;
    }

    bool HasTempFile() const;

private:
    string m_strTempFileName;
    string m_strOriginalFileName;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_TEMPFILEPAKEXTRACTION_H
