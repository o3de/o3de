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
#include "SoundTrack.h"

//////////////////////////////////////////////////////////////////////////
void CSoundTrack::SerializeKey(ISoundKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* sTemp;
        sTemp = keyNode->getAttr("StartTrigger");
        key.sStartTrigger = sTemp;

        sTemp = keyNode->getAttr("StopTrigger");
        key.sStopTrigger = sTemp;

        float fDuration = 0.0f;

        if (keyNode->getAttr("Duration", fDuration))
        {
            key.fDuration = fDuration;
        }

        keyNode->getAttr("CustomColor", key.customColor);
    }
    else
    {
        keyNode->setAttr("StartTrigger", key.sStartTrigger.c_str());
        keyNode->setAttr("StopTrigger", key.sStopTrigger.c_str());
        keyNode->setAttr("Duration", key.fDuration);
        keyNode->setAttr("CustomColor", key.customColor);
    }
}

//////////////////////////////////////////////////////////////////////////
void CSoundTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = m_keys[key].fDuration;

    if (!m_keys[key].sStartTrigger.empty())
    {
        description = m_keys[key].sStartTrigger.c_str();
    }
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimTrack<ISoundKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TAnimTrack<ISoundKey> >()
        ->Version(2)
        ->Field("Flags", &TAnimTrack<ISoundKey>::m_flags)
        ->Field("Range", &TAnimTrack<ISoundKey>::m_timeRange)
        ->Field("ParamType", &TAnimTrack<ISoundKey>::m_nParamType)
        ->Field("Keys", &TAnimTrack<ISoundKey>::m_keys)
        ->Field("Id", &TAnimTrack<ISoundKey>::m_id);
}

//////////////////////////////////////////////////////////////////////////
void CSoundTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TAnimTrack<ISoundKey>::Reflect(serializeContext);

    serializeContext->Class<CSoundTrack, TAnimTrack<ISoundKey> >()
        ->Version(1);
}
