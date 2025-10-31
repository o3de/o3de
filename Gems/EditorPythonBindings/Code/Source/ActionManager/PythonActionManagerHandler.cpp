/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ActionManager/PythonActionManagerHandler.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInterface.h>
#include <EditorPythonBindings/PythonCommon.h>
#include <pybind11/pybind11.h>

namespace EditorPythonBindings
{
    PythonActionManagerHandler::PythonActionManagerHandler()
    {
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();

        if (m_actionManagerInterface)
        {
            EditorPythonBindings::CustomTypeBindingNotificationBus::Handler::BusConnect(azrtti_typeid<PythonEditorAction>());
            ActionManagerRequestBus::Handler::BusConnect();
        }
        
        m_menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        
        if (m_menuManagerInterface)
        {
            MenuManagerRequestBus::Handler::BusConnect();
        }

        m_toolBarManagerInterface = AZ::Interface<AzToolsFramework::ToolBarManagerInterface>::Get();

        if (m_toolBarManagerInterface)
        {
            ToolBarManagerRequestBus::Handler::BusConnect();
        }
    }

    PythonActionManagerHandler::~PythonActionManagerHandler()
    {
        if (m_actionManagerInterface)
        {
            ActionManagerRequestBus::Handler::BusDisconnect();
            EditorPythonBindings::CustomTypeBindingNotificationBus::Handler::BusDisconnect();
        }
        
        if (m_menuManagerInterface)
        {
            MenuManagerRequestBus::Handler::BusDisconnect();
        }

        if (m_toolBarManagerInterface)
        {
            ToolBarManagerRequestBus::Handler::BusDisconnect();
        }
    }
    
    void PythonActionManagerHandler::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext
                ->Class<AzToolsFramework::ActionProperties>("ActionProperties")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Action")
                ->Attribute(AZ::Script::Attributes::Module, "action")
                ->Property("name", BehaviorValueProperty(&AzToolsFramework::ActionProperties::m_name))
                ->Property("description", BehaviorValueProperty(&AzToolsFramework::ActionProperties::m_description))
                ->Property("category", BehaviorValueProperty(&AzToolsFramework::ActionProperties::m_category))
                ->Property("iconPath", BehaviorValueProperty(&AzToolsFramework::ActionProperties::m_iconPath))
                ;

            behaviorContext
                ->EBus<ActionManagerRequestBus>("ActionManagerPythonRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Action")
                ->Attribute(AZ::Script::Attributes::Module, "action")
                ->Event("RegisterAction", &ActionManagerRequestBus::Handler::RegisterAction)
                ->Event("RegisterCheckableAction", &ActionManagerRequestBus::Handler::RegisterCheckableAction)
                ->Event("TriggerAction", &ActionManagerRequestBus::Handler::TriggerAction)
                ->Event("UpdateAction", &ActionManagerRequestBus::Handler::UpdateAction)
                ;

            behaviorContext->Class<AzToolsFramework::MenuProperties>("MenuProperties")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Action")
                ->Attribute(AZ::Script::Attributes::Module, "action")
                ->Property("name", BehaviorValueProperty(&AzToolsFramework::MenuProperties::m_name))
                ;

            behaviorContext
                ->EBus<MenuManagerRequestBus>("MenuManagerPythonRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Action")
                ->Attribute(AZ::Script::Attributes::Module, "action")
                ->Event("RegisterMenu", &MenuManagerRequestBus::Handler::RegisterMenu)
                ->Event("AddActionToMenu", &MenuManagerRequestBus::Handler::AddActionToMenu)
                ->Event("AddSeparatorToMenu", &MenuManagerRequestBus::Handler::AddSeparatorToMenu)
                ->Event("AddSubMenuToMenu", &MenuManagerRequestBus::Handler::AddSubMenuToMenu)
                ->Event("AddWidgetToMenu", &MenuManagerRequestBus::Handler::AddWidgetToMenu)
                ->Event("AddMenuToMenuBar", &MenuManagerRequestBus::Handler::AddMenuToMenuBar)
                ;

            behaviorContext->Class<AzToolsFramework::ToolBarProperties>("ToolBarProperties")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Action")
                ->Attribute(AZ::Script::Attributes::Module, "action")
                ->Property("name", BehaviorValueProperty(&AzToolsFramework::ToolBarProperties::m_name))
                ;

            behaviorContext
                ->EBus<ToolBarManagerRequestBus>("ToolBarManagerPythonRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Action")
                ->Attribute(AZ::Script::Attributes::Module, "action")
                ->Event("RegisterToolBar", &ToolBarManagerRequestBus::Handler::RegisterToolBar)
                ->Event("AddActionToToolBar", &ToolBarManagerRequestBus::Handler::AddActionToToolBar)
                ->Event("AddActionWithSubMenuToToolBar", &ToolBarManagerRequestBus::Handler::AddActionWithSubMenuToToolBar)
                ->Event("AddActionsToToolBar", &ToolBarManagerRequestBus::Handler::AddActionsToToolBar)
                ->Event("RemoveActionFromToolBar", &ToolBarManagerRequestBus::Handler::RemoveActionFromToolBar)
                ->Event("RemoveActionsFromToolBar", &ToolBarManagerRequestBus::Handler::RemoveActionsFromToolBar)
                ->Event("AddSeparatorToToolBar", &ToolBarManagerRequestBus::Handler::AddSeparatorToToolBar)
                ->Event("AddWidgetToToolBar", &ToolBarManagerRequestBus::Handler::AddWidgetToToolBar)
                ->Event("GetSortKeyOfActionInToolBar", &ToolBarManagerRequestBus::Handler::GetSortKeyOfActionInToolBar)
                ->Event("GetSortKeyOfWidgetInToolBar", &ToolBarManagerRequestBus::Handler::GetSortKeyOfWidgetInToolBar)
                ;
        }
    }

    AzToolsFramework::ActionManagerOperationResult PythonActionManagerHandler::RegisterAction(
        const AZStd::string& contextIdentifier,
        const AZStd::string& actionIdentifier,
        const AzToolsFramework::ActionProperties& properties,
        PythonEditorAction handler)
    {
        auto outcome = m_actionManagerInterface->RegisterAction(
            contextIdentifier,
            actionIdentifier,
            properties,
            [h = AZStd::move(handler)]() mutable
            {
                PyObject_CallObject(h.GetPyObject(), nullptr);
            }
        );

        if (outcome.IsSuccess())
        {
            // Store the callable to handle reference counting correctly.
            m_actionHandlerMap.insert({ actionIdentifier, PythonFunctionObject(handler.GetPyObject()) });
        }

        return outcome;
    }

    AzToolsFramework::ActionManagerOperationResult PythonActionManagerHandler::RegisterCheckableAction(
        const AZStd::string& contextIdentifier,
        const AZStd::string& actionIdentifier,
        const AzToolsFramework::ActionProperties& properties,
        PythonEditorAction handler,
        PythonEditorAction updateCallback)
    {
        PyObject* handlerObject = handler.GetPyObject();
        PyObject* updateCallbackObject = updateCallback.GetPyObject();

        auto outcome = m_actionManagerInterface->RegisterCheckableAction(
            contextIdentifier,
            actionIdentifier,
            properties,
            [h = AZStd::move(handler)]() mutable
            {
                PyObject_CallObject(h.GetPyObject(), nullptr);
            },
            [u = AZStd::move(updateCallback)]() mutable -> bool
            {
                return PyObject_IsTrue(PyObject_CallObject(u.GetPyObject(), nullptr));
            }
        );

        if (outcome.IsSuccess())
        {
            // Store the callable to handle reference counting correctly.
            m_actionHandlerMap.insert({ actionIdentifier, PythonFunctionObject(handlerObject) });
            m_actionUpdateCallbackMap.insert({ actionIdentifier, PythonFunctionObject(updateCallbackObject) });
        }

        return outcome;
    }

    AzToolsFramework::ActionManagerOperationResult PythonActionManagerHandler::TriggerAction(const AZStd::string& actionIdentifier)
    {
        return m_actionManagerInterface->TriggerAction(actionIdentifier);
    }

    AzToolsFramework::ActionManagerOperationResult PythonActionManagerHandler::UpdateAction(const AZStd::string& actionIdentifier)
    {
        return m_actionManagerInterface->UpdateAction(actionIdentifier);
    }
    
    AzToolsFramework::MenuManagerOperationResult PythonActionManagerHandler::RegisterMenu(
        const AZStd::string& identifier, const AzToolsFramework::MenuProperties& properties)
    {
        return m_menuManagerInterface->RegisterMenu(identifier, properties);
    }

    AzToolsFramework::MenuManagerOperationResult PythonActionManagerHandler::AddActionToMenu(
        const AZStd::string& menuIdentifier, const AZStd::string& actionIdentifier, int sortIndex)
    {
        return m_menuManagerInterface->AddActionToMenu(menuIdentifier, actionIdentifier, sortIndex);
    }

    AzToolsFramework::MenuManagerOperationResult PythonActionManagerHandler::AddSeparatorToMenu(
        const AZStd::string& menuIdentifier, int sortIndex)
    {
        return m_menuManagerInterface->AddSeparatorToMenu(menuIdentifier, sortIndex);
    }

    AzToolsFramework::MenuManagerOperationResult PythonActionManagerHandler::AddSubMenuToMenu(
        const AZStd::string& menuIdentifier, const AZStd::string& subMenuIdentifier, int sortIndex)
    {
        return m_menuManagerInterface->AddSubMenuToMenu(menuIdentifier, subMenuIdentifier, sortIndex);
    }

    AzToolsFramework::MenuManagerOperationResult PythonActionManagerHandler::AddWidgetToMenu(
        const AZStd::string& menuIdentifier, const AZStd::string& widgetActionIdentifier, int sortIndex)
    {
        return m_menuManagerInterface->AddWidgetToMenu(menuIdentifier, widgetActionIdentifier, sortIndex);
    }

    AzToolsFramework::MenuManagerOperationResult PythonActionManagerHandler::AddMenuToMenuBar(
        const AZStd::string& menuBarIdentifier, const AZStd::string& menuIdentifier, int sortIndex)
    {
        return m_menuManagerInterface->AddMenuToMenuBar(menuBarIdentifier, menuIdentifier, sortIndex);
    }

    AzToolsFramework::ToolBarManagerOperationResult PythonActionManagerHandler::RegisterToolBar(
        const AZStd::string& toolBarIdentifier, const AzToolsFramework::ToolBarProperties& properties)
    {
        return m_toolBarManagerInterface->RegisterToolBar(toolBarIdentifier, properties);
    }

    AzToolsFramework::ToolBarManagerOperationResult PythonActionManagerHandler::RegisterToolBarArea(
        const AZStd::string& toolBarAreaIdentifier, QMainWindow* mainWindow, Qt::ToolBarArea toolBarArea)
    {
        return m_toolBarManagerInterface->RegisterToolBarArea(toolBarAreaIdentifier, mainWindow, toolBarArea);
    }

    AzToolsFramework::ToolBarManagerOperationResult PythonActionManagerHandler::AddActionToToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier, int sortIndex)
    {
        return m_toolBarManagerInterface->AddActionToToolBar(toolBarIdentifier, actionIdentifier, sortIndex);
    }

    AzToolsFramework::ToolBarManagerOperationResult PythonActionManagerHandler::AddActionWithSubMenuToToolBar(
        const AZStd::string& toolBarIdentifier,
        const AZStd::string& actionIdentifier,
        const AZStd::string& subMenuIdentifier,
        int sortIndex)
    {
        return m_toolBarManagerInterface->AddActionWithSubMenuToToolBar(toolBarIdentifier, actionIdentifier, subMenuIdentifier, sortIndex);
    }

    AzToolsFramework::ToolBarManagerOperationResult PythonActionManagerHandler::AddActionsToToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::vector<AZStd::pair<AZStd::string, int>>& actions)
    {
        return m_toolBarManagerInterface->AddActionsToToolBar(toolBarIdentifier, actions);
    }

    AzToolsFramework::ToolBarManagerOperationResult PythonActionManagerHandler::RemoveActionFromToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier)
    {
        return m_toolBarManagerInterface->RemoveActionFromToolBar(toolBarIdentifier, actionIdentifier);
    }

    AzToolsFramework::ToolBarManagerOperationResult PythonActionManagerHandler::RemoveActionsFromToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::vector<AZStd::string>& actionIdentifiers)
    {
        return m_toolBarManagerInterface->RemoveActionsFromToolBar(toolBarIdentifier, actionIdentifiers);
    }

    AzToolsFramework::ToolBarManagerOperationResult PythonActionManagerHandler::AddSeparatorToToolBar(
        const AZStd::string& toolBarIdentifier, int sortIndex)
    {
        return m_toolBarManagerInterface->AddSeparatorToToolBar(toolBarIdentifier, sortIndex);
    }

    AzToolsFramework::ToolBarManagerOperationResult PythonActionManagerHandler::AddWidgetToToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::string& widgetActionIdentifier, int sortIndex)
    {
        return m_toolBarManagerInterface->AddWidgetToToolBar(toolBarIdentifier, widgetActionIdentifier, sortIndex);
    }

    AzToolsFramework::ToolBarManagerOperationResult PythonActionManagerHandler::AddToolBarToToolBarArea(
        const AZStd::string& toolBarAreaIdentifier, const AZStd::string& toolBarIdentifier, int sortIndex)
    {
        return m_toolBarManagerInterface->AddToolBarToToolBarArea(toolBarAreaIdentifier, toolBarIdentifier, sortIndex);
    }

    QToolBar* PythonActionManagerHandler::GenerateToolBar(const AZStd::string& toolBarIdentifier)
    {
        return m_toolBarManagerInterface->GenerateToolBar(toolBarIdentifier);
    }

    AzToolsFramework::ToolBarManagerIntegerResult PythonActionManagerHandler::GetSortKeyOfActionInToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::string& actionIdentifier) const
    {
        return m_toolBarManagerInterface->GetSortKeyOfActionInToolBar(toolBarIdentifier, actionIdentifier);
    }

    AzToolsFramework::ToolBarManagerIntegerResult PythonActionManagerHandler::GetSortKeyOfWidgetInToolBar(
        const AZStd::string& toolBarIdentifier, const AZStd::string& widgetActionIdentifier) const
    {
        return m_toolBarManagerInterface->GetSortKeyOfWidgetInToolBar(toolBarIdentifier, widgetActionIdentifier);
    }

    EditorPythonBindings::CustomTypeBindingNotifications::AllocationHandle PythonActionManagerHandler::AllocateDefault()
    {
        AZ::BehaviorObject behaviorObject;

        behaviorObject.m_address = azmalloc(sizeof(PythonEditorAction));
        behaviorObject.m_typeId = azrtti_typeid<PythonEditorAction>();
        m_allocationMap[behaviorObject.m_address] = behaviorObject.m_typeId;
        return { { reinterpret_cast<Handle>(behaviorObject.m_address), AZStd::move(behaviorObject) } };
    }

    AZStd::optional<EditorPythonBindings::CustomTypeBindingNotifications::ValueHandle> PythonActionManagerHandler::PythonToBehavior(
        PyObject* pyObj, [[maybe_unused]] AZ::BehaviorParameter::Traits traits, AZ::BehaviorArgument& outValue)
    {
        outValue.ConvertTo<PythonEditorAction>();
        outValue.StoreInTempData<PythonEditorAction>({ PythonEditorAction(pyObj) });
        return { NoAllocation };
    }

    AZStd::optional<EditorPythonBindings::CustomTypeBindingNotifications::ValueHandle> PythonActionManagerHandler::BehaviorToPython(
        const AZ::BehaviorArgument& behaviorValue, PyObject*& outPyObj)
    {
        PythonEditorAction* value = behaviorValue.GetAsUnsafe<PythonEditorAction>();
        outPyObj = value->GetPyObject();
        return { NoAllocation };
    }

    bool PythonActionManagerHandler::CanConvertPythonToBehavior([[maybe_unused]] AZ::BehaviorParameter::Traits traits, PyObject* pyObj) const
    {
        return PyCallable_Check(pyObj);
    }

    void PythonActionManagerHandler::CleanUpValue(ValueHandle handle)
    {
        if (auto handleEntry = m_allocationMap.find(reinterpret_cast<void*>(handle)); handleEntry != m_allocationMap.end())
        {
            m_allocationMap.erase(handleEntry);
            azfree(reinterpret_cast<void*>(handle));
        }
    }

    PythonActionManagerHandler::PythonFunctionObject::PythonFunctionObject(PyObject* handler)
        : m_functionObject(handler)
    {
        // Increment the reference counter for the handler on the Python side to ensure the function isn't garbage collected.
        if (m_functionObject)
        {
            Py_INCREF(m_functionObject);
        }
    }

    PythonActionManagerHandler::PythonFunctionObject::PythonFunctionObject(const PythonFunctionObject& obj)
        : m_functionObject(obj.m_functionObject)
    {
        if (m_functionObject)
        {
            Py_INCREF(m_functionObject);
        }
    }

    PythonActionManagerHandler::PythonFunctionObject::PythonFunctionObject(PythonFunctionObject&& obj)
        : m_functionObject(obj.m_functionObject)
    {
        // Reference counter does not need to be touched since we're moving ownership.
        obj.m_functionObject = nullptr;
    }

    PythonActionManagerHandler::PythonFunctionObject& PythonActionManagerHandler::PythonFunctionObject::operator=(
        const PythonFunctionObject& obj)
    {
        if (m_functionObject)
        {
            Py_DECREF(m_functionObject);
        }

        m_functionObject = obj.m_functionObject;

        if (m_functionObject)
        {
            Py_INCREF(m_functionObject);
        }

        return *this;
    }

    PythonActionManagerHandler::PythonFunctionObject::~PythonFunctionObject()
    {
        if (m_functionObject)
        {
            Py_DECREF(m_functionObject);
            // Clear the pointer in case the destructor is called multiple times.
            m_functionObject = nullptr;
        }
    }

} // namespace EditorPythonBindings
