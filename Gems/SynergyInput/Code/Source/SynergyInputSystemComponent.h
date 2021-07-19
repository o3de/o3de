/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SynergyInputClient.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SynergyInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for changes to Synergy connection related CVars.
    class SynergyInputConnectionNotifications : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when a CVar relating to the Synergy input connection changes.
        virtual void OnSynergyConnectionCVarChanged() {}
    };
    using SynergyInputConnectionNotificationBus = AZ::EBus<SynergyInputConnectionNotifications>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! A system component providing functionality related to Synergy input.
    class SynergyInputSystemComponent : public AZ::Component
                                      , public SynergyInputConnectionNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Component Setup
        AZ_COMPONENT(SynergyInputSystemComponent, "{720B6420-8A76-46F9-80C7-0DBF0CD467C2}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        SynergyInputSystemComponent() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~SynergyInputSystemComponent() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Activate
        void Activate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Deactivate
        void Deactivate() override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref SynergyInput::SynergyInputConnectionNotifications::OnSynergyConnectionCVarChanged
        void OnSynergyConnectionCVarChanged() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Try to create the Synergy client and input device implementations.
        void TryCreateSynergyClientAndInputDeviceImplementations();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destroy the Synergy client and input device implementations (if they've been created).
        void DestroySynergyClientAndInputDeviceImplementations();

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The synergy client instance.
        AZStd::unique_ptr<SynergyClient> m_synergyClient;
    };
} // namespace SynergyInput
