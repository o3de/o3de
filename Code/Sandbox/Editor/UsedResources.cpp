/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
