/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UiCanvasEditor_precompiled.h"
#include "UiAnimViewEventNode.h"
#include "UiAnimViewSequence.h"

//////////////////////////////////////////////////////////////////////////
CUiAnimViewEventNode::CUiAnimViewEventNode(IUiAnimSequence* pSequence, IUiAnimNode* pAnimNode, CUiAnimViewNode* pParentNode)
    : CUiAnimViewAnimNode(pSequence, pAnimNode, pParentNode)
{
    if (GetAnimNode() && GetAnimNode()->GetSequence())
    {
        GetAnimNode()->GetSequence()->AddTrackEventListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewEventNode::~CUiAnimViewEventNode()
{
    if (GetAnimNode() && GetAnimNode()->GetSequence())
    {
        GetAnimNode()->GetSequence()->RemoveTrackEventListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewEventNode::OnTrackEvent([[maybe_unused]] IUiAnimSequence* pSequence, int reason, const char* event, void* pUserData)
{
    IUiTrackEventListener::ETrackEventReason  eReason = static_cast<IUiTrackEventListener::ETrackEventReason>(reason);
    switch (eReason)
    {
    case IUiTrackEventListener::eTrackEventReason_Renamed:
        RenameTrackEvent(event, static_cast<const char*>(pUserData));
        break;
    case IUiTrackEventListener::eTrackEventReason_Removed:
        RemoveTrackEvent(event);
    default:
        // do nothing for events we're not interested in
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
CUiAnimViewKeyBundle CUiAnimViewEventNode::GetTrackEventKeys(CUiAnimViewAnimNode* pAnimNode, const char* eventName)
{
    CUiAnimViewKeyBundle foundKeys;

    CUiAnimViewTrackBundle eventTracks = pAnimNode->GetTracksByParam(eUiAnimParamType_TrackEvent);
    const uint numEventTracks = eventTracks.GetCount();

    for (uint trackIndex = 0; trackIndex < numEventTracks; ++trackIndex)
    {
        CUiAnimViewTrack* eventTrack = eventTracks.GetTrack(trackIndex);
        if (eventTrack)
        {
            // Go through all keys searching for match to the eventKey
            CUiAnimViewKeyBundle allKeys = eventTrack->GetAllKeys();
            const uint numKeys = allKeys.GetKeyCount();
            for (uint keyIndex = 0; keyIndex < numKeys; ++keyIndex)
            {
                CUiAnimViewKeyHandle keyHandle = allKeys.GetKey(keyIndex);
                IEventKey eventKey;

                keyHandle.GetKey(&eventKey);
                if (strcmp(eventKey.event.c_str(), eventName) == 0)
                {
                    // we have a match - add to KeyBundle
                    foundKeys.AppendKey(keyHandle);
                }
            }
        }
    }

    return foundKeys;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewEventNode::RenameTrackEvent(const char* fromName, const char* toName)
{
    CUiAnimViewKeyBundle keysToRename(GetTrackEventKeys(this, fromName));

    const uint numKeys = keysToRename.GetKeyCount();
    for (uint keyIndex = 0; keyIndex < numKeys; ++keyIndex)
    {
        CUiAnimViewKeyHandle keyHandle = keysToRename.GetKey(keyIndex);
        IEventKey eventKey;

        keyHandle.GetKey(&eventKey);
        // re-set the eventKey with the toName
        eventKey.event = toName;
        keyHandle.SetKey(&eventKey);
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimViewEventNode::RemoveTrackEvent(const char* removedEventName)
{
    // rename the removedEventName keys to the empty string, which represents an unset event key
    RenameTrackEvent(removedEventName, "");
}
