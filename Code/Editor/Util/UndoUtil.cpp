/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "UndoUtil.h"
#include "Include/EditorCoreAPI.h"

CUndo::CUndo(const char* description)
    : m_bCancelled(false)
{
    if (!IsRecording())
    {
        GetIEditor()->BeginUndo();
        azstrncpy(m_description, scDescSize, description, scDescSize);
        m_description[scDescSize - 1] = '\0';
        m_bStartedRecord = true;
    }
    else
    {
        m_bStartedRecord = false;
    }
};

CUndo::~CUndo()
{
    if (m_bStartedRecord)
    {
        if (m_bCancelled)
        {
            GetIEditor()->CancelUndo();
        }
        else
        {
            GetIEditor()->AcceptUndo(m_description);
        }
    }
};

bool CUndo::IsRecording()
{
    if (IEditor* editor = GetIEditor())
    {
        return editor->IsUndoRecording();
    }

    return false;
}

bool CUndo::IsSuspended()
{
    if (IEditor* editor = GetIEditor())
    {
        return editor->IsUndoSuspended();
    }

    return false;
}

void CUndo::Record(IUndoObject* undo)
{
    if (IEditor* editor = GetIEditor())
    {
        editor->RecordUndo(undo);
    }
}

CUndoSuspend::CUndoSuspend()
{
    if (IEditor* editor = GetIEditor())
    {
        editor->SuspendUndo();
    }
};

CUndoSuspend::~CUndoSuspend()
{
    if (IEditor* editor = GetIEditor())
    {
        editor->ResumeUndo();
    }
};
