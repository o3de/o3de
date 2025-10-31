/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "EventHandler.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(EventHandler, EventHandlerAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphInstanceEventHandler, AnimGraphEventHandlerAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(MotionInstanceEventHandler, MotionEventHandlerAllocator)

}
