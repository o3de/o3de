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

#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/fixed_string.h>

#include <stdlib.h>

namespace AZ
{
    namespace Utils
    {
        void RequestAbnormalTermination()
        {
            abort();
        }

        GetExecutablePathReturnType GetExecutablePath(char* exeStorageBuffer, size_t exeStorageSize)
        {
            GetExecutablePathReturnType result;
            result.m_pathIncludesFilename = true;
            // Platform specific get exe path: http://stackoverflow.com/a/1024937
            // https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamea
            const DWORD pathLen = GetModuleFileNameA(nullptr, exeStorageBuffer, static_cast<DWORD>(exeStorageSize));
            const DWORD errorCode = GetLastError();
            if (pathLen == exeStorageSize && errorCode == ERROR_INSUFFICIENT_BUFFER)
            {
                result.m_pathStored = ExecutablePathResult::BufferSizeNotLargeEnough;
            }
            else if (pathLen == 0 && errorCode != ERROR_SUCCESS)
            {
                result.m_pathStored = ExecutablePathResult::GeneralError;
            }

            return result;
        }

        AZStd::optional<AZStd::fixed_string<MaxPathLength>> GetDefaultAppRootPath()
        {
            return AZStd::nullopt;
        }

        AZStd::optional<AZ::IO::FixedMaxPathString> GetDevWriteStoragePath()
        {
            return AZStd::nullopt;
        }

        AZStd::optional<AZStd::fixed_string<MaxPathLength>> ConvertToAbsolutePath(AZStd::string_view path)
        {
            AZStd::fixed_string<MaxPathLength> absolutePath;
            AZStd::fixed_string<MaxPathLength> srcPath{ path };
            char* result = _fullpath(absolutePath.data(), srcPath.c_str(), absolutePath.capacity());
            // Force update of the fixed_string size() value
            absolutePath.resize_no_construct(AZStd::char_traits<char>::length(absolutePath.data()));
            if (result)
            {
                return absolutePath;
            }

            return AZStd::nullopt;
        }
    }
}
