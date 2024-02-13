/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <shlobj.h>
#include <tchar.h>

#include <AzToolsFramework/API/PythonLoader.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/conversions.h>

#include <AzFramework/IO/LocalFileIO.h>


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

        // First check if the `LY_3RD_PARTY_PATH` is set in the environment
        size_t envBufferSize{ 0 };
        getenv_s(&envBufferSize, nullptr, 0, "LY_3RDPARTY_PATH");
        if (envBufferSize > 0)
        {
            AZ_Assert(
                envBufferSize < AZ::IO::MaxPathLength,
                "The environment variable for 'LY_3RDPARTY_PATH' must not exceed the max path length of %ld",
                AZ::IO::MaxPathLength);

            char* envBuffer = new char[envBufferSize];
            getenv_s(&envBufferSize, envBuffer, envBufferSize, "LY_3RDPARTY_PATH");
            
            thirdPartyEnvPathPath = AZ::IO::FixedMaxPath(envBuffer);
            delete[] envBuffer;
        }
        else
        {
            WCHAR userProfilePath[AZ::IO::MaxPathLength] = { '\0' };
            HRESULT result = SHGetFolderPathW(NULL, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL, 0, userProfilePath);
            AZ_Assert(SUCCEEDED(result), "Unable to determine profile path needed for the 3rd folder");
            size_t userProfilePathLength = wcslen(userProfilePath);
            AZStd::string profilePathUtf8;

            // Convert UTF-16 to UTF-8
            AZStd::to_string(profilePathUtf8, { userProfilePath, aznumeric_cast<size_t>(userProfilePathLength) });

            AZ::IO::FixedMaxPath profileFixedPath(profilePathUtf8);

            thirdPartyEnvPathPath = profileFixedPath / ".o3de" / "3rdParty";
        }
        AZStd::string thirdPartyPathString = thirdPartyEnvPathPath.String();
        if ((!AZ::IO::FileIOBase::GetInstance()->IsDirectory(thirdPartyPathString.c_str())) && createOnDemand)
        {
            auto createPathResult = AZ::IO::FileIOBase::GetInstance()->CreatePath(thirdPartyPathString.c_str());
            AZ_Assert(createPathResult, "Unable to create missing 3rd Party Folder '%s'", thirdPartyPathString.c_str())

        }

        return thirdPartyEnvPathPath;
    }


    AZ::IO::FixedMaxPath PythonLoader::GetPythonHomePath(AZStd::string_view engineRoot)
    {
        AZ::IO::FixedMaxPath thirdPartyFolder = GetDefault3rdPartyPath(true);


        // On Windows, the executable folder is $PYTHONHOME, so return the same path for $PYTHONHOME
        AZ::IO::FixedMaxPath pythonHomePath = PythonLoader::GetPythonExecutablePath(thirdPartyFolder, engineRoot);
        return pythonHomePath;
    }
} // namespace AzToolsFramework::EmbeddedPython
