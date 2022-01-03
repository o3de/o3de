/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    struct BehaviorParameter;
}

namespace AzToolsFramework
{
    //! Interface into the Python virtual machine's data
    class EditorPythonConsoleInterface
    {
    protected:
        EditorPythonConsoleInterface() = default;
        virtual ~EditorPythonConsoleInterface() = default;
        EditorPythonConsoleInterface(EditorPythonConsoleInterface&&) = delete;
        EditorPythonConsoleInterface& operator=(EditorPythonConsoleInterface&&) = delete;

    public:
        AZ_RTTI(EditorPythonConsoleInterface, "{CAE877C8-DAA8-4535-BCBF-F392EAA7CEA9}");

        //! Returns the known list of modules exported to Python
        virtual void GetModuleList(AZStd::vector<AZStd::string_view>& moduleList) const = 0;

        //! Returns the known list of global functions inside a Python module
        struct GlobalFunction
        {
            AZStd::string_view m_moduleName;
            AZStd::string_view m_functionName;
            AZStd::string_view m_description;
        };
        using GlobalFunctionCollection = AZStd::vector<GlobalFunction>;
        virtual void GetGlobalFunctionList(GlobalFunctionCollection& globalFunctionCollection) const = 0;

        virtual AZStd::string FetchPythonTypeName(const AZ::BehaviorParameter& param) = 0;
    };

    //! Interface to signal the phases for the Python virtual machine
    class EditorPythonEventsInterface
    {
    protected:
        EditorPythonEventsInterface() = default;
        virtual ~EditorPythonEventsInterface() = default;
        EditorPythonEventsInterface(EditorPythonEventsInterface&&) = delete;
        EditorPythonEventsInterface& operator=(EditorPythonEventsInterface&&) = delete;

    public:
        AZ_RTTI(EditorPythonEventsInterface, "{F50AE641-2C80-4E07-B4B3-7CB34FFAB393}");

        //! Signal the Python handler to start
        virtual bool StartPython(bool silenceWarnings = false) = 0;

        //! Signal the Python handler to stop
        virtual bool StopPython(bool silenceWarnings = false) = 0;

        //! Query to determine if the Python VM has been initialized indicating an active state 
        virtual bool IsPythonActive() = 0;

        //! Determines if the caller needs to wait for the Python VM to initialize (non-main thread only)
        virtual void WaitForInitialization() {}

        //! Acquires the Python global interpreter lock (GIL) and execute the callback
        virtual void ExecuteWithLock(AZStd::function<void()> executionCallback) = 0;

        //! Tries to acquire the Python global interpreter lock (GIL) and execute the callback.
        //! @return Whether it was able to lock the mutex or not.
        virtual bool TryExecuteWithLock(AZStd::function<void()> executionCallback) = 0;
    };

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
}

