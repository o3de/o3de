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

#ifndef CRYINCLUDE_CRYMOVIE_SELECTTRACK_H
#define CRYINCLUDE_CRYMOVIE_SELECTTRACK_H

#pragma once

#include "IMovieSystem.h"
#include "AnimTrack.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/** Select track. Used to select Cameras on a Director's Camera Track
*/
class CSelectTrack
    : public TAnimTrack<ISelectKey>
{
public:
    AZ_CLASS_ALLOCATOR(CSelectTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CSelectTrack, "{D05D53BF-86D1-4D38-A3C6-4EFC09C16431}", IAnimTrack);

    AnimValueType GetValueType();

    void GetKeyInfo(int key, const char*& description, float& duration);
    void SerializeKey(ISelectKey& key, XmlNodeRef& keyNode, bool bLoading);

    static void Reflect(AZ::SerializeContext* serializeContext);
};

#endif // CRYINCLUDE_CRYMOVIE_SELECTTRACK_H
