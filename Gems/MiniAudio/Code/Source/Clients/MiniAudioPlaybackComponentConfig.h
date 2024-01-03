/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <MiniAudio/SoundAsset.h>

namespace MiniAudio
{
    // AttenuationModel hidden here to prevent needing to include miniaudio.h here.
    enum class AttenuationModel  // must match ma_attenuation_model from miniaudio.   
    {
        Inverse = 1, //ma_attenuation_model_inverse,
        Linear = 2, // ma_attenuation_model_linear,
        Exponential = 3, //= ma_attenuation_model_exponential
    };

    class MiniAudioPlaybackComponentConfig final
        : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(MiniAudioPlaybackComponentConfig, "{b829e7ae-690f-4cf4-a350-e39929f206c2}");

        static void Reflect(AZ::ReflectContext* context);

        AZ::Data::Asset<SoundAsset> m_sound;

        //! If true, automatically play when the entity activates, useful for
        //! environment audio.
        bool m_autoplayOnActivate = false;

        float m_volume = 1.f;

        //! If true, follow the position of the entity.
        bool m_autoFollowEntity = false;

        //! If true, loops the sound.
        bool m_loop = false;

        bool m_enableSpatialization = false;
        AttenuationModel m_attenuationModel = AttenuationModel::Inverse;
        float m_minimumDistance = 3.f;        
        float m_maximumDistance = 30.f;
    };
} // namespace MiniAudio
