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
#include <AzFramework/Components/CameraBus.h>

#include <OpenXRVk/OpenXRVkActionsInterface.h>

namespace OpenXRVk
{
    //! XRControllerComponent uses the OpenXRVk::OpenXRActionsInterface to read user input to control an VR controller.
    class XRControllerComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public Camera::CameraNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(XRControllerComponent, "{5DA45A04-A900-345C-23DE-23BD03BC3820}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // Camera::CameraNotificationBus::Handler overrides
        void OnActiveViewChanged(const AZ::EntityId&) override;

        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;

    private:
        void ProcessOpenXRActions();

        AZ::EntityId m_cameraEntity;

        // Transient data...
        AZ::Transform m_movement = AZ::Transform::CreateIdentity();

        // Serialized data...
        AZStd::string m_controllerPoseActionLabel;

        //! A cache of OpenXRVk Action Handles that provide straight
        //! access into the user's input.
        IOpenXRActions::ActionHandle m_controllerPoseHandle;
    };

} // namespace OpenXRVk
