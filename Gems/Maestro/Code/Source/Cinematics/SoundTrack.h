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

#ifndef CRYINCLUDE_CRYMOVIE_SOUNDTRACK_H
#define CRYINCLUDE_CRYMOVIE_SOUNDTRACK_H

#include "AnimTrack.h"

struct SSoundInfo
{
    SSoundInfo()
        : nSoundKeyStart(-1)
        , nSoundKeyStop(-1)
    {
        Reset();
    }

    void Reset()
    {
        nSoundKeyStart = -1;
        nSoundKeyStop = -1;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    int nSoundKeyStart;
    int nSoundKeyStop;
};

class CSoundTrack
    : public TAnimTrack<ISoundKey>
{
public:
    AZ_CLASS_ALLOCATOR(CSoundTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CSoundTrack, "{B87D8805-F583-4154-B554-45518BC487F4}", IAnimTrack);

    void GetKeyInfo(int key, const char*& description, float& duration);
    void SerializeKey(ISoundKey& key, XmlNodeRef& keyNode, bool bLoading);

    //! Check if track is masked
    virtual bool IsMasked(const uint32 mask) const { return (mask & eTrackMask_MaskSound) != 0; }

    bool UsesMute() const override { return true; }

    static void Reflect(AZ::SerializeContext* serializeContext);
};

#endif // CRYINCLUDE_CRYMOVIE_SOUNDTRACK_H
