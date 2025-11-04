/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "KeyUIControls.h"
#include "TrackViewKeyPropertiesDlg.h"  // for CTrackViewKeyUIControls

#include <CryCommon/Maestro/Types/AnimParamType.h>  // AnimParamType

bool CEventKeyUIControls::OnKeySelectionChange(const CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Event)
        {
            mv_event.SetEnumList(nullptr);
            mv_animation.SetEnumList(nullptr);

            // Add <None> for empty, unset event
            mv_event->AddEnumItem(QObject::tr("<None>"), "");
            mv_animation->AddEnumItem(QObject::tr("<None>"), "");

            IEventKey eventKey;
            keyHandle.GetKey(&eventKey);

            mv_event = eventKey.event.c_str();
            mv_value = eventKey.eventValue.c_str();
            mv_animation = eventKey.animation.c_str();
            mv_notrigger_in_scrubbing = eventKey.bNoTriggerInScrubbing;

            bAssigned = true;
        }
    }
    return bAssigned;
}

// Called when UI variable changes.
void CEventKeyUIControls::OnUIChange(IVariable* pVar, CTrackViewKeyBundle& selectedKeys)
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
        if (paramType == AnimParamType::Event)
        {
            IEventKey eventKey;
            keyHandle.GetKey(&eventKey);

            QByteArray event, value, animation;
            event = static_cast<QString>(mv_event).toUtf8();
            value = static_cast<QString>(mv_value).toUtf8();
            animation = static_cast<QString>(mv_animation).toUtf8();

            if (pVar == mv_event.GetVar())
            {
                eventKey.event = event.data();
            }
            if (pVar == mv_value.GetVar())
            {
                eventKey.eventValue = value.data();
            }
            if (pVar == mv_animation.GetVar())
            {
                eventKey.animation = animation.data();
            }
            SyncValue(mv_notrigger_in_scrubbing, eventKey.bNoTriggerInScrubbing, false, pVar);

            bool isDuringUndo = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsDuringUndoRedo);

            if (isDuringUndo)
            {
                keyHandle.SetKey(&eventKey);
            }
            else
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Set Key Value");
                keyHandle.SetKey(&eventKey);
                undoBatch.MarkEntityDirty(sequence->GetSequenceComponentEntityId());
            }
        }
    }
}
