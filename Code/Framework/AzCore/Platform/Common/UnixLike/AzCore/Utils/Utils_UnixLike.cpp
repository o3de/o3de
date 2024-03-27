/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>

#include <cerrno>
#include <cstdlib>
#include <pwd.h>

namespace AZ::Utils
{
    void RequestAbnormalTermination()
    {
        abort();
    }

    void NativeErrorMessageBox(const char*, const char*) {}

    AZ::IO::FixedMaxPathString GetHomeDirectory(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        constexpr AZStd::string_view overrideHomeDirKey = "/Amazon/Settings/override_home_dir";
        AZ::IO::FixedMaxPathString overrideHomeDir;
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            if (settingsRegistry->Get(overrideHomeDir, overrideHomeDirKey))
            {
                AZ::IO::FixedMaxPath path{overrideHomeDir};
                return path.Native();
            }
        }

        struct passwd* pass = getpwuid(getuid());
        if (pass)
        {
            AZ::IO::FixedMaxPath path{pass->pw_dir};
            return path.Native();
        }

        if (const char* homePath = std::getenv("HOME"); homePath != nullptr)
        {
            AZ::IO::FixedMaxPath path{homePath};
            return path.Native();
        }

        return {};
    }

    bool ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength)
    {
#ifdef PATH_MAX
        static constexpr size_t UnixMaxPathLength = PATH_MAX;
#else
        // Fallback to 4096 if the PATH_MAX macro isn't defined on the Unix System
        static constexpr size_t UnixMaxPathLength = 4096;
#endif
        if (!AZ::IO::PathView(path).IsAbsolute())
        {
            // note that realpath fails if the path does not exist and actually changes the return value
            // to be the actual place that FAILED, which we don't want.
            // if we fail, we'd prefer to fall through and at least use the original path.
            char absolutePathBuffer[UnixMaxPathLength];
            if (const char* result = realpath(path, absolutePathBuffer); result != nullptr)
            {
                azstrcpy(absolutePath, maxLength, absolutePathBuffer);
                return true;
            }
            else if (errno == ENOENT)
            {
                // realpath will fail if the source path doesn't refer to an existing file
                // and set errno to [ENOENT]
                // > A component of file_name does not name an existing file or file_name points to an empty string.

                // Attempt to create an absolute path by appending path to current working
                // directory and normalizing it
                if (AZ::IO::FixedMaxPath normalizedPath; getcwd(absolutePathBuffer, UnixMaxPathLength) != nullptr)
                {
                    normalizedPath = (AZ::IO::FixedMaxPath(absolutePathBuffer) / path).LexicallyNormal();
                    azstrcpy(absolutePath, maxLength, normalizedPath.c_str());
                    return true;
                }
            }
        }
        azstrcpy(absolutePath, maxLength, path);
        return AZ::IO::PathView(absolutePath).IsAbsolute();
    }

    GetEnvOutcome GetEnv(AZStd::span<char> valueBuffer, const char* envname)
    {
        if (const char* envValue = std::getenv(envname);
            envValue != nullptr)
        {
            if (AZStd::string_view utf8Value(envValue);
                valueBuffer.size() >= utf8Value.size())
            {
                // copy the utf8 string value over to the value buffer
                utf8Value.copy(valueBuffer.data(), valueBuffer.size());
                // return a string that points the beginning of the span buffer
                // with a size that is set to the environment variable value string
                return AZStd::string_view(valueBuffer.data(), utf8Value.size());
            }
            else
            {
                return AZ::Failure(GetEnvErrorResult{ GetEnvErrorCode::BufferTooSmall, utf8Value.size() });
            }
        }

        return AZ::Failure(GetEnvErrorResult{ GetEnvErrorCode::EnvNotSet });
    }

    bool IsEnvSet(const char* envname)
    {
        return std::getenv(envname) != nullptr;
    }

    bool SetEnv(const char* envname, const char* envvalue, bool overwrite)
    {
        return setenv(envname, envvalue, overwrite) != -1;
    }

    bool UnsetEnv(const char* envname)
    {
        return unsetenv(envname) != -1;
    }
} // namespace AZ::Utils
