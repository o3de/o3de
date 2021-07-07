/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
