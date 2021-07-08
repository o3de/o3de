/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>

namespace AtomToolsFramework
{
    //! A collection of dynamic properties that can be serialized or added to an RPE as a group
    struct DynamicPropertyGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicPropertyGroup, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(DynamicPropertyGroup, "{F2267292-05E0-43AB-8506-725FA30CD5DF}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<AtomToolsFramework::DynamicProperty> m_properties;
    };
} // namespace AtomToolsFramework
