/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <dlfcn.h>

#include <AzCore/Debug/Trace.h>

namespace QtForPython
{
    [[maybe_unused]] const char* s_errorModule = "QtForPython";
    const char* s_libPythonLibraryFile = "libpython3.7m.so.1.0";
    const char* s_libPyside2LibraryFile = "libpyside2.abi3.so.5.14";
    const char* s_libShibokenLibraryFile = "libshiboken2.abi3.so.5.14";

    class PysideLibraries
    {
    public:
        PysideLibraries()
        {
            m_libPythonLibraryFile = PysideLibraries::LoadModule(s_libPythonLibraryFile);
            m_libPyside2LibraryFile = PysideLibraries::LoadModule(s_libPyside2LibraryFile);
            m_libShibokenLibraryFile = PysideLibraries::LoadModule(s_libShibokenLibraryFile);
        }
        ~PysideLibraries()
        {
            PysideLibraries::UnloadModule(m_libPythonLibraryFile);
            PysideLibraries::UnloadModule(m_libPyside2LibraryFile);
            PysideLibraries::UnloadModule(m_libShibokenLibraryFile);
        }
        
    private:
        void* m_libPythonLibraryFile;
        void* m_libPyside2LibraryFile;
        void* m_libShibokenLibraryFile;
        
        static void* LoadModule(const char* moduleToLoad)
        {
            void* moduleHandle = dlopen(moduleToLoad, RTLD_NOW | RTLD_GLOBAL); 
            if (!moduleHandle)
            {
                const char* loadError = dlerror();
                AZ_Error(s_errorModule, false, "Unable to load python library %s for Pyside2: %s", moduleToLoad, loadError?loadError:"Unknown Error");
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
    };

    void LoadPysideModules()
    {
        static PysideLibraries pysideLibraries;
    }
}
