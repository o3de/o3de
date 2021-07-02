/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        AZStd::optional<AZ::IO::FixedMaxPathString> ConvertToAbsolutePath(AZStd::string_view path)
        {
            AZ::IO::FixedMaxPathString absolutePath;
            AZ::IO::FixedMaxPathString srcPath{ path };
            if (AZ::Android::Utils::IsApkPath(srcPath.c_str()))
            {
                return srcPath;
            }

            if(char* result = realpath(srcPath.c_str(), absolutePath.data()); result)
            {
                // Fix the size value of the fixed string by calculating the c-string length using char traits
                absolutePath.resize_no_construct(AZStd::char_traits<char>::length(absolutePath.data()));
                return absolutePath;
            }

            return AZStd::nullopt;
        }
    }
}
