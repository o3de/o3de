/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Trace.h>
#include <dlfcn.h>

namespace EditorPythonBindings
{
    // s_libPythonLibraryFile must match the library name listed in (O3DE Engine Root)/python/runtime/.../python-config.cmake
    // in the set(${MY}_LIBRARY_xxxx sections.
    const char* s_libPythonLibraryFile = "libpython3.10.so.1.0"; 

    class InitializePython
    {
    public:
        InitializePython()
        {
            m_libPythonLibraryFile = InitializePython::LoadModule(s_libPythonLibraryFile);
        }
        virtual ~InitializePython()
        {
            InitializePython::UnloadModule(m_libPythonLibraryFile);
        }

    private:
        static void* LoadModule(const char* moduleToLoad)
        {
            void* moduleHandle = dlopen(moduleToLoad, RTLD_NOW | RTLD_GLOBAL);
            if (!moduleHandle)
            {
                [[maybe_unused]] const char* loadError = dlerror();
                AZ_Error("EditorPythonBindings", false, "Unable to load python library %s for EditorPythonBindings: %s", moduleToLoad,
                         loadError ? loadError : "Unknown Error");
            }
            return moduleHandle;
        }

        static void UnloadModule(void* moduleHandle)
        {
            if (moduleHandle)
            {
                dlclose(moduleHandle);
            }
        }

        void* m_libPythonLibraryFile;
    };
} // namespace EditorPythonBindings
