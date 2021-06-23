/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom_RHI_Metal_precompiled_Platform.h>
#import <QuartzCore/CAMetalLayer.h>


@interface RHIMetalView : NativeViewType {}

@property  (nonatomic, assign )CAMetalLayer *metalLayer;
+ (id)layerClass;

@end
 


