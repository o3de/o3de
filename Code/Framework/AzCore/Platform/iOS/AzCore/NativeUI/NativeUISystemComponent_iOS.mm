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
    
        // There is no perfect solution because we do not know how the blocker thread handles the issue after the asserton.
        // We assume the asserted thread handles the issue well so the main thread is unblocked later.
        // If there is a crash in the asserted thread then the app just crashes. From user perspective it is  similar to an abort call.
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
            NSString* fileName = [NSString stringWithFormat:@"BlockingDialogDeadlock %@.txt", dateString];
            
            NSArray* allPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
            NSString* documentsDirectory = [allPaths objectAtIndex:0];
            NSString* pathForLog = [documentsDirectory stringByAppendingPathComponent:fileName];

            // Redirect NSLog messages to the file.
            freopen([pathForLog cStringUsingEncoding:NSASCIIStringEncoding], "a+", stderr);
            
            // These messages go to the file.
            NSLog(@"Main thread is locked (waits for a semaphore or else) when another thread requests a blocking popup display at %@."
                        @" This is likely an assertion. The popup might be failed to display, the app might crash."
                            , dateString);

            NSLog(@"Dialog title: %@", nsTitle);
            NSLog(@"Dialog message: %@\n", nsMessage);
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
            __block bool postActionPopup = false;

            NSString* nsTitle = [NSString stringWithUTF8String:title.c_str()];
            NSString* nsMessage = [NSString stringWithUTF8String:message.c_str()];
            
#if defined(CARBONATED)
            // these 3 can be used later after this call is over
            [nsTitle retain];
            [nsMessage retain];
            AZStd::vector<AZStd::string>* pOptionsCopy = new AZStd::vector<AZStd::string>(options);
            
            void (^DisplayBlockingDialogCommon)(void) = ^{
                UIAlertController* alert = [UIAlertController alertControllerWithTitle:nsTitle message:nsMessage preferredStyle:UIAlertControllerStyleAlert];
                
                if (postActionPopup)
                {
                    UIAlertAction* okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) { userSelection = [action.title UTF8String]; }];
                    [alert addAction:okAction];
                }
                else
                {
                    for (int i = 0; i < pOptionsCopy->size(); i++)
                    {
                        UIAlertAction* okAction = [UIAlertAction actionWithTitle:[NSString stringWithUTF8String:(*pOptionsCopy)[i].c_str()] style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) { userSelection = [action.title UTF8String]; }];
                        [alert addAction:okAction];
                    }
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
                
                [nsTitle release];
                [nsMessage release];
                delete pOptionsCopy;
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
                static constexpr int WaitMainThreadMs = 2000;
                while (dispatch_semaphore_wait(blockSem, DISPATCH_TIME_NOW))
                {
                    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0/30]];
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
                postActionPopup = true;  // there is no need to display all the buttons, the assert is likely already ignored
                OnDeadlock(nsTitle, nsMessage);
            }
            
            return userSelection;
        }
    }
}

