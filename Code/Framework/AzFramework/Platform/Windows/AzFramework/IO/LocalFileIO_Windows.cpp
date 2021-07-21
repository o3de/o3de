/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/IO/SystemFile.h>

namespace AZ
{
    namespace IO
    {

        Result LocalFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
        {
            char resolvedSourcePath[AZ_MAX_PATH_LEN];
            ResolvePath(sourceFilePath, resolvedSourcePath, AZ_MAX_PATH_LEN);
            char resolvedDestPath[AZ_MAX_PATH_LEN];
            ResolvePath(destinationFilePath, resolvedDestPath, AZ_MAX_PATH_LEN);

            if (::CopyFileA(resolvedSourcePath, resolvedDestPath, false) == 0)
            {
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }
    } // namespace IO
}//namespace AZ
