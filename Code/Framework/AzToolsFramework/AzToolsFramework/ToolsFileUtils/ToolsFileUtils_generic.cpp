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

#include "ToolsFileUtils.h"
#include <sys/stat.h>
#include <utime.h>

#include <QStorageInfo>

namespace AzToolsFramework
{
    namespace ToolsFileUtils
    {
        bool SetModificationTime(const char* const filename, AZ::u64 modificationTime)
        {
            struct stat statResult;
            if (stat(filename, &statResult) != 0)
            {
                return false;
            }

            struct utimbuf puttime;
            puttime.modtime = modificationTime;
            puttime.actime = static_cast<AZ::u64>(statResult.st_ctime);

            if (utime(filename, &puttime) == 0)
            {
                return true;
            }

            return false;
        }

        bool GetFreeDiskSpace(const QString& path, qint64& outFreeDiskSpace)
        {
            QStorageInfo storageInfo(path);
            outFreeDiskSpace = storageInfo.bytesFree();

            return outFreeDiskSpace >= 0;
        }
    }
}