/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <EditorPythonBindings/PythonCommon.h>
#include <pybind11/pybind11.h>

namespace pybind11
{
    namespace detail
    {
        //! Converts AZStd::string to/from Python String
        template <>
        struct type_caster<AZStd::string>
        {
        public:
            PYBIND11_TYPE_CASTER(AZStd::string, _("AZStd::string"));

            bool load(handle pythonSource, bool)
            {
                Py_ssize_t size = 0;
                const char* pythonString = PyUnicode_AsUTF8AndSize(pythonSource.ptr(), &size);
                if (PyErr_Occurred() || !pythonString)
                {
                    return false;
                }
                value.assign(pythonString, size);
                return true;
            }

            static handle cast(const AZStd::string& src, return_value_policy, handle)
            {
                return pybind11::str(src.c_str()).release();
            }
        };

        //! Converts AZStd::string_view to/from Python String
        template <>
        struct type_caster<AZStd::string_view>
        {
        public:
            PYBIND11_TYPE_CASTER(AZStd::string_view, _("AZStd::string_view"));

            bool load(handle pythonSource, bool)
            {
                Py_ssize_t size = 0;
                const char* pythonString = PyUnicode_AsUTF8AndSize(pythonSource.ptr(), &size);
                if (PyErr_Occurred() || !pythonString)
                {
                    return false;
                }
                value = { pythonString, static_cast<size_t>(size) };
                return true;
            }

            static handle cast(const AZStd::string_view& src, return_value_policy, handle)
            {
                return pybind11::str(src.data(), src.size()).release();
            }
        };
    }
}
