/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    void AddVars(class CVarBlock* vb, ReflectedPropertyControl::UpdateVarCallback* func = nullptr, const char* category = nullptr);

    void SetVarBlock(class CVarBlock* vb, ReflectedPropertyControl::UpdateVarCallback* func = nullptr, const char* category = nullptr);

protected:
    void OnPropertyChanged(IVariable* pVar);

protected:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    TSmartPtr<CVarBlock> m_varBlock;

    std::list<ReflectedPropertyControl::UpdateVarCallback*> m_updateCallbacks;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};


#endif // CRYINCLUDE_EDITOR_REFLECTEDPROPERTIESPANEL_H
