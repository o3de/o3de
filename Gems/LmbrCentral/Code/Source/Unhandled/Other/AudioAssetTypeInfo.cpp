/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
