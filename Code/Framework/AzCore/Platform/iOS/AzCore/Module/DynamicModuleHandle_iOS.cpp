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
            fullPath.ReplaceFilename(fileName + ".framework");
            fullPath /= fileName;
        }
    }
}
