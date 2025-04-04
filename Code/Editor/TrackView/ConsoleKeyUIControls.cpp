/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>  // for AnimParamType

// Editor
#include "KeyUIControls.h"
#include "TrackViewKeyPropertiesDlg.h"  // for CTrackViewKeyUIControls

bool CConsoleKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    bool bAssigned = false;
    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::Console)
        {
            IConsoleKey consoleKey;
            keyHandle.GetKey(&consoleKey);

            mv_command = ((QString)consoleKey.command.c_str()).toUtf8().data();

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CConsoleKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!sequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);

        CAnimParamType paramType = keyHandle.GetTrack()->GetParameterType();
        if (paramType == AnimParamType::Console)
        {
            IConsoleKey consoleKey;
            keyHandle.GetKey(&consoleKey);

            if (pVar == mv_command.GetVar())
            {
                consoleKey.command = ((QString)mv_command).toUtf8().data();
            }

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (isDuringUndo)
            {
                keyHandle.SetKey(&consoleKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&consoleKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }

        }
    }
}
