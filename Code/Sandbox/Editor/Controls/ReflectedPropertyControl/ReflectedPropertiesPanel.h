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

#ifndef CRYINCLUDE_EDITOR_REFLECTEDPROPERTIESPANEL_H
#define CRYINCLUDE_EDITOR_REFLECTEDPROPERTIESPANEL_H

#pragma once

#include "Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.h"
#include "Util/Variable.h"

/////////////////////////////////////////////////////////////////////////////
// ReflectedPropertiesPanel dialog

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
//This class is a port of ReflectedPropertiesPanel to use the ReflectedPropertyControl
class SANDBOX_API ReflectedPropertiesPanel
    : public ReflectedPropertyControl
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    ReflectedPropertiesPanel(QWidget* pParent = nullptr);   // standard constructor

    void DeleteVars();
    void AddVars(class CVarBlock* vb, const ReflectedPropertyControl::UpdateVarCallback& func = nullptr, const char* category = nullptr);

    void SetVarBlock(class CVarBlock* vb, const ReflectedPropertyControl::UpdateVarCallback& func = nullptr, const char* category = nullptr);

protected:
    void OnPropertyChanged(IVariable* pVar);

protected:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    TSmartPtr<CVarBlock> m_varBlock;

    std::list<ReflectedPropertyControl::UpdateVarCallback> m_updateCallbacks;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};


#endif // CRYINCLUDE_EDITOR_REFLECTEDPROPERTIESPANEL_H
