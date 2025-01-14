/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AnimTrack.h"

namespace Maestro
{

    struct SSoundInfo
    {
        void Reset()
        {
            nSoundKeyStart = -1;
            nSoundKeyStop = -1;
        }

        int nSoundKeyStart = -1;
        int nSoundKeyStop = -1;
    };

    class CSoundTrack : public TAnimTrack<ISoundKey>
    {
    public:
        AZ_CLASS_ALLOCATOR(CSoundTrack, AZ::SystemAllocator);
        AZ_RTTI(CSoundTrack, "{B87D8805-F583-4154-B554-45518BC487F4}", IAnimTrack);

        void GetKeyInfo(int key, const char*& description, float& duration) override;
        void SerializeKey(ISoundKey& key, XmlNodeRef& keyNode, bool bLoading) override;

        //! Check if track is masked
        bool IsMasked(const uint32 mask) const override
        {
            return (mask & eTrackMask_MaskSound) != 0;
        }

        bool UsesMute() const override
        {
            return true;
        }

        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace Maestro
