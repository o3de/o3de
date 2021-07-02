/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/functional.h>

namespace AZ
{
    namespace IO
    {
        bool LocalFileIO::IsDirectory(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            DWORD fileAttributes = GetFileAttributesA(resolvedPath);
            if (fileAttributes == INVALID_FILE_ATTRIBUTES)
            {
                return false;
            }

            return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }

        Result LocalFileIO::FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            AZ::OSString searchPattern;
            if ((resolvedPath[0] == 0) || (resolvedPath[1] == 0))
            {
                return ResultCode::Error; // not a valid path.
            }

            if ((strchr(resolvedPath, ':')) || (resolvedPath[0] == '\\') || (resolvedPath[0] == '/'))
            {
                // an absolute path was provided
                searchPattern = resolvedPath;
                AZStd::replace(searchPattern.begin(), searchPattern.end(), '/', '\\');
                searchPattern = RemoveTrailingSlash(searchPattern);
            }
            else
            {
                searchPattern = RemoveTrailingSlash(resolvedPath);
            }

            searchPattern += "\\*.*"; // use our own filtering function!

            WIN32_FIND_DATA findData;
            HANDLE hFind = FindFirstFile(searchPattern.c_str(), &findData);

            if (hFind != INVALID_HANDLE_VALUE)
            {
                // because the absolute path might actually be SHORTER than the alias ("c:/r/dev" -> "@devroot@"), we need to
                // use a static buffer here.
                char tempBuffer[AZ_MAX_PATH_LEN];
                do
                {
                    AZStd::string_view filenameView = findData.cFileName;
                    // Skip over the current directory and parent directory paths to prevent infinite recursion
                    if (filenameView == "." || filenameView == ".." || !NameMatchesFilter(findData.cFileName, filter))
                    {
                        continue;
                    }

                    AZ::OSString foundFilePath = CheckForTrailingSlash(resolvedPath);
                    foundFilePath += findData.cFileName;
                    AZStd::replace(foundFilePath.begin(), foundFilePath.end(), '\\', '/');

                    // if aliased, de-alias!
                    azstrcpy(tempBuffer, AZ_MAX_PATH_LEN, foundFilePath.c_str());
                    ConvertToAlias(tempBuffer, AZ_MAX_PATH_LEN);

                    if (!callback(tempBuffer))
                    {
                        //we are done
                        FindClose(hFind);
                        return ResultCode::Success;
                    }
                } while (FindNextFile(hFind, &findData));

                FindClose(hFind);
                return ResultCode::Success;
            }
            return ResultCode::Error;
        }

        int64_t ConvertUnixStatFileTimeToWindowsFileTime(int64_t timeValue)
        {
            // KB article on microsoft : https://support.microsoft.com/en-us/kb/167296
            // you need to adjust for the base time epoch difference between unix epoch and FILETIME epoch!
            // note that Int32x32To64 is a windows only function.
            // the magic numbers represent the difference in epoch between the windows and unix file time units
            // (in FILETIME units, which are 100-nanosecond intervals, starting on Jan 1, 1601 UTC.)
            // the first 10,000,000 is the number of 100-nanosecond intervals in 1 second
            // the second 116,444,736,000,000,000 is the number of nanoseconds between unix epoch start
            // which started on Jan 1, 1970 UTC (369 years worth of 100-nanosecond chunks), and the windows
            // filetime epoch.

            int64_t longTime = Int32x32To64(timeValue, 10000000) + 116444736000000000;
            return longTime;
        }

        Result LocalFileIO::CreatePath(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
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
                        CreateDirectoryA(buf.c_str(), NULL);
                        if (!IsDirectory(buf.c_str()))
                        {
                            return ResultCode::Error;
                        }
                    }
                }
                buf.push_back(resolvedPath[pos]);
            }

            return SystemFile::CreateDir(buf.c_str()) ? ResultCode::Success : ResultCode::Error;
        }

        bool LocalFileIO::ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength) const
        {
            char* result = _fullpath(absolutePath, path, maxLength);
            size_t len = ::strlen(absolutePath);
            if (len > 0)
            {
                // strip trailing slash
                if (absolutePath[len - 1] == '/' || absolutePath[len - 1] == '\\')
                {
                    absolutePath[len - 1] = 0;
                }

                // For some reason, at least on windows, _fullpath returns a lowercase drive letter even though other systems like Qt, use upper case.
                if (len > 2)
                {
                    if (absolutePath[1] == ':')
                    {
                        absolutePath[0] = (char)toupper(absolutePath[0]);
                    }
                }
            }
            return result != nullptr;
        }

        bool LocalFileIO::IsAbsolutePath(const char* path) const
        {
            char drive[16];
            _splitpath_s(path, drive, 16, nullptr, 0, nullptr, 0, nullptr, 0);
            return strlen(drive) > 0;
        }
    } // namespace IO
}//namespace AZ
