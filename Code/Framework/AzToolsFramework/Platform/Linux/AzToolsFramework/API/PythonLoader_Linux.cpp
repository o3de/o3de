/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/PythonLoader.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <dlfcn.h>

namespace AzToolsFramework::EmbeddedPython
{


    PythonLoader::PythonLoader()
    {
        // PYTHON_SHARED_LIBRARY_PATH must be defined in the build scripts and referencing the path to the python shared library
        #if !defined(PYTHON_SHARED_LIBRARY_PATH)
        #error "PYTHON_SHARED_LIBRARY_PATH is not defined"
        #endif
        AZ::IO::FixedMaxPath libPythonName = AZ::IO::PathView(PYTHON_SHARED_LIBRARY_PATH).Filename();
        m_embeddedLibPythonHandle = dlopen(libPythonName.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (m_embeddedLibPythonHandle == nullptr)
        {
            [[maybe_unused]] const char* err = dlerror();
            AZ_Error("PythonLoader", false, "Failed to load %s with error: %s\n", libPythonName.c_str(), err ? err : "Unknown Error");
        }
    }

    PythonLoader::~PythonLoader()
    {
        if (m_embeddedLibPythonHandle)
        {
            dlclose(m_embeddedLibPythonHandle);
        }
    }
    
} // namespace AzToolsFramework::EmbeddedPython
