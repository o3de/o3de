/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Allocators.h>
#include <Atom/RHI.Reflect/AllocatorManager.h>

namespace AZ::RHI
{
    SystemAllocatorBase::SystemAllocatorBase()
    {
        // Disable global registration since this is a pass through to another allocator (that should be registered)
        DisableRegistration();
        PostCreate();
        // Register into the RHI manager
        AllocatorManager::Instance().RegisterAllocator(this);
    }

    SystemAllocatorBase::~SystemAllocatorBase()
    {
        PreDestroy();
        if (RHI::AllocatorManager::IsReady())
        {
            AllocatorManager::Instance().UnRegisterAllocator(this);
        }
    }
} // namespace AZ::RHI
