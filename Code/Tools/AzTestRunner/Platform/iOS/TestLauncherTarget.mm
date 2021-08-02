/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#import <XCTest/XCTest.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Utils/Utils.h>
#include "aztestrunner.h"

constexpr size_t s_maxArgCount = 8;
constexpr size_t s_maxArgLength = 256;

namespace AzTestRunner
{
    void set_quiet_mode()
    {
    }

    const char* get_current_working_directory()
    {
        static char cwd_buffer[AZ_MAX_PATH_LEN] = { '\0' };

        AZ::Utils::ExecutablePathResult result = AZ::Utils::GetExecutableDirectory(cwd_buffer, AZ_ARRAY_SIZE(cwd_buffer));
        AZ_Assert(result == AZ::Utils::ExecutablePathResult::Success, "Error retrieving executable path");

        return static_cast<const char*>(cwd_buffer);
    }

    void pause_on_completion()
    {
    }
}

@interface TestLauncherTarget : XCTestCase
    @property(strong) NSArray* commandLineArgs;
    @property size_t argCount;
    @property bool testSuccessful;
@end

@implementation TestLauncherTarget

-(void) setUp
{
    NSLog(@"TEST STARTED");
    self.commandLineArgs = [[NSProcessInfo processInfo] arguments];
    self.argCount = [self.commandLineArgs count];
    self.testSuccessful = false;
}

-(void) tearDown
{
    if (self.testSuccessful)
    {
        NSLog(@"TEST SUCCEEDED");
    }
    else
    {
        NSLog(@"TEST FAILED");
    }
    
}

-(void) testLibrary
{
    char args[s_maxArgCount][s_maxArgLength] = { {'\0'} };
    char *argv[s_maxArgCount];
    size_t argc = self.argCount;
    
    for (size_t argIndex = 0; argIndex < s_maxArgCount && argIndex < argc; ++argIndex)
    {
        const char* arg = [self.commandLineArgs[argIndex] UTF8String];
        azstrncpy(args[argIndex], s_maxArgLength, arg, strlen(arg));
        argv[argIndex] = args[argIndex];
    }
    
    int status = AzTestRunner::wrapped_main(argc, argv);
    self.testSuccessful = (status == 0);
    XCTAssert(self.testSuccessful);
}

@end
