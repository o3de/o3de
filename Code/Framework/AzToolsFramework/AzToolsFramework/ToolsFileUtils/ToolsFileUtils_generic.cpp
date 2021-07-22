/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
