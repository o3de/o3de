/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "Atom_RHI_Metal_precompiled.h"
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
}
@end


