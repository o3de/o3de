/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once
#ifndef CRYINCLUDE_EDITORCOMMON_EDITORCOMMONAPI_H
#define CRYINCLUDE_EDITORCOMMON_EDITORCOMMONAPI_H

#include <AzCore/PlatformDef.h>

#if defined(EDITOR_COMMON_EXPORTS)

#define EDITOR_COMMON_API AZ_DLL_EXPORT

#elif defined(EDITOR_COMMON_IMPORTS)

#define EDITOR_COMMON_API AZ_DLL_IMPORT

#else

#define EDITOR_COMMON_API

#endif

struct IEditor;
struct ISystem;

void EDITOR_COMMON_API InitializeEditorCommon(IEditor* editor);
void EDITOR_COMMON_API UninitializeEditorCommon();

void EDITOR_COMMON_API InitializeEditorCommonISystem(ISystem* pSystem);
void EDITOR_COMMON_API UninitializeEditorCommonISystem(ISystem* pSystem);

#endif // CRYINCLUDE_EDITORCOMMON_EDITORCOMMONAPI_H
