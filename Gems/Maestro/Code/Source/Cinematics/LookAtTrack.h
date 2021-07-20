/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_LOOKATTRACK_H
#define CRYINCLUDE_CRYMOVIE_LOOKATTRACK_H

#pragma once

#include "IMovieSystem.h"
#include "AnimTrack.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/** Look at target track, keys represent new lookat targets for entity.
*/
class CLookAtTrack
    : public TAnimTrack<ILookAtKey>
{
public:
    AZ_CLASS_ALLOCATOR(CLookAtTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CLookAtTrack, "{30A5C53C-F158-4CCE-A7A0-1A902D13B91C}", IAnimTrack);

    CLookAtTrack()
        : m_iAnimationLayer(-1) {}

    bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    void GetKeyInfo(int key, const char*& description, float& duration);
    void SerializeKey(ILookAtKey& key, XmlNodeRef& keyNode, bool bLoading);

    int GetAnimationLayerIndex() const { return m_iAnimationLayer; }
    void SetAnimationLayerIndex(int index) { m_iAnimationLayer = index; }

    static void Reflect(AZ::ReflectContext* context);
private:
    int m_iAnimationLayer;
};

#endif // CRYINCLUDE_CRYMOVIE_LOOKATTRACK_H
