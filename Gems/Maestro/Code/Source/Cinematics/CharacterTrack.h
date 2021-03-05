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

#ifndef CRYINCLUDE_CRYMOVIE_CHARACTERTRACK_H
#define CRYINCLUDE_CRYMOVIE_CHARACTERTRACK_H

#pragma once

//forward declarations.
#include "IMovieSystem.h"
#include "AnimTrack.h"

/** CCharacterTrack contains entity keys, when time reach event key, it fires script event or start animation etc...
*/
class CCharacterTrack
    : public TAnimTrack<ICharacterKey>
{
public:
    AZ_CLASS_ALLOCATOR(CCharacterTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CCharacterTrack, "{3F701860-78BC-451A-B1DD-90F75DB9A7A2}", IAnimTrack);

    CCharacterTrack()
        : m_iAnimationLayer(-1) {}

    //////////////////////////////////////////////////////////////////////////
    // Overrides of IAnimTrack.
    //////////////////////////////////////////////////////////////////////////
    bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    void GetKeyInfo(int key, const char*& description, float& duration);
    void SerializeKey(ICharacterKey& key, XmlNodeRef& keyNode, bool bLoading);

    //! Gets the duration of an animation key. If it's a looped animation,
    //! a special consideration is required to compute the actual duration.
    float GetKeyDuration(int key) const;

    int GetAnimationLayerIndex() const { return m_iAnimationLayer; }
    void SetAnimationLayerIndex(int index) { m_iAnimationLayer = index; }

    virtual AnimValueType GetValueType();

    float GetEndTime() const { return m_timeRange.end; }

    static void Reflect(AZ::SerializeContext* serializeContext);

private:
    int m_iAnimationLayer;
};

#endif // CRYINCLUDE_CRYMOVIE_CHARACTERTRACK_H
