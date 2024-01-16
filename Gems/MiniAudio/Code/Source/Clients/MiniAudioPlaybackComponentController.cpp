/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MiniAudioPlaybackComponentController.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>

#include "MiniAudioIncludes.h"

namespace MiniAudio
{
    MiniAudioPlaybackComponentController::MiniAudioPlaybackComponentController()
    {
    }

    // placement of this destructor is intentional.  It forces unique_ptr<ma_sound> to declare its destructor here
    // instead of in the header before inclusion of the giant MiniAudioIncludes.h file
    MiniAudioPlaybackComponentController::~MiniAudioPlaybackComponentController() 
    {
    }

    MiniAudioPlaybackComponentController::MiniAudioPlaybackComponentController(
        const MiniAudioPlaybackComponentConfig& config)
    {
        m_config = config;
    }

    void MiniAudioPlaybackComponentController::Reflect(AZ::ReflectContext* context)
    {
        MiniAudioPlaybackComponentConfig::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MiniAudioPlaybackComponentController>()
                ->Field("Config", &MiniAudioPlaybackComponentController::m_config)
                ->Version(1);
        }
    }

    void MiniAudioPlaybackComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MiniAudioPlaybackComponent"));
    }

    void MiniAudioPlaybackComponentController::Activate(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityComponentIdPair = entityComponentIdPair;

        MiniAudioPlaybackRequestBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());
        OnConfigurationUpdated();
    }

    void MiniAudioPlaybackComponentController::SetConfiguration(const MiniAudioPlaybackComponentConfig& config)
    {
        m_config = config;
        OnConfigurationUpdated();
    }

    const MiniAudioPlaybackComponentConfig& MiniAudioPlaybackComponentController::GetConfiguration() const
    {
        return m_config;
    }

    void MiniAudioPlaybackComponentController::Deactivate()
    {
        m_entityMovedHandler.Disconnect();
        UnloadSound();
        m_config.m_sound.Release();

        MiniAudioPlaybackRequestBus::Handler::BusDisconnect();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }

    void MiniAudioPlaybackComponentController::Play()
    {
        if (m_sound)
        {
            ma_sound_seek_to_pcm_frame(m_sound.get(), 0);
            ma_sound_start(m_sound.get());
        }
    }

    void MiniAudioPlaybackComponentController::Stop()
    {
        if (m_sound)
        {
            ma_sound_stop(m_sound.get());
        }
    }

    void MiniAudioPlaybackComponentController::SetVolume(float volume)
    {
        if (m_sound)
        {
            ma_sound_set_volume(m_sound.get(), volume);
        }
    }

    void MiniAudioPlaybackComponentController::SetLooping(bool loop)
    {
        m_config.m_loop = loop;
        if (m_sound)
        {
            ma_sound_set_looping(m_sound.get(), loop);
        }
    }

    bool MiniAudioPlaybackComponentController::IsLooping() const
    {
        if (m_sound)
        {
            return ma_sound_is_looping(m_sound.get());
        }

        return false;
    }

    AZ::Data::Asset<SoundAsset> MiniAudioPlaybackComponentController::GetSoundAsset() const
    {
        return m_config.m_sound;
    }

    void MiniAudioPlaybackComponentController::SetSoundAsset(AZ::Data::Asset<SoundAsset> soundAsset)
    {
        if (m_config.m_sound.GetId() != soundAsset.GetId())
        {
            m_config.m_sound = soundAsset;
            OnConfigurationUpdated();
        }
    }

    SoundAssetRef MiniAudioPlaybackComponentController::GetSoundAssetRef() const
    {
        SoundAssetRef ref;
        ref.SetAsset(GetSoundAsset());
        return ref;
    }

    void MiniAudioPlaybackComponentController::SetSoundAssetRef(const SoundAssetRef& soundAssetRef)
    {
        SetSoundAsset(soundAssetRef.GetAsset());
    }

    void MiniAudioPlaybackComponentController::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

        // Re-assign the sound before attempting to load it if it was
        // released and the asset is now ready.
        // This can happen in the Editor when returning from game mode
        if (!m_config.m_sound.IsReady())
        {
            m_config.m_sound = asset;
        }

        LoadSound();
    }

    void MiniAudioPlaybackComponentController::OnWorldTransformChanged(const AZ::Transform& world)
    {
        if (m_sound)
        {
            ma_sound_set_position(m_sound.get(), world.GetTranslation().GetX(), world.GetTranslation().GetY(), world.GetTranslation().GetZ());
        }
    }

    void MiniAudioPlaybackComponentController::OnConfigurationUpdated()
    {
        if (m_config.m_sound.IsReady() == false)
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_config.m_sound.GetId());
            m_config.m_sound.QueueLoad();
        }
        else
        {
            LoadSound();
        }
    }

    void MiniAudioPlaybackComponentController::LoadSound()
    {
        UnloadSound();

        if (ma_engine* engine = MiniAudioInterface::Get()->GetSoundEngine())
        {
            if (GetConfiguration().m_sound.IsReady())
            {
                m_soundName = GetConfiguration().m_sound.GetHint();

                const auto& assetBuffer = GetConfiguration().m_sound->m_data;
                if (assetBuffer.empty())
                {
                    return;
                }

                ma_result result = ma_resource_manager_register_encoded_data(ma_engine_get_resource_manager(engine),
                    m_soundName.c_str(), assetBuffer.data(), assetBuffer.size());
                if (result != MA_SUCCESS)
                {
                    // An error occurred.
                    return;
                }

                if (m_sound)
                {
                    ma_sound_uninit(m_sound.get());
                    m_sound.reset();
                }
                m_sound = AZStd::make_unique<ma_sound>();

                const ma_uint32 flags = MA_SOUND_FLAG_DECODE;
                result = ma_sound_init_from_file(engine, m_soundName.c_str(), flags, nullptr, nullptr, m_sound.get());
                if (result != MA_SUCCESS)
                {
                    // An error occurred.
                    return;
                }

                ma_sound_set_volume(m_sound.get(), m_config.m_volume);
                ma_sound_set_looping(m_sound.get(), m_config.m_loop);

                ma_sound_set_spatialization_enabled(m_sound.get(), m_config.m_enableSpatialization);
                if (m_config.m_enableSpatialization)
                {
                    ma_sound_set_min_distance(m_sound.get(), m_config.m_minimumDistance);
                    ma_sound_set_max_distance(m_sound.get(), m_config.m_maximumDistance);
                    ma_sound_set_attenuation_model(m_sound.get(), static_cast<ma_attenuation_model>(m_config.m_attenuationModel));
                }

                if (m_config.m_autoFollowEntity)
                {
                    m_entityMovedHandler.Disconnect();
                    AZ::TransformBus::Event(m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::BindTransformChangedEventHandler, m_entityMovedHandler);

                    AZ::Transform worldTm = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(worldTm, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
                    OnWorldTransformChanged(worldTm);
                }
                else
                {
                    m_entityMovedHandler.Disconnect();
                }

                // Automatically play after the sound loads if requested
                // This will play automatically in Editor and Game
                if (m_config.m_autoplayOnActivate)
                {
                    Play();
                }
            }
        }
    }

    void MiniAudioPlaybackComponentController::UnloadSound()
    {
        if (ma_engine* engine = MiniAudioInterface::Get()->GetSoundEngine())
        {
            if (m_sound)
            {
                ma_sound_stop(m_sound.get());
                ma_sound_uninit(m_sound.get());
                m_sound.reset();
            }

            if (m_soundName.empty() == false)
            {
                ma_resource_manager_unregister_data(ma_engine_get_resource_manager(engine), m_soundName.c_str());
                m_soundName.clear();
            }
        }
    }
} // namespace MiniAudio
