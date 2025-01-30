/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMovieSystem.h>
#include "AnimTrack.h"

namespace Maestro
{

    /** CTimeRangesTrack contains keys that represent generic time ranges
     */
    class CTimeRangesTrack : public TAnimTrack<ITimeRangeKey>
    {
    public:
        AZ_CLASS_ALLOCATOR(CTimeRangesTrack, AZ::SystemAllocator);
        AZ_RTTI(CTimeRangesTrack, "{6BD2B893-7E42-47C7-92B3-5C58F8AE33F3}", IAnimTrack);

        // IAnimTrack.
        void GetKeyInfo(int key, const char*& description, float& duration) override;
        bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;
        void SerializeKey(ITimeRangeKey& key, XmlNodeRef& keyNode, bool bLoading) override;

        //! Gets the duration of an animation key.
        float GetKeyDuration(int key) const;

        int GetActiveKeyIndexForTime(const float time);

        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace Maestro
