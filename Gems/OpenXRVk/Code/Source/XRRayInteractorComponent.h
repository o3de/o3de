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
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <AzCore/Component/Entity.h>
#include <OpenXRVk/OpenXRVkActionsInterface.h>
#include <AzFramework/Components/CameraBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignmentId.h>
#include <XRInteractableComponent.h>

namespace OpenXRVk
{
    //! XRRayInteractorComponent draws line from controller
    class XRRayInteractorComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(OpenXRVk::XRRayInteractorComponent, "{ABD92123-AB30-233B-23AA-123BDEFC3821}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;

    private:
		AZ::EntityId m_currentlyHoveredEntity = AZ::EntityId();
        float m_maxLength = 10;
        AZStd::string m_gripControlActionLabel;

        AZ::Color m_initialRayColor = AZ::Color(0.2f, 0.8f, 0.1f, 1.0f);
        AZ::Color m_hoveringRayColor = AZ::Color(0.2f, 0.2f, 0.8f, 1.0f);
        AZ::Render::MaterialAssignmentId assignmentId;
        bool m_colorDefined = false;

        float m_XZnonUniformScale = 0;

        void CheckEndHovering();

        void ProcessOpenXRActions();
        //! A cache of OpenXRVk Action Handles that provide straight
        //! access into the user's input.
        IOpenXRActions::ActionHandle m_controllerSqueezeHandle;

        float m_currentSqueezeValue = 0;

        // Grab functionality
        AZ::EntityId m_heldEntity = AZ::EntityId();
        AZ::Transform m_grabOffset = AZ::Transform::CreateIdentity();
        AZ::Vector3 m_lastFramePosition = AZ::Vector3::CreateZero();
        AZ::Vector3 m_currentVelocity = AZ::Vector3::CreateZero();

        XRInteractableComponent* GetXRInterctableComponent(AZ::EntityId entityId);
        void GrabHoveredEntity();
        void ReleaseHeldEntity();

    };

} // namespace OpenXRVk
