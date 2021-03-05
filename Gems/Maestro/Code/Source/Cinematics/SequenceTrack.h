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

    static void Reflect(AZ::SerializeContext* serializeContext);
};

#endif // CRYINCLUDE_CRYMOVIE_SEQUENCETRACK_H
