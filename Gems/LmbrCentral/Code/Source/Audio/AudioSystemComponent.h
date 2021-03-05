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

#pragma once

#include <AzCore/Component/Component.h>

#include <LmbrCentral/Audio/AudioSystemComponentBus.h>

#include <CrySystemBus.h>
#include <IAudioInterfacesCommonData.h>

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
        void GlobalStopAllSounds() override;
        void GlobalExecuteAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId) override;
        void GlobalKillAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId) override;
        void GlobalSetAudioRtpc(const char* rtpcName, float value) override;
        void GlobalResetAudioRtpcs() override;
        void GlobalSetAudioSwitchState(const char* switchName, const char* stateName) override;

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEventBus::Handler interface implementation
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        void OnCrySystemShutdown(ISystem&) override;

    private:
        static void OnAudioEvent(const Audio::SAudioRequestInfo* const);
    };

} // namespace LmbrCentral
