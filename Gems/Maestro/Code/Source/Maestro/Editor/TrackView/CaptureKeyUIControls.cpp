/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"


// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>
// Editor
#include "KeyUIControls.h"
#include "TrackViewKeyPropertiesDlg.h"

bool CCaptureKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Capture)
        {
            ICaptureKey captureKey;
            keyHandle.GetKey(&captureKey);

            mv_duration = captureKey.duration;
            mv_timeStep = captureKey.timeStep;
            mv_prefix = captureKey.prefix.c_str();
            mv_folder = captureKey.folder.c_str();
            mv_once = captureKey.once;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CCaptureKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Capture)
        {
            ICaptureKey captureKey;
            keyHandle.GetKey(&captureKey);

            SyncValue(mv_duration, captureKey.duration, false, pVar);
            SyncValue(mv_timeStep, captureKey.timeStep, false, pVar);

            if (pVar == mv_folder.GetVar())
            {
                QString sFolder = mv_folder;
                captureKey.folder = sFolder.toUtf8().data();
            }
            if (pVar == mv_prefix.GetVar())
            {
                QString sPrefix = mv_prefix;
                captureKey.prefix = sPrefix.toUtf8().data();
            }

            SyncValue(mv_once, captureKey.once, false, pVar);

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (isDuringUndo)
            {
                keyHandle.SetKey(&captureKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&captureKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
    }
}
