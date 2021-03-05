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

#ifndef CRYINCLUDE_CRYCOMMON_RESOURCECOMPILERHELPER_H
#define CRYINCLUDE_CRYCOMMON_RESOURCECOMPILERHELPER_H

#pragma once

#if defined(CRY_ENABLE_RC_HELPER)

#include "IResourceCompilerHelper.h"

//////////////////////////////////////////////////////////////////////////
// Provides settings and functions to make calls to RC.
// calls RC locally.  only works on windows, does not exist on other platforms
// note: You shouldn't be calling this directly
// instead, you should be calling it via the IResourceCompilerHelper interface.
// since it may be replaced with a custom RC for your platform or a remote invocation
class CResourceCompilerHelper
    : public IResourceCompilerHelper
{
public:
    virtual ERcCallResult CallResourceCompiler(
        const char* szFileName = 0,
        const char* szAdditionalSettings = 0,
        IResourceCompilerListener* listener = 0,
        bool bMayShowWindow = true,
        bool bSilent = false,
        bool bNoUserDialog = false,
        const wchar_t* szWorkingDirectory = 0,
        const wchar_t* szRootPath = 0) override;
};

#endif // CRY_ENABLE_RC_HELPER

#endif // CRYINCLUDE_CRYCOMMON_RESOURCECOMPILERHELPER_H
