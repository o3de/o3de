/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNodeUtil.h>

namespace AtomToolsFramework
{
    void VisitDynamicNodeSlotConfigs(const DynamicNodeConfig& nodeConfig, const SlotConfigVisitorFn& visitorFn)
    {
        for (const auto& slotConfig : nodeConfig.m_propertySlots)
        {
            visitorFn(slotConfig);
        }

        for (const auto& slotConfig : nodeConfig.m_inputSlots)
        {
            visitorFn(slotConfig);
        }

        for (const auto& slotConfig : nodeConfig.m_outputSlots)
        {
            visitorFn(slotConfig);
        }
    }

    void VisitDynamicNodeSettings(const DynamicNodeConfig& nodeConfig, const SettingsVisitorFn& visitorFn)
    {
        visitorFn(nodeConfig.m_settings);

        VisitDynamicNodeSlotConfigs(
            nodeConfig,
            [visitorFn](const DynamicNodeSlotConfig& slotConfig)
            {
                visitorFn(slotConfig.m_settings);
            });
    }

    void CollectDynamicNodeSettings(
        const DynamicNodeSettingsMap& settings, const AZStd::string& settingName, AZStd::set<AZStd::string>& container)
    {
        if (auto settingsItr = settings.find(settingName); settingsItr != settings.end())
        {
            container.insert(settingsItr->second.begin(), settingsItr->second.end());
        }
    }

    void CollectDynamicNodeSettings(
        const DynamicNodeSettingsMap& settings, const AZStd::string& settingName, AZStd::vector<AZStd::string>& container)
    {
        if (auto settingsItr = settings.find(settingName); settingsItr != settings.end())
        {
            container.insert(container.end(), settingsItr->second.begin(), settingsItr->second.end());
        }
    }
} // namespace AtomToolsFramework
