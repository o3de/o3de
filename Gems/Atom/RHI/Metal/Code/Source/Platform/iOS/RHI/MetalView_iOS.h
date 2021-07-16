/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#import <RHI/MetalView.h>

@interface RHIMetalView (PlatformImpl)
- (id)initWithFrame: (CGRect)frame
              scale: (CGFloat)scale
             device: (id<MTLDevice>)device;
- (void)setFrameSize: (CGSize) size;
@end
 
void Import_RHIMetalView();
