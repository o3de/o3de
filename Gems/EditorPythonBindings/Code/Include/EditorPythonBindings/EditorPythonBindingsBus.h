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

