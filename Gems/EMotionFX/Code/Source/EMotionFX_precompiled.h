/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
