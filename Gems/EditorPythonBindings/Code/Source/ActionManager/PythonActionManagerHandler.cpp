/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ActionManager/PythonActionManagerHandler.h>

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <Source/PythonCommon.h>
#include <pybind11/pybind11.h>

namespace EditorPythonBindings
{
    PythonEditorActionHandler::PythonEditorActionHandler()
    {
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();

        if (m_actionManagerInterface)
        {
            EditorPythonBindings::CustomTypeBindingNotificationBus::Handler::BusConnect(azrtti_typeid<PythonEditorAction>());
            ActionManagerRequestBus::Handler::BusConnect();
        }
    }

    PythonEditorActionHandler ::~PythonEditorActionHandler()
    {
        if (m_actionManagerInterface)
        {
            ActionManagerRequestBus::Handler::BusDisconnect();
            EditorPythonBindings::CustomTypeBindingNotificationBus::Handler::BusDisconnect();
        }
    }

    AzToolsFramework::ActionManagerOperationResult PythonEditorActionHandler::RegisterAction(
        const AZStd::string& contextIdentifier,
        const AZStd::string& identifier,
        const AZStd::string& name,
        const AZStd::string& description,
        const AZStd::string& category,
        const AZStd::string& iconPath,
        PythonEditorAction handler)
    {
        return m_actionManagerInterface->RegisterAction(
            contextIdentifier, identifier, name, description, category, iconPath,
            [h = AZStd::move(handler)]() mutable
            {
                PyObject_CallObject(h.GetHandler(), NULL);
            });
    }

    AzToolsFramework::ActionManagerOperationResult PythonEditorActionHandler::TriggerAction(const AZStd::string& actionIdentifier)
    {
        return m_actionManagerInterface->TriggerAction(actionIdentifier);
    }

    EditorPythonBindings::CustomTypeBindingNotifications::AllocationHandle PythonEditorActionHandler::AllocateDefault()
    {
        AZ::BehaviorObject behaviorObject;

        behaviorObject.m_address = azmalloc(sizeof(PythonEditorAction));
        behaviorObject.m_typeId = azrtti_typeid<PythonEditorAction>();
        m_allocationMap[behaviorObject.m_address] = behaviorObject.m_typeId;
        return { { reinterpret_cast<Handle>(behaviorObject.m_address), AZStd::move(behaviorObject) } };
    }

    AZStd::optional<EditorPythonBindings::CustomTypeBindingNotifications::ValueHandle> PythonEditorActionHandler::PythonToBehavior(
        PyObject* pyObj, [[maybe_unused]] AZ::BehaviorParameter::Traits traits, AZ::BehaviorArgument& outValue)
    {
        outValue.ConvertTo<PythonEditorAction>();
        outValue.StoreInTempData<PythonEditorAction>({ PythonEditorAction(pyObj) });
        return { NoAllocation };
    }

    AZStd::optional<EditorPythonBindings::CustomTypeBindingNotifications::ValueHandle> PythonEditorActionHandler::BehaviorToPython(
        const AZ::BehaviorArgument& behaviorValue, PyObject*& outPyObj)
    {
        PythonEditorAction* value = behaviorValue.GetAsUnsafe<PythonEditorAction>();
        outPyObj = value->GetHandler();
        return { NoAllocation };
    }

    bool PythonEditorActionHandler::CanConvertPythonToBehavior([[maybe_unused]] AZ::BehaviorParameter::Traits traits, PyObject* pyObj) const
    {
        return PyCallable_Check(pyObj);
    }

    void PythonEditorActionHandler::CleanUpValue(ValueHandle handle)
    {
        if (auto handleEntry = m_allocationMap.find(reinterpret_cast<void*>(handle)); handleEntry != m_allocationMap.end())
        {
            m_allocationMap.erase(handleEntry);
            azfree(reinterpret_cast<void*>(handle));
        }
    }

} // namespace EditorPythonBindings
