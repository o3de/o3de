/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/NativeUI/NativeUISystemComponent.h>

#import <AppKit/AppKit.h>

namespace
{
// NSAlertStyle enum constant names were changed in MacOS 10.12, but our min-spec is still 10.10
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 101200 // __MAC_10_12 may not be defined by all earlier sdks
    static const NSAlertStyle NSAlertStyleWarning = NSWarningAlertStyle;
#endif // __MAC_OS_VERSION_MAX_ALLOWED < __MAC_10_12
}

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

            __block NSModalResponse response = -1;
            
            auto showDialog = ^()
            {
                NSAlert* alert = [[NSAlert alloc] init];
                
                [alert setMessageText: [NSString stringWithUTF8String:title.c_str()]];
                [alert setInformativeText: [NSString stringWithUTF8String:message.c_str()]];
                
                for(int i = 0; i < options.size(); i++)
                {
                    [alert addButtonWithTitle: [NSString stringWithUTF8String:options[i].c_str()]];
                }
                
                [alert setAlertStyle: NSAlertStyleWarning];
                
                response = [alert runModal];
                
                [alert release];
            };
            
            // Cannot invoke an alert from a thread that's not the main thread.
            if (!NSThread.isMainThread)
            {
                // Dispatch_sync is necessary becuase we want this thread to wait till the main thread recieves user input for the alert.
                // We also can't release the alert object below till it's done.
                dispatch_sync(dispatch_get_main_queue(), showDialog);
            }
            else
            {
                showDialog();
            }
            
            if (response > 0)
            {
                return options[response - NSAlertFirstButtonReturn];
            }
            
            return "";
        }
    }
}
