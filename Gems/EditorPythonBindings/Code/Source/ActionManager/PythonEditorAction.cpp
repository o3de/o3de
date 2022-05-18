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
        : m_handler(handler)
    {
    }

    PyObject* PythonEditorAction::GetHandler()
    {
        return m_handler;
    }

    const PyObject* PythonEditorAction::GetHandler() const
    {
        return m_handler;
    }

} // namespace EditorPythonBindings
