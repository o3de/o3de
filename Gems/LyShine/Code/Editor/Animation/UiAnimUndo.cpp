/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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
