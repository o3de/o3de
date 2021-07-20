/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_EDITOR_CORE_INCLUDE_API_H
#define CRYINCLUDE_EDITOR_CORE_INCLUDE_API_H
#pragma once

#include <AzCore/PlatformDef.h>

#if defined(AZ_PLATFORM_WINDOWS)
#if defined(EDITOR_CORE)
    #define EDITOR_CORE_API AZ_DLL_EXPORT
#else
    #define EDITOR_CORE_API AZ_DLL_IMPORT
#endif
#elif defined(AZ_PLATFORM_MAC) || defined(AZ_PLATFORM_LINUX)
#if defined(EDITOR_CORE)
    #define EDITOR_CORE_API __attribute__ ((visibility ("default")))
#else
    #define EDITOR_CORE_API 
#endif
#endif

struct IEditor;


namespace AZ
{
    namespace Internal
    {
        class EnvironmentInterface;
    }
    typedef Internal::EnvironmentInterface* EnvironmentInstance;
}

EDITOR_CORE_API void SetIEditor(IEditor* pEditor);
EDITOR_CORE_API IEditor* GetIEditor();

//! Attach the editorcore dll to the system environmen in the System DLL
EDITOR_CORE_API void SetEditorCoreEnvironment(struct SSystemGlobalEnvironment* pEnv);

//! Attach the editorcore dll to the AZ Environment which allows ebus and memory allocation - should be done really early
EDITOR_CORE_API void AttachEditorCoreAZEnvironment(AZ::EnvironmentInstance pAzEnv);

//! Detach the editorcore dll from the AZ Environment, should be done last.
EDITOR_CORE_API void DetachEditorCoreAZEnvironment();

#if defined(AZ_PLATFORM_WINDOWS)
#include "../IEditor.h"
#endif

#endif // CRYINCLUDE_EDITOR_CORE_INCLUDE_API_H
