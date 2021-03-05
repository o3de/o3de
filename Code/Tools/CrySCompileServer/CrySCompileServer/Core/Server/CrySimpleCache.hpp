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

#ifndef __CRYSIMPLECACHE__
#define __CRYSIMPLECACHE__

#include "CrySimpleMutex.hpp"

#include <Core/STLHelper.hpp>

#include <map>
#include <vector>
#include <list>

/*class CCrySimpleCacheEntry
{
    tdCache
public:
protected:
private:
};*/

typedef std::map<tdHash, tdHash>             tdEntries;
typedef std::map<tdHash, tdDataVector> tdData;

class CCrySimpleCache
{
    volatile bool                               m_CachingEnabled;
    int                                                 m_Hit;
    int                                                 m_Miss;
    tdEntries                                       m_Entries;
    tdData                                          m_Data;
    CCrySimpleMutex                         m_Mutex;
    CCrySimpleMutex                         m_FileMutex;

    std::list<tdDataVector*>        m_PendingCacheEntries;
    std::string                                 CreateFileName(const tdHash& rHash) const;

public:
    void                                                Init();
    bool                                                Find(const tdHash& rHash, tdDataVector& rData);
    void                                                Add(const tdHash& rHash, const tdDataVector& rData);

    bool                                                LoadCacheFile(const std::string& filename);
    void                                                Finalize();

    void                        ThreadFunc_SavePendingCacheEntries();

    static CCrySimpleCache&         Instance();


    std::list<tdDataVector*>&       PendingCacheEntries(){return m_PendingCacheEntries; }
    int                                                 Hit() const{return m_Hit; }
    int                                                 Miss() const{return m_Miss; }
    int                                                 EntryCount() const{return static_cast<int>(m_Entries.size()); }
};

#endif
