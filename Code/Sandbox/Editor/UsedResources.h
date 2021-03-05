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

#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2012
// -------------------------------------------------------------------------
//  File name:   UsedResources.h
//  Created:     28/11/2003 by Timur.
//  Description: Class to gather used resources
//
////////////////////////////////////////////////////////////////////////////

//! Class passed to resource gathering functions
#ifndef CRYINCLUDE_EDITOR_USEDRESOURCES_H
#define CRYINCLUDE_EDITOR_USEDRESOURCES_H

#include "Include/EditorCoreAPI.h"

class EDITOR_CORE_API CUsedResources
{
public:
    typedef std::set<QString, stl::less_stricmp<QString> > TResourceFiles;

    CUsedResources();
    void Add(const char* pResourceFileName);
    //! validate gathered resources, reports warning if resource is not found
    void Validate(struct IErrorReport* pReport);

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    TResourceFiles files;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};
#endif // CRYINCLUDE_EDITOR_USEDRESOURCES_H
