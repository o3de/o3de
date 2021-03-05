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

#include "EditorDefs.h"

#include "LevelIndependentFileMan.h"

CLevelIndependentFileMan::CLevelIndependentFileMan()
{
}

CLevelIndependentFileMan::~CLevelIndependentFileMan()
{
    assert(m_Modules.size() == 0);
}

bool CLevelIndependentFileMan::PromptChangedFiles()
{
    for (std::vector<ILevelIndependentFileModule*>::iterator it = m_Modules.begin(); it != m_Modules.end(); ++it)
    {
        if (!(*it)->PromptChanges())
        {
            return false;
        }
    }
    return true;
}

void CLevelIndependentFileMan::RegisterModule(ILevelIndependentFileModule* pModule)
{
    stl::push_back_unique(m_Modules, pModule);
}
void CLevelIndependentFileMan::UnregisterModule(ILevelIndependentFileModule* pModule)
{
    stl::find_and_erase(m_Modules, pModule);
}
