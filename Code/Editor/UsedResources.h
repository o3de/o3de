/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once
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

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    TResourceFiles files;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};
#endif // CRYINCLUDE_EDITOR_USEDRESOURCES_H
