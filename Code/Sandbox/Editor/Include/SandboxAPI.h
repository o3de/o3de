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

// Description : Main header included by every file in Editor.


#ifndef CRYINCLUDE_EDITOR_INCLUDE_SANDBOXAPI_H
#define CRYINCLUDE_EDITOR_INCLUDE_SANDBOXAPI_H
#pragma once

#include <AzCore/PlatformDef.h>

#if defined(SANDBOX_API) || defined(CRYEDIT_API)
#error SANDBOX_API and CRYEDIT_API should only be defined in this header. Use SANDBOX_EXPORTS and SANDBOX_IMPROTS to control it.
#endif

#if defined(SANDBOX_IMPORTS) && defined(SANDBOX_EXPORTS)
#error SANDBOX_EXPORTS and SANDBOX_IMPORTS can't be defined at the same time
#endif

#if defined(SANDBOX_EXPORTS)
// Editor.exe case
#define CRYEDIT_API AZ_DLL_EXPORT
#define SANDBOX_API AZ_DLL_EXPORT
#elif defined(SANDBOX_IMPORTS)
// Sandbox plugins that rely on/extend Editor types.
#define CRYEDIT_API AZ_DLL_IMPORT
#define SANDBOX_API AZ_DLL_IMPORT
#else
// Standalone plugins that use Editor plugins.
#define CRYEDIT_API
#define SANDBOX_API
#endif
#endif // CRYINCLUDE_EDITOR_INCLUDE_SANDBOXAPI_H
