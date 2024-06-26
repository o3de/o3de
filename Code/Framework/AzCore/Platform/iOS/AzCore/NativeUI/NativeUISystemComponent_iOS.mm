/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/NativeUI/NativeUISystemComponent.h>

#import <UIKit/UIKit.h>

#if defined(CARBONATED)
#include <chrono>
#endif

namespace AZ
{
    namespace NativeUI
    {
#if defined(CARBONATED)
        // TODO : implement for platform
        bool NativeUISystem::IsDisplayingBlockingDialog() const
        {
            return false;
        }
    
        template <
            class result_t   = std::chrono::milliseconds,
            class clock_t    = std::chrono::steady_clock,
            class duration_t = std::chrono::milliseconds
        >
        auto TimeSince(std::chrono::time_point<clock_t, duration_t> const& start)
        {
            return std::chrono::duration_cast<result_t>(clock_t::now() - start);
        }
    
        static void OnDeadlock(NSString* nsTitle, NSString* nsMessage)
        {
            // Save the popup data into a file and abort the application.
            // The file will be located in the Documents dir and can be fetched to Mac as a part of the app container.
            NSDate* date = [NSDate date];
            NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
            NSTimeZone* destinationTimeZone = [NSTimeZone systemTimeZone];
            formatter.timeZone = destinationTimeZone;
            [formatter setDateStyle:NSDateFormatterLongStyle];
            [formatter setDateFormat:@"MM-dd-yyyy hh.mma"];
            NSString* dateString = [formatter stringFromDate:date];
            
            // Include the date in the file name to not overwrite previous artifacts.
            NSString* fileName = [NSString stringWithFormat:@"BlockingDialogAborted %@.txt", dateString];
            
            NSArray* allPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
            NSString* documentsDirectory = [allPaths objectAtIndex:0];
            NSString* pathForLog = [documentsDirectory stringByAppendingPathComponent:fileName];

            // Redirect NSLog messages to the file.
            freopen([pathForLog cStringUsingEncoding:NSASCIIStringEncoding], "a+", stderr);
            
            // These messages go to the file.
            NSLog(@"BlockingDialog failed to display at %@", dateString);
            
            NSLog(@"Title: %@", nsTitle);
            NSLog(@"Message: %@\n", nsMessage);
            
            NSLog(@"App terminated");
            
            exit(0);
        }
#endif
    
        AZStd::string NativeUISystem::DisplayBlockingDialog(const AZStd::string& title, const AZStd::string& message, const AZStd::vector<AZStd::string>& options) const
        {
            if (m_mode == NativeUI::Mode::DISABLED)
            {
                return {};
            }

            __block AZStd::string userSelection = "";
            
            __block bool mainThreadRunning = false;
            
            NSString* nsTitle = [NSString stringWithUTF8String:title.c_str()];
            NSString* nsMessage = [NSString stringWithUTF8String:message.c_str()];
            
#if defined(CARBONATED)
            void (^DisplayBlockingDialogCommon)(void) = ^{
                UIAlertController* alert = [UIAlertController alertControllerWithTitle:nsTitle message:nsMessage preferredStyle:UIAlertControllerStyleAlert];
                
                for (int i = 0; i < options.size(); i++)
                {
                    UIAlertAction* okAction = [UIAlertAction actionWithTitle:[NSString stringWithUTF8String:options[i].c_str()] style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) { userSelection = [action.title UTF8String]; }];
                    [alert addAction:okAction];
                }
                
                UIWindow* foundWindow = nil;
                NSArray* windows = [[UIApplication sharedApplication] windows];
                for (UIWindow* window in windows)
                {
                    if (window.isKeyWindow)
                    {
                        foundWindow = window;
                        break;
                    }
                }
                UIViewController* rootViewController = foundWindow ? foundWindow.rootViewController : nil;
                if (rootViewController)
                {
                    mainThreadRunning = true;
                    [rootViewController presentViewController:alert animated:YES completion:nil];
                }
                else
                {
                    userSelection = "BlockingDialog Error";
                }
            };

            if (!NSThread.isMainThread)
            {
                __block dispatch_semaphore_t blockSem = dispatch_semaphore_create(0);
                
                const auto start = std::chrono::steady_clock::now();
                
                // Dialog boxes need to be triggered from the main thread.
                CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopDefaultMode, ^(){
                    DisplayBlockingDialogCommon();
                    while (userSelection.empty())
                    {
                        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE);
                    }
                    dispatch_semaphore_signal(blockSem);
                });
                
                // Wait till the user responds to the message box.
                static constexpr int WaitMainThreadMs = 3000;
                while (dispatch_semaphore_wait(blockSem, DISPATCH_TIME_NOW))
                {
                    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1]];
                    // If we did not enter the main thread and the given time elapsed, exit the wait cycle
                    if (!mainThreadRunning && TimeSince(start).count() > WaitMainThreadMs)
                    {
                        userSelection = "Timeout";
                        break;
                    }
                }
            }
            else
            {
                DisplayBlockingDialogCommon();
            }
#else
            UIAlertController* alert = [UIAlertController alertControllerWithTitle:nsTitle message:nsMessage preferredStyle:UIAlertControllerStyleAlert];
            
            for (int i = 0; i < options.size(); i++)
            {
                UIAlertAction* okAction = [UIAlertAction actionWithTitle:[NSString stringWithUTF8String:options[i].c_str()] style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) { userSelection = [action.title UTF8String]; }];
                [alert addAction:okAction];
            }
            
            UIWindow* foundWindow = nil;
#if defined(__IPHONE_13_0) || defined(__TVOS_13_0)
            if(@available(iOS 13.0, tvOS 13.0, *))
            {
                NSArray* windows = [[UIApplication sharedApplication] windows];
                for (UIWindow* window in windows)
                {
                    if (window.isKeyWindow)
                    {
                        foundWindow = window;
                        break;
                    }
                }
            }
#else
            foundWindow = [[UIApplication sharedApplication] keyWindow];
#endif
            
            UIViewController* rootViewController = foundWindow ? foundWindow.rootViewController : nullptr;
            if (!rootViewController)
            {
                return "";
            }
            
            if (!NSThread.isMainThread)
            {
                __block dispatch_semaphore_t blockSem = dispatch_semaphore_create(0);
                // Dialog boxes need to be triggered from the main thread.
                CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopDefaultMode, ^(){
                    [rootViewController presentViewController:alert animated:YES completion:nil];
                    while (userSelection.empty())
                    {
                        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE);
                    }
                    dispatch_semaphore_signal(blockSem);
                });
                // Wait till the user responds to the message box.
                dispatch_semaphore_wait(blockSem, DISPATCH_TIME_FOREVER);
            }
            else
            {
                [rootViewController presentViewController:alert animated:YES completion:nil];
            }
#endif
            while (userSelection.empty())
            {
                CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE);
            }
            
            if (!NSThread.isMainThread && !mainThreadRunning)
            {
                // Probably, deadlock detected.
                OnDeadlock(nsTitle, nsMessage);
            }
            
            return userSelection;
        }
    }
}
