/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
