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
#include "EditorCommon_precompiled.h"

#include <platform.h>
#include "EditorCommon.h"

#include "EditorCommonAPI.h"

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
