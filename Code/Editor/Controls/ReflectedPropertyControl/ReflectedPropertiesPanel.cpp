/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : implementation file

#include "EditorDefs.h"

#include "ReflectedPropertiesPanel.h"

/////////////////////////////////////////////////////////////////////////////
// ReflectedPropertiesPanel dialog


ReflectedPropertiesPanel::ReflectedPropertiesPanel(QWidget* pParent)
    : ReflectedPropertyControl(pParent)
{
}

//////////////////////////////////////////////////////////////////////////
void ReflectedPropertiesPanel::DeleteVars()
{
    ClearVarBlock();
    m_updateCallbacks.clear();
    m_varBlock = 0;
}

//////////////////////////////////////////////////////////////////////////
void ReflectedPropertiesPanel::SetVarBlock(class CVarBlock* vb, ReflectedPropertyControl::UpdateVarCallback* updCallback, const char* category)
{
    assert(vb);

    m_varBlock = vb;

    RemoveAllItems();
    m_varBlock = vb;
    AddVarBlock(m_varBlock, category);

    SetUpdateCallback(AZStd::bind(&ReflectedPropertiesPanel::OnPropertyChanged, this, AZStd::placeholders::_1));

    // When new object set all previous callbacks freed.
    m_updateCallbacks.clear();
    if (updCallback)
    {
        stl::push_back_unique(m_updateCallbacks, updCallback);
    }
}

//////////////////////////////////////////////////////////////////////////
void ReflectedPropertiesPanel::AddVars(CVarBlock* vb, ReflectedPropertyControl::UpdateVarCallback* updCallback, const char* category)
{
    assert(vb);

    bool bNewBlock = false;
    // Make a clone of properties.
    if (!m_varBlock)
    {
        RemoveAllItems();
        m_varBlock = vb->Clone(true);
        AddVarBlock(m_varBlock, category);
        bNewBlock = true;
    }
    m_varBlock->Wire(vb);

    if (bNewBlock)
    {
        SetUpdateCallback(AZStd::bind(&ReflectedPropertiesPanel::OnPropertyChanged, this, AZStd::placeholders::_1));

        // When new object set all previous callbacks freed.
        m_updateCallbacks.clear();
    }

    if (updCallback)
    {
        stl::push_back_unique(m_updateCallbacks, updCallback);
    }
}

void ReflectedPropertiesPanel::OnPropertyChanged(IVariable* pVar)
{
    std::list<ReflectedPropertyControl::UpdateVarCallback*>::iterator iter;
    for (iter = m_updateCallbacks.begin(); iter != m_updateCallbacks.end(); ++iter)
    {
        (*iter)->operator()(pVar);
    }
}


