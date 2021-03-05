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

#include "LmbrCentral_precompiled.h"
#include "AnimationEventsAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType animationEventsType("{C1D209C1-F81A-4586-A34E-1615995F9F3F}");

    AnimationEventsAssetTypeInfo::~AnimationEventsAssetTypeInfo()
    {
        Unregister();
    }

    void AnimationEventsAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(animationEventsType);
    }

    void AnimationEventsAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(animationEventsType);
    }

    AZ::Data::AssetType AnimationEventsAssetTypeInfo::GetAssetType() const
    {
        return animationEventsType;
    }

    const char* AnimationEventsAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Animation Events";
    }

    const char* AnimationEventsAssetTypeInfo::GetGroup() const
    {
        return "Animation";
    }
} // namespace LmbrCentral
