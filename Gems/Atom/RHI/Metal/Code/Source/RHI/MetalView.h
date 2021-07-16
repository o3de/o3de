/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom_RHI_Metal_Platform.h>
#import <QuartzCore/CAMetalLayer.h>


@interface RHIMetalView : NativeViewType {}

@property  (nonatomic, assign )CAMetalLayer *metalLayer;
+ (id)layerClass;

@end
 


