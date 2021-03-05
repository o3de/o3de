#pragma once

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

#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Utilities
        {
            class FileUtilities
            {
            public:
                SCENE_CORE_API static AZStd::string CreateOutputFileName(const AZStd::string& groupName, const AZStd::string& outputDirectory, const AZStd::string& extension);
                SCENE_CORE_API static bool EnsureTargetFolderExists(const AZStd::string& path);
                SCENE_CORE_API static AZStd::string GetRelativePath(const AZStd::string& path, const AZStd::string& rootPath);
            };
        }
    }
}