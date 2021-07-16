/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
