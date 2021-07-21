/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZStd::string persistentName = "O3DE";

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
