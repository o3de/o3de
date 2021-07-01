/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

    static void Reflect(AZ::ReflectContext* context);
};

#endif // CRYINCLUDE_CRYMOVIE_SOUNDTRACK_H
