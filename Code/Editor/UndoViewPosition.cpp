/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Undo for Python function (PySetCurrentViewPosition)


#include "EditorDefs.h"

#include "UndoViewPosition.h"

// Editor
#include "ViewManager.h"

CUndoViewPosition::CUndoViewPosition(const QString& pUndoDescription)
{
    m_undoDescription = pUndoDescription;

    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (pRenderViewport)
    {
        Matrix34 tm = pRenderViewport->GetViewTM();
        m_undo = tm.GetTranslation();
    }
}

int CUndoViewPosition::GetSize()
{
    return sizeof(*this);
}

QString CUndoViewPosition::GetDescription()
{
    return m_undoDescription;
}

void CUndoViewPosition::Undo(bool bUndo)
{
    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (pRenderViewport)
    {
        Matrix34 tm = pRenderViewport->GetViewTM();
        if (bUndo)
        {
            m_redo = tm.GetTranslation();
        }

        tm.SetTranslation(m_undo);
        pRenderViewport->SetViewTM(tm);
    }
}

void CUndoViewPosition::Redo()
{
    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (pRenderViewport)
    {
        Matrix34 tm = pRenderViewport->GetViewTM();
        tm.SetTranslation(m_redo);
        pRenderViewport->SetViewTM(tm);
    }
}
