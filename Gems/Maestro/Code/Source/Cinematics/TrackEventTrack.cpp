/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TrackEventTrack.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace Maestro
{

    CAnimStringTable::CAnimStringTable()
        : m_refCount(0)
    {
        m_pLastPage = new Page;
        m_pLastPage->pPrev = nullptr;
        m_pEnd = m_pLastPage->mem;
    }

    CAnimStringTable::~CAnimStringTable()
    {
        for (Page *p = m_pLastPage, *pp; p; p = pp)
        {
            pp = p->pPrev;
            delete p;
        }
    }

    void CAnimStringTable::add_ref()
    {
        ++m_refCount;
    }

    void CAnimStringTable::release()
    {
        if (--m_refCount <= 0)
        {
            delete this;
        }
    }

    const char* CAnimStringTable::Add(const char* p)
    {
        TableMap::iterator it = m_table.find(p);
        if (it != m_table.end())
        {
            return it->second;
        }

        char* pPageEnd = &m_pLastPage->mem[sizeof(m_pLastPage->mem)];
        size_t nAvailable = pPageEnd - m_pEnd;
        const size_t nLen = strlen(p);

        if (nLen >= sizeof(m_pLastPage->mem))
        {
            AZ_Fatal("CAnimStringTable", "String table can't accommodate string");
        }

        if (nAvailable <= nLen)
        {
            // Not enough room. Allocate another page.
            Page* pPage = new Page;
            pPage->pPrev = m_pLastPage;
            m_pLastPage = pPage;

            m_pEnd = pPage->mem;

            pPageEnd = &m_pLastPage->mem[sizeof(m_pLastPage->mem)];
            nAvailable = pPageEnd - m_pEnd;
        }

        char* pRet = m_pEnd;
        m_pEnd += nLen + 1;

        azstrcpy(pRet, nAvailable, p);

        m_table.insert(AZStd::pair<const char*, const char*>(pRet, pRet));
        return pRet;
    }

    void CTrackEventTrack::SerializeKey(IEventKey& key, XmlNodeRef& keyNode, bool bLoading)
    {
        if (bLoading)
        {
            const char* str;
            str = keyNode->getAttr("event");
            key.event = m_pStrings->Add(str);

            str = keyNode->getAttr("eventValue");
            key.eventValue = m_pStrings->Add(str);
        }
        else
        {
            if (!key.event.empty())
            {
                keyNode->setAttr("event", key.event.c_str());
            }
            if (!key.eventValue.empty())
            {
                keyNode->setAttr("eventValue", key.eventValue.c_str());
            }
        }
    }

    void CTrackEventTrack::SetKey(int index, IKey* key)
    {
        IEventKey* pEvKey = static_cast<IEventKey*>(key);

        // Intern string values
        pEvKey->event = m_pStrings->Add(pEvKey->event.c_str());
        pEvKey->eventValue = m_pStrings->Add(pEvKey->eventValue.c_str());
        pEvKey->animation = m_pStrings->Add(pEvKey->animation.c_str());

        TAnimTrack<IEventKey>::SetKey(index, pEvKey);
    }

    void CTrackEventTrack::InitPostLoad(IAnimSequence* sequence)
    {
        m_pStrings = sequence->GetTrackEventStringTable();
    }

    CTrackEventTrack::CTrackEventTrack(IAnimStringTable* pStrings)
        : m_pStrings(pStrings)
    {
    }

    CTrackEventTrack::CTrackEventTrack()
        : CTrackEventTrack(nullptr)
    {
    }

    void CTrackEventTrack::GetKeyInfo(int key, const char*& description, float& duration)
    {
        static char desc[128];

        AZ_Assert(key >= 0 && key < (int)m_keys.size(), "Key index %i is out of range", key);
        CheckValid();
        description = 0;
        duration = 0;
        azstrcpy(desc, AZ_ARRAY_SIZE(desc), m_keys[key].event.c_str());
        if (!m_keys[key].eventValue.empty())
        {
            azstrcat(desc, AZ_ARRAY_SIZE(desc), ", ");
            azstrcat(desc, AZ_ARRAY_SIZE(desc), m_keys[key].eventValue.c_str());
        }

        description = desc;
    }

    static bool EventTrackVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() < 3)
        {
            rootElement.AddElement(serializeContext, "BaseClass1", azrtti_typeid<IAnimTrack>());
        }

        return true;
    }

    template<>
    inline void TAnimTrack<IEventKey>::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TAnimTrack<IEventKey>, IAnimTrack>()
                ->Version(3, &EventTrackVersionConverter)
                ->Field("Flags", &TAnimTrack<IEventKey>::m_flags)
                ->Field("Range", &TAnimTrack<IEventKey>::m_timeRange)
                ->Field("ParamType", &TAnimTrack<IEventKey>::m_nParamType)
                ->Field("Keys", &TAnimTrack<IEventKey>::m_keys)
                ->Field("Id", &TAnimTrack<IEventKey>::m_id);
        }
    }

    void CTrackEventTrack::Reflect(AZ::ReflectContext* context)
    {
        TAnimTrack<IEventKey>::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CTrackEventTrack, TAnimTrack<IEventKey>>()->Version(1);
        }
    }

} // namespace Maestro
