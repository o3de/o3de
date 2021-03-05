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

#if defined(BUILD_GAMELIFT_CLIENT)

#include <AzCore/PlatformDef.h>
// The AWS Native SDK AWSAllocator triggers a warning due to accessing members of std::allocator directly.
// AWSAllocator.h(70): warning C4996: 'std::allocator<T>::pointer': warning STL4010: Various members of std::allocator are deprecated in C++17.
// Use std::allocator_traits instead of accessing these members directly.
// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/memory/stl/AWSString.h>
AZ_POP_DISABLE_WARNING
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>

namespace GridMate
{
    namespace Platform
    {
        void ResolveCaCertFilePath(Aws::String& caFile)
        {
            char resolvedCAFile[AZ_MAX_PATH_LEN] = { 0 };
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@user@/certs/aws/cacert.pem", resolvedCAFile, AZ_MAX_PATH_LEN);
            caFile = resolvedCAFile;
        }
    }
}

#endif
