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
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    // Contains tables of strings representing application or context specific settings for each node
    using DynamicNodeSettingsMap = AZStd::unordered_map<AZStd::string, AZStd::vector<AZStd::string>>;

    //! Contains all of the settings for an individual input or output slot on a DynamicNode
    struct DynamicNodeSlotConfig final
    {
        AZ_CLASS_ALLOCATOR(DynamicNodeSlotConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(DynamicNodeSlotConfig, "{F2C95A99-41FD-4077-B9A7-B0BF8F76C2CE}");
        static void Reflect(AZ::ReflectContext* context);

        DynamicNodeSlotConfig(
            const AZStd::string& name,
            const AZStd::string& displayName,
            const AZStd::string& description,
            const AZStd::any& defaultValue,
            const AZStd::vector<AZStd::string>& supportedDataTypes,
            const DynamicNodeSettingsMap& settings);
        DynamicNodeSlotConfig() = default;
        ~DynamicNodeSlotConfig() = default;

        //! Unique name or ID of a slot
        AZStd::string m_name = "Unnamed";
        //! Name displayed next to a slot in the node UI
        AZStd::string m_displayName = "Unnamed";
        //! Longer description display for tooltips and other UI
        AZStd::string m_description;
        //! The default value associated with a slot
        AZStd::any m_defaultValue;
        //! Names of all supported data types that a slot can connect to
        AZStd::vector<AZStd::string> m_supportedDataTypes;
        //! Container of generic or application specific settings for a slot
        DynamicNodeSettingsMap m_settings;
        //! Specifies whether or not UI will be displayed for editing the slot value on the node
        bool m_supportsEditingOnNode = true;
    };
} // namespace AtomToolsFramework
