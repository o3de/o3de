/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "VirtualGamepadThumbStickRequestBus.h"

#include <LyShine/Bus/UiInteractableBus.h>

#include <AzCore/Component/Component.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace VirtualGamepad
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class VirtualGamepadThumbStickComponent : public AZ::Component
                                            , public UiInteractableBus::Handler
                                            , public VirtualGamepadThumbStickRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Component Setup
        AZ_COMPONENT(VirtualGamepadThumbStickComponent, "{F3B59A92-BD6F-9CEC-A751-2EBC699992C5}", AZ::Component);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::ComponentDescriptor Services
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* context);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Init
        void Init() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Activate
        void Activate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Deactivate
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::CanHandleEvent
        bool CanHandleEvent(AZ::Vector2 point) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::HandlePressed
        bool HandlePressed(AZ::Vector2 point, bool& shouldStayActive) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::HandleReleased
        bool HandleReleased(AZ::Vector2 point) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::HandleMultiTouchPressed
        bool HandleMultiTouchPressed(AZ::Vector2 point, int multiTouchIndex) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::HandleMultiTouchReleased
        bool HandleMultiTouchReleased(AZ::Vector2 point, int multiTouchIndex) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::InputPositionUpdate
        void InputPositionUpdate(AZ::Vector2 point) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::MultiTouchPositionUpdate
        void MultiTouchPositionUpdate(AZ::Vector2 point, int multiTouchIndex) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::HandleHoverStart
        void HandleHoverStart() override {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::HandleHoverEnd
        void HandleHoverEnd() override {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::GetIsAutoActivationEnabled
        bool GetIsAutoActivationEnabled() override { return false; }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref UiInteractableInterface::SetIsAutoActivationEnabled
        void SetIsAutoActivationEnabled(bool) override {}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref VirtualGamepad::VirtualGamepadThumbStickRequests::GetCurrentAxisValuesNormalized
        AZ::Vector2 GetCurrentAxisValuesNormalized() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when any touch is pressed
        //! \param[in] viewportPositionPixels The viewport position of the touch in pixels
        //! \param[in] touchIndex The touch index (0 based)
        //! \return True if the touch was handled, false otherwise
        bool OnAnyTouchPressed(AZ::Vector2 viewportPositionPixels, int touchIndex);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when any touch is released
        //! \param[in] viewportPositionPixels The viewport position of the touch in pixels
        //! \param[in] touchIndex The touch index (0 based)
        //! \return True if the touch was handled, false otherwise
        bool OnAnyTouchReleased(AZ::Vector2 viewportPositionPixels, int touchIndex);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when any touch position is updated
        //! \param[in] viewportPositionPixels The viewport position of the touch in pixels
        //! \param[in] touchIndex The touch index (0 based)
        void OnAnyTouchPositionUpdate(AZ::Vector2 viewportPositionPixels, int touchIndex);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get all potentially assignable input channel names
        AZStd::vector<AZStd::string> GetAssignableInputChannelNames() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get all child entity id/name pairs
        AZStd::vector<AZStd::pair<AZ::EntityId, AZStd::string>> GetChildEntityIdNamePairs() const;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The input channel that will be updated when the user interacts with this virtual control
        AZStd::string m_assignedInputChannelName;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The ui element that will be drawn at the centre of the virtual thumb-stick while active
        AZ::EntityId m_thumbStickImageCentre;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The ui element that will be drawn at the radius of the virtual thumb-stick while active
        AZ::EntityId m_thumbStickImageRadial;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The default viewport position of the virtual thumb-stick in pixels
        AZ::Vector2 m_defaultViewportPositionPixels;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The current viewport position of the virtual thumb-stick in pixels
        AZ::Vector2 m_currentViewportPositionPixels;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The current virtual thumb-stick axis values normalized
        AZ::Vector2 m_currentAxisValuesNormalized;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The pixel radius of the virtual thumb-stick in pixels
        float m_thumbStickPixelRadius;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The index of the currently active touch index
        int m_activeTouchIndex;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Whether or not to centre the virtual thumb-stick when it is pressed
        bool m_centreWhenPressed = true;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Whether or not to adjust the position of the virtual thumb-stick while it is pressed, so
        //! that the pressed finger will always remain within the radius of the thumb-stick image.
        bool m_adjustPositionWhilePressed = true;
    };
} // namespace VirtualGamepad
