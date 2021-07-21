/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_CAPTURETRACK_H
#define CRYINCLUDE_CRYMOVIE_CAPTURETRACK_H

#pragma once

#include "IMovieSystem.h"
#include "AnimTrack.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/** A track for capturing a movie from the engine rendering.
*/
class CCaptureTrack
    : public TAnimTrack<ICaptureKey>
{
public:
    AZ_CLASS_ALLOCATOR(CCaptureTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CCaptureTrack, "{72505F9F-C098-4435-9C95-79013C4DD70B}", IAnimTrack);

    void SerializeKey(ICaptureKey& key, XmlNodeRef& keyNode, bool bLoading);
    void GetKeyInfo(int key, const char*& description, float& duration);

    static void Reflect(AZ::ReflectContext* context);
};

#endif // CRYINCLUDE_CRYMOVIE_CAPTURETRACK_H
