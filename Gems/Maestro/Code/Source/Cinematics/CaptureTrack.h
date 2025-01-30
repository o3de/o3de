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

    /** A track for capturing a movie from the engine rendering.
     */
    class CCaptureTrack : public TAnimTrack<ICaptureKey>
    {
    public:
        AZ_CLASS_ALLOCATOR(CCaptureTrack, AZ::SystemAllocator);
        AZ_RTTI(CCaptureTrack, "{72505F9F-C098-4435-9C95-79013C4DD70B}", IAnimTrack);

        void SerializeKey(ICaptureKey& key, XmlNodeRef& keyNode, bool bLoading) override;
        void GetKeyInfo(int key, const char*& description, float& duration) override;

        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace Maestro
