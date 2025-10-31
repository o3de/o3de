/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>

#include <LmbrCentral/Audio/AudioListenerComponentBus.h>

namespace LmbrCentral
{
    /*!
     * AudioListenerComponent
     *  A component wrapper for an Audio Listener,
     *  which acts as a sink for audio sources.
     *  There is only 1 AudioListenerComponent allowed on an Entity,
     *  and they are typically paired with a Camera for orientation and position
     *  information.
     *
     *  Ideally we'd like the ability to "split" the listener position.  That is,
     *  to route different positions to different calculations.  For example, in a
     *  third-person-view game we'd like to have panning be based off the camera's
     *  location, but have attenuation curves be based off the player's location.
     *  This feature is not yet available in Wwise middleware, but it is on their
     *  roadmap (WG-21449).
     */

    class AudioListenerComponent
        : public AZ::Component
        , private AZ::TransformNotificationBus::MultiHandler
        , private AZ::EntityBus::MultiHandler
        , private AudioListenerComponentRequestBus::Handler
    {
        friend class EditorAudioListenerComponent;
    public:
        /*!
         * AZ::Component
         */
        AZ_COMPONENT(AudioListenerComponent, "{00B5358C-3EEE-4012-93FC-6222B0004404}");
        void Activate() override;
        void Deactivate() override;

        /*!
         * AudioListenerRequestBus::Handler
         * Required Interface
         */
        void SetRotationEntity(const AZ::EntityId entityId) override;
        void SetPositionEntity(const AZ::EntityId entityId) override;
        void SetListenerEnabled(bool enabled) override;

        /*!
         * AZ::TransformNotificationBus::MultiHandler
         */
        void OnTransformChanged(const AZ::Transform&, const AZ::Transform&) override;

        /*!
         * AZ::EntityBus::Handler
         */
        void OnEntityActivated(const AZ::EntityId&) override;
        void OnEntityDeactivated(const AZ::EntityId&) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AudioListenerService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AudioListenerService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        void SendListenerPosition();
        void RefreshBusConnections(const AZ::EntityId rotationEntityId, const AZ::EntityId positionEntityId);

        //! Transient data
        AZ::Transform m_transform;
        AZ::EntityId m_currentRotationEntity;
        AZ::EntityId m_currentPositionEntity;
        Audio::TAudioObjectID m_listenerObjectId = INVALID_AUDIO_OBJECT_ID;

        //! Serialized data
        bool m_defaultListenerState = true;
        AZ::EntityId m_rotationEntity;
        AZ::EntityId m_positionEntity;
        AZ::Vector3 m_fixedOffset = AZ::Vector3::CreateZero();
    };

} // namespace LmbrCentral
