/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <BarrierInputClient.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace BarrierInput
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to listen for changes to Barrier connection related CVars.
    class BarrierInputConnectionNotifications : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Called when a CVar relating to the Barrier input connection changes.
        virtual void OnBarrierConnectionCVarChanged() {}
    };
    using BarrierInputConnectionNotificationBus = AZ::EBus<BarrierInputConnectionNotifications>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! A system component providing functionality related to Barrier input.
    class BarrierInputSystemComponent : public AZ::Component
                                      , public BarrierInputConnectionNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Component Setup
        AZ_COMPONENT(BarrierInputSystemComponent, "{720B6420-8A76-46F9-80C7-0DBF0CD467C2}");

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
        BarrierInputSystemComponent() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~BarrierInputSystemComponent() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Activate
        void Activate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Deactivate
        void Deactivate() override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref BarrierInput::BarrierInputConnectionNotifications::OnBarrierConnectionCVarChanged
        void OnBarrierConnectionCVarChanged() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Try to create the Barrier client and input device implementations.
        void TryCreateBarrierClientAndInputDeviceImplementations();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destroy the Barrier client and input device implementations (if they've been created).
        void DestroyBarrierClientAndInputDeviceImplementations();

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The Barrier client instance.
        AZStd::unique_ptr<BarrierClient> m_barrierClient;
    };
} // namespace BarrierInput
