/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <LyShine/UiAssetTypes.h>

#include <VirtualGamepad/VirtualGamepadBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace VirtualGamepad
{
    class InputDeviceVirtualGamepad;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class VirtualGamepadSystemComponent : public AZ::Component
                                        , public VirtualGamepadRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Component Setup
        AZ_COMPONENT(VirtualGamepadSystemComponent, "{0FA16F21-B2A6-4057-BC0A-2D783973531E}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::ComponentDescriptor Services
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        VirtualGamepadSystemComponent();

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
        //! \ref VirtualGamepad::VirtualGamepadRequests::GetButtonNames
        const AZStd::unordered_set<AZStd::string>& GetButtonNames() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref VirtualGamepad::VirtualGamepadRequests::GetThumbStickNames
        const AZStd::unordered_set<AZStd::string>& GetThumbStickNames() const override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The list of button names made available by the virtual gamepad. These can be customized
        //! by editing the virtual gamepad system component, but these default values have been set
        //! (and are used by the provided canvas) so that the gem is able to work "out of the box".
        AZStd::unordered_set<AZStd::string> m_buttonNames;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The list of thumb-stick names made available by the virtual gamepad. Can be customized
        //! by editing the virtual gamepad system component, but these default values have been set
        //! (and are used by the provided canvas) so that the gem is able to work "out of the box".
        AZStd::unordered_set<AZStd::string> m_thumbStickNames;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Unique pointer to the virtual gamepad device
        AZStd::unique_ptr<InputDeviceVirtualGamepad> m_virtualGamepad;
    };
} // namespace VirtualGamepad
