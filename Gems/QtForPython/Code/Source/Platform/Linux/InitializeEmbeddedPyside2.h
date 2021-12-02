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
    const char* s_libPythonLibraryFile = "libpython3.7m.so.1.0";
    const char* s_libPyside2LibraryFile = "libpyside2.abi3.so.5.14";
    const char* s_libShibokenLibraryFile = "libshiboken2.abi3.so.5.14";
    const char* s_libQt5TestLibraryFile = "libQt5Test.so.5";

    class InitializeEmbeddedPyside2
    {
    public:
        InitializeEmbeddedPyside2()
        {
            m_libPythonLibraryFile = InitializeEmbeddedPyside2::LoadModule(s_libPythonLibraryFile);
            m_libPyside2LibraryFile = InitializeEmbeddedPyside2::LoadModule(s_libPyside2LibraryFile);
            m_libShibokenLibraryFile = InitializeEmbeddedPyside2::LoadModule(s_libShibokenLibraryFile);
            m_libQt5TestLibraryFile = InitializeEmbeddedPyside2::LoadModule(s_libQt5TestLibraryFile);
        }
        virtual ~InitializeEmbeddedPyside2()
        {
            InitializeEmbeddedPyside2::UnloadModule(m_libQt5TestLibraryFile);
            InitializeEmbeddedPyside2::UnloadModule(m_libShibokenLibraryFile);
            InitializeEmbeddedPyside2::UnloadModule(m_libPyside2LibraryFile);
            InitializeEmbeddedPyside2::UnloadModule(m_libPythonLibraryFile);
        }

    private:
        static void* LoadModule(const char* moduleToLoad)
        {
            void* moduleHandle = dlopen(moduleToLoad, RTLD_NOW | RTLD_GLOBAL);
            if (!moduleHandle)
            {
                [[maybe_unused]] const char* loadError = dlerror();
                AZ_Error("QtForPython", false, "Unable to load python library %s for Pyside2: %s", moduleToLoad,
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
        void* m_libPyside2LibraryFile;
        void* m_libShibokenLibraryFile;
        void* m_libQt5TestLibraryFile;
    };
} // namespace QtForPython
