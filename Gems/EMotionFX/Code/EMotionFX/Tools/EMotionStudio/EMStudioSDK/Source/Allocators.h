/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>

namespace EMStudio
{
    /**
     * System allocator to be used for UI-related objects.
     */
    AZ_CHILD_ALLOCATOR_WITH_NAME(UIAllocator, "UIAllocator", "{98AED295-91AE-4598-B253-90A67FE4DABC}", AZ::SystemAllocator);
}
