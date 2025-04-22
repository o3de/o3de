/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "EventNode.h"
#include "AnimTrack.h"
#include "TrackEventTrack.h"
#include "Maestro/Types/AnimNodeType.h"
#include "Maestro/Types/AnimValueType.h"
#include "Maestro/Types/AnimParamType.h"

#include <ISystem.h>

namespace Maestro
{

    CAnimEventNode::CAnimEventNode()
        : CAnimEventNode(0)
    {
    }

    CAnimEventNode::CAnimEventNode(const int id)
        : CAnimNode(id, AnimNodeType::Event)
    {
        SetFlags(GetFlags() | eAnimNodeFlags_CanChangeName);
        m_lastEventKey = -1;
    }

    void CAnimEventNode::CreateDefaultTracks()
    {
        CreateTrack(AnimParamType::TrackEvent);
    }

    unsigned int CAnimEventNode::GetParamCount() const
    {
        return 1;
    }

    CAnimParamType CAnimEventNode::GetParamType(unsigned int nIndex) const
    {
        if (nIndex == 0)
        {
            return AnimParamType::TrackEvent;
        }

        return AnimParamType::Invalid;
    }

    bool CAnimEventNode::GetParamInfoFromType(const CAnimParamType& animParamType, SParamInfo& info) const
    {
        if (animParamType.GetType() == AnimParamType::TrackEvent)
        {
            info.flags = IAnimNode::ESupportedParamFlags(0);
            info.name = "Track Event";
            info.paramType = AnimParamType::TrackEvent;
            info.valueType = AnimValueType::Unknown;
            return true;
        }
        return false;
    }

    void CAnimEventNode::Animate(SAnimContext& ec)
    {
        // Get track event
        int trackCount = NumTracks();
        for (int paramIndex = 0; paramIndex < trackCount; paramIndex++)
        {
            CAnimParamType trackType = m_tracks[paramIndex]->GetParameterType();
            IAnimTrack* pTrack = m_tracks[paramIndex].get();

            if (pTrack && pTrack->GetFlags() & IAnimTrack::eAnimTrackFlags_Disabled)
            {
                continue;
            }

            // Check for fire
            if (CTrackEventTrack* pEventTrack = (CTrackEventTrack*)pTrack)
            {
                IEventKey key;
                int nEventKey = pEventTrack->GetActiveKey(ec.time, &key);
                if (nEventKey != m_lastEventKey && nEventKey >= 0)
                {
                    bool bKeyAfterStartTime = key.time >= ec.startTime;
                    if (bKeyAfterStartTime)
                    {
                        ec.sequence->TriggerTrackEvent(key.event.c_str(), key.eventValue.c_str());
                    }
                }
                m_lastEventKey = nEventKey;
            }
        }
    }

    void CAnimEventNode::OnReset()
    {
        m_lastEventKey = -1;
    }

    void CAnimEventNode::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CAnimEventNode, CAnimNode>()->Version(1);
        }
    }

} // namespace Maestro
