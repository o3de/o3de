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
    AZ_CLASS_ALLOCATOR(UiBoolTrack, AZ::SystemAllocator, 0)
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
