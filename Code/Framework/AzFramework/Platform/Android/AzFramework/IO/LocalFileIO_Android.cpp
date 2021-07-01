/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Android/APKFileHandler.h>
#include <AzCore/Android/Utils.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/functional.h>

#include <android/api-level.h>

#if __ANDROID_API__ == 19
    // The following were apparently introduced in API 21, however in earlier versions of the
    // platform specific headers they were defines.  In the move to unified headers, the following
    // defines were removed from stat.h
    #ifndef stat64
        #define stat64 stat
    #endif

    #ifndef fstat64
        #define fstat64 fstat
    #endif

    #ifndef lstat64
        #define lstat64 lstat
    #endif
#endif // __ANDROID_API__ == 19


namespace AZ
{
    namespace IO
    {
        bool LocalFileIO::IsDirectory(const char* filePath)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("IsDir:%s", filePath);

            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            if (AZ::Android::Utils::IsApkPath(resolvedPath))
            {
                return AZ::Android::APKFileHandler::IsDirectory(AZ::Android::Utils::StripApkPrefix(resolvedPath).c_str());
            }

            struct stat result;
            if (stat(resolvedPath, &result) == 0)
            {
                return S_ISDIR(result.st_mode);
            }
            return false;
        }

        Result LocalFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
        {
            char resolvedSourcePath[AZ_MAX_PATH_LEN];
            char resolvedDestPath[AZ_MAX_PATH_LEN];
            ResolvePath(sourceFilePath, resolvedSourcePath, AZ_MAX_PATH_LEN);
            ResolvePath(destinationFilePath, resolvedDestPath, AZ_MAX_PATH_LEN);

            if (AZ::Android::Utils::IsApkPath(sourceFilePath) || AZ::Android::Utils::IsApkPath(destinationFilePath))
            {
                return ResultCode::Error; //copy from APK still to be implemented (and of course you can't copy to an APK)
            }

            // note:  Android, without root, has no reliable way to update modtimes
            //        on files on internal storage - this includes "emulated" SDCARD storage
            //        that actually resides on internal, and thus we can't depend on modtimes.
            {
                std::ifstream  sourceFile(resolvedSourcePath, std::ios::binary);
                if (sourceFile.fail())
                {
                    return ResultCode::Error;
                }

                std::ofstream  destFile(resolvedDestPath, std::ios::binary);
                if (destFile.fail())
                {
                    return ResultCode::Error;
                }
                destFile << sourceFile.rdbuf();
            }
            return ResultCode::Success;
        }


        Result LocalFileIO::FindFiles(const char* filePath, const char* filter, LocalFileIO::FindFilesCallbackType callback)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("FindFiles:%s", filePath);

            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            AZ::OSString pathWithoutSlash = RemoveTrailingSlash(resolvedPath);
            bool isInAPK = AZ::Android::Utils::IsApkPath(pathWithoutSlash.c_str());

            if (isInAPK)
            {
                AZ::IO::FixedMaxPath strippedPath = AZ::Android::Utils::StripApkPrefix(pathWithoutSlash.c_str());

                char tempBuffer[AZ_MAX_PATH_LEN] = {0};

                AZ::Android::APKFileHandler::ParseDirectory(strippedPath.c_str(), [&](const char* name)
                    {
                        AZStd::string_view filenameView = name;
                        // Skip over the current and parent directory paths
                        if (filenameView != "." && filenameView != ".." && NameMatchesFilter(name, filter))
                        {
                            AZ::OSString foundFilePath = CheckForTrailingSlash(resolvedPath);
                            foundFilePath += name;
                            // if aliased, de-alias!
                            azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, foundFilePath.c_str());
                            ConvertToAlias(tempBuffer, AZ_MAX_PATH_LEN);

                            if (!callback(tempBuffer))
                            {
                                return false;
                            }
                        }
                        return true;
                    });
            }
            else
            {
                DIR* dir = opendir(pathWithoutSlash.c_str());

                if (dir != nullptr)
                {
                    // because the absolute path might actually be SHORTER than the alias ("c:/r/dev" -> "@devroot@"), we need to
                    // use a static buffer here.
                    char tempBuffer[AZ_MAX_PATH_LEN];

                    // clear the errno state so we can distinguish between errors and end of stream
                    errno = 0;
                    struct dirent* entry = readdir(dir);

                    // List all the other files in the directory.
                    while (entry != nullptr)
                    {
                        AZStd::string_view filenameView = entry->d_name;
                        // Skip over the current and parent directory paths
                        if (filenameView != "." && filenameView != ".." && NameMatchesFilter(entry->d_name, filter))
                        {
                            AZ::OSString foundFilePath = CheckForTrailingSlash(resolvedPath);
                            foundFilePath += entry->d_name;
                            // if aliased, de-alias!
                            azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, foundFilePath.c_str());
                            ConvertToAlias(tempBuffer, AZ_MAX_PATH_LEN);

                            if (!callback(tempBuffer))
                            {
                                break;
                            }
                        }
                        entry = readdir(dir);
                    }

                    if (errno != 0)
                    {
                        closedir(dir);
                        return ResultCode::Error;
                    }

                    closedir(dir);
                    return ResultCode::Success;
                }
                else
                {
                    return ResultCode::Error;
                }
            }
            return ResultCode::Success;
        }

        Result LocalFileIO::CreatePath(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            if (AZ::Android::Utils::IsApkPath(resolvedPath))
            {
                return ResultCode::Error; //you can't write to the APK
            }

            // create all paths up to that directory.
            // its not an error if the path exists.
            if ((Exists(resolvedPath)) && (!IsDirectory(resolvedPath)))
            {
                return ResultCode::Error; // that path exists, but is not a directory.
            }

            // make directories from bottom to top.
            AZ::OSString pathBuffer;
            size_t pathLength = strlen(resolvedPath);
            pathBuffer.reserve(pathLength);
            for (size_t pathPos = 0; pathPos < pathLength; ++pathPos)
            {
                if ((resolvedPath[pathPos] == '\\') || (resolvedPath[pathPos] == '/'))
                {
                    if (pathPos > 0)
                    {
                        mkdir(pathBuffer.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                        if (!IsDirectory(pathBuffer.c_str()))
                        {
                            return ResultCode::Error;
                        }
                    }
                }
                pathBuffer.push_back(resolvedPath[pathPos]);
            }

            mkdir(pathBuffer.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            return IsDirectory(resolvedPath) ? ResultCode::Success : ResultCode::Error;
        }

        bool LocalFileIO::IsAbsolutePath(const char* path) const
        {
            return path && path[0] == '/';
        }

        bool LocalFileIO::ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength) const
        {
            if (AZ::Android::Utils::IsApkPath(path))
            {
                azstrncpy(absolutePath, maxLength, path, maxLength);
                return true;
            }
            AZ_Assert(maxLength >= AZ_MAX_PATH_LEN, "Path length is larger than AZ_MAX_PATH_LEN");
            if (!IsAbsolutePath(path))
            {
                // note that realpath fails if the path does not exist and actually changes the return value
                // to be the actual place that FAILED, which we don't want.
                // if we fail, we'd prefer to fall through and at least use the original path.
                const char* result = realpath(path, absolutePath);
                if (result)
                {
                    return true;
                }
            }
            azstrcpy(absolutePath, maxLength, path);
            return IsAbsolutePath(absolutePath);
        }
    } // namespace IO
}//namespace AZ
