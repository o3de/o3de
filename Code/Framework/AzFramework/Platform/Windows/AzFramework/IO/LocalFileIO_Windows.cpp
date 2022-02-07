/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>

namespace AZ::IO
{
    Result LocalFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
    {
        AZ::IO::FixedMaxPath resolvedSourcePath;
        ResolvePath(resolvedSourcePath, sourceFilePath);
        AZ::IO::FixedMaxPath resolvedDestPath;
        ResolvePath(resolvedDestPath, destinationFilePath);

        AZStd::fixed_wstring<AZ::IO::MaxPathLength> resolvedSourcePathW;
        AZStd::fixed_wstring<AZ::IO::MaxPathLength> resolvedDestPathW;
        AZStd::to_wstring(resolvedSourcePathW, resolvedSourcePath.Native());
        AZStd::to_wstring(resolvedDestPathW, resolvedDestPath.Native());

        return ::CopyFileW(resolvedSourcePathW.c_str(), resolvedDestPathW.c_str(), false) != 0 ? ResultCode::Success : ResultCode::Error;
    }
}//namespace AZ::IO
