/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MiniAudioPlaybackComponentConfig.h"

#include <AzCore/Asset/AssetSerializer.h>

namespace MiniAudio
{
    void MiniAudioPlaybackComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MiniAudioPlaybackComponentConfig>()
                ->Version(4)
                ->Field("Autoplay", &MiniAudioPlaybackComponentConfig::m_autoplayOnActivate)
                ->Field("Sound", &MiniAudioPlaybackComponentConfig::m_sound)
                ->Field("Volume", &MiniAudioPlaybackComponentConfig::m_volume)
                ->Field("Auto-follow", &MiniAudioPlaybackComponentConfig::m_autoFollowEntity)
                ->Field("Loop", &MiniAudioPlaybackComponentConfig::m_loop)
                ->Field("Spatialization", &MiniAudioPlaybackComponentConfig::m_enableSpatialization)
                ->Field("Fixed Direction", &MiniAudioPlaybackComponentConfig::m_fixedDirection)
                ->Field("Direction", &MiniAudioPlaybackComponentConfig::m_direction)
                ->Field("Attenuation Model", &MiniAudioPlaybackComponentConfig::m_attenuationModel)
                ->Field("Directional Attenuation Factor", &MiniAudioPlaybackComponentConfig::m_directionalAttenuationFactor)
                ->Field("Min Distance", &MiniAudioPlaybackComponentConfig::m_minimumDistance)
                ->Field("Max Distance", &MiniAudioPlaybackComponentConfig::m_maximumDistance)
                ->Field("Inner Cone Angle", &MiniAudioPlaybackComponentConfig::m_innerAngleInDegrees)
                ->Field("Outer Cone Angle", &MiniAudioPlaybackComponentConfig::m_outerAngleInDegrees)
                ->Field("Outer Volume", &MiniAudioPlaybackComponentConfig::m_outerVolume);
        }
    }
} // namespace MiniAudio
