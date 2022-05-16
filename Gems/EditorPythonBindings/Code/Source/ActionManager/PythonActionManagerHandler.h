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
        : public EditorPythonBindings::CustomTypeBindingNotificationBus::Handler
        , public ActionManagerRequestBus::Handler
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
            const AZStd::string& name,
            const AZStd::string& description,
            const AZStd::string& category,
            const AZStd::string& iconPath,
            PythonEditorAction handler) override;
        ActionManagerOperationResult TriggerAction(const AZStd::string& actionIdentifier) override;

        AZStd::unordered_map<void*, AZ::TypeId> m_allocationMap;

        AllocationHandle AllocateDefault() override;
        AZStd::optional<ValueHandle> PythonToBehavior(
            PyObject* pyObj, AZ::BehaviorParameter::Traits traits, AZ::BehaviorArgument& outValue) override;
        AZStd::optional<ValueHandle> BehaviorToPython(const AZ::BehaviorArgument& behaviorValue, PyObject*& outPyObj) override;
        bool CanConvertPythonToBehavior(AZ::BehaviorParameter::Traits traits, PyObject* pyObj) const override;
        void CleanUpValue(ValueHandle handle) override;

    private:
        AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
    };

} // namespace EditorPythonBindings
