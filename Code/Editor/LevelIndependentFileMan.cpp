/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
