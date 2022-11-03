/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicNode/DynamicNodeManagerRequestBus.h>
#include <AtomToolsFramework/DynamicNode/DynamicNodeUtil.h>

namespace AtomToolsFramework
{
    void VisitDynamicNodeSlotConfigs(
        DynamicNodeConfig& nodeConfig, const AZStd::function<void(DynamicNodeSlotConfig&)>& visitorFn)
    {
        for (auto& slotConfig : nodeConfig.m_propertySlots)
        {
            visitorFn(slotConfig);
        }

        for (auto& slotConfig : nodeConfig.m_inputSlots)
        {
            visitorFn(slotConfig);
        }

        for (auto& slotConfig : nodeConfig.m_outputSlots)
        {
            visitorFn(slotConfig);
        }
    }

    void VisitDynamicNodeSlotConfigs(
        const DynamicNodeConfig& nodeConfig, const AZStd::function<void(const DynamicNodeSlotConfig&)>& visitorFn)
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

    void VisitDynamicNodeSettings(
        DynamicNodeConfig& nodeConfig, const AZStd::function<void(DynamicNodeSettingsMap&)>& visitorFn)
    {
        visitorFn(nodeConfig.m_settings);

        VisitDynamicNodeSlotConfigs(
            nodeConfig,
            [visitorFn](DynamicNodeSlotConfig& slotConfig)
            {
                visitorFn(slotConfig.m_settings);
            });
    }

    void VisitDynamicNodeSettings(
        const DynamicNodeConfig& nodeConfig, const AZStd::function<void(const DynamicNodeSettingsMap&)>& visitorFn)
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

    AZStd::vector<AZStd::string> GetRegisteredDataTypeNames()
    {
        GraphModel::DataTypeList registeredDataTypes;
        DynamicNodeManagerRequestBus::BroadcastResult(registeredDataTypes, &DynamicNodeManagerRequestBus::Events::GetRegisteredDataTypes);

        AZStd::vector<AZStd::string> names;
        names.reserve(registeredDataTypes.size());
        for (const auto& dataType : registeredDataTypes)
        {
            names.push_back(dataType->GetDisplayName());
        }
        return names;
    }

    const AZ::Edit::ElementData* FindDynamicEditDataForSetting(const DynamicNodeSettingsMap& settings, const void* elementPtr)
    {
        for (const auto& settingsGroup : settings)
        {
            for (const auto& setting : settingsGroup.second)
            {
                if (elementPtr == &setting)
                {
                    const AZ::Edit::ElementData* registeredEditData = nullptr;
                    DynamicNodeManagerRequestBus::BroadcastResult(
                        registeredEditData, &DynamicNodeManagerRequestBus::Events::GetEditDataForSetting, settingsGroup.first);
                    return registeredEditData;
                }
            }
        }
        return nullptr;
    }
} // namespace AtomToolsFramework
