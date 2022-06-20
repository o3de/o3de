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

namespace AZ
{
    namespace Utils
    {
        void RequestAbnormalTermination()
        {
            abort();
        }

        void NativeErrorMessageBox(const char*, const char*) {}

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

        AZStd::optional<AZ::IO::FixedMaxPathString> GetDevWriteStoragePath()
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
    }
}
