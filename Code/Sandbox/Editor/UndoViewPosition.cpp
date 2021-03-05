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
