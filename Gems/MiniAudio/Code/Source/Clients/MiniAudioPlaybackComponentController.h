/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <MiniAudio/MiniAudioBus.h>
#include <MiniAudio/MiniAudioPlaybackBus.h>
#include "MiniAudioPlaybackComponentConfig.h"

struct ma_sound;

namespace MiniAudio
{
    class MiniAudioPlaybackComponentController
        : public MiniAudioPlaybackRequestBus::Handler
        , public AZ::Data::AssetBus::MultiHandler
    {
        friend class EditorMiniAudioPlaybackComponent;
    public:
        AZ_CLASS_ALLOCATOR(MiniAudioPlaybackComponentController, AZ::SystemAllocator, 0);
        AZ_RTTI(MiniAudioPlaybackComponentController, "{1c3f1578-b190-4b49-a0c6-223f40bd9fe5}");

        MiniAudioPlaybackComponentController();
        explicit MiniAudioPlaybackComponentController(const MiniAudioPlaybackComponentConfig& config);
        ~MiniAudioPlaybackComponentController() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        void Activate(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Deactivate();
        void SetConfiguration(const MiniAudioPlaybackComponentConfig& config);
        const MiniAudioPlaybackComponentConfig& GetConfiguration() const;

        // MiniAudioPlaybackRequestBus
        void Play() override;
        void Stop() override;
        void Pause() override;
        void SetVolumePercentage(float volume) override;
        float GetVolumePercentage() const override;
        void SetVolumeDecibels(float volumeDecibels) override;
        float GetVolumeDecibels() const override;
        void SetLooping(bool loop) override;
        bool IsLooping() const override;
        AZ::Data::Asset<SoundAsset> GetSoundAsset() const override;
        void SetSoundAsset(AZ::Data::Asset<SoundAsset> soundAsset) override;
        SoundAssetRef GetSoundAssetRef() const override;
        void SetSoundAssetRef(const SoundAssetRef& soundAssetRef) override;
        float GetInnerAngleInRadians() const override;
        void SetInnerAngleInRadians(float innerAngleInRadians) override;
        float GetInnerAngleInDegrees() const override;
        void SetInnerAngleInDegrees(float innerAngleInDegrees) override;
        float GetOuterAngleInRadians() const override;
        void SetOuterAngleInRadians(float outerAngleInRadians) override;
        float GetOuterAngleInDegrees() const override;
        void SetOuterAngleInDegrees(float outerAngleInDegrees) override;
        float GetOuterVolumePercentage() const override;
        void SetOuterVolumePercentage(float outerVolume) override;
        float GetOuterVolumeDecibels() const override;
        void SetOuterVolumeDecibels(float outerVolumeDecibels) override;
        bool GetFixedDirecion() const override;
        void SetFixedDirecion(bool fixedDirection) override;
        float GetDirectionalAttenuationFactor() const override;
        void SetDirectionalAttenuationFactor(float directionalAttenuationFactor) override;
        AZ::Vector3 GetDirection() const override;
        void SetDirection(const AZ::Vector3& direction) override;

        // MultiHandler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    private:
        AZ::EntityComponentIdPair m_entityComponentIdPair;

        void OnWorldTransformChanged(const AZ::Transform& world);
        AZ::TransformChangedEvent::Handler m_entityMovedHandler{[this](
            [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            OnWorldTransformChanged(world);
        }};

        MiniAudioPlaybackComponentConfig m_config;
        void OnConfigurationUpdated();

        void LoadSound();
        void UnloadSound();

        AZStd::unique_ptr<ma_sound> m_sound;
    };
} // namespace MiniAudio
