/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_SEQUENCETRACK_H
#define CRYINCLUDE_CRYMOVIE_SEQUENCETRACK_H

#pragma once

#include "IMovieSystem.h"
#include "AnimTrack.h"

class CSequenceTrack
    : public TAnimTrack<ISequenceKey>
{
public:
    AZ_CLASS_ALLOCATOR(CSequenceTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CSequenceTrack, "{5801883A-5289-4FA1-BECE-9EF02C1D62F5}", IAnimTrack);

    void GetKeyInfo(int key, const char*& description, float& duration);
    void SerializeKey(ISequenceKey& key, XmlNodeRef& keyNode, bool bLoading);

    static void Reflect(AZ::ReflectContext* context);
};

#endif // CRYINCLUDE_CRYMOVIE_SEQUENCETRACK_H
