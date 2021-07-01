/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Undo for Python function (PySetConfigSpec)


#include "EditorDefs.h"

#include "UndoConfigSpec.h"

CUndoConficSpec::CUndoConficSpec(const QString& pUndoDescription)
{
    m_undo = GetIEditor()->GetEditorConfigSpec();
    m_undoDescription = pUndoDescription;
}

int CUndoConficSpec::GetSize()
{
    return sizeof(*this);
}

QString CUndoConficSpec::GetDescription()
{
    return m_undoDescription;
}

void CUndoConficSpec::Undo(bool bUndo)
{
    if (bUndo)
    {
        m_redo = GetIEditor()->GetEditorConfigSpec();
    }
    GetIEditor()->SetEditorConfigSpec((ESystemConfigSpec)m_undo, GetIEditor()->GetEditorConfigPlatform());
}

void CUndoConficSpec::Redo()
{
    GetIEditor()->SetEditorConfigSpec((ESystemConfigSpec)m_redo, GetIEditor()->GetEditorConfigPlatform());
}
