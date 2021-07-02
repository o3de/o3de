/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/NativeUI/NativeUISystemComponent.h>

#import <UIKit/UIKit.h>

namespace AZ
{
    namespace NativeUI
    {
        AZStd::string NativeUISystem::DisplayBlockingDialog(const AZStd::string& title, const AZStd::string& message, const AZStd::vector<AZStd::string>& options) const
        {
            if (m_mode == NativeUI::Mode::DISABLED)
            {
                return {};
            }

            __block AZStd::string userSelection = "";
            
            NSString* nsTitle = [NSString stringWithUTF8String:title.c_str()];
            NSString* nsMessage = [NSString stringWithUTF8String:message.c_str()];
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

            while (userSelection.empty())
            {
                CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0, TRUE);
            }
            
            return userSelection;
        }
    }
}
