/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_GOTOTRACK_H
#define CRYINCLUDE_CRYMOVIE_GOTOTRACK_H

#pragma once

#include "IMovieSystem.h"
#include "AnimTrack.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/** Goto track, every key on this track negates boolean value.
*/
class CGotoTrack
    : public TAnimTrack<IDiscreteFloatKey>
{
public:
    AZ_CLASS_ALLOCATOR(CGotoTrack, AZ::SystemAllocator, 0);
    AZ_RTTI(CGotoTrack, "{B9A6BD22-F669-4D84-AD1D-B7BD07165C5D}", IAnimTrack);

    CGotoTrack();

    virtual AnimValueType GetValueType();

    void GetValue(float time, float& value, bool applyMultiplier=false);
    void SetValue(float time, const float& value, bool bDefault = false, bool applyMultiplier = false);

    void SerializeKey(IDiscreteFloatKey& key, XmlNodeRef& keyNode, bool bLoading);
    void GetKeyInfo(int key, const char*& description, float& duration);

    static void Reflect(AZ::ReflectContext* context);

protected:
    void SetKeyAtTime(float time, IKey* key);

    float m_DefaultValue;
};

#endif // CRYINCLUDE_CRYMOVIE_GOTOTRACK_H
