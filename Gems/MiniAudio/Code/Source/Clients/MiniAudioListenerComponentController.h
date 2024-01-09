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
#include <MiniAudio/MiniAudioListenerBus.h>

#include "MiniAudioListenerComponentConfig.h"

namespace MiniAudio
{
    class MiniAudioListenerComponentController
        : public MiniAudioListenerRequestBus::Handler
    {
        friend class EditorMiniAudioListenerComponent;
    public:
        AZ_CLASS_ALLOCATOR(MiniAudioListenerComponentController, AZ::SystemAllocator, 0);
        AZ_RTTI(MiniAudioListenerComponentController, "{59297F11-FE85-421E-A3D6-BF58A7BCFD92}");

        MiniAudioListenerComponentController();
        explicit MiniAudioListenerComponentController(const MiniAudioListenerComponentConfig& config);
        ~MiniAudioListenerComponentController() override = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        void Activate(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Deactivate();
        void SetConfiguration(const MiniAudioListenerComponentConfig& config);
        const MiniAudioListenerComponentConfig& GetConfiguration() const;

        // MiniAudioListenerRequestBus
        void SetFollowEntity(AZ::EntityId followEntity) override;
        void SetPosition(const AZ::Vector3& position) override;

    private:
        AZ::EntityComponentIdPair m_entityComponentIdPair;

        void OnWorldTransformChanged(const AZ::Transform& world);
        AZ::TransformChangedEvent::Handler m_entityMovedHandler{[this](
            [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
        {
            OnWorldTransformChanged(world);
        }};

        MiniAudioListenerComponentConfig m_config;
        void OnConfigurationUpdated();
    };
} // namespace MiniAudio
