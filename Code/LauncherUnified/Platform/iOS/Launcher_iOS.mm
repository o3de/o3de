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

#import <UIKit/UIKit.h>


int main(int argc, char* argv[])
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    UIApplicationMain(argc,
                      argv,
                      @"O3DEApplication_iOS",
                      @"O3DEApplicationDelegate_iOS");
    [pool release];
    return 0;
}

void CVar_OnViewportPosition([[maybe_unused]] const AZ::Vector2& value) {}
