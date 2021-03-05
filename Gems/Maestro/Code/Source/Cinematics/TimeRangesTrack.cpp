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

#include "Maestro_precompiled.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "TimeRangesTrack.h"

/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
bool CTimeRangesTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    return TAnimTrack<ITimeRangeKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
}

void CTimeRangesTrack::SerializeKey(ITimeRangeKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        key.m_duration = 0;
        key.m_endTime = 0;
        key.m_startTime = 0;
        key.m_speed = 1;
        keyNode->getAttr("length", key.m_duration);
        keyNode->getAttr("end", key.m_endTime);
        keyNode->getAttr("speed", key.m_speed);
        keyNode->getAttr("start", key.m_startTime);
        keyNode->getAttr("loop", key.m_bLoop);
    }
    else
    {
        if (key.m_duration > 0)
        {
            keyNode->setAttr("length", key.m_duration);
        }
        if (key.m_endTime > 0)
        {
            keyNode->setAttr("end", key.m_endTime);
        }
        if (key.m_speed != 1)
        {
            keyNode->setAttr("speed", key.m_speed);
        }
        if (key.m_startTime != 0)
        {
            keyNode->setAttr("start", key.m_startTime);
        }
        if (key.m_bLoop)
        {
            keyNode->setAttr("loop", key.m_bLoop);
        }
    }
}

float CTimeRangesTrack::GetKeyDuration(int key) const
{
    assert(key >= 0 && key < (int)m_keys.size());
    return m_keys[key].GetActualDuration();
}

void CTimeRangesTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    duration = GetKeyDuration(key);
    description = "";
}

int CTimeRangesTrack::GetActiveKeyIndexForTime(const float time)
{
    const unsigned int numKeys = m_keys.size();

    if (numKeys == 0 || m_keys[0].time > time)
    {
        return -1;
    }

    int lastFound = 0;

    for (unsigned int i = 0; i < numKeys; ++i)
    {
        ITimeRangeKey& key = m_keys[i];
        if (key.time > time)
        {
            break;
        }
        else if (key.time <= time)
        {
            lastFound = i;
        }
    }

    return lastFound;
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimTrack<ITimeRangeKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TAnimTrack<ITimeRangeKey> >()
        ->Version(2)
        ->Field("Flags", &TAnimTrack<ITimeRangeKey>::m_flags)
        ->Field("Range", &TAnimTrack<ITimeRangeKey>::m_timeRange)
        ->Field("ParamType", &TAnimTrack<ITimeRangeKey>::m_nParamType)
        ->Field("Keys", &TAnimTrack<ITimeRangeKey>::m_keys)
        ->Field("Id", &TAnimTrack<ITimeRangeKey>::m_id);
}

//////////////////////////////////////////////////////////////////////////
void CTimeRangesTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TAnimTrack<IBoolKey>::Reflect(serializeContext);

    serializeContext->Class<CTimeRangesTrack, TAnimTrack<ITimeRangeKey> >()
        ->Version(1);
}
