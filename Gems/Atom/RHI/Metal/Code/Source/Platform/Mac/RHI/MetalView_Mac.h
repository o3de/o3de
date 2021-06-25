/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#import <RHI/MetalView.h>
#import <RHI/MetalViewController.h>

////////////////////////////////////////////////////////////////////////////////
@interface RHIMetalView (PlatformImpl)
- (id)initWithFrame: (CGRect)frame
              scale: (CGFloat)scale
             device: (id<MTLDevice>)device;
- (void)setFrameSize: (CGSize) size;
@end
 
@interface RHIMetalViewController (PlatformImpl)
- (void)keyDown: (NSEvent*)nsEvent;
@end

void Import_RHIMetalView();
