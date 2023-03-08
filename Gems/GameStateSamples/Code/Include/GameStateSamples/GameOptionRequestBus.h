/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <LmbrCentral/Audio/AudioSystemComponentBus.h>

namespace AZ
{
    class SerializeContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game options that can be modified via the options menu and saved to persistent storage.
    class GameOptions final
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Name of the game options save data file.
        static constexpr const char* SaveDataBufferName = "GameOptions";

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default value for the specified game option.
        ///@{
        static constexpr float DefaultAmbientVolume = 100.0f;
        static constexpr float DefaultEffectsVolume = 100.0f;
        static constexpr float DefaultMainVolume = 100.0f;
        static constexpr float DefaultMusicVolume = 100.0f;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameOptions, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameOptions, "{DC3C8011-7E2B-458F-8C95-FC1A06C9D8F4}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::SerializeContext& sc);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when loaded from persistent data.
        void OnLoadedFromPersistentData();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Effects volume accessor function.
        ///@{
        float GetAmbientVolume() const;
        void SetAmbientVolume(float ambientVolume);
        void ApplyAmbientVolume();
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Effects volume accessor function.
        ///@{
        float GetEffectsVolume() const;
        void SetEffectsVolume(float effectsVolume);
        void ApplyEffectsVolume();
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Main volume accessor function.
        ///@{
        float GetMainVolume() const;
        void SetMainVolume(float mainVolume);
        void ApplyMainVolume();
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Music volume accessor function.
        ///@{
        float GetMusicVolume() const;
        void SetMusicVolume(float musicVolume);
        void ApplyMusicVolume();
        ///@}

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        float m_ambientVolume = DefaultAmbientVolume;   //!< The current ambient volume.
        float m_effectsVolume = DefaultEffectsVolume;   //!< The current effects volume.
        float m_mainVolume = DefaultMainVolume;         //!< The current main volume.
        float m_musicVolume = DefaultMusicVolume;       //!< The current music volume.
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to submit requests related to game options.
    class GameOptionRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: requests can only be sent to and addressed by a single instance (singleton)
        ///@{
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Retrieve the game options.
        virtual AZStd::shared_ptr<GameOptions> GetGameOptions() = 0;
    };
    using GameOptionRequestBus = AZ::EBus<GameOptionRequests>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameOptions::OnLoadedFromPersistentData()
    {
        ApplyAmbientVolume();
        ApplyEffectsVolume();
        ApplyMainVolume();
        ApplyMusicVolume();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline float GameOptions::GetAmbientVolume() const
    {
        return m_ambientVolume;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameOptions::SetAmbientVolume(float ambientVolume)
    {
        m_ambientVolume = ambientVolume;
        ApplyAmbientVolume();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameOptions::ApplyAmbientVolume()
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequests::GlobalSetAudioRtpc,
                                                               "AmbientVolume",
                                                               m_ambientVolume);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline float GameOptions::GetEffectsVolume() const
    {
        return m_effectsVolume;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameOptions::SetEffectsVolume(float effectsVolume)
    {
        m_effectsVolume = effectsVolume;
        ApplyEffectsVolume();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameOptions::ApplyEffectsVolume()
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequests::GlobalSetAudioRtpc,
                                                               "EffectsVolume",
                                                               m_effectsVolume);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline float GameOptions::GetMainVolume() const
    {
        return m_mainVolume;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameOptions::SetMainVolume(float mainVolume)
    {
        m_mainVolume = mainVolume;
        ApplyMainVolume();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameOptions::ApplyMainVolume()
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequests::GlobalSetAudioRtpc,
                                                               "MainVolume",
                                                               m_mainVolume);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline float GameOptions::GetMusicVolume() const
    {
        return m_musicVolume;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameOptions::SetMusicVolume(float musicVolume)
    {
        m_musicVolume = musicVolume;
        ApplyMusicVolume();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameOptions::ApplyMusicVolume()
    {
        LmbrCentral::AudioSystemComponentRequestBus::Broadcast(&LmbrCentral::AudioSystemComponentRequests::GlobalSetAudioRtpc,
                                                               "MusicVolume",
                                                               m_musicVolume);
    }
} // namespace GameState
