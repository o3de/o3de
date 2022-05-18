/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorPythonBindings/CustomTypeBindingBus.h>
#include <Source/ActionManager/ActionManagerBus.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
}

namespace EditorPythonBindings
{
    class PythonEditorActionHandler final
        : public ActionManagerRequestBus::Handler
        , private EditorPythonBindings::CustomTypeBindingNotificationBus::Handler
    {
    public:
        using Handle = EditorPythonBindings::CustomTypeBindingNotifications::ValueHandle;
        using AllocationHandle = EditorPythonBindings::CustomTypeBindingNotifications::AllocationHandle;
        constexpr static Handle NoAllocation{ ~0LL };

        PythonEditorActionHandler();
        ~PythonEditorActionHandler();

        // ActionManagerPythonRequestBus overrides ...
        ActionManagerOperationResult RegisterAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& identifier,
            const AzToolsFramework::ActionProperties& properties,
            PythonEditorAction handler) override;
        ActionManagerOperationResult TriggerAction(const AZStd::string& actionIdentifier) override;

    private:
        AZStd::unordered_map<void*, AZ::TypeId> m_allocationMap;

        AllocationHandle AllocateDefault() override;
        AZStd::optional<ValueHandle> PythonToBehavior(
            PyObject* pyObj, AZ::BehaviorParameter::Traits traits, AZ::BehaviorArgument& outValue) override;
        AZStd::optional<ValueHandle> BehaviorToPython(const AZ::BehaviorArgument& behaviorValue, PyObject*& outPyObj) override;
        bool CanConvertPythonToBehavior(AZ::BehaviorParameter::Traits traits, PyObject* pyObj) const override;
        void CleanUpValue(ValueHandle handle) override;

        class PythonActionHandler
        {
        public:
            explicit PythonActionHandler(PyObject* handler);
            PythonActionHandler(const PythonActionHandler& obj);
            PythonActionHandler(PythonActionHandler&& obj);
            PythonActionHandler& operator=(const PythonActionHandler& obj);
            virtual ~PythonActionHandler();

        private:
            PyObject* m_handler = nullptr;
        };

        AZStd::unordered_map<AZStd::string, PythonActionHandler> m_actionHandlerMap;

        AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
    };

} // namespace EditorPythonBindings
