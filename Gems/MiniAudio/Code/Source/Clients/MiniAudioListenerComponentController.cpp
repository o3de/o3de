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

#include "AzCore/Math/MathUtils.h"
#include "MiniAudioIncludes.h"

namespace MiniAudio
{
    MiniAudioListenerComponentController::MiniAudioListenerComponentController()
    {
    }

    MiniAudioListenerComponentController::MiniAudioListenerComponentController(const MiniAudioListenerComponentConfig& config)
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

        m_config.m_innerAngleInRadians = AZ::DegToRad(m_config.m_innerAngleInDegrees);
        m_config.m_outerAngleInRadians = AZ::DegToRad(m_config.m_outerAngleInDegrees);

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

    float MiniAudioListenerComponentController::GetGlobalVolumePercentage() const
    {
        return MiniAudioInterface::Get()->GetGlobalVolume() * 100.f;
    }

    void MiniAudioListenerComponentController::SetGlobalVolumePercentage(float globalVolume)
    {
        m_config.m_globalVolume = globalVolume;
        MiniAudioInterface::Get()->SetGlobalVolume(m_config.m_globalVolume / 100.f);
    }

    float MiniAudioListenerComponentController::GetGlobalVolumeDecibels() const
    {
        return ma_volume_linear_to_db(MiniAudioInterface::Get()->GetGlobalVolume());
    }

    void MiniAudioListenerComponentController::SetGlobalVolumeDecibels(float globalVolumeDecibels)
    {
        m_config.m_globalVolume = ma_volume_db_to_linear(globalVolumeDecibels) * 100.f;
        MiniAudioInterface::Get()->SetGlobalVolume(m_config.m_globalVolume / 100.f);
    }

    void MiniAudioListenerComponentController::SetFollowEntity(const AZ::EntityId& followEntity)
    {
        m_config.m_followEntity = followEntity;
        OnConfigurationUpdated();
    }

    AZ::u32 MiniAudioListenerComponentController::GetChannelCount() const
    {
        return MiniAudioInterface::Get()->GetChannelCount();
    }

    float MiniAudioListenerComponentController::GetInnerAngleInRadians() const
    {
        return m_config.m_innerAngleInRadians;
    }

    void MiniAudioListenerComponentController::SetInnerAngleInRadians(float innerAngleInRadians)
    {
        m_config.m_innerAngleInRadians = innerAngleInRadians;
        m_config.m_innerAngleInDegrees = AZ::RadToDeg(m_config.m_innerAngleInRadians);
        OnConfigurationUpdated();
    }

    float MiniAudioListenerComponentController::GetInnerAngleInDegrees() const
    {
        return m_config.m_innerAngleInDegrees;
    }

    void MiniAudioListenerComponentController::SetInnerAngleInDegrees(float innerAngleInDegrees)
    {
        m_config.m_innerAngleInDegrees = innerAngleInDegrees;
        m_config.m_innerAngleInRadians = AZ::DegToRad(m_config.m_innerAngleInDegrees);
        OnConfigurationUpdated();
    }

    float MiniAudioListenerComponentController::GetOuterAngleInRadians() const
    {
        return m_config.m_outerAngleInRadians;
    }

    void MiniAudioListenerComponentController::SetOuterAngleInRadians(float outerAngleInRadians)
    {
        m_config.m_outerAngleInRadians = outerAngleInRadians;
        m_config.m_outerAngleInDegrees = AZ::RadToDeg(m_config.m_outerAngleInRadians);
        OnConfigurationUpdated();
    }

    float MiniAudioListenerComponentController::GetOuterAngleInDegrees() const
    {
        return m_config.m_outerAngleInDegrees;
    }

    void MiniAudioListenerComponentController::SetOuterAngleInDegrees(float outerAngleInDegrees)
    {
        m_config.m_outerAngleInDegrees = outerAngleInDegrees;
        m_config.m_outerAngleInRadians = AZ::DegToRad(m_config.m_outerAngleInDegrees);
        OnConfigurationUpdated();
    }

    float MiniAudioListenerComponentController::GetOuterVolumePercentage() const
    {
        return m_config.m_outerVolume;
    }

    void MiniAudioListenerComponentController::SetOuterVolumePercentage(float outerVolume)
    {
        m_config.m_outerVolume = AZ::GetClamp(outerVolume, 0.f, 100.f);
        OnConfigurationUpdated();
    }

    float MiniAudioListenerComponentController::GetOuterVolumeDecibels() const
    {
        return ma_volume_linear_to_db(m_config.m_outerVolume / 100.f);
    }

    void MiniAudioListenerComponentController::SetOuterVolumeDecibels(float outerVolumeDecibels)
    {
        m_config.m_outerVolume = ma_volume_db_to_linear(outerVolumeDecibels) * 100.f;
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
            ma_engine_listener_set_position(
                engine,
                m_config.m_listenerIndex,
                world.GetTranslation().GetX(),
                world.GetTranslation().GetY(),
                world.GetTranslation().GetZ());

            const AZ::Vector3 forward = world.GetBasisY();
            const AZ::Vector3 up = world.GetBasisZ();
            ma_engine_listener_set_direction(engine, m_config.m_listenerIndex, forward.GetX(), forward.GetY(), forward.GetZ());

            ma_engine_listener_set_world_up(engine, m_config.m_listenerIndex, up.GetX(), up.GetY(), up.GetZ());
        }
    }

    void MiniAudioListenerComponentController::OnConfigurationUpdated()
    {
        if (m_config.m_followEntity.IsValid())
        {
            m_entityMovedHandler.Disconnect();
            AZ::TransformBus::Event(
                m_config.m_followEntity, &AZ::TransformBus::Events::BindTransformChangedEventHandler, m_entityMovedHandler);

            AZ::Transform worldTm = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldTm, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            OnWorldTransformChanged(worldTm);
        }
        else
        {
            m_entityMovedHandler.Disconnect();
        }

        if (ma_engine* engine = MiniAudioInterface::Get()->GetSoundEngine())
        {
            MiniAudioInterface::Get()->SetGlobalVolume(m_config.m_globalVolume / 100.f);
            ma_engine_listener_set_cone(
                engine,
                m_config.m_listenerIndex,
                m_config.m_innerAngleInRadians,
                m_config.m_outerAngleInRadians,
                (m_config.m_outerVolume / 100.f));
        }
    }
} // namespace MiniAudio
