/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "EventTrack.h"

#include "TrackEventTrack.h"

namespace Maestro
{

    CEventTrack::CEventTrack()
        : CEventTrack(nullptr)
    {
    }

    CEventTrack::CEventTrack(IAnimStringTable* pStrings)
        : m_pStrings(pStrings)
    {
    }

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

    void CEventTrack::GetKeyInfo(int keyIndex, const char*& description, float& duration) const
    {
        description = 0;
        duration = 0;

        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        static char desc[128];
        azstrcpy(desc, AZ_ARRAY_SIZE(desc), m_keys[keyIndex].event.c_str());
        if (!m_keys[keyIndex].eventValue.empty())
        {
            azstrcat(desc, AZ_ARRAY_SIZE(desc), ", ");
            azstrcat(desc, AZ_ARRAY_SIZE(desc), m_keys[keyIndex].eventValue.c_str());
        }

        description = desc;
    }

    void CEventTrack::SetKey(int keyIndex, IKey* key)
    {

        if (keyIndex < 0 || keyIndex >= GetNumKeys())
        {
            AZ_Assert(false, "Key index (%d) is out of range (0 .. %d).", keyIndex, GetNumKeys());
            return;
        }

        IEventKey* pEvKey = static_cast<IEventKey*>(key);

        // Intern string values
        pEvKey->event = m_pStrings->Add(pEvKey->event.c_str());
        pEvKey->eventValue = m_pStrings->Add(pEvKey->eventValue.c_str());
        pEvKey->animation = m_pStrings->Add(pEvKey->animation.c_str());

        TAnimTrack<IEventKey>::SetKey(keyIndex, pEvKey);

        SortKeys();
    }

    void CEventTrack::InitPostLoad(IAnimSequence* sequence)
    {
        m_pStrings = sequence->GetTrackEventStringTable();
    }

    void CEventTrack::Reflect(AZ::ReflectContext* context)
    {
        // Note the template base class TAnimTrack<IEventKey>::Reflect() is reflected by CTrackEventTrack::Reflect()

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CEventTrack, TAnimTrack<IEventKey>>()->Version(1);
        }
    }

} // namespace Maestro
