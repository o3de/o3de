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

#include "EditTool.h"

// Editor
#include "Include/IObjectManager.h"
#include "Objects/SelectionGroup.h"

//////////////////////////////////////////////////////////////////////////
// Class description.
//////////////////////////////////////////////////////////////////////////
class CEditTool_ClassDesc
    : public CRefCountClassDesc
{
    virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_EDITTOOL; }
    virtual REFGUID ClassID()
    {
        // {0A43AB8E-B1AE-44aa-93B1-229F73D58CA4}
        static const GUID guid = {
            0xa43ab8e, 0xb1ae, 0x44aa, { 0x93, 0xb1, 0x22, 0x9f, 0x73, 0xd5, 0x8c, 0xa4 }
        };
        return guid;
    }
    virtual QString ClassName() { return "EditTool.Default"; };
    virtual QString Category() { return "EditTool"; };
};
CEditTool_ClassDesc g_stdClassDesc;

//////////////////////////////////////////////////////////////////////////
CEditTool::CEditTool(QObject* parent)
    : QObject(parent)
{
    m_pClassDesc = &g_stdClassDesc;
    m_nRefCount = 0;
};

//////////////////////////////////////////////////////////////////////////
void CEditTool::SetParentTool(CEditTool* pTool)
{
    m_pParentTool = pTool;
}

//////////////////////////////////////////////////////////////////////////
CEditTool* CEditTool::GetParentTool()
{
    return m_pParentTool;
}

//////////////////////////////////////////////////////////////////////////
void CEditTool::Abort()
{
    if (m_pParentTool)
    {
        GetIEditor()->SetEditTool(m_pParentTool);
    }
    else
    {
        GetIEditor()->SetEditTool(0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEditTool::GetAffectedObjects(DynArray<CBaseObject*>& outAffectedObjects)
{
    CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
    if (pSelection == NULL)
    {
        return;
    }
    for (int i = 0, iCount(pSelection->GetCount()); i < iCount; ++i)
    {
        outAffectedObjects.push_back(pSelection->GetObject(i));
    }
}

#include <moc_EditTool.cpp>
