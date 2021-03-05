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
