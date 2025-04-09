/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Android/AndroidEnv.h>
#include <AzCore/Android/Utils.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/fixed_string.h>

#include <AzCore/Android/JNI/Internal/ClassName.h>
#include <AzCore/Android/Utils.h>

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

        // Try reading the $HOME environment variable
        AZ::IO::FixedMaxPathString homePath;
        auto StoreHomeEnv = [](char* buffer, size_t size)
        {
            auto getEnvOutcome = GetEnv(AZStd::span(buffer, size), "HOME");
            return getEnvOutcome ? getEnvOutcome.GetValue().size() : 0U;
        };

        if (homePath.resize_and_overwrite(homePath.capacity(), StoreHomeEnv);
            !homePath.empty())
        {
            return homePath;
        }

        return {};
    }

    GetExecutablePathReturnType GetExecutablePath(char* exeStorageBuffer, size_t exeStorageSize)
    {
        GetExecutablePathReturnType result;
        result.m_pathIncludesFilename = false;
        const char* privateStoragePath  = AZ::Android::AndroidEnv::Get()->GetAppPrivateStoragePath();
        if(exeStorageSize > strlen(privateStoragePath))
        {
            azstrcpy(exeStorageBuffer, exeStorageSize, privateStoragePath);
        }
        else
        {
            result.m_pathStored = ExecutablePathResult::BufferSizeNotLargeEnough;
        }

        return result;
    }

    // Android attempts to find a bootstrap.cfg file within the public storage for an application
    // in non-release builds.
    // If a bootstrap.cfg file is not found in the public storage, it is then searched for
    // within the APK itself
    AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultAppRootPath()
    {
        const char* appRoot = AZ::Android::Utils::FindAssetsDirectory();
        return appRoot ? AZStd::make_optional<AZ::IO::FixedMaxPathString>(appRoot) : AZStd::nullopt;
    }

    AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultDevWriteStoragePath()
    {
        const char* writeStorage = AZ::Android::Utils::GetAppPublicStoragePath();
        return writeStorage ? AZStd::make_optional<AZ::IO::FixedMaxPathString>(writeStorage) : AZStd::nullopt;
    }

    bool ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength)
    {
        if (AZ::Android::Utils::IsApkPath(path))
        {
            azstrcpy(absolutePath, maxLength, path);
            return true;
        }

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
