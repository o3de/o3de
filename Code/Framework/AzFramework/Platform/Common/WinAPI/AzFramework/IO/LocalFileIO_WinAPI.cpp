/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    namespace IO
    {
        Result LocalFileIO::FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            AZStd::string searchPattern;
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
            AZStd::wstring searchPatternW;
            AZStd::to_wstring(searchPatternW, searchPattern.c_str());
            HANDLE hFind = FindFirstFileW(searchPatternW.c_str(), &findData);

            if (hFind != INVALID_HANDLE_VALUE)
            {
                // because the absolute path might actually be SHORTER than the alias ("D:/o3de" -> "@engroot@"), we need to
                // use a static buffer here.
                char tempBuffer[AZ_MAX_PATH_LEN];
                do
                {
                    AZStd::string fileName;
                    AZStd::to_string(fileName, findData.cFileName);
                    AZStd::string_view filenameView = fileName;
                    // Skip over the current directory and parent directory paths to prevent infinite recursion
                    if (filenameView == "." || filenameView == ".." || !NameMatchesFilter(fileName.c_str(), filter))
                    {
                        continue;
                    }

                    AZStd::string foundFilePath = CheckForTrailingSlash(resolvedPath);
                    foundFilePath += fileName;
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
            AZStd::string buf;
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
    } // namespace IO
}//namespace AZ
