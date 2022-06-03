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
    PythonActionManagerHandler::PythonActionManagerHandler()
    {
        m_actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();

        if (m_actionManagerInterface)
        {
            EditorPythonBindings::CustomTypeBindingNotificationBus::Handler::BusConnect(azrtti_typeid<PythonEditorAction>());
            ActionManagerRequestBus::Handler::BusConnect();
        }
    }

    PythonActionManagerHandler::~PythonActionManagerHandler()
    {
        if (m_actionManagerInterface)
        {
            ActionManagerRequestBus::Handler::BusDisconnect();
            EditorPythonBindings::CustomTypeBindingNotificationBus::Handler::BusDisconnect();
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
        auto outcome = m_actionManagerInterface->RegisterCheckableAction(
            contextIdentifier,
            actionIdentifier,
            properties,
            [h = AZStd::move(handler)]() mutable
            {
                PyObject_CallObject(h.GetHandler(), nullptr);
            },
            [u = AZStd::move(updateCallback)]() mutable -> bool
            {
                PyObject* result = PyObject_CallObject(u.GetHandler(), nullptr);
                return PyObject_IsTrue(result);
            }
        );

        if (outcome.IsSuccess())
        {
            // Store the callable to handle reference counting correctly.
            m_actionHandlerMap.insert({ actionIdentifier, PythonActionHandler(handler.GetHandler()) });
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
