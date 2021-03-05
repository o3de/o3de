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

#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>


@interface LumberyardApplication_iOS : UIApplication
{
}
@end    // LumberyardApplication_iOS Interface

@implementation LumberyardApplication_iOS

- (void)touchesBegan: (NSSet<UITouch*>*)touches withEvent: (UIEvent*)event
{
    for (const UITouch* touch in touches)
    {
        AzFramework::RawInputNotificationBusIos::Broadcast(
            &AzFramework::RawInputNotificationBusIos::Events::OnRawTouchEventBegan, touch);
    }
}

- (void)touchesMoved: (NSSet<UITouch*>*)touches withEvent: (UIEvent*)event
{
    for (const UITouch* touch in touches)
    {
        AzFramework::RawInputNotificationBusIos::Broadcast(
            &AzFramework::RawInputNotificationBusIos::Events::OnRawTouchEventMoved, touch);
    }
}

- (void)touchesEnded: (NSSet<UITouch*>*)touches withEvent: (UIEvent*)event
{
    for (const UITouch* touch in touches)
    {
        AzFramework::RawInputNotificationBusIos::Broadcast(
            &AzFramework::RawInputNotificationBusIos::Events::OnRawTouchEventEnded, touch);

    }
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent: (UIEvent*)event
{
    // Active touches can be cancelled (as opposed to ended) for a variety of reasons, including:
    // - The active view being rotated to match the device orientation.
    // - The application resigning it's active status (eg. when receiving a message or phone call).
    // - Exceeding the max number of active touches tracked by the system (which as explained above
    //   is device dependent). For some reason this causes all active touches to be cancelled.
    // In any case, for the purposes of a game (or really any application that I can think of),
    // there really isn't any reason to distinguish between a touch ending or being cancelled.
    // They are mutually exclusive events, and both result in the touch being discarded by the
    // system.
    [self touchesEnded: touches withEvent: event];
}

@end // LumberyardApplication_iOS Implementation
