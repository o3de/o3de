/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "KeyUIControls.h"

#include "TrackViewKeyPropertiesDlg.h"
#include <CryCommon/Maestro/Types/AnimValueType.h>

bool CStringKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
{
    if (!selectedKeys.AreAllKeysOfSameType())
    {
        return false;
    }

    if (selectedKeys.GetKeyCount() == 1)
    {
        const CTrackViewKeyHandle& keyHandle = selectedKeys.GetKey(0);

        AnimValueType valueType = keyHandle.GetTrack()->GetValueType();
        if (valueType == AnimValueType::String)
        {
            IStringKey stringKey;
            keyHandle.GetKey(&stringKey);
            mv_value = QString::fromStdString(stringKey.m_strValue.c_str());            
            return true;        
        }
    }
    return false;
}

void CStringKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
{
    CTrackViewSequence* sequence = GetIEditor()->GetAnimation()->GetSequence();

    if (!sequence || !selectedKeys.AreAllKeysOfSameType())
    {
        return;
    }

    for (unsigned int keyIndex = 0; keyIndex < selectedKeys.GetKeyCount(); ++keyIndex)
    {
        CTrackViewKeyHandle keyHandle = selectedKeys.GetKey(keyIndex);
        if (keyHandle.GetTrack()->GetValueType() != AnimValueType::String)
        {
            continue;
        }

        IStringKey stringKey;
        keyHandle.GetKey(&stringKey);

        if (pVar == mv_value.GetVar())
        {
            stringKey.m_strValue = qUtf8Printable(mv_value);
        }

        bool isDuringUndo = false;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

        if (isDuringUndo)
        {
            keyHandle.SetKey(&stringKey);
        }
        else
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
            keyHandle.SetKey(&stringKey);
            undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
        }
    }
}
