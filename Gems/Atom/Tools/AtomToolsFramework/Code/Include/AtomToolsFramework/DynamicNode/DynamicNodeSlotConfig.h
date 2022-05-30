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
    using DynamicNodeSettingsMap = AZStd::unordered_map<AZStd::string, AZStd::string>;

    //! Structure containing settings for configuring individual input and output slots on a dynamic node
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

        AZStd::string m_name = "Unnamed";
        AZStd::string m_displayName = "Unnamed";
        AZStd::string m_description;
        AZStd::any m_defaultValue;
        AZStd::vector<AZStd::string> m_supportedDataTypes;
        DynamicNodeSettingsMap m_settings;
    };
} // namespace AtomToolsFramework
