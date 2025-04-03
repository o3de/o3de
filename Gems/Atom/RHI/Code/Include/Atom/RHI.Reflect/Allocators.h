/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>

namespace AZ::RHI
{
    AZ_CHILD_ALLOCATOR_WITH_NAME(
        ShaderStageFunctionAllocator, "Atom::ShaderStageFunctionAllocator", "{15F285F1-74D5-4FAE-8CE4-B7D235A92F23}", AZ::SystemAllocator);
} // namespace RHI
