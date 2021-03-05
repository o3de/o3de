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
