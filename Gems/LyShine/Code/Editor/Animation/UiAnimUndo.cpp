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
#include "UiCanvasEditor_precompiled.h"
#include "UiAnimUndo.h"
#include "UiAnimUndoManager.h"

UiAnimUndo::UiAnimUndo(const char* description)
    : m_bCancelled(false)
{
    if (!IsRecording())
    {
        UiAnimUndoManager::Get()->Begin();
        m_description = description;
        m_bStartedRecord = true;
    }
    else
    {
        m_bStartedRecord = false;
    }
};

UiAnimUndo::~UiAnimUndo()
{
    if (m_bStartedRecord)
    {
        if (m_bCancelled)
        {
            UiAnimUndoManager::Get()->Cancel();
        }
        else
        {
            UiAnimUndoManager::Get()->Accept(m_description);
        }
    }
};

bool UiAnimUndo::IsRecording()
{
    return UiAnimUndoManager::Get()->IsUndoRecording();
}

bool UiAnimUndo::IsSuspended()
{
    return UiAnimUndoManager::Get()->IsUndoSuspended();
}

void UiAnimUndo::Record(UiAnimUndoObject* undo)
{
    return UiAnimUndoManager::Get()->RecordUndo(undo);
}
