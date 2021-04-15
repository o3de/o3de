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
#include <Launcher.h>

#include <O3DEApplication_Mac.h>
#include <../Common/Apple/Launcher_Apple.h>
#include <../Common/UnixLike/Launcher_UnixLike.h>

#if AZ_TESTS_ENABLED

int main(int argc, char* argv[])
{
    // TODO: Implement for Mac
    return static_cast<int>(O3DELauncher::ReturnCode::ErrUnitTestNotSupported);
}
#else

int main(int argc, char* argv[])
{
    // Ensure the process is a foreground application. Must be done before creating the application.
    ProcessSerialNumber processSerialNumber = { 0, kCurrentProcess };
    TransformProcessType(&processSerialNumber, kProcessTransformToForegroundApplication);

    // Create a memory pool, a custom AppKit application, and a custom AppKit application delegate.
    NSAutoreleasePool* autoreleasePool = [[NSAutoreleasePool alloc] init];
    [O3DEApplication_Mac sharedApplication];
    [NSApp setDelegate: [[O3DEApplicationDelegate_Mac alloc] init]];

    // Register some default application behaviours
    [[NSUserDefaults standardUserDefaults] registerDefaults:
        [[NSDictionary alloc] initWithObjectsAndKeys:
            [NSNumber numberWithBool: FALSE], @"AppleMomentumScrollSupported",
            [NSNumber numberWithBool: FALSE], @"ApplePressAndHoldEnabled",
            nil]];

    // Launch the AppKit application and release the memory pool.
    [NSApp finishLaunching];
    [autoreleasePool release];

    // run the Open 3D Engine application
    using namespace O3DELauncher;

    PlatformMainInfo mainInfo;
    mainInfo.m_updateResourceLimits = IncreaseResourceLimits;
    mainInfo.m_appResourcesPath = GetAppResourcePath();

    bool ret = mainInfo.CopyCommandLine(argc, argv);

    ReturnCode status = ret ? 
        Run(mainInfo) : 
        ReturnCode::ErrCommandLine;

    return static_cast<int>(status);
}

#endif // AZ_TESTS_ENABLED
