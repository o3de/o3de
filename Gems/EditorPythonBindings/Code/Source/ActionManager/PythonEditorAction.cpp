/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ActionManager/PythonEditorAction.h>

namespace EditorPythonBindings
{
    PythonEditorAction::PythonEditorAction(PyObject* handler)
        : m_pythonCallableObject(handler)
    {
    }

    PyObject* PythonEditorAction::GetPyObject()
    {
        return m_pythonCallableObject;
    }

    const PyObject* PythonEditorAction::GetPyObject() const
    {
        return m_pythonCallableObject;
    }

} // namespace EditorPythonBindings
