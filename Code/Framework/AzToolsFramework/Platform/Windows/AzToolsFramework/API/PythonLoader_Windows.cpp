/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzToolsFramework/API/PythonLoader.h>

#include <AzCore/IO/Path/Path.h>

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

} // namespace AzToolsFramework::EmbeddedPython
