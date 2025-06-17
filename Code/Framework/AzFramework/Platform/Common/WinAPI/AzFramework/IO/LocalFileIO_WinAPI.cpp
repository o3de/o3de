/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    namespace IO
    {
        Result LocalFileIO::FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback)
        {
            FixedMaxPath resolvedPath;
            ResolvePath(resolvedPath, filePath);

            if (resolvedPath.empty())
            {
                return ResultCode::Error; // not a valid path.
            }

            // Normalize the path to normalize the path separators
            resolvedPath = resolvedPath.LexicallyNormal();

            WIN32_FIND_DATA findData;
            AZStd::fixed_wstring<AZ::IO::MaxPathLength> searchPatternW;
            // Filter all files
            AZStd::to_wstring(searchPatternW, (resolvedPath / "*.*").Native());

            if (HANDLE hFind = FindFirstFileW(searchPatternW.c_str(), &findData); hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    AZ::IO::FixedMaxPath fileName;
                    AZStd::to_string(fileName.Native(), findData.cFileName);
                    // Skip over the current directory and parent directory paths to prevent infinite recursion
                    if (fileName == "." || fileName == ".." || !AZStd::wildcard_match(filter, fileName.Native()))
                    {
                        continue;
                    }

                    // Concatenate the fileName to the resolvedPath
                    if (!callback((resolvedPath / fileName).c_str()))
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
            // the magic numbers represent the difference in epoch between the windows and unix file time units
            // (in FILETIME units, which are 100-nanosecond intervals, starting on Jan 1, 1601 UTC.)
            // the first 10,000,000 is the number of 100-nanosecond intervals in 1 second
            // the second 116,444,736,000,000,000 is the number of nanoseconds between unix epoch start
            // which started on Jan 1, 1970 UTC (369 years worth of 100-nanosecond chunks), and the windows
            // filetime epoch.

            int64_t longTime = (timeValue * 10000000LL) + 116444736000000000LL;
            return longTime;
        }

        Result LocalFileIO::CreatePath(const char* filePath)
        {
            FixedMaxPath resolvedPath;
            ResolvePath(resolvedPath, filePath);
            resolvedPath = resolvedPath.LexicallyNormal();

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
                CreateDirectoryA(directoryPath.c_str(), nullptr);
                if (!IsDirectory(directoryPath.c_str()))
                {
                    return ResultCode::Error;
                }
            }

            return IsDirectory(directoryPath.c_str()) ? ResultCode::Success : ResultCode::Error;
        }
    } // namespace IO
}//namespace AZ
