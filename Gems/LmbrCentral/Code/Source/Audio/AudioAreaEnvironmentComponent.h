/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>

#include <IAudioSystem.h>

namespace LmbrCentral
{
    /*!
     * AudioAreaEnvironmentComponent
     * This component contains an Entity reference which should link to an Entity
     * that has a TriggerAreaComponent or PhysX Collider with Trigger enabled. That Trigger Area (and shape) will act as the
     * broad-phase trigger.  Once Entities go inside, this component will track their
     * movement until they leave the Trigger Area.
     * The AudioAreaEnvironmentComponent's Entity requires it's own Shape that defines
     * where the Environment is fully applied.  This shape should be placed interior
     * to the Trigger Area.  Entities that are between the two shapes will 'fade'
     * the environment amount based on the Environment fade distance property.
     */
    class AudioAreaEnvironmentComponent
        : public AZ::Component
        , private AZ::TransformNotificationBus::MultiHandler
    {
        friend class EditorAudioAreaEnvironmentComponent;

    public:
        AudioAreaEnvironmentComponent();

        /*!
         * AZ::Component
         */
        AZ_COMPONENT(AudioAreaEnvironmentComponent, "{52300012-FFCD-4559-9479-20F463940320}");
        void Activate() override;
        void Deactivate() override;

        /*!
         * AZ::TransformNotificationBus::MultiHandler
         */
        void OnTransformChanged(const AZ::Transform&, const AZ::Transform&) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("AudioAreaEnvironmentService", 0x6ba54d6c));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("AudioPreloadService", 0x20c917d8));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("AudioAreaEnvironmentService", 0x6ba54d6c));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        void OnTriggerEnter(const AzPhysics::TriggerEvent& triggerEvent);
        void OnTriggerExit(const AzPhysics::TriggerEvent& triggerEvent);

        AzPhysics::SimulatedBodyEvents::OnTriggerEnter::Handler m_onTriggerEnterHandler;
        AzPhysics::SimulatedBodyEvents::OnTriggerExit::Handler m_onTriggerExitHandler;

        //! Transient data
        Audio::TAudioEnvironmentID m_environmentId;

        //! Serialized data
        AZ::EntityId m_broadPhaseTriggerArea;
        AZStd::string m_environmentName;
        float m_environmentFadeDistance = 1.f;
    };

} // namespace LmbrCentral
