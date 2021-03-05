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

#include "SubObjSelection.h"

SSubObjSelOptions g_SubObjSelOptions;

/*

//////////////////////////////////////////////////////////////////////////
bool CSubObjSelContext::IsEmpty() const
{
    if (GetCount() == 0)
        return false;
    for (int i = 0; i < GetCount(); i++)
    {
        CSubObjectSelection *pSel = GetSelection(i);
        if (!pSel->IsEmpty())
            return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CSubObjSelContext::ModifySelection( SSubObjSelectionModifyContext &modCtx )
{
    for (int n = 0; n < GetCount(); n++)
    {
        CSubObjectSelection *pSel = GetSelection(n);
        if (pSel->IsEmpty())
            continue;
        modCtx.pSubObjSelection = pSel;
        pSel->pGeometry->SubObjSelectionModify( modCtx );
    }
    if (modCtx.type == SO_MODIFY_MOVE)
    {
        OnSelectionChange();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSubObjSelContext::AcceptModifySelection()
{
    for (int n = 0; n < GetCount(); n++)
    {
        CSubObjectSelection *pSel = GetSelection(n);
        if (pSel->IsEmpty())
            continue;
        if (pSel->pGeometry)
            pSel->pGeometry->Update();
    }
}

*/
