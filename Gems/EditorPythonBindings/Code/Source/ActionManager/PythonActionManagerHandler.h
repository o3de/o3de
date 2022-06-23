/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorPythonBindings/CustomTypeBindingBus.h>
#include <Source/ActionManager/ActionManagerBus.h>
#include <Source/ActionManager/MenuManagerBus.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class MenuManagerInterface;
}

namespace EditorPythonBindings
{
    //! Handler for the Python integration of the Action Manager system.
    //! Provides implementation for the Action Manager buses, and for marshaling Python callable objects as functions
    //! for use in C++ with correct reference counting to prevent them from being garbage collected.
    class PythonActionManagerHandler final
        : public ActionManagerRequestBus::Handler
        , public MenuManagerRequestBus::Handler
        , private EditorPythonBindings::CustomTypeBindingNotificationBus::Handler
    {
    public:
        using Handle = EditorPythonBindings::CustomTypeBindingNotifications::ValueHandle;
        using AllocationHandle = EditorPythonBindings::CustomTypeBindingNotifications::AllocationHandle;
        constexpr static Handle NoAllocation{ ~0LL };

        PythonActionManagerHandler();
        ~PythonActionManagerHandler();
        
        static void Reflect(AZ::ReflectContext* context);

        // ActionManagerPythonRequestBus overrides ...
        AzToolsFramework::ActionManagerOperationResult RegisterAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& actionIdentifier,
            const AzToolsFramework::ActionProperties& properties,
            PythonEditorAction handler) override;
        AzToolsFramework::ActionManagerOperationResult RegisterCheckableAction(
            const AZStd::string& contextIdentifier,
            const AZStd::string& actionIdentifier,
            const AzToolsFramework::ActionProperties& properties,
            PythonEditorAction handler,
            PythonEditorAction updateCallback) override;
        AzToolsFramework::ActionManagerOperationResult TriggerAction(const AZStd::string& actionIdentifier) override;
        AzToolsFramework::ActionManagerOperationResult UpdateAction(const AZStd::string& actionIdentifier) override;
        
        // MenuManagerPythonRequestBus overrides ...
        AzToolsFramework::MenuManagerOperationResult RegisterMenu(
            const AZStd::string& identifier, const AzToolsFramework::MenuProperties& properties) override;
        AzToolsFramework::MenuManagerOperationResult AddActionToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex) override;
        AzToolsFramework::MenuManagerOperationResult AddSeparatorToMenu(
            const AZStd::string& menuIdentifier, int sortIndex) override;
        AzToolsFramework::MenuManagerOperationResult AddSubMenuToMenu(
            const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex) override;

    private:
        AZStd::unordered_map<void*, AZ::TypeId> m_allocationMap;

        AllocationHandle AllocateDefault() override;
        AZStd::optional<ValueHandle> PythonToBehavior(
            PyObject* pyObj, AZ::BehaviorParameter::Traits traits, AZ::BehaviorArgument& outValue) override;
        AZStd::optional<ValueHandle> BehaviorToPython(const AZ::BehaviorArgument& behaviorValue, PyObject*& outPyObj) override;
        bool CanConvertPythonToBehavior(AZ::BehaviorParameter::Traits traits, PyObject* pyObj) const override;
        void CleanUpValue(ValueHandle handle) override;

        class PythonFunctionObject
        {
        public:
            explicit PythonFunctionObject(PyObject* handler);
            PythonFunctionObject(const PythonFunctionObject& obj);
            PythonFunctionObject(PythonFunctionObject&& obj);
            PythonFunctionObject& operator=(const PythonFunctionObject& obj);
            PythonFunctionObject& operator=(PythonFunctionObject&&) = delete;
            virtual ~PythonFunctionObject();

        private:
            PyObject* m_functionObject = nullptr;
        };

        AZStd::unordered_map<AZStd::string, PythonFunctionObject> m_actionHandlerMap;
        AZStd::unordered_map<AZStd::string, PythonFunctionObject> m_actionUpdateCallbackMap;

        AzToolsFramework::ActionManagerInterface* m_actionManagerInterface = nullptr;
        AzToolsFramework::MenuManagerInterface* m_menuManagerInterface = nullptr;
    };

} // namespace EditorPythonBindings
