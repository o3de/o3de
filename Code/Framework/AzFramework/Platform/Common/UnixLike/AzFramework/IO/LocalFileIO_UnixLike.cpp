/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/functional.h>

namespace AZ
{
    namespace IO
    {
        bool LocalFileIO::IsDirectory(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN] = {0};
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            struct stat result;
            if (stat(resolvedPath, &result) == 0)
            {
                return S_ISDIR(result.st_mode);
            }
            return false;
        }

        Result LocalFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
        {
            char resolvedSourceFilePath[AZ_MAX_PATH_LEN] = {0};
            ResolvePath(sourceFilePath, resolvedSourceFilePath, AZ_MAX_PATH_LEN);

            char resolvedDestinationFilePath[AZ_MAX_PATH_LEN] = {0};
            ResolvePath(destinationFilePath, resolvedDestinationFilePath, AZ_MAX_PATH_LEN);

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
            char resolvedPath[AZ_MAX_PATH_LEN] = {0};
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            AZ::OSString withoutSlash = RemoveTrailingSlash(resolvedPath);
            DIR* dir = opendir(withoutSlash.c_str());

            if (dir != nullptr)
            {
                // because the absolute path might actually be SHORTER than the alias ("c:/r/dev" -> "@devroot@"), we need to
                // use a static buffer here.
                char tempBuffer[AZ_MAX_PATH_LEN];

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
                        // if aliased, dealias!
                        azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, foundFilePath.c_str());
                        ConvertToAlias(tempBuffer, AZ_MAX_PATH_LEN);

                        if (!callback(tempBuffer))
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
            char resolvedPath[AZ_MAX_PATH_LEN] = {0};
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            // create all paths up to that directory.
            // its not an error if the path exists.
            if ((Exists(resolvedPath)) && (!IsDirectory(resolvedPath)))
            {
                return ResultCode::Error; // that path exists, but is not a directory.
            }

            // make directories from bottom to top.
            AZ::OSString buf;
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

        bool LocalFileIO::IsAbsolutePath(const char* path) const
        {
            return path && path[0] == '/';
        }

        bool LocalFileIO::ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength) const
        {
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
} // namespace AZ
