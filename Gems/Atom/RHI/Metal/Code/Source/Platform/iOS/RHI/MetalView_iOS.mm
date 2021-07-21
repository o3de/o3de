/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
