/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace EMotionFX
{
    AZ_CHILD_ALLOCATOR_WITH_NAME(PropertyWidgetAllocator, "PropertyWidgetAllocator", "{5A2780C1-3660-4F47-A529-8E4F7B2B2F84}", AZ::SystemAllocator);
} // namespace EMotionFX
