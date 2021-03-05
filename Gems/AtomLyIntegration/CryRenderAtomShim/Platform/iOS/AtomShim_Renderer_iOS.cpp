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

#include "CryRenderOther_precompiled.h"
#import <UIKit/UIKit.h>

using NativeScreenType = UIScreen;
using NativeWindowType = UIWindow;

bool UIDeviceIsTablet()
{
    if([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad)
    {
        return true;
    }
    return false;
}

bool UIKitGetPrimaryPhysicalDisplayDimensions(int& o_widthPixels, int& o_heightPixels)
{
    UIScreen* nativeScreen = [UIScreen mainScreen];

    CGRect screenBounds = [nativeScreen bounds];
    CGFloat screenScale = [nativeScreen scale];

    o_widthPixels = static_cast<int>(screenBounds.size.width * screenScale);
    o_heightPixels = static_cast<int>(screenBounds.size.height * screenScale);
    
    const bool isScreenLandscape = o_widthPixels > o_heightPixels;
    UIInterfaceOrientation uiOrientation = UIInterfaceOrientationUnknown;
#if defined(__IPHONE_13_0) || defined(__TVOS_13_0)
    if(@available(iOS 13.0, tvOS 13.0, *))
    {
        UIWindow* foundWindow = nil;
        
        //Find the key window
        NSArray* windows = [[UIApplication sharedApplication] windows];
        for (UIWindow* window in windows)
        {
            if (window.isKeyWindow)
            {
                foundWindow = window;
                break;
            }
        }
        
        //Check if the key window is found
        if(foundWindow)
        {
            uiOrientation = foundWindow.windowScene.interfaceOrientation;
        }
        else
        {
            //If no key window is found create a temporary window in order to extract the orientation
            //This can happen as this function gets called before the renderer is initialized
            CGRect screenBounds = [[UIScreen mainScreen] bounds];
            UIWindow* tempWindow = [[UIWindow alloc] initWithFrame: screenBounds];
            uiOrientation = tempWindow.windowScene.interfaceOrientation;
            [tempWindow release];
        }
    }
#else
    uiOrientation = UIApplication.sharedApplication.statusBarOrientation;
#endif
    
    const bool isInterfaceLandscape = UIInterfaceOrientationIsLandscape(uiOrientation);
    if (isScreenLandscape != isInterfaceLandscape)
    {
        const int width = o_widthPixels;
        o_widthPixels = o_heightPixels;
        o_heightPixels = width;
    }
    
    return true;
}

