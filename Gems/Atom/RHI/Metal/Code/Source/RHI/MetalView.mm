/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#import <RHI/MetalView.h>

@implementation RHIMetalView

@synthesize metalLayer = _metalLayer;

+ (id)layerClass
{
    return [CAMetalLayer class];
}
@end
