/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzToolsFramework/API/PythonLoader.h>

namespace AzToolsFramework::EmbeddedPython
{
    void PythonLoader::LoadRequiredModules()
    {
    }

    void PythonLoader::UnloadRequiredModules()
    {
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonHomePath(AZStd::string_view engineRoot)
    {
        // On Mac, the executable folder is $PYTHONHOME/bin, so move up one folder to determine $PYTHONHOME
        AZ::IO::FixedMaxPath pythonHomePath = PythonLoader::GetPythonExecutablePath(engineRoot).ParentPath();
        return pythonHomePath;
    }
} // namespace AzToolsFramework::EmbeddedPython
