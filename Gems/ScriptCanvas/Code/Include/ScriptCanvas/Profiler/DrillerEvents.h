/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>

namespace ScriptCanvas
{
    namespace Profiler
    {
        namespace Event
        {
            class GraphExecutionStart
            {
            public:
                AZ_CLASS_ALLOCATOR(GraphExecutionStart, AZ::SystemAllocator, 0)
                AZ_RTTI(GraphExecutionStart, "{AAB7E2E1-9096-4F51-90CA-259A057F2D88}");

                GraphExecutionStart()
                {}
            };
        }

    }
}
