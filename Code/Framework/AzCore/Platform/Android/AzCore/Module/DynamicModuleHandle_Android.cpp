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
#include <dlfcn.h>

namespace AZ
{
    namespace Platform
    {
        void GetModulePath(AZ::OSString& path)
        {
        }

        void* OpenModule(const AZ::OSString& fileName, bool&)
        {
            // Android 19 does not have RTLD_NOLOAD but it should be OK since only the Editor expects to reopen modules
            return dlopen(fileName.c_str(), RTLD_NOW);
        }
    
        void ConstructModuleFullFileName(const AZ::OSString& path, const AZ::OSString& fileName, AZ::OSString& fullPath)
        {
            fullPath = path + fileName;
        }
    }
}
