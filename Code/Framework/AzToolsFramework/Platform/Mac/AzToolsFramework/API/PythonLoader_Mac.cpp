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
        // No required modules need explicit loading on this platform
    }

    void PythonLoader::UnloadRequiredModules()
    {
        // No required modules need explicit unloading on this platform
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonHomePath(AZStd::string_view engineRoot)
    {
        AZ::IO::FixedMaxPath thirdPartyFolder = GetDefault3rdPartyPath(true);

        // On Mac, the executable folder is $PYTHONHOME/bin, so move up one folder to determine $PYTHONHOME
        AZ::IO::FixedMaxPath pythonHomePath = PythonLoader::GetPythonExecutablePath(thirdPartyFolder, engineRoot).ParentPath();
        return pythonHomePath;
    }
} // namespace AzToolsFramework::EmbeddedPython
