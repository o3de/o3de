/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <dlfcn.h>

namespace AzToolsFramework::EmbeddedPython
{
    class PythonLoader
    {
    public:
        PythonLoader()
        {
            m_embeddedLibPythonHandle = dlopen("libpython3.7m.so.1.0", RTLD_NOW | RTLD_GLOBAL);
            if (!m_embeddedLibPythonHandle)
            {
                char* err = dlerror();
                if (err)
                {
                    AZ_Error("PythonLoader", false, "Failed to load 'libpython' with error: %s\n", err);
                }
            }
        }

        ~PythonLoader() //override
        {
            if (m_embeddedLibPythonHandle)
            {
                dlclose(m_embeddedLibPythonHandle);
            }
        }

    private:
        void* m_embeddedLibPythonHandle{ nullptr };
    };

    void LoadLibPython()
    {
        static PythonLoader embeddedPython;
    }

} // namespace AzToolsFramework::EmbeddedPython
