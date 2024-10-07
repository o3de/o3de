/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MiniAudioListenerComponentConfig.h"

#include <AzCore/Asset/AssetSerializer.h>

namespace MiniAudio
{
    void MiniAudioListenerComponentConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MiniAudioListenerComponentConfig>()
                ->Version(1)
                ->Field("Follow Entity", &MiniAudioListenerComponentConfig::m_followEntity)
                ->Field("Listener Index", &MiniAudioListenerComponentConfig::m_listenerIndex)
                ->Field("Global Volume", &MiniAudioListenerComponentConfig::m_globalVolume)
                ->Field("Inner Cone Angle", &MiniAudioListenerComponentConfig::m_innerAngleInDegrees)
                ->Field("Outer Cone Angle", &MiniAudioListenerComponentConfig::m_outerAngleInDegrees)
                ->Field("Outer Volume", &MiniAudioListenerComponentConfig::m_outerVolume);
        }
    }
} // namespace MiniAudio
