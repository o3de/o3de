/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <LyShine/Animation/IUiAnimation.h>
#include "AnimTrack.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/** Boolean track, every key on this track negates boolean value.
*/
class UiBoolTrack
    : public TUiAnimTrack<IBoolKey>
{
public:
    AZ_CLASS_ALLOCATOR(UiBoolTrack, AZ::SystemAllocator)
    AZ_RTTI(UiBoolTrack, "{F0EDB82F-B3D7-47FC-AA97-91358A7F1168}", IUiAnimTrack);

    UiBoolTrack();

    virtual EUiAnimValue GetValueType() { return eUiAnimValue_Bool; };


    virtual void GetValue(float time, bool& value);
    virtual void SetValue(float time, const bool& value, bool bDefault = false);

    void SerializeKey([[maybe_unused]] IBoolKey& key, [[maybe_unused]] XmlNodeRef& keyNode, [[maybe_unused]] bool bLoading) {};
    void GetKeyInfo(int key, const char*& description, float& duration);

    void SetDefaultValue(const bool bDefaultValue);

    static void Reflect(AZ::SerializeContext* serializeContext);

private:
    bool m_bDefaultValue;
};
