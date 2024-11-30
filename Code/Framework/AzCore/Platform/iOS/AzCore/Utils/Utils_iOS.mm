/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/fixed_string.h>

#import <Foundation/Foundation.h>


namespace AZ::Utils
{
    AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultAppRootPath()
    {
        const char* pathToResources = [[[NSBundle mainBundle] resourcePath] UTF8String];
        return AZ::IO::FixedMaxPathString::format("%s/assets", pathToResources);
    }

    AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultDevWriteStoragePath()
    {
        NSArray* appSupportDirectoryPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        if ([appSupportDirectoryPaths count] == 0)
        {
            return AZStd::nullopt;
        }

        NSString* appSupportDir = static_cast<NSString*>([appSupportDirectoryPaths objectAtIndex:0]);
        if (!appSupportDir)
        {
            return AZStd::nullopt;
        }

        const char* src = [appSupportDir UTF8String];
        const size_t srcLen = strlen(src);
        if (srcLen > AZ::IO::MaxPathLength - 1)
        {
            return AZStd::nullopt;
        }

        return AZStd::make_optional<AZ::IO::FixedMaxPathString>(src);
    }

}
