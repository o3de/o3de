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

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class Registry
            {
            public:
                using ComponentDescriptorList = AZStd::vector<AZ::ComponentDescriptor*>;

                AZ_CLASS_ALLOCATOR(Registry, SystemAllocator);

                static void RegisterComponents(ComponentDescriptorList& components);
            };
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
