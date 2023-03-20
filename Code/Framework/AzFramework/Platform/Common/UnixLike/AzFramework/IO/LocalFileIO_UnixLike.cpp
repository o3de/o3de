/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/functional.h>

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

            // Use standard C++ method of file copy.
            {
                std::ifstream src(resolvedSourcePath.c_str(), std::ios::binary);
                if (src.fail())
                {
                    return ResultCode::Error;
                }
                std::ofstream dst(resolvedDestPath.c_str(), std::ios::binary);

                if (dst.fail())
                {
                    return ResultCode::Error;
                }
                dst << src.rdbuf();
            }
            return ResultCode::Success;
        }

        Result LocalFileIO::FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback)
        {
            FixedMaxPath resolvedPath;
            ResolvePath(resolvedPath, filePath);
            resolvedPath = resolvedPath.LexicallyNormal();

            DIR* dir = opendir(resolvedPath.c_str());

            if (dir != nullptr)
            {
                errno = 0;
                struct dirent* entry = readdir(dir);

                // List all the other files in the directory.
                while (entry != nullptr)
                {
                    AZStd::string_view filenameView = entry->d_name;
                    // Skip over the current and parent directory paths
                    if (filenameView != "." && filenameView != ".." && AZStd::wildcard_match(filter, entry->d_name))
                    {
                        if (!callback((resolvedPath / filenameView).c_str()))
                        {
                            break;
                        }
                    }
                    entry = readdir(dir);
                }

                closedir(dir);
                return (errno != 0) ? ResultCode::Error : ResultCode::Success;
            }
            else
            {
                return ResultCode::Error;
            }
        }

        Result LocalFileIO::CreatePath(const char* filePath)
        {
            FixedMaxPath resolvedPath;
            ResolvePath(resolvedPath, filePath);

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
} // namespace AZ
