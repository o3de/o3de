/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
