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

#include <IAudioInterfacesCommonData.h>
#include <LmbrCentral/Audio/AudioMultiPositionComponentBus.h>

namespace UnitTest
{
    class AudioMultiPositionComponentTests;
}

namespace LmbrCentral
{
    /*!
     * Audio Multi-Position Component
     * Used to simulate "area" sounds and consume less resources.
     * Example: A river sound can be created by placing a bunch of entities along the river
     * and adding them to this component.  The positions of those entities will be sent to
     * audio system and treated as one sound.
     * Example: A hallway lined with torches.  The torches are individual sources, but they
     * can all use the same resources via this component.
     * 
     * Note: This component doesn't yet support full orientation of the entities, only position.
     * Note: This component doesn't yet support tracking movement of the entities.
     */
    class AudioMultiPositionComponent
        : public AZ::Component
        , public AudioMultiPositionComponentRequestBus::Handler
        , private AZ::EntityBus::MultiHandler
    {
        friend class UnitTest::AudioMultiPositionComponentTests;

    public:
        //! AZ::Component interface
        AZ_COMPONENT(AudioMultiPositionComponent, "{CF3B3C77-746C-4EB0-83C6-FE4AAA4203B0}");
        void Activate() override;
        void Deactivate() override;

        AudioMultiPositionComponent() = default;
        AudioMultiPositionComponent(const AZStd::vector<AZ::EntityId>& entities, Audio::MultiPositionBehaviorType type);

        //! AudioMultiPositionComponentRequestBus interface
        void AddEntity(const AZ::EntityId& entityId) override;
        void RemoveEntity(const AZ::EntityId& entityId) override;
        void SetBehaviorType(Audio::MultiPositionBehaviorType type) override;

        //! AZ::EntityBus interface
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("AudioTriggerService"));
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AudioMultiPositionService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("AudioTriggerService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AudioMultiPositionService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::size_t GetNumEntityRefs() const
        {
            return m_entityRefs.size();
        }

        AZStd::size_t GetNumEntityPositions() const
        {
            return m_entityPositions.size();
        }

    protected:
        //! Send the positions to the audio system.
        void SendMultiplePositions();

    private:
        //! Serialized Data
        AZStd::vector<AZ::EntityId> m_entityRefs;
        Audio::MultiPositionBehaviorType m_behaviorType;

        //! Transient Data
        using EntityPosPair = AZStd::pair<AZ::EntityId, AZ::Vector3>;
        AZStd::vector<EntityPosPair> m_entityPositions;
    };

} // namespace LmbrCentral
