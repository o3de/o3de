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
    // The python shared library to preload so that python extension modules
    // resolve the interpreter's symbols. The name is provided by the build
    // from the LY_PYTHON_SHARED_LIB cmake variable, so it always matches the
    // python the engine is configured against.
#if !defined(O3DE_PYTHON_SHARED_LIBRARY_NAME)
    #error O3DE_PYTHON_SHARED_LIBRARY_NAME must be defined; it is wired from LY_PYTHON_SHARED_LIB in the gem CMakeLists.txt
#endif
    const char* s_libPythonLibraryFile = O3DE_PYTHON_SHARED_LIBRARY_NAME;

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
