/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/osstring.h>
#include <AzCore/Utils/SystemUtilsApple_Platform.h>
#include <AzCore/Utils/Utils.h>
#include <dlfcn.h>

namespace AZ::Platform
{
    AZ::IO::FixedMaxPath GetModulePath()
    {
        return AZ::IO::FixedMaxPath(AZ::Utils::GetExecutableDirectory()) / "Frameworks";
    }

    void ConstructModuleFullFileName(AZ::IO::FixedMaxPath& fullPath)
    {
        // Append .framework to the name of full path
        // Afterwards use the AZ::IO::Path Append function append the filename as a child
        // of the framework directory
        AZ::IO::FixedMaxPathString fileName{ fullPath.Filename().Native() };
        fullPath.ReplaceFilename(AZ::IO::PathView(AZStd::string_view(fileName + ".framework")));
        fullPath /= fileName;
    }

    AZ::IO::FixedMaxPath CreateFrameworkModulePath(const AZ::IO::PathView& moduleName)
    {
        AZ::IO::FixedMaxPath frameworksPath;
        AZ::IO::FixedMaxPathString& frameworksPathString = frameworksPath.Native();
        auto GetBundleFrameworkPath = [](char* buffer, size_t size) -> size_t
        {
            auto frameworkPathOutcome = AZ::SystemUtilsApple::GetPathToApplicationFrameworks(AZStd::span(buffer, size));
            return frameworkPathOutcome ? frameworkPathOutcome.GetValue().size() : 0U;
        };
        frameworksPathString.resize_and_overwrite(frameworksPathString.capacity(), GetBundleFrameworkPath);
        if (!frameworksPath.empty())
        {
            frameworksPath /= moduleName;
        }

        return frameworksPath;
    }
} // namespace AZ::Platform
