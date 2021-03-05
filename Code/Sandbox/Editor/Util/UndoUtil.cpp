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
