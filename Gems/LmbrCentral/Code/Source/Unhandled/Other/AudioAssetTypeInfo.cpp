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
#include "AudioAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType audioAssetType("{692CB2C8-0CC3-41A1-88A5-AA09C6EC2AD8}");

    AudioAssetTypeInfo::~AudioAssetTypeInfo()
    {
        Unregister();
    }

    void AudioAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(audioAssetType);
    }

    void AudioAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(audioAssetType);
    }

    AZ::Data::AssetType AudioAssetTypeInfo::GetAssetType() const
    {
        return audioAssetType;
    }

    const char* AudioAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "Audio";
    }

    const char* AudioAssetTypeInfo::GetGroup() const
    {
        return "Other";
    }
} // namespace LmbrCentral
