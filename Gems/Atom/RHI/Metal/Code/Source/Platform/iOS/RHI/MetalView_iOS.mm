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

#include <RHI/MetalView_iOS.h>

@implementation RHIMetalView (PlatformImpl)

- (id)initWithFrame: (CGRect)frame
              scale: (CGFloat)scale
             device: (id<MTLDevice>)device
{
    if ((self = [super initWithFrame: frame]))
    {
        self.metalLayer = (CAMetalLayer*)self.layer;
        self.autoresizingMask = (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);
        if ([self respondsToSelector : @selector(contentScaleFactor)])
        {
            self.contentScaleFactor = scale;
        }
        self.multipleTouchEnabled = TRUE;
        self.metalLayer.device = device;
        self.metalLayer.framebufferOnly = TRUE;
        self.metalLayer.opaque = YES;
        self.metalLayer.drawsAsynchronously = TRUE;
        self.metalLayer.presentsWithTransaction = FALSE;
        self.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        CGFloat components[] = { 0.0, 0.0, 0.0, 1 };
        self.metalLayer.backgroundColor = CGColorCreate(CGColorSpaceCreateDeviceRGB(), components);

        self.autoresizesSubviews = TRUE;
    }

    return self;
}

- (void)setFrameSize: (CGSize) size
{
    [self.metalLayer setDrawableSize: size];
}

@end

void Import_RHIMetalView() {}
