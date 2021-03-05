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
#include "TrackEventTrack.h"

#include <AzCore/Serialization/SerializeContext.h>

CAnimStringTable::CAnimStringTable()
    : m_refCount(0)
{
    m_pLastPage = new Page;
    m_pLastPage->pPrev = NULL;
    m_pEnd = m_pLastPage->mem;
}

CAnimStringTable::~CAnimStringTable()
{
    for (Page* p = m_pLastPage, * pp; p; p = pp)
    {
        pp = p->pPrev;
        delete p;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAnimStringTable::add_ref()
{
    ++m_refCount;
}

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimTrack<IEventKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TAnimTrack<IEventKey> >()
        ->Version(2)
        ->Field("Flags", &TAnimTrack<IEventKey>::m_flags)
        ->Field("Range", &TAnimTrack<IEventKey>::m_timeRange)
        ->Field("ParamType", &TAnimTrack<IEventKey>::m_nParamType)
        ->Field("Keys", &TAnimTrack<IEventKey>::m_keys)
        ->Field("Id", &TAnimTrack<IEventKey>::m_id);
}

//////////////////////////////////////////////////////////////////////////
void CTrackEventTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TAnimTrack<IEventKey>::Reflect(serializeContext);

    serializeContext->Class<CTrackEventTrack, TAnimTrack<IEventKey> >()
        ->Version(1);
}
