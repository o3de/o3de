/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzToolsFramework/API/PythonLoader.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <pwd.h>
#include <dlfcn.h>

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

    AZ::IO::FixedMaxPath PythonLoader::GetDefault3rdPartyPath(bool createOnDemand)
    {
        AZ::IO::FixedMaxPath thirdPartyEnvPathPath;
        static constexpr const char* thirdPartySubpath = ".o3de/3rdParty";
        const char* envValue = nullptr;

        // First check if `LY_3RDPARTY_PATH` is explicitly set in the environment
        if ((envValue=std::getenv("LY_3RDPARTY_PATH")) != nullptr)
        {
            // If so, then use the path that is set as the third party path
            thirdPartyEnvPathPath = AZ::IO::FixedMaxPath(envValue);
        }
        // If not, then attempt to get the current user's $HOME path from the environment, which is the
        // default behavior for O3DE
        else if ((envValue=std::getenv("HOME")) != nullptr)
        {
            // If successful, build the path by appending the 3rd party subpath
            thirdPartyEnvPathPath = AZ::IO::FixedMaxPath(envValue) / thirdPartySubpath;
        }
        else 
        {
            // If for some reason the current user's $HOME path cannot be queried from the environment,
            // then use `getpwuid` to get the current user information to read the home directory. 
            envValue = getpwuid(getuid())->pw_dir;
            AZ_Assert(envValue!=nullptr, "Unable to calculate home directory");
            thirdPartyEnvPathPath = AZ::IO::FixedMaxPath(envValue) / thirdPartySubpath;
        }

        AZStd::string thirdPartyPathString = thirdPartyEnvPathPath.String();
        if ((!AZ::IO::FileIOBase::GetDirectInstance()->IsDirectory(thirdPartyPathString.c_str())) && createOnDemand)
        {
            auto createPathResult = AZ::IO::FileIOBase::GetDirectInstance()->CreatePath(thirdPartyPathString.c_str());
            AZ_Assert(createPathResult, "Unable to create missing 3rd Party Folder '%s'", thirdPartyPathString.c_str())
        }
        return thirdPartyEnvPathPath;
    }

    AZ::IO::FixedMaxPath PythonLoader::GetPythonHomePath(AZStd::string_view engineRoot)
    {
        AZ::IO::FixedMaxPath thirdPartyFolder = GetDefault3rdPartyPath(true);

        // On Mac, the executable folder is $PYTHONHOME/bin, so move up one folder to determine $PYTHONHOME
        AZ::IO::FixedMaxPath pythonHomePath = PythonLoader::GetPythonExecutablePath(thirdPartyFolder, engineRoot).ParentPath();
        return pythonHomePath;
    }
} // namespace AzToolsFramework::EmbeddedPython
