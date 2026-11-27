/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XRCameraMovementComponent.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <OpenXRVk/OpenXRVkActionsInterface.h>

#include <Atom/RPI.Public/ViewProviderBus.h>
#include <Atom/RPI.Public/View.h>
#include <AzFramework/Components/CameraBus.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

#include "OpenXRVk/OpenXRVkUtils.h"


namespace OpenXRVk
{
    void XRCameraMovementComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<XRCameraMovementComponent, AZ::Component>()
                ->Version(1)
                ->Field("Move Speed", &XRCameraMovementComponent::m_moveSpeed)
                ->Field("Movement Sensitivity", &XRCameraMovementComponent::m_movementSensitivity)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<XRCameraMovementComponent>("XR Camera Movement", "Provides XR controller input to control the camera")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "XR")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XRCameraMovementComponent::m_moveSpeed, "Move Speed", "Speed of camera movement")
                        ->Attribute(AZ::Edit::Attributes::Min, 1.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XRCameraMovementComponent::m_movementSensitivity, "Move Sensitivity", "Fine movement sensitivity factor")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<XRCameraMovementComponent>("XRCameraMovement Component Group")
                ->Attribute(AZ::Script::Attributes::Category, "OpenXRVk Gem Group")
                ;
        }
    }

    void XRCameraMovementComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("CameraMovementService"));
    }

    void XRCameraMovementComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("CameraMovementService"));
    }

    void XRCameraMovementComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void XRCameraMovementComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void XRCameraMovementComponent::Activate()
    {
        Camera::CameraNotificationBus::Handler::BusConnect();
        if (m_isActive)
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void XRCameraMovementComponent::Deactivate()
    {
        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
        
        Camera::CameraNotificationBus::Handler::BusDisconnect();
    }

    void XRCameraMovementComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        ProcessOpenXRActions();

        AZ::Transform cameraTransform;
        AZ::TransformBus::EventResult(cameraTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        // Update movement...
        const float moveSpeed = m_moveSpeed * deltaTime;
        const AZ::Vector3 movementVec = 
              (cameraTransform.GetBasisX() * m_movement.GetX())
            + (cameraTransform.GetBasisY() * m_movement.GetY())
            + (cameraTransform.GetBasisZ() * m_movement.GetZ());
        const AZ::Vector3 newPosition{ (cameraTransform.GetTranslation() + (movementVec * moveSpeed)) };
        cameraTransform.SetTranslation(newPosition);

        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTM, cameraTransform);
    }

    void XRCameraMovementComponent::ProcessOpenXRActions()
    {
        auto actionsIFace = OpenXRActionsInterface::Get();
        if (!actionsIFace)
        {
            return;
        }

        if (!m_moveFrontwaysHandle.IsValid())
        {
            // Try to cache all handles.
            m_moveFrontwaysHandle = actionsIFace->GetActionHandle("main_action_set", "move_frontways");
            if (!m_moveFrontwaysHandle.IsValid())
            {
                // Most likely the Action System failed to load the ActionSets asset.
                return;
            }

            m_moveSidewaysHandle = actionsIFace->GetActionHandle("main_action_set", "move_sideways");
            AZ_Assert(m_moveSidewaysHandle.IsValid(), "Invalid action handle");
            m_moveUpHandle = actionsIFace->GetActionHandle("main_action_set", "move_up");
            AZ_Assert(m_moveUpHandle.IsValid(), "Invalid action handle");
            m_moveDownHandle = actionsIFace->GetActionHandle("main_action_set", "move_down");
            AZ_Assert(m_moveDownHandle.IsValid(), "Invalid action handle");
        }


        m_movement.Set(0.0f);
        m_movement.SetY(ReadActionHandleFloat(actionsIFace, m_moveFrontwaysHandle) * m_movementSensitivity);
        m_movement.SetX(ReadActionHandleFloat(actionsIFace, m_moveSidewaysHandle) * m_movementSensitivity);

        {
            auto outcome = actionsIFace->GetActionStateBoolean(m_moveUpHandle);
            if (outcome.IsSuccess() && outcome.GetValue())
            {
                m_movement.SetZ(m_movementSensitivity);
            }
        }

        {
            auto outcome = actionsIFace->GetActionStateBoolean(m_moveDownHandle);
            if (outcome.IsSuccess() && outcome.GetValue())
            {
                m_movement.SetZ(-m_movementSensitivity);
            }
        }

    }

    // Camera::CameraNotificationBus::Handler overrides
    void XRCameraMovementComponent::OnActiveViewChanged(const AZ::EntityId& activeEntityId)
    {
        m_isActive = activeEntityId == GetEntityId();
        if (m_isActive)
        {
            if (!AZ::TickBus::Handler::BusIsConnected())
            {
                AZ::TickBus::Handler::BusConnect();
            }
        }
        else
        {
            if (AZ::TickBus::Handler::BusIsConnected())
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
    }

} // namespace OpenXRVk
