#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                SCENE_CORE_API static AZStd::string CreateOutputFileName(
                    const AZStd::string& groupName, const AZStd::string& outputDirectory, const AZStd::string& extension,
                    const AZStd::string& sourceFileExtension);
                SCENE_CORE_API static bool EnsureTargetFolderExists(const AZStd::string& path);
                SCENE_CORE_API static AZStd::string GetRelativePath(const AZStd::string& path, const AZStd::string& rootPath);
            };
        }
    }
}
