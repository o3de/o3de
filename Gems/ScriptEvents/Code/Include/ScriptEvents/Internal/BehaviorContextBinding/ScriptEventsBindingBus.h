/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/string/string.h>

namespace ScriptEvents
{
    class ScriptEventsHandler;

    namespace Internal
    {

        //! Script Events are bound to their respective handlers through Bind requests
        class BindingRequest
            : public AZ::EBusTraits
        {
        public:

            struct BindingParameters
            {
                AZStd::string_view m_eventName;
                AZ::BehaviorValueParameter* m_address;
                AZ::BehaviorValueParameter* m_parameters;
                AZ::u32 m_parameterCount;
                AZ::BehaviorValueParameter* m_returnValue;

                BindingParameters()
                    : m_address(nullptr)
                    , m_parameterCount(0)
                    , m_parameters(nullptr)
                    , m_returnValue(nullptr)
                {}
            };


            //! Binding requests are done using a unique ID from the EBus/method name as the address
            using BusIdType = AZ::Uuid;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

            //! Request a bound event to be invoked given the parameters specified
            virtual void Bind(const BindingParameters&) = 0;

            virtual void Connect(const AZ::BehaviorValueParameter* address, ScriptEventsHandler* handler) = 0;
            virtual void Disconnect(const AZ::BehaviorValueParameter* address, ScriptEventsHandler* handler) = 0;

            virtual void RemoveHandler(ScriptEventsHandler*) = 0;
        };

        using BindingRequestBus = AZ::EBus<BindingRequest>;

    }
}
