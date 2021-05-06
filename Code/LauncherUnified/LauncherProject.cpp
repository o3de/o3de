/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/std/string/string_view.h>

#if defined(AZ_MONOLITHIC_BUILD)
    #include <StaticModules.inl>
#endif //  defined(AZ_MONOLITHIC_BUILD)

namespace O3DELauncher
{
    //! This file is to be added only to the ${project}.[Game|Server]Launcher build target
    //! This function returns the build system target name
    AZStd::string_view GetBuildTargetName()
    {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return { LY_CMAKE_TARGET };
    }


    AZStd::string_view GetProjectName()
    {
#if !defined (LY_PROJECT_NAME)
#error "LY_PROJECT_NAME must be defined in order to for the Launcher to run using a Game Project"
#endif
        return { LY_PROJECT_NAME };
    }
}
