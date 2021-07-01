/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "VirtualGamepadButtonRequestBus.h"

#include <AzCore/Component/Component.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace VirtualGamepad
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class VirtualGamepadButtonComponent : public AZ::Component
                                        , public VirtualGamepadButtonRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Component Setup
        AZ_COMPONENT(VirtualGamepadButtonComponent, "{F3B59A12-BD6F-4CEC-A151-2EBC619912C5}", AZ::Component);

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
        //! \ref VirtualGamepad::VirtualGamepadButtonRequests::IsPressed
        bool IsPressed() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Get all potentially assignable input channel names
        AZStd::vector<AZStd::string> GetAssignableInputChannelNames() const;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The input channel that will be updated when the user interacts with this virtual control
        AZStd::string m_assignedInputChannelName;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Is the interactable attached to the same component currently pressed or not?
        bool m_isPressed = false;
    };
} // namespace VirtualGamepad
