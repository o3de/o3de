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
