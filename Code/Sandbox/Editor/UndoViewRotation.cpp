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

#include "UndoViewRotation.h"

// Editor
#include "ViewManager.h"

CUndoViewRotation::CUndoViewRotation(const QString& pUndoDescription)
{
    m_undoDescription = pUndoDescription;
    m_undo = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(GetIEditor()->GetSystem()->GetViewCamera().GetMatrix())));
}

int CUndoViewRotation::GetSize()
{
    return sizeof(*this);
}

QString CUndoViewRotation::GetDescription()
{
    return m_undoDescription;
}

void CUndoViewRotation::Undo(bool bUndo)
{
    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (pRenderViewport)
    {
        if (bUndo)
        {
            m_redo = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(GetIEditor()->GetSystem()->GetViewCamera().GetMatrix())));
        }

        Matrix34 tm = pRenderViewport->GetViewTM();
        tm.SetRotationXYZ(Ang3(DEG2RAD(m_undo.x), DEG2RAD(m_undo.y), DEG2RAD(m_undo.z)), tm.GetTranslation());
        pRenderViewport->SetViewTM(tm);
    }
}

void CUndoViewRotation::Redo()
{
    CViewport* pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
    if (pRenderViewport)
    {
        Matrix34 tm = pRenderViewport->GetViewTM();
        tm.SetRotationXYZ(Ang3(DEG2RAD(m_redo.x), DEG2RAD(m_redo.y), DEG2RAD(m_redo.z)), tm.GetTranslation());
        pRenderViewport->SetViewTM(tm);
    }
}
