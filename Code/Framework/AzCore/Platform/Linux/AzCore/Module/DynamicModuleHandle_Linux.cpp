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

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/std/string/osstring.h>
#include <AzCore/Utils/Utils.h>
#include <dlfcn.h>

namespace AZ
{
    namespace Platform
    {
        void GetModulePath(AZ::OSString& path)
        {
            char exePath[AZ_MAX_PATH_LEN];
            if (AZ::Utils::GetExecutableDirectory(exePath, AZ_ARRAY_SIZE(exePath)) ==
                AZ::Utils::ExecutablePathResult::Success)
            {
                path = exePath;
                path.push_back('/');
            }
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
    
        void ConstructModuleFullFileName(const AZ::OSString& path, const AZ::OSString& fileName, AZ::OSString& fullPath)
        {
            fullPath = path + fileName;
        }
    }
}
