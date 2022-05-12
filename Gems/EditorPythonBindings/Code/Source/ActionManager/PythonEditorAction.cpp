/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ActionManager/PythonEditorAction.h>

PythonEditorAction::PythonEditorAction(PyObject* handler)
    : m_handler(handler)
{
    // Increment the reference handler on the Python side to ensure the function isn't garbage collected.
    if (m_handler)
    {
        Py_INCREF(m_handler);
    }
}

PythonEditorAction::PythonEditorAction(const PythonEditorAction& obj)
    : m_handler(obj.m_handler)
{
    if (m_handler)
    {
        Py_INCREF(m_handler);
    }
}

PythonEditorAction& PythonEditorAction::operator=(PythonEditorAction obj)
{
    if (m_handler)
    {
        Py_DECREF(m_handler);
    }

    m_handler = obj.m_handler;

    if (m_handler)
    {
        Py_INCREF(m_handler);
    }

    return *this;
}

PythonEditorAction::~PythonEditorAction()
{
    if (m_handler)
    {
        Py_DECREF(m_handler);
        // Clear the pointer in case the destructor is called multiple times.
        m_handler = nullptr;
    }
}

PyObject* PythonEditorAction::GetHandler() const
{
    return m_handler;
}
