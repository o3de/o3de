/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#import <RHI/MetalViewController.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <RHI/MetalView_Platform.h>

@implementation RHIMetalViewController

- (BOOL)prefersStatusBarHidden
{
    return TRUE;
}

- (void)windowWillClose:(NSNotification *)notification
{
    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
}

- (void)windowDidResize:(NSNotification *)notification
{
    RHIMetalView* metalView = reinterpret_cast<RHIMetalView*>([notification.object contentView]);
    CGSize drawableSize = metalView.metalLayer.drawableSize;
    AzFramework::WindowNotificationBus::Event(notification.object, &AzFramework::WindowNotificationBus::Events::OnWindowResized, drawableSize.width, drawableSize.height);
    AzFramework::WindowNotificationBus::Event(notification.object, &AzFramework::WindowNotificationBus::Events::OnResolutionChanged, drawableSize.width, drawableSize.height);
}
@end


