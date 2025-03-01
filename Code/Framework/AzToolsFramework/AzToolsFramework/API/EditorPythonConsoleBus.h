/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

//AZTF-EBUS
#include <AzToolsFramework/AzToolsFrameworkEBus.h>
#include <AzToolsFramework/API/EditorPythonInterface.h>

namespace AzToolsFramework
{
    //! A bus to handle post notifications to the console views of Python output
    class EditorPythonConsoleNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        // used with the AZ::EBusHandlerPolicy::MultipleAndOrdered HandlerPolicy
        struct BusHandlerOrderCompare
        {
            bool operator()(EditorPythonConsoleNotifications* left, EditorPythonConsoleNotifications* right) const
            {
                return left->GetOrder() < right->GetOrder();
            }
        };
        /**
        * Specifies the order a handler receives events relative to other handlers
        * @return a value specifying this handler's relative order
        */
        virtual int GetOrder()
        {
            return std::numeric_limits<int>::max();
        }
        //////////////////////////////////////////////////////////////////////////

        //! post a normal message to the console
        virtual void OnTraceMessage(AZStd::string_view message) = 0;

        //! post an error message to the console
        virtual void OnErrorMessage(AZStd::string_view message) = 0;

        //! post an internal Python exception from a script call
        virtual void OnExceptionMessage(AZStd::string_view message) = 0;
    };
    using EditorPythonConsoleNotificationBus = AZ::EBus<EditorPythonConsoleNotifications>;
} // AzToolsFramework

AZTF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(AzToolsFramework::EditorPythonConsoleNotifications);
