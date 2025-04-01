/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>

namespace AtomToolsFramework
{
    //! A collection of dynamic properties that can be serialized or added to an RPE as a group
    struct DynamicPropertyGroup
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicPropertyGroup, AZ::SystemAllocator);
        AZ_TYPE_INFO(DynamicPropertyGroup, "{F2267292-05E0-43AB-8506-725FA30CD5DF}");

        static void Reflect(AZ::ReflectContext* context);

        bool m_visible = true;
        AZStd::string m_name;
        AZStd::string m_displayName;
        AZStd::string m_description;
        AZStd::vector<AtomToolsFramework::DynamicProperty> m_properties;
        AZStd::vector<AZStd::shared_ptr<DynamicPropertyGroup>> m_groups;
    };
} // namespace AtomToolsFramework
