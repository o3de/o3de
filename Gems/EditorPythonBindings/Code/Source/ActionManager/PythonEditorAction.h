/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/function/function_template.h>

#include <EditorPythonBindings/PythonCommon.h>
#include <pybind11/pybind11.h>

namespace EditorPythonBindings
{
    class PythonEditorAction final
    {
    public:
        AZ_TYPE_INFO(PythonEditorAction, "{1A5676D2-767B-4C2F-BC35-9CDDCE1430BB}");
        AZ_CLASS_ALLOCATOR(PythonEditorAction, AZ::SystemAllocator);

        explicit PythonEditorAction(PyObject* handler);

        PyObject* GetPyObject();
        const PyObject* GetPyObject() const;

    private:
        PyObject* m_pythonCallableObject = nullptr;
    };

} // namespace EditorPythonBindings
