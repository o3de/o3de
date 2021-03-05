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
#include "EventTrack.h"

#include "TrackEventTrack.h"

CEventTrack::CEventTrack()
    : CEventTrack(nullptr)
{
}

CEventTrack::CEventTrack(IAnimStringTable* pStrings)
    : m_pStrings(pStrings)
{
}

//////////////////////////////////////////////////////////////////////////
void CEventTrack::SerializeKey(IEventKey& key, XmlNodeRef& keyNode, bool bLoading)
{
    if (bLoading)
    {
        const char* str;
        str = keyNode->getAttr("event");
        key.event = m_pStrings->Add(str);

        str = keyNode->getAttr("eventValue");
        key.eventValue = m_pStrings->Add(str);

        str = keyNode->getAttr("anim");
        key.animation = m_pStrings->Add(str);

        key.duration = 0;
        keyNode->getAttr("length", key.duration);
    }
    else
    {
        if (strlen(key.event.c_str()) > 0)
        {
            keyNode->setAttr("event", key.event.c_str());
        }
        if (strlen(key.eventValue.c_str()) > 0)
        {
            keyNode->setAttr("eventValue", key.eventValue.c_str());
        }
        if (strlen(key.animation.c_str()) > 0)
        {
            keyNode->setAttr("anim", key.animation.c_str());
        }
        if (key.duration > 0)
        {
            keyNode->setAttr("length", key.duration);
        }
    }
}

void CEventTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    static char desc[128];

    assert(key >= 0 && key < (int)m_keys.size());
    CheckValid();
    description = 0;
    duration = 0;
    cry_strcpy(desc, m_keys[key].event.c_str());
    if (!m_keys[key].eventValue.empty())
    {
        cry_strcat(desc, ", ");
        cry_strcat(desc, m_keys[key].eventValue.c_str());
    }

    description = desc;
}

void CEventTrack::SetKey(int index, IKey* key)
{
    IEventKey* pEvKey = static_cast<IEventKey*>(key);

    // Intern string values
    pEvKey->event = m_pStrings->Add(pEvKey->event.c_str());
    pEvKey->eventValue = m_pStrings->Add(pEvKey->eventValue.c_str());
    pEvKey->animation = m_pStrings->Add(pEvKey->animation.c_str());

    TAnimTrack<IEventKey>::SetKey(index, pEvKey);
}

//////////////////////////////////////////////////////////////////////////
void CEventTrack::InitPostLoad(IAnimSequence* sequence)
{
    m_pStrings = sequence->GetTrackEventStringTable();
}

//////////////////////////////////////////////////////////////////////////
void CEventTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    // Note the template base class TAnimTrack<IEventKey>::Reflect() is reflected by CTrackEventTrack::Reflect()

    serializeContext->Class<CEventTrack, TAnimTrack<IEventKey> >()
        ->Version(1);
}