/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/DynamicNode/DynamicNodeSlotConfig.h>

namespace AtomToolsFramework
{
    //! Structure containing settings for data driving dynamic nodes
    struct DynamicNodeConfig final
    {
        AZ_CLASS_ALLOCATOR(DynamicNodeConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(DynamicNodeConfig, "{D43A2D1A-B67F-4144-99AF-72EA606CA026}");
        static void Reflect(AZ::ReflectContext* context);

        DynamicNodeConfig(
            const AZStd::string& category,
            const AZStd::string& title,
            const AZStd::string& subTitle,
            const DynamicNodeSettingsMap& settings,
            const AZStd::vector<DynamicNodeSlotConfig>& inputSlots,
            const AZStd::vector<DynamicNodeSlotConfig>& outputSlots,
            const AZStd::vector<DynamicNodeSlotConfig>& propertySlots);
        DynamicNodeConfig() = default;
        ~DynamicNodeConfig() = default;

        bool Save(const AZStd::string& path) const;
        bool Load(const AZStd::string& path);

        AZStd::string m_category;
        AZStd::string m_title = "Unnamed";
        AZStd::string m_subTitle;
        DynamicNodeSettingsMap m_settings;
        AZStd::vector<DynamicNodeSlotConfig> m_inputSlots;
        AZStd::vector<DynamicNodeSlotConfig> m_outputSlots;
        AZStd::vector<DynamicNodeSlotConfig> m_propertySlots;
    };
} // namespace AtomToolsFramework
