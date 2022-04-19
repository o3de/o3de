/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <LmbrCentral/Audio/AudioSystemComponentBus.h>

#include <CrySystemBus.h>
#include <IAudioInterfacesCommonData.h>
#include <ILevelSystem.h>

namespace LmbrCentral
{
    /*!
     * AudioSystemComponent
     * Handles requests of a global context for the audio system.
     * The requests are not tied to an AZ::Entity.
     */
    class AudioSystemComponent
        : public AZ::Component
        , protected AudioSystemComponentRequestBus::Handler
        , public CrySystemEventBus::Handler
        , public ILevelSystemListener
    {
    public:
        AZ_COMPONENT(AudioSystemComponent, "{666E28D2-FC99-4D41-861D-3758C5070653}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        AudioSystemComponent() = default;
        ~AudioSystemComponent() override = default;

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AudioSystemComponentRequestBus::Handler interface implementation
        bool IsAudioSystemInitialized() override;
        void GlobalStopAllSounds() override;
        void GlobalMuteAudio() override;
        void GlobalUnmuteAudio() override;
        void GlobalRefreshAudio(AZStd::string_view levelName) override;
        void GlobalExecuteAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId) override;
        void GlobalKillAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId) override;
        void GlobalSetAudioRtpc(const char* rtpcName, float value) override;
        void GlobalResetAudioRtpcs() override;
        void GlobalSetAudioSwitchState(const char* switchName, const char* stateName) override;
        void LevelLoadAudio(AZStd::string_view levelName) override;
        void LevelUnloadAudio() override;

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEventBus::Handler interface implementation
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        void OnCrySystemShutdown(ISystem&) override;

        ////////////////////////////////////////////////////////////////////////
        // ILevelSystemListener interface implementation
        void OnLoadingStart(const char* levelName) override;
        void OnUnloadComplete(const char* levelName) override;
    };

} // namespace LmbrCentral
