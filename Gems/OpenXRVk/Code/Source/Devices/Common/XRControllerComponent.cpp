/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "XRControllerComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Components/CameraBus.h>

#include <OpenXRVk/OpenXRVkActionsInterface.h>

#include <AzCore/Math/Color.h>

namespace OpenXRVk
{
    void XRControllerComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<XRControllerComponent, AZ::Component>()
                ->Version(1)
                ->Field("Label of Pose Label", &XRControllerComponent::m_controllerPoseActionLabel)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<XRControllerComponent>("XR Controller", "Provides movement/orientation of VR controller")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "XR")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XRControllerComponent::m_controllerPoseActionLabel, "Action name of pose", "OpenXRActionsInterface ActionHandle label of pose")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<XRControllerComponent>("XR Component Group")
                ->Attribute(AZ::Script::Attributes::Category, "OpenXRVk Gem Group")
                ;
        }
    }

    void XRControllerComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("XRControllerMovementService"));
    }

    void XRControllerComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("XRControllerMovementService"));
    }

    void XRControllerComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void XRControllerComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void XRControllerComponent::Activate()
    {
        Camera::CameraNotificationBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void XRControllerComponent::Deactivate()
    {
        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
        Camera::CameraNotificationBus::Handler::BusDisconnect();
    }

    void XRControllerComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        if (m_cameraEntity == AZ::EntityId())
        {
            return;
        }
        ProcessOpenXRActions();

        // Get the camera's transform
        AZ::Transform cameraTransform;
        AZ::TransformBus::EventResult(cameraTransform, m_cameraEntity, &AZ::TransformBus::Events::GetWorldTM);

        // Current transform of the controller (in local space relative to the camera)
        AZ::Transform controllerLocalTransform = m_movement;

        // Convert the controller's local transform to world space by multiplying with the camera's transform
        // Ensure correct order of multiplication: local transform first, then camera transform
        AZ::Transform controllerWorldTransform = cameraTransform * controllerLocalTransform;

        // Apply the new world transform to the controller's render object
        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTM, controllerWorldTransform);
    }

    static AZ::Transform ReadActionHandlePose(IOpenXRActions* iface, IOpenXRActions::ActionHandle actionHandle)
    {
        auto outcome = iface->GetActionStatePose(actionHandle);
        if (!outcome.IsSuccess())
        {
            AZ::Transform value = AZ::Transform::CreateIdentity();
            value.SetTranslation(AZ::Vector3(-1000, -1000, -1000));
            // Most likely the controller went to sleep.
            return value;
        }
        AZ::Transform value = outcome.GetValue();
        if (value.GetTranslation().IsClose(AZ::Vector3::CreateZero()))
        {
            // To avoid rendering controllers when the camera is inside of them
            value.SetTranslation(AZ::Vector3(-1000, -1000, -1000));
        }
        return value;
    }

    void XRControllerComponent::ProcessOpenXRActions()
    {
        auto actionsIFace = OpenXRActionsInterface::Get();
        if (!actionsIFace)
        {
            return;
        }

        if (!m_controllerPoseHandle.IsValid())
        {
            // Try to cache all handles.
            m_controllerPoseHandle = actionsIFace->GetActionHandle("main_action_set", m_controllerPoseActionLabel);
            if (!m_controllerPoseHandle.IsValid())
            {
                // Most likely the Action System failed to load the ActionSets asset.
                return;
            }
        }

        m_movement = ReadActionHandlePose(actionsIFace, m_controllerPoseHandle);
    }


    // Camera::CameraNotificationBus::Handler overrides
    void XRControllerComponent::OnActiveViewChanged(const AZ::EntityId& activeEntityId)
    {
        m_cameraEntity = activeEntityId;
    }
} // namespace OpenXRVk
