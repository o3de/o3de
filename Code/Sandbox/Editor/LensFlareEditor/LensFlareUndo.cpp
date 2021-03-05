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

#include "LensFlareUndo.h"

// Editor
#include "LensFlareEditor.h"
#include "LensFlareItem.h"
#include "LensFlareManager.h"
#include "LensFlareElementTree.h"


void RestoreLensFlareItem(CLensFlareItem* pLensFlareItem, IOpticsElementBasePtr pOptics, const QString& flarePathName)
{
    pLensFlareItem->ReplaceOptics(pOptics);

    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor)
    {
        if (pOptics->GetType() == eFT_Root)
        {
            pEditor->RenameLensFlareItem(pLensFlareItem, pLensFlareItem->GetGroupName(), LensFlareUtil::GetShortName(flarePathName));
            pEditor->UpdateLensFlareItem(pLensFlareItem);
        }
    }
}

void ActivateLensFlareItem(CLensFlareItem* pLensFlareItem, bool bRestoreSelectInfo, const QString& selectedFlareItemName)
{
    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor == NULL)
    {
        return;
    }
    if (pLensFlareItem == pEditor->GetLensFlareElementTree()->GetLensFlareItem())
    {
        pEditor->UpdateLensFlareItem(pLensFlareItem);
        if (bRestoreSelectInfo)
        {
            pEditor->SelectItemInLensFlareElementTreeByName(selectedFlareItemName);
        }
    }
}

CUndoLensFlareItem::CUndoLensFlareItem(CLensFlareItem* pLensFlareItem, const QString& undoDescription)
{
    if (pLensFlareItem)
    {
        m_Undo.m_pOptics = LensFlareUtil::CreateOptics(pLensFlareItem->GetOptics());
        m_flarePathName = pLensFlareItem->GetFullName();
        m_Undo.m_bRestoreSelectInfo = false;
        CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
        if (pEditor)
        {
            m_Undo.m_bRestoreSelectInfo = pEditor->GetSelectedLensFlareName(m_Undo.m_selectedFlareItemName);
        }
        m_undoDescription = undoDescription;
    }
}

CUndoLensFlareItem::~CUndoLensFlareItem()
{
}

void CUndoLensFlareItem::Undo(bool bUndo)
{
    if (bUndo)
    {
        CLensFlareItem* pLensFlareItem = (CLensFlareItem*)GetIEditor()->GetLensFlareManager()->FindItemByName(m_flarePathName);
        CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
        if (pEditor && pLensFlareItem)
        {
            m_Redo.m_bRestoreSelectInfo = false;
            m_Redo.m_pOptics = LensFlareUtil::CreateOptics(pLensFlareItem->GetOptics());
            m_Redo.m_bRestoreSelectInfo = pEditor->GetSelectedLensFlareName(m_Redo.m_selectedFlareItemName);
        }
    }
    Restore(m_Undo);
}

void CUndoLensFlareItem::Redo()
{
    Restore(m_Redo);
}

void CUndoLensFlareItem::Restore(const SData& data)
{
    if (data.m_pOptics == NULL)
    {
        return;
    }

    CLensFlareItem* pLensFlareItem = (CLensFlareItem*)GetIEditor()->GetLensFlareManager()->FindItemByName(m_flarePathName);
    if (pLensFlareItem == NULL)
    {
        return;
    }
    RestoreLensFlareItem(pLensFlareItem, data.m_pOptics, m_flarePathName);
    ActivateLensFlareItem(pLensFlareItem, data.m_bRestoreSelectInfo, data.m_selectedFlareItemName);
}

CUndoRenameLensFlareItem::CUndoRenameLensFlareItem(const QString& oldFullName, const QString& newFullName)
{
    m_undo.m_oldFullItemName = oldFullName;
    m_undo.m_newFullItemName = newFullName;
}

void CUndoRenameLensFlareItem::Undo(bool bUndo)
{
    if (bUndo)
    {
        m_redo.m_oldFullItemName = m_undo.m_newFullItemName;
        m_redo.m_newFullItemName = m_undo.m_oldFullItemName;
    }

    Rename(m_undo);
}

void CUndoRenameLensFlareItem::Redo()
{
    Rename(m_redo);
}

void CUndoRenameLensFlareItem::Rename(const SUndoDataStruct& data)
{
    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor == NULL)
    {
        return;
    }

    CLensFlareItem* pLensFlareItem = (CLensFlareItem*)GetIEditor()->GetLensFlareManager()->FindItemByName(data.m_newFullItemName);
    if (pLensFlareItem == NULL)
    {
        return;
    }

    int nDotPos = data.m_oldFullItemName.indexOf(".");
    if (nDotPos == -1)
    {
        return;
    }

    QString shortName(LensFlareUtil::GetShortName(data.m_oldFullItemName));
    QString groupName(LensFlareUtil::GetGroupNameFromFullName(data.m_oldFullItemName));
    pEditor->RenameLensFlareItem(pLensFlareItem, groupName, shortName);
    if (pLensFlareItem->GetOptics())
    {
        pLensFlareItem->GetOptics()->SetName(shortName.toUtf8().data());
        LensFlareUtil::UpdateOpticsName(pLensFlareItem->GetOptics());
    }
    pEditor->UpdateLensFlareItem(pLensFlareItem);
}

CUndoLensFlareElementSelection::CUndoLensFlareElementSelection(CLensFlareItem* pLensFlareItem, const QString& flareTreeItemFullName, const QString& undoDescription)
{
    if (pLensFlareItem)
    {
        m_flarePathNameForUndo = pLensFlareItem->GetFullName();
        m_flareTreeItemFullNameForUndo = flareTreeItemFullName;
        m_undoDescription = undoDescription;
    }
}

void CUndoLensFlareElementSelection::Undo(bool bUndo)
{
    if (bUndo)
    {
        m_flarePathNameForRedo = m_flarePathNameForUndo;
        CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
        if (pEditor)
        {
            pEditor->GetSelectedLensFlareName(m_flareTreeItemFullNameForRedo);
        }
    }

    CLensFlareItem* pLensFlareItem = (CLensFlareItem*)GetIEditor()->GetLensFlareManager()->FindItemByName(m_flarePathNameForUndo);
    if (pLensFlareItem == NULL)
    {
        return;
    }
    ActivateLensFlareItem(pLensFlareItem, true, m_flareTreeItemFullNameForUndo);
}

void CUndoLensFlareElementSelection::Redo()
{
    CLensFlareItem* pLensFlareItem = (CLensFlareItem*)GetIEditor()->GetLensFlareManager()->FindItemByName(m_flarePathNameForRedo);
    if (pLensFlareItem == NULL)
    {
        return;
    }
    ActivateLensFlareItem(pLensFlareItem, true, m_flareTreeItemFullNameForRedo);
}

CUndoLensFlareItemSelectionChange::CUndoLensFlareItemSelectionChange(const QString& fullLensFlareItemName, const QString& undoDescription)
{
    m_FullLensFlareItemNameForUndo = fullLensFlareItemName;
    m_undoDescription = undoDescription;
}

void CUndoLensFlareItemSelectionChange::Undo(bool bUndo)
{
    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor == NULL)
    {
        return;
    }

    if (bUndo)
    {
        QString currentFullName;
        pEditor->GetFullSelectedFlareItemName(currentFullName);
        m_FullLensFlareItemNameForRedo = currentFullName;
    }

    pEditor->SelectLensFlareItem(m_FullLensFlareItemNameForUndo);
}

void CUndoLensFlareItemSelectionChange::Redo()
{
    CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
    if (pEditor == NULL)
    {
        return;
    }
    pEditor->SelectLensFlareItem(m_FullLensFlareItemNameForRedo);
}
