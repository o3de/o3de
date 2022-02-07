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
            char resolvedSourceFilePath[AZ::IO::MaxPathLength] = {0};
            ResolvePath(sourceFilePath, resolvedSourceFilePath, AZ::IO::MaxPathLength);

            char resolvedDestinationFilePath[AZ::IO::MaxPathLength] = {0};
            ResolvePath(destinationFilePath, resolvedDestinationFilePath, AZ::IO::MaxPathLength);

            // Use standard C++ method of file copy.
            {
                std::ifstream  src(resolvedSourceFilePath, std::ios::binary);
                if (src.fail())
                {
                    return ResultCode::Error;
                }
                std::ofstream  dst(resolvedDestinationFilePath, std::ios::binary);

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
            char resolvedPath[AZ::IO::MaxPathLength] = {0};
            ResolvePath(filePath, resolvedPath, AZ::IO::MaxPathLength);

            AZStd::string withoutSlash = RemoveTrailingSlash(resolvedPath);
            DIR* dir = opendir(withoutSlash.c_str());

            if (dir != nullptr)
            {
                AZ::IO::FixedMaxPath tempBuffer;

                errno = 0;
                struct dirent* entry = readdir(dir);

                // List all the other files in the directory.
                while (entry != nullptr)
                {
                    AZStd::string_view filenameView = entry->d_name;
                    // Skip over the current and parent directory paths
                    if (filenameView != "." && filenameView != ".." && NameMatchesFilter(entry->d_name, filter))
                    {
                        AZStd::string foundFilePath = CheckForTrailingSlash(resolvedPath);
                        foundFilePath += entry->d_name;
                        // if aliased, dealias!
                        ConvertToAlias(tempBuffer, AZ::IO::PathView{ foundFilePath });

                        if (!callback(tempBuffer.c_str()))
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
            char resolvedPath[AZ::IO::MaxPathLength] = {0};
            ResolvePath(filePath, resolvedPath, AZ::IO::MaxPathLength);

            // create all paths up to that directory.
            // its not an error if the path exists.
            if ((Exists(resolvedPath)) && (!IsDirectory(resolvedPath)))
            {
                return ResultCode::Error; // that path exists, but is not a directory.
            }

            // make directories from bottom to top.
            AZStd::string buf;
            size_t pathLength = strlen(resolvedPath);
            buf.reserve(pathLength);
            for (size_t pos = 0; pos < pathLength; ++pos)
            {
                if ((resolvedPath[pos] == '\\') || (resolvedPath[pos] == '/'))
                {
                    if (pos > 0)
                    {
                        mkdir(buf.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                        if (!IsDirectory(buf.c_str()))
                        {
                            return ResultCode::Error;
                        }
                    }
                }
                buf.push_back(resolvedPath[pos]);
            }

            mkdir(buf.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            return IsDirectory(resolvedPath) ? ResultCode::Success : ResultCode::Error;
        }
    } // namespace IO
} // namespace AZ
