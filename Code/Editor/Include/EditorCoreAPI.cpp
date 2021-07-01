/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "EditorCoreAPI.h"

#include <AzCore/Module/Environment.h>


static IEditor* s_pEditor = nullptr;

void SetIEditor(IEditor* pEditor)
{
    //this function is called multiple times so only check once if the static editor pointer is null
    if (s_pEditor == nullptr)
    {
        assert(pEditor);
        s_pEditor = pEditor;
    }
    else if (pEditor == nullptr)
    {
        //clearing the static editor pointer
        s_pEditor = nullptr;
    }
    else
    {
        assert(pEditor == s_pEditor); //trigger a warning that multiple instances of the editor are attempting to register
    }
}

IEditor* GetIEditor()
{
    return s_pEditor;
}

void SetEditorCoreEnvironment(SSystemGlobalEnvironment* pEnv)
{
    assert(!gEnv);
    gEnv = pEnv;
}

void AttachEditorCoreAZEnvironment(AZ::EnvironmentInstance pAzEnv)
{
    AZ::Environment::Attach(pAzEnv);
}

void DetachEditorCoreAZEnvironment()
{
    AZ::Environment::Detach();
}
