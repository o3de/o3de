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

#ifndef CRYINCLUDE_CRYMOVIE_TRACKEVENTTRACK_H
#define CRYINCLUDE_CRYMOVIE_TRACKEVENTTRACK_H

#pragma once

#include "IMovieSystem.h"
#include "AnimTrack.h"
#include "AnimKey.h"

class CAnimStringTable
    : public IAnimStringTable
{
public:
    AZ_CLASS_ALLOCATOR(CAnimStringTable, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimStringTable, "{B7C435CF-A763-41B5-AA1E-3BA2CD4232B2}", IAnimStringTable);

    CAnimStringTable();
    ~CAnimStringTable();

    const char* Add(const char* p) override;

    //////////////////////////////////////////////////////////////////////////
    // for intrusive_ptr support
    void add_ref() override;
    void release() override;
    //////////////////////////////////////////////////////////////////////////

    static void Reflect([[maybe_unused]] AZ::SerializeContext* serializeContext) {}

private:
    struct Page
    {
        Page* pPrev;
        char mem[512 - sizeof(Page*)];
    };

    typedef std::unordered_map<const char*, const char*, stl::hash_string<const char*>, stl::equality_string<const char*> > TableMap;

private:
    CAnimStringTable(const CAnimStringTable&);
    CAnimStringTable& operator = (const CAnimStringTable&);

private:
    int m_refCount;
    Page* m_pLastPage;
    char* m_pEnd;
    TableMap m_table;
};

class CTrackEventTrack
    : public TAnimTrack<IEventKey>
{
public:
    AZ_CLASS_ALLOCATOR(CTrackEventTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CTrackEventTrack, "{3F659864-D66B-4211-93FB-1401EF4614D4}", IAnimTrack);

    explicit CTrackEventTrack(IAnimStringTable* pStrings);
    CTrackEventTrack();     // default constr needed for AZ Serialization

    //////////////////////////////////////////////////////////////////////////
    // Overrides of IAnimTrack.
    //////////////////////////////////////////////////////////////////////////
    void GetKeyInfo(int key, const char*& description, float& duration);
    void SerializeKey(IEventKey& key, XmlNodeRef& keyNode, bool bLoading);
    void SetKey(int index, IKey* key);
    void InitPostLoad(IAnimSequence* sequence) override;

    static void Reflect(AZ::SerializeContext* serializeContext);

private:
    AZStd::intrusive_ptr< IAnimStringTable> m_pStrings;
};

#endif // CRYINCLUDE_CRYMOVIE_TRACKEVENTTRACK_H
