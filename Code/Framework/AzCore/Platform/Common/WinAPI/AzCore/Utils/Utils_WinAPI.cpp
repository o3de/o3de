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

        AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultDevWriteStoragePath()
        {
            return AZStd::nullopt;
        }

        bool ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength)
        {
            char* result = _fullpath(absolutePath, path, maxLength);
            return result != nullptr;
        }

        GetEnvOutcome GetEnv(AZStd::span<char> valueBuffer, const char* envname)
        {
            // Set the environment value capacity to 64KiB which is larger
            // than any value that can be stored
            constexpr size_t envValueCapacity = 1024 * 64;
            wchar_t envValueBuffer[envValueCapacity];
            // Restrict the environment variable key to 1024 characters
            AZStd::fixed_wstring<1024> wEnvname;
            AZStd::to_wstring(wEnvname, envname);
            size_t variableSize = 0;

            if (auto err = _wgetenv_s(&variableSize, envValueBuffer, envValueCapacity, wEnvname.c_str());
                !err && variableSize > 0)
            {
                const size_t envValueLen = AZStd::min(AZStd::char_traits<wchar_t>::length(envValueBuffer), variableSize);
                AZStd::fixed_string<envValueCapacity> utf8Value;
                AZStd::to_string(utf8Value, AZStd::wstring_view(envValueBuffer, envValueLen));

                // Now copy the string to the span if it has enough capacity
                if (valueBuffer.size() >= utf8Value.size())
                {
                    // copy the utf8 string value over to the value buffer
                    utf8Value.copy(valueBuffer.data(), valueBuffer.size());
                    // return a string that points the beginning of the span buffer
                    // with a size that is set to the environment variable value string
                    return AZStd::string_view(valueBuffer.data(), utf8Value.size());
                }

                return AZ::Failure(GetEnvErrorResult{ GetEnvErrorCode::BufferTooSmall, utf8Value.size() });
            }

            return AZ::Failure(GetEnvErrorResult{ GetEnvErrorCode::EnvNotSet });
        }

        bool IsEnvSet(const char* envname)
        {
            // Set the environment value capacity to 64KiB which is larger
            // than any value that can be stored
            constexpr size_t envValueCapacity = 1024 * 64;
            wchar_t envValueBuffer[envValueCapacity];
            // Restrict the environment variable key to 1024 characters
            AZStd::fixed_wstring<1024> wKey;
            AZStd::to_wstring(wKey, AZStd::string_view(envname));
            size_t variableSize = 0;
            errno_t err = _wgetenv_s(&variableSize, envValueBuffer, envValueCapacity, wKey.c_str());
            // A required size of zero indicates the environment variable is not set according
            // to the microsoft example for _wgetenv_s
            // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/getenv-s-wgetenv-s?view=msvc-170#example
            return !err && variableSize > 0;
        }

        bool SetEnv(const char* envname, const char* envvalue, [[maybe_unused]] bool overwrite)
        {
            constexpr size_t envValueCapacity = 1024 * 64;
            // Copy over the envvalue to a wide character string ubffer
            AZStd::fixed_wstring<envValueCapacity> wEnvvalue;
            AZStd::to_wstring(wEnvvalue, envvalue);

            // Restrict the environment variable key to 1024 characters
            AZStd::fixed_wstring<1024> wEnvname;
            AZStd::to_wstring(wEnvname, AZStd::string_view(envname));
            // Use the _wputenv_s version to get unicode support
            return _wputenv_s(wEnvname.c_str(), wEnvvalue.c_str()) == 0;
        }

        bool UnsetEnv(const char* envname)
        {
            return SetEnv(envname, "", true);
        }
    }
}
