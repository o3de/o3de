/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "TrackEventTrack.h"

#include <AzCore/Serialization/SerializeContext.h>

CUiAnimStringTable::CUiAnimStringTable()
    : m_refCount(0)
{
    m_pLastPage = new Page;
    m_pLastPage->pPrev = NULL;
    m_pEnd = m_pLastPage->mem;
}

CUiAnimStringTable::~CUiAnimStringTable()
{
    for (Page* p = m_pLastPage, * pp; p; p = pp)
    {
        pp = p->pPrev;
        delete p;
    }
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimStringTable::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
void CUiAnimStringTable::release()
{
    if (--m_refCount <= 0)
    {
        delete this;
    }
}

const char* CUiAnimStringTable::Add(const char* p)
{
    TableMap::iterator it = m_table.find(p);
    if (it != m_table.end())
    {
        return it->second;
    }

    char* pPageEnd = &m_pLastPage->mem[sizeof(m_pLastPage->mem)];
    size_t nAvailable = pPageEnd - m_pEnd;
    size_t nLen = strlen(p);

    if (nLen >= sizeof(m_pLastPage->mem))
    {
        CryFatalError("String table can't accomodate string");
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

    m_table.insert(std::make_pair(pRet, pRet));
    return pRet;
}

//////////////////////////////////////////////////////////////////////////
void CUiTrackEventTrack::SerializeKey(IEventKey& key, XmlNodeRef& keyNode, bool bLoading)
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

void CUiTrackEventTrack::SetKey(int index, IKey* key)
{
    IEventKey* pEvKey = static_cast<IEventKey*>(key);

    // Intern string values
    pEvKey->event = m_pStrings->Add(pEvKey->event.c_str());
    pEvKey->eventValue = m_pStrings->Add(pEvKey->eventValue.c_str());
    pEvKey->animation = m_pStrings->Add(pEvKey->animation.c_str());

    TUiAnimTrack<IEventKey>::SetKey(index, pEvKey);
}

//////////////////////////////////////////////////////////////////////////
void CUiTrackEventTrack::InitPostLoad(IUiAnimSequence* sequence)
{
    m_pStrings = sequence->GetTrackEventStringTable();
}

CUiTrackEventTrack::CUiTrackEventTrack(IUiAnimStringTable* pStrings)
    : m_pStrings(pStrings)
{
}

CUiTrackEventTrack::CUiTrackEventTrack()
    : CUiTrackEventTrack(nullptr)
{
}

void CUiTrackEventTrack::GetKeyInfo(int key, const char*& description, float& duration)
{
    static char desc[128];

    assert(key >= 0 && key < (int)m_keys.size());
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

//////////////////////////////////////////////////////////////////////////
template<>
inline void TUiAnimTrack<IEventKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TUiAnimTrack<IEventKey> >()
        ->Version(2)
        ->Field("Flags", &TUiAnimTrack<IEventKey>::m_flags)
        ->Field("Range", &TUiAnimTrack<IEventKey>::m_timeRange)
        ->Field("ParamType", &TUiAnimTrack<IEventKey>::m_nParamType)
        ->Field("Keys", &TUiAnimTrack<IEventKey>::m_keys);
}

//////////////////////////////////////////////////////////////////////////
void CUiTrackEventTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TUiAnimTrack<IEventKey>::Reflect(serializeContext);

    serializeContext->Class<CUiTrackEventTrack, TUiAnimTrack<IEventKey> >()
        ->Version(1);
}
