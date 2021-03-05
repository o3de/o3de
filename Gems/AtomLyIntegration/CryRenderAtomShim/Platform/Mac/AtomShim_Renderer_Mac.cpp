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
#import <AppKit/AppKit.h>

bool UIDeviceIsTablet()
{
    return false;
}

bool UIKitGetPrimaryPhysicalDisplayDimensions(int& o_widthPixels, int& o_heightPixels)
{
    NSScreen* nativeScreen = [NSScreen mainScreen];
    CGRect screenBounds = [nativeScreen frame];
    CGFloat screenScale = [nativeScreen backingScaleFactor];
    o_widthPixels = static_cast<int>(screenBounds.size.width * screenScale);
    o_heightPixels = static_cast<int>(screenBounds.size.height * screenScale);
    return true;
}


