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

namespace QtForPython
{
    // #QT6_TODO lib names might need adjustments
    // s_libPythonLibraryFile must match the library name listed in (O3DE Engine Root)/python/runtime/.../python-config.cmake
    // in the set(${MY}_LIBRARY_xxxx sections.
    const char* s_libPythonLibraryFile = "libpython3.10.so.1.0"; 
    const char* s_libPysideLibraryFile = "libpyside6.abi3.so.6.10";
    const char* s_libShibokenLibraryFile = "libshiboken6.abi3.so.6.10";
    const char* s_libQtTestLibraryFile = "libQt6Test.so.6";

    class InitializeEmbeddedPyside
    {
    public:
        InitializeEmbeddedPyside()
        {
            m_libPythonLibraryFile = InitializeEmbeddedPyside::LoadModule(s_libPythonLibraryFile);
            m_libPysideLibraryFile = InitializeEmbeddedPyside::LoadModule(s_libPysideLibraryFile);
            m_libShibokenLibraryFile = InitializeEmbeddedPyside::LoadModule(s_libShibokenLibraryFile);
            m_libQtTestLibraryFile = InitializeEmbeddedPyside::LoadModule(s_libQtTestLibraryFile);
        }
        virtual ~InitializeEmbeddedPyside()
        {
            InitializeEmbeddedPyside::UnloadModule(m_libQtTestLibraryFile);
            InitializeEmbeddedPyside::UnloadModule(m_libShibokenLibraryFile);
            InitializeEmbeddedPyside::UnloadModule(m_libPysideLibraryFile);
            InitializeEmbeddedPyside::UnloadModule(m_libPythonLibraryFile);
        }

    private:
        static void* LoadModule(const char* moduleToLoad)
        {
            void* moduleHandle = dlopen(moduleToLoad, RTLD_NOW | RTLD_GLOBAL);
            if (!moduleHandle)
            {
                [[maybe_unused]] const char* loadError = dlerror();
                AZ_Error("QtForPython", false, "Unable to load python library %s for Pyside: %s", moduleToLoad,
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
        void* m_libPysideLibraryFile;
        void* m_libShibokenLibraryFile;
        void* m_libQtTestLibraryFile;
    };
} // namespace QtForPython
