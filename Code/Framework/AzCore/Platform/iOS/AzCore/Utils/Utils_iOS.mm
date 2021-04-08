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

    AZStd::optional<AZ::IO::FixedMaxPathString> GetDevWriteStoragePath()
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
