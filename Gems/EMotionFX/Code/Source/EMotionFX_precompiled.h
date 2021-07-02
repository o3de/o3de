/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

// Minimal integration system bits, such as allocator definition, smart pointers, etc.
#include <platform.h> // Many CryCommon files require that this is included first.
#include <Integration/System/SystemCommon.h>
// When we go full-source for EMotion FX, we'll want to ensure we're no longer pulling it all in via PCH,
// and instead apply our normal philosophy around minimal includes.
// Though if EMotion FX has any internal defines that drive behavior, we'd still potentially want an include wrapper.
#include <MCore/Source/AzCoreConversions.h>
#include <EMotionFX/Source/EMotionFX.h>
