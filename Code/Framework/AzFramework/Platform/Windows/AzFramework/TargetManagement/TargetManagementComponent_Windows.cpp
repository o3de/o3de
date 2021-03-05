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

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Utils/Utils.h>

namespace AzFramework
{
    namespace Platform
    {
        AZStd::string GetPersistentName()
        {
            AZStd::string persistentName = "Lumberyard";

            char procPath[AZ_MAX_PATH_LEN];
            AZ::Utils::GetExecutablePathReturnType ret = AZ::Utils::GetExecutablePath(procPath, AZ_MAX_PATH_LEN);
            if (ret.m_pathStored == AZ::Utils::ExecutablePathResult::Success)
            {
                AzFramework::StringFunc::Path::GetFileName(procPath, persistentName);
            }
            
            return persistentName;
        }

        //! On windows, if the neighborhood name was not provided we
        //! will use the local computer name as the name since most of
        //! the time the hub should be running on the local machine.
        AZStd::string GetNeighborhoodName()
        {
            AZStd::string neighborhoodName;
            
            char localhost[MAX_COMPUTERNAME_LENGTH + 1];
            DWORD len = AZ_ARRAY_SIZE(localhost);
            if (GetComputerName(localhost, &len))
            {
                neighborhoodName = localhost;
            }

            return neighborhoodName;
        }
    }
}
