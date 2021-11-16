/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/PythonLoader.h>
#include <AzCore/Debug/Trace.h>
#include <dlfcn.h>

namespace AzToolsFramework::EmbeddedPython
{
    PythonLoader::PythonLoader()
    {
        constexpr char libPythonName[] = "libpython3.7m.so.1.0";
        if (m_embeddedLibPythonHandle = dlopen(libPythonName, RTLD_NOW | RTLD_GLOBAL);
            m_embeddedLibPythonHandle == nullptr)
        {
            char* err = dlerror();
            AZ_Error("PythonLoader", false, "Failed to load %s with error: %s\n", libPythonName, err ? err : "Unknown Error");
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
