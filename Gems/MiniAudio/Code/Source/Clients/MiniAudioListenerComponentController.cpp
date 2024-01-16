/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MiniAudioListenerComponentController.h"

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>

#include "MiniAudioIncludes.h"

namespace MiniAudio
{
    MiniAudioListenerComponentController::MiniAudioListenerComponentController()
    {
    }

    MiniAudioListenerComponentController::MiniAudioListenerComponentController(
        const MiniAudioListenerComponentConfig& config)
    {
        m_config = config;
    }

    void MiniAudioListenerComponentController::Reflect(AZ::ReflectContext* context)
    {
        MiniAudioListenerComponentConfig::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MiniAudioListenerComponentController>()
                ->Field("Config", &MiniAudioListenerComponentController::m_config)
                ->Version(1);
        }
    }

    void MiniAudioListenerComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MiniAudioListenerComponent"));
    }

    void MiniAudioListenerComponentController::Activate(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        m_entityComponentIdPair = entityComponentIdPair;

        MiniAudioListenerRequestBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());
        OnConfigurationUpdated();
    }

    void MiniAudioListenerComponentController::SetConfiguration(const MiniAudioListenerComponentConfig& config)
    {
        m_config = config;
        OnConfigurationUpdated();
    }

    const MiniAudioListenerComponentConfig& MiniAudioListenerComponentController::GetConfiguration() const
    {
        return m_config;
    }

    void MiniAudioListenerComponentController::SetFollowEntity(AZ::EntityId followEntity)
    {
        m_config.m_followEntity = followEntity;
        OnConfigurationUpdated();
    }

    void MiniAudioListenerComponentController::SetPosition(const AZ::Vector3& position)
    {
        if (ma_engine* engine = MiniAudioInterface::Get()->GetSoundEngine())
        {
            ma_engine_listener_set_position(engine, m_config.m_listenerIndex, position.GetX(), position.GetY(), position.GetZ());
        }
    }

    void MiniAudioListenerComponentController::Deactivate()
    {
        m_entityMovedHandler.Disconnect();

        MiniAudioListenerRequestBus::Handler::BusDisconnect();
    }

    void MiniAudioListenerComponentController::OnWorldTransformChanged(const AZ::Transform& world)
    {
        if (ma_engine* engine = MiniAudioInterface::Get()->GetSoundEngine())
        {
            ma_engine_listener_set_position(engine, m_config.m_listenerIndex, world.GetTranslation().GetX(), world.GetTranslation().GetY(), world.GetTranslation().GetZ());

            const AZ::Vector3 forward = world.TransformVector(AZ::Vector3::CreateAxisY(-1.f));
            ma_engine_listener_set_direction(engine, m_config.m_listenerIndex, forward.GetX(), forward.GetY(), forward.GetZ());

            ma_engine_listener_set_world_up(engine, m_config.m_listenerIndex, 0.f, 0.f, 1.f);
        }
    }

    void MiniAudioListenerComponentController::OnConfigurationUpdated()
    {
        if (m_config.m_followEntity.IsValid())
        {
            m_entityMovedHandler.Disconnect();
            AZ::TransformBus::Event(m_config.m_followEntity, &AZ::TransformBus::Events::BindTransformChangedEventHandler, m_entityMovedHandler);

            AZ::Transform worldTm = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldTm, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            OnWorldTransformChanged(worldTm);
        }
        else
        {
            m_entityMovedHandler.Disconnect();
        }
    }
} // namespace MiniAudio
