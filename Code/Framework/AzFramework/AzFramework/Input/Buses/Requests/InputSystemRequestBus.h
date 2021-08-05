/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! EBus interface used to send requests to the input system
    class InputSystemRequests : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: the input system is a singleton, requests are addressed to a single address.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! EBus Trait: the input system is a singleton, requests are handled by a single listener.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Recreates all enabled input devices after destroying any that happen to already exist.
        virtual void RecreateEnabledInputDevices() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Tick/update the input system. This is called during the AZ::ComponentTickBus::TICK_INPUT
        //! priority update of the AZ::TickBus, but can be called independently any time when needed.
        virtual void TickInput() = 0;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        virtual ~InputSystemRequests() = default;
    };
    using InputSystemRequestBus = AZ::EBus<InputSystemRequests>;
} // namespace AzFramework
