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

#include <../Common/Apple/Launcher_Apple.h>
#include <../Common/UnixLike/Launcher_UnixLike.h>

#include <AzFramework/API/ApplicationAPI_Platform.h>

#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

#include <CrySystem/SystemUtilsApple.h>

#import <UIKit/UIKit.h>


namespace
{
    const char* GetAppWriteStoragePath()
    {
        static char pathToApplicationPersistentStorage[AZ_MAX_PATH_LEN] = { 0 };

        // Unlike Mac where we have unrestricted access to the filesystem, iOS apps are sandboxed such
        // that you can only access a pre-defined set of directories.
        // https://developer.apple.com/library/mac/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/FileSystemOverview/FileSystemOverview.html
        SystemUtilsApple::GetPathToUserApplicationSupportDirectory(pathToApplicationPersistentStorage, AZ_MAX_PATH_LEN);
        return pathToApplicationPersistentStorage;
    }
}


@interface LumberyardApplicationDelegate_iOS : NSObject<UIApplicationDelegate>
{
}
@end    // LumberyardApplicationDelegate_iOS Interface

@implementation LumberyardApplicationDelegate_iOS


- (int)runLumberyardApplication
{
#if AZ_TESTS_ENABLED

    // TODO: iOS needs to determine how to get around being able to run in monolithic mode (ie no dynamic modules)
    return static_cast<int>(ReturnCode::ErrUnitTestNotSupported);

#else
    using namespace LumberyardLauncher;

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

- (void)launchLumberyardApplication
{
    const int exitCode = [self runLumberyardApplication];
    exit(exitCode);
}

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    // prevent the lumberyard runtime from running when launched in a xctest environment, otherwise the
    // testing framework will kill the "app" due to the lengthy bootstrap process
    if ([[NSProcessInfo processInfo] environment][@"XCTestConfigurationFilePath"] == nil)
    {
        [self performSelector:@selector(launchLumberyardApplication) withObject:nil afterDelay:0.0];
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

@end // LumberyardApplicationDelegate_iOS Implementation
