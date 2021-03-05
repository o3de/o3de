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
#include "LookAtTrack.h"

//////////////////////////////////////////////////////////////////////////
/// @deprecated Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
bool CLookAtTrack::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
        xmlNode->getAttr("AnimationLayer", m_iAnimationLayer);
    }
    else
    {
        xmlNode->setAttr("AnimationLayer", m_iAnimationLayer);
    }

    return TAnimTrack<ILookAtKey>::Serialize(xmlNode, bLoading, bLoadEmptyTracks);
}

void CLookAtTrack::SerializeKey(ILookAtKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* szSelection;
        f32 smoothTime;
        if (!keyNode->getAttr("smoothTime", smoothTime))
        {
            smoothTime = 0.0f;
        }

        const char* lookPose  = keyNode->getAttr("lookPose");
        if (lookPose)
        {
            key.lookPose = lookPose;
        }

        szSelection = keyNode->getAttr("node");
        if (szSelection)
        {
            key.szSelection = szSelection;
        }

        key.smoothTime = smoothTime;
    }
    else
    {
        keyNode->setAttr("node", key.szSelection.c_str());
        keyNode->setAttr("smoothTime", key.smoothTime);
        keyNode->setAttr("lookPose", key.lookPose.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
void CLookAtTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = m_keys[key].fDuration;
    if (!m_keys[key].szSelection.empty())
    {
        description = m_keys[key].szSelection.c_str();
    }
}


//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimTrack<ILookAtKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TAnimTrack<ILookAtKey> >()
        ->Version(2)
        ->Field("Flags", &TAnimTrack<ILookAtKey>::m_flags)
        ->Field("Range", &TAnimTrack<ILookAtKey>::m_timeRange)
        ->Field("ParamType", &TAnimTrack<ILookAtKey>::m_nParamType)
        ->Field("Keys", &TAnimTrack<ILookAtKey>::m_keys)
        ->Field("Id", &TAnimTrack<ILookAtKey>::m_id);
}

//////////////////////////////////////////////////////////////////////////
void CLookAtTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TAnimTrack<ILookAtKey>::Reflect(serializeContext);

    serializeContext->Class<CLookAtTrack, TAnimTrack<ILookAtKey> >()
        ->Version(1)
        ->Field("AnimationLayer", &CLookAtTrack::m_iAnimationLayer);
}