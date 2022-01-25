/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/conversions.h>

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
            wchar_t pathBufferW[AZ::IO::MaxPathLength] = { 0 };
            const DWORD pathLen = GetModuleFileNameW(nullptr, pathBufferW, static_cast<DWORD>(AZStd::size(pathBufferW)));
            const DWORD errorCode = GetLastError();
            if (pathLen == exeStorageSize && errorCode == ERROR_INSUFFICIENT_BUFFER)
            {
                result.m_pathStored = ExecutablePathResult::BufferSizeNotLargeEnough;
            }
            else if (pathLen == 0 && errorCode != ERROR_SUCCESS)
            {
                result.m_pathStored = ExecutablePathResult::GeneralError;
            }
            else
            {
                size_t utf8PathSize = AZStd::to_string_length({ pathBufferW, pathLen });
                if (utf8PathSize >= exeStorageSize)
                {
                    result.m_pathStored = ExecutablePathResult::BufferSizeNotLargeEnough;
                }
                else
                {
                    AZStd::to_string(exeStorageBuffer, exeStorageSize, pathBufferW);
                }
            }

            return result;
        }

        AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultAppRootPath()
        {
            return AZStd::nullopt;
        }

        AZStd::optional<AZ::IO::FixedMaxPathString> GetDevWriteStoragePath()
        {
            return AZStd::nullopt;
        }

        bool ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength)
        {
            char* result = _fullpath(absolutePath, path, maxLength);
            return result != nullptr;
        }


    }
}
