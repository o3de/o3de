/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        Result LocalFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
        {
            FixedMaxPath resolvedSourcePath;
            ResolvePath(resolvedSourcePath, sourceFilePath);

            FixedMaxPath resolvedDestPath;
            ResolvePath(resolvedDestPath, destinationFilePath);

            if (AZ::Android::Utils::IsApkPath(sourceFilePath) || AZ::Android::Utils::IsApkPath(destinationFilePath))
            {
                return ResultCode::Error; //copy from APK still to be implemented (and of course you can't copy to an APK)
            }

            // note:  Android, without root, has no reliable way to update modtimes
            //        on files on internal storage - this includes "emulated" SDCARD storage
            //        that actually resides on internal, and thus we can't depend on modtimes.
            {
                std::ifstream sourceFile(resolvedSourcePath.c_str(), std::ios::binary);
                if (sourceFile.fail())
                {
                    return ResultCode::Error;
                }

                std::ofstream destFile(resolvedDestPath.c_str(), std::ios::binary);
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

            FixedMaxPath resolvedPath;
            ResolvePath(resolvedPath, filePath);
            resolvedPath = resolvedPath.LexicallyNormal();

            bool isInAPK = AZ::Android::Utils::IsApkPath(resolvedPath.c_str());

            if (isInAPK)
            {
                AZ::IO::FixedMaxPath strippedPath = AZ::Android::Utils::StripApkPrefix(resolvedPath.c_str());

                AZ::Android::APKFileHandler::ParseDirectory(strippedPath.c_str(), [&](AZStd::string_view filenameView)
                    {
                        // Skip over the current and parent directory paths
                        if (filenameView != "." && filenameView != ".." && AZStd::wildcard_match(filter, filenameView))
                        {
                            if (!callback((resolvedPath / filenameView).c_str()))
                            {
                                return false;
                            }
                        }
                        return true;
                    });
            }
            else
            {
                DIR* dir = opendir(resolvedPath.c_str());

                if (dir != nullptr)
                {
                    // clear the errno state so we can distinguish between errors and end of stream
                    errno = 0;
                    struct dirent* entry = readdir(dir);

                    // List all the other files in the directory.
                    while (entry != nullptr)
                    {
                        AZStd::string_view filenameView = entry->d_name;
                        // Skip over the current and parent directory paths
                        if (filenameView != "." && filenameView != ".." && AZStd::wildcard_match(filter, filenameView))
                        {
                            if (!callback((resolvedPath / filenameView).c_str()))
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
            FixedMaxPath resolvedPath;
            ResolvePath(resolvedPath, filePath);

            if (AZ::Android::Utils::IsApkPath(resolvedPath.c_str()))
            {
                return ResultCode::Error; //you can't write to the APK
            }

            // create all paths up to that directory.
            // its not an error if the path exists.
            if (Exists(resolvedPath.c_str()) && !IsDirectory(resolvedPath.c_str()))
            {
                return ResultCode::Error; // that path exists, but is not a directory.
            }

            // make directories from bottom to top.
            FixedMaxPath directoryPath = resolvedPath.RootPath();
            for (AZ::IO::PathView pathSegment : resolvedPath.RelativePath())
            {
                directoryPath /= pathSegment;
                mkdir(directoryPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                if (!IsDirectory(directoryPath.c_str()))
                {
                    return ResultCode::Error;
                }
            }

            return IsDirectory(resolvedPath.c_str()) ? ResultCode::Success : ResultCode::Error;
        }
    } // namespace IO
}//namespace AZ
