/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_LEVELINDEPENDENTFILEMAN_H
#define CRYINCLUDE_EDITOR_LEVELINDEPENDENTFILEMAN_H
#pragma once


struct ILevelIndependentFileModule
{
    // this function should prompt some message box if changed files need to be saved
    // if return false, editor will not continue with current action (e.g close the editor)
    virtual bool PromptChanges() = 0;
};

class CLevelIndependentFileMan
{
public:
    CLevelIndependentFileMan();
    ~CLevelIndependentFileMan();

    bool PromptChangedFiles();

    void RegisterModule(ILevelIndependentFileModule* pModule);
    void UnregisterModule(ILevelIndependentFileModule* pModule);
private:
    std::vector<ILevelIndependentFileModule* > m_Modules;
};

#endif // CRYINCLUDE_EDITOR_LEVELINDEPENDENTFILEMAN_H
