/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <platform.h>
#include "EditorCommon.h"

#include "EditorCommonAPI.h"

#include <CryCommon/ISystem.h>

CEditorCommonApp::CEditorCommonApp()
{
}


void EDITOR_COMMON_API InitializeEditorCommon([[maybe_unused]] IEditor* editor)
{
}

void EDITOR_COMMON_API UninitializeEditorCommon()
{
}

void EDITOR_COMMON_API InitializeEditorCommonISystem(ISystem* pSystem)
{
    ModuleInitISystem(pSystem, "EditorCommon");
}

void EDITOR_COMMON_API UninitializeEditorCommonISystem(ISystem* pSystem)
{
    ModuleShutdownISystem(pSystem);
}
