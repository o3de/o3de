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

#include "EditorDefs.h"

#include "TrackViewEventNode.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimParamType.h>


//////////////////////////////////////////////////////////////////////////
CTrackViewEventNode::CTrackViewEventNode(IAnimSequence* pSequence, IAnimNode* pAnimNode, CTrackViewNode* pParentNode)
    : CTrackViewAnimNode(pSequence, pAnimNode, pParentNode)
{
    if (GetAnimNode() && GetAnimNode()->GetSequence())
    {
        GetAnimNode()->GetSequence()->AddTrackEventListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
CTrackViewEventNode::~CTrackViewEventNode()
{
    if (GetAnimNode() && GetAnimNode()->GetSequence())
    {
        GetAnimNode()->GetSequence()->RemoveTrackEventListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewEventNode::OnTrackEvent([[maybe_unused]] IAnimSequence* pSequence, int reason, const char* event, void* pUserData)
{
    ITrackEventListener::ETrackEventReason  eReason = static_cast<ITrackEventListener::ETrackEventReason>(reason);
    switch (eReason)
    {
    case ITrackEventListener::eTrackEventReason_Renamed:
        RenameTrackEvent(event, static_cast<const char*>(pUserData));
        break;
    case ITrackEventListener::eTrackEventReason_Removed:
        RemoveTrackEvent(event);
    default:
        // do nothing for events we're not interested in
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewEventNode::RenameTrackEvent(const char* fromName, const char* toName)
{
    CTrackViewTrackBundle eventTracks = GetTracksByParam(AnimParamType::TrackEvent);
    const uint numEventTracks = eventTracks.GetCount();

    for (uint i = 0; i < numEventTracks; ++i)
    {
        CTrackViewTrack* eventTrack = eventTracks.GetTrack(i);
        if (eventTrack)
        {
            // Go through all keys searching for match to the fromName and re-set these keys to use the toName
            CTrackViewKeyBundle allKeys = eventTrack->GetAllKeys();
            const uint numKeys = allKeys.GetKeyCount();
            for (uint k = 0; k < numKeys; ++k)
            {
                CTrackViewKeyHandle keyHandle = allKeys.GetKey(k);
                IEventKey eventKey;

                keyHandle.GetKey(&eventKey);
                if (strcmp(eventKey.event.c_str(), fromName) == 0)
                {
                    // we have a match - re-set the eventKey with the toName
                    eventKey.event = toName;
                    keyHandle.SetKey(&eventKey);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewEventNode::RemoveTrackEvent(const char* removedEventName)
{
    // rename the removedEventName keys to the empty string, which represents an unset event key
    RenameTrackEvent(removedEventName, "");
}
