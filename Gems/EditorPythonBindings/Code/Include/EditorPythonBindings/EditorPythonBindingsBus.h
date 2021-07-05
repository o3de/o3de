/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

// forward declare the C typedef of a PyObject*
struct _object;
using PyObject = _object;

namespace EditorPythonBindings
{
    /**
      * Python notifications during interpreter operations
      */      
    class EditorPythonBindingsNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Sent when the Python VM is about to start
        virtual void OnPreInitialize() {}

        //! Sent when the Python VM has started
        virtual void OnPostInitialize() {}

        //! Sent when the Python VM is about to shutdown
        virtual void OnPreFinalize() {}

        //! Sent when the Python VM has shutdown
        virtual void OnPostFinalize() {}

        //! Sent when any module is being installed from Python script code (normally from an import statement in a script)
        virtual void OnImportModule(PyObject* module) { AZ_UNUSED(module); }
    };
    using EditorPythonBindingsNotificationBus = AZ::EBus<EditorPythonBindingsNotifications>;

} // namespace EditorPythonBindings

