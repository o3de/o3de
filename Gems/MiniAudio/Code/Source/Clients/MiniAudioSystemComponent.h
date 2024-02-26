/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetManager.h>
#include <MiniAudio/MiniAudioBus.h>

// avoid including MiniAudioIncludes here to speed up compilation

struct ma_engine;

namespace MiniAudio
{
    class MiniAudioSystemComponent
        : public AZ::Component
        , public MiniAudioRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MiniAudioSystemComponent, "{9F15877E-3FC6-4479-867F-A31883DFC945}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        MiniAudioSystemComponent();
        MiniAudioSystemComponent(const MiniAudioSystemComponent&) = delete;
        MiniAudioSystemComponent& operator=(const MiniAudioSystemComponent&) = delete;
        virtual ~MiniAudioSystemComponent() override;

        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // MiniAudioRequestBus interface implementation
        ma_engine* GetSoundEngine() override;
        void SetGlobalVolume(float scale) override;
        float GetGlobalVolume() const override;
        void SetGlobalVolumeInDecibels(float decibels) override;
        AZ::u32 GetChannelCount() const override;

    private:
        std::unique_ptr<ma_engine> m_engine;

        float m_globalVolume = 1.f;

        // Number of audio output channels
        AZ::u32 m_channelCount = 0;

        // Assets related data
        AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>> m_assetHandlers;
    };

} // namespace MiniAudio
