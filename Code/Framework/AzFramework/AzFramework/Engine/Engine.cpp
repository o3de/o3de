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

#include <AzFramework/Engine/Engine.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils/Utils.h>

namespace AzFramework
{
    namespace Engine
    {
        AZ::IO::FixedMaxPath FindEngineRoot(AZStd::string_view searchPath)
        {
            // File to locate
            const char engineRootMarker[] = "engine.json";

            AZ::IO::FixedMaxPath currentSearchPath{searchPath};
            if (currentSearchPath.empty())
            {
                char executablePath[AZ_MAX_PATH_LEN];
                AZ::Utils::GetExecutablePathReturnType result = AZ::Utils::GetExecutablePath(executablePath, AZ_MAX_PATH_LEN);
                currentSearchPath = executablePath;
            }
            do
            {
                currentSearchPath = currentSearchPath.ParentPath();
                if (AZ::IO::SystemFile::Exists((currentSearchPath / engineRootMarker).c_str()))
                {
                    return currentSearchPath;
                }
            } while (currentSearchPath.ParentPath() != currentSearchPath);

            return {};
        }
    }
} // AzFramework

