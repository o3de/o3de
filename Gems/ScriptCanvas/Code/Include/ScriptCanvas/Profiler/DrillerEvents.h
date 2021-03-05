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