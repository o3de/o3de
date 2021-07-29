/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/osstring.h>
#include <AzCore/Utils/Utils.h>
#include <dlfcn.h>

namespace AZ
{
    namespace Platform
    {
        AZ::IO::FixedMaxPath GetModulePath()
        {
            return AZ::IO::FixedMaxPath(AZ::Utils::GetExecutableDirectory()) / "Frameworks";
        }

        void* OpenModule(const AZ::OSString& fileName, bool& alreadyOpen)
        {
            void* handle = dlopen(fileName.c_str(), RTLD_NOLOAD);
            alreadyOpen = (handle != nullptr);
            if (!alreadyOpen)
            {
                handle = dlopen(fileName.c_str(), RTLD_NOW);
            }
            return handle;
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
    }
}
