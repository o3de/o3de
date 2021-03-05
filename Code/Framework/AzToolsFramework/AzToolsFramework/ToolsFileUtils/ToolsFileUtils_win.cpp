/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/PlatformIncl.h>
#include "ToolsFileUtils.h"

namespace AzToolsFramework
{
    namespace ToolsFileUtils
    {

        //converts AZ::u64 to FILETIME
        FILETIME Time64BitToFiletime(AZ::u64 time)
        {
            FILETIME fileTime;
            fileTime.dwLowDateTime = static_cast<DWORD>(time & 0xFFFFFFFF);
            fileTime.dwHighDateTime = static_cast<DWORD>(time >> 32);
            return fileTime;
        }

        bool SetModificationTime(const char* const filename, AZ::u64 modificationTime)
        {
            FILETIME modificationFileTime;
            modificationFileTime = Time64BitToFiletime(modificationTime);

            const HANDLE hf = CreateFileA(filename, FILE_WRITE_ATTRIBUTES, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
            if (hf != INVALID_HANDLE_VALUE)
            {
                if (SetFileTime(hf, nullptr, nullptr, &modificationFileTime))
                {
                    if (CloseHandle(hf))
                    {
                        return true;
                    }
                }

                CloseHandle(hf);
            }
            return false;
        }

        bool GetFreeDiskSpace(const QString& path, qint64& outFreeDiskSpace)
        {
            ULARGE_INTEGER freeBytesAvailableToCaller;
            BOOL result = GetDiskFreeSpaceExW(path.toStdWString().c_str(), &freeBytesAvailableToCaller, nullptr, nullptr);

            outFreeDiskSpace = freeBytesAvailableToCaller.QuadPart;

            return result != 0;
        }
    }
}
