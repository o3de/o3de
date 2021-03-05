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
