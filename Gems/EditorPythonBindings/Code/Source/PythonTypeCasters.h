/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Source/PythonCommon.h>
#include <pybind11/pybind11.h>
#include <AzCore/std/string/string.h>

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
