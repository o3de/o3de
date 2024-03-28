/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <EditorPythonBindings/PythonCommon.h>
#include <pybind11/pybind11.h>

namespace EditorPythonBindings
{
    namespace PythonProxyBusManagement
    {
        //! Creates the 'azlmbr.bus' module so that Python script can use Open 3D Engine buses
        void CreateSubmodule(pybind11::module module);
    }
}
