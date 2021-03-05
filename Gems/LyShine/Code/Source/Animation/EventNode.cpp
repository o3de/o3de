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

#include "LyShine_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "EventNode.h"
#include "AnimTrack.h"
#include "TrackEventTrack.h"

#include <ISystem.h>

//////////////////////////////////////////////////////////////////////////
CUiAnimEventNode::CUiAnimEventNode()
    : CUiAnimEventNode(0)
{
}

//////////////////////////////////////////////////////////////////////////
CUiAnimEventNode::CUiAnimEventNode(const int id)
    : CUiAnimNode(id, eUiAnimNodeType_Event)
{
    SetFlags(GetFlags() | eUiAnimNodeFlags_CanChangeName);
    m_lastEventKey = -1;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimEventNode::CreateDefaultTracks()
{
    CreateTrack(eUiAnimParamType_TrackEvent);
}

//////////////////////////////////////////////////////////////////////////
unsigned int CUiAnimEventNode::GetParamCount() const
{
    return 1;
}

//////////////////////////////////////////////////////////////////////////
CUiAnimParamType CUiAnimEventNode::GetParamType(unsigned int nIndex) const
{
    if (nIndex == 0)
    {
        return eUiAnimParamType_TrackEvent;
    }

    return eUiAnimParamType_Invalid;
}

//////////////////////////////////////////////////////////////////////////
bool CUiAnimEventNode::GetParamInfoFromType(const CUiAnimParamType& animParamType, SParamInfo& info) const
{
    if (animParamType.GetType() == eUiAnimParamType_TrackEvent)
    {
        info.flags = IUiAnimNode::ESupportedParamFlags(0);
        info.name = "Track Event";
        info.paramType = eUiAnimParamType_TrackEvent;
        info.valueType = eUiAnimValue_Unknown;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimEventNode::Animate(SUiAnimContext& ec)
{
    // Get track event
    int trackCount = NumTracks();
    for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
    {
        IUiAnimTrack* track = m_tracks[paramIndex].get();

        if (track && track->GetFlags() & IUiAnimTrack::eUiAnimTrackFlags_Disabled)
        {
            continue;
        }

        // Check for fire
        if (CUiTrackEventTrack* pEventTrack = (CUiTrackEventTrack*)track)
        {
            IEventKey key;
            int nEventKey = pEventTrack->GetActiveKey(ec.time, &key);
            if (nEventKey != m_lastEventKey && nEventKey >= 0)
            {
                bool bKeyAfterStartTime = key.time >= ec.startTime;
                if (bKeyAfterStartTime)
                {
                    ec.pSequence->TriggerTrackEvent(key.event.c_str(), key.eventValue.c_str());
                }
            }
            m_lastEventKey = nEventKey;
        }
    }
}

void CUiAnimEventNode::OnReset()
{
    m_lastEventKey = -1;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimEventNode::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<CUiAnimEventNode, CUiAnimNode>()
        ->Version(1);
}