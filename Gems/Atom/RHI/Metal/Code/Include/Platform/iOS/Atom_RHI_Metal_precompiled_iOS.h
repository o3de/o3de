/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// RHI/Metal requires Catalina which we have just one node in Jenkins. Atom can enable
// this again in their stream, but cannot push this change until we put more Catalina
// nodes in Jenkins
#if 1
#include <metal/metal.h> //Apple metal header

#include <Metal_Traits_Platform.h>
#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/PlatformDef.h>

#import <UIKit/UIKit.h>
using NativeViewType = UIView;
using NativeViewControllerType = UIViewController;
using NativeWindowType = UIWindow;
using NativeScreenType = UIScreen;
#endif
