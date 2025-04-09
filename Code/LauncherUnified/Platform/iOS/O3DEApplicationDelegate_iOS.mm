/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Launcher.h>

#include <../Common/Apple/Launcher_Apple.h>
#include <../Common/UnixLike/Launcher_UnixLike.h>

#include <AzFramework/API/ApplicationAPI_Platform.h>

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

#include <AzCore/Utils/SystemUtilsApple_Platform.h>

#import <UIKit/UIKit.h>


namespace
{
    const char* GetAppWriteStoragePath()
    {
        static char pathToApplicationPersistentStorage[AZ::IO::MaxPathLength];

        // Unlike Mac where we have unrestricted access to the filesystem, iOS apps are sandboxed such
        // that you can only access a pre-defined set of directories.
        // https://developer.apple.com/library/mac/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/FileSystemOverview/FileSystemOverview.html
        AZ::SystemUtilsApple::GetPathToUserApplicationSupportDirectory(AZStd::span(pathToApplicationPersistentStorage));
        return pathToApplicationPersistentStorage;
    }
}


@interface O3DEApplicationDelegate_iOS : NSObject<UIApplicationDelegate>
{
}
@end    // O3DEApplicationDelegate_iOS Interface

@implementation O3DEApplicationDelegate_iOS


- (int)runO3DEApplication
{
#if AZ_TESTS_ENABLED

    // TODO: iOS needs to determine how to get around being able to run in monolithic mode (ie no dynamic modules)
    return static_cast<int>(ReturnCode::ErrUnitTestNotSupported);

#else
    using namespace O3DELauncher;

    PlatformMainInfo mainInfo;
    mainInfo.m_updateResourceLimits = IncreaseResourceLimits;
    mainInfo.m_appResourcesPath = GetAppResourcePath();
    mainInfo.m_appWriteStoragePath = GetAppWriteStoragePath();
    mainInfo.m_additionalVfsResolution = "\t- Check that usbmuxconnect is running and not reporting any errors when connecting to localhost";

    NSArray* commandLine = [[NSProcessInfo processInfo] arguments];

    for (size_t argIndex = 0; argIndex < [commandLine count]; ++argIndex)
    {
        NSString* arg = commandLine[argIndex];
        if (!mainInfo.AddArgument([arg UTF8String]))
        {
            return static_cast<int>(ReturnCode::ErrCommandLine);
        }
    }

    ReturnCode status = Run(mainInfo);
    return static_cast<int>(status);
#endif // AZ_TESTS_ENABLED
}

- (void)launchO3DEApplication
{
    const int exitCode = [self runO3DEApplication];
    exit(exitCode);
}

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    // prevent the o3de runtime from running when launched in a xctest environment, otherwise the
    // testing framework will kill the "app" due to the lengthy bootstrap process
    if ([[NSProcessInfo processInfo] environment][@"XCTestConfigurationFilePath"] == nil)
    {
        [self performSelector:@selector(launchO3DEApplication) withObject:nil afterDelay:0.0];
    }
    return YES;
}

- (void)applicationWillResignActive:(UIApplication*)application
{
    AzFramework::IosLifecycleEvents::Bus::Broadcast(
        &AzFramework::IosLifecycleEvents::Bus::Events::OnWillResignActive);
}

- (void)applicationDidEnterBackground:(UIApplication*)application
{
    AzFramework::IosLifecycleEvents::Bus::Broadcast(
        &AzFramework::IosLifecycleEvents::Bus::Events::OnDidEnterBackground);
}

- (void)applicationWillEnterForeground:(UIApplication*)application
{
    AzFramework::IosLifecycleEvents::Bus::Broadcast(
        &AzFramework::IosLifecycleEvents::Bus::Events::OnWillEnterForeground);
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
    AzFramework::IosLifecycleEvents::Bus::Broadcast(
        &AzFramework::IosLifecycleEvents::Bus::Events::OnDidBecomeActive);
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    AzFramework::IosLifecycleEvents::Bus::Broadcast(
        &AzFramework::IosLifecycleEvents::Bus::Events::OnWillTerminate);
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
    AzFramework::IosLifecycleEvents::Bus::Broadcast(
        &AzFramework::IosLifecycleEvents::Bus::Events::OnDidReceiveMemoryWarning);
}

@end // O3DEApplicationDelegate_iOS Implementation
