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
