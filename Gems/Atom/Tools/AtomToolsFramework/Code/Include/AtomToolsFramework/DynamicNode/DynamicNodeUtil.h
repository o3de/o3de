/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/DynamicNode/DynamicNodeConfig.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    // Visit the dynamic node and all of its slot configurations calling the visitor function.
    void VisitDynamicNodeSlotConfigs(
        DynamicNodeConfig& nodeConfig, const AZStd::function<void(DynamicNodeSlotConfig&)>& visitorFn);

    // Visit the dynamic node and all of its slot configurations calling the visitor function.
    void VisitDynamicNodeSlotConfigs(
        const DynamicNodeConfig& nodeConfig, const AZStd::function<void(const DynamicNodeSlotConfig&)>& visitorFn);

    // Visit the dynamic node and all of its slot configurations calling the visitor function for their settings maps.
    void VisitDynamicNodeSettings(
        DynamicNodeConfig& nodeConfig, const AZStd::function<void(DynamicNodeSettingsMap&)>& visitorFn);

    // Visit the dynamic node and all of its slot configurations calling the visitor function for their settings maps.
    void VisitDynamicNodeSettings(
        const DynamicNodeConfig& nodeConfig, const AZStd::function<void(const DynamicNodeSettingsMap&)>& visitorFn);

    // Build a unique set of settings found on a node or slot configuration.
    void CollectDynamicNodeSettings(
        const DynamicNodeSettingsMap& settings, const AZStd::string& settingName, AZStd::set<AZStd::string>& container);

    // Build an accumulated list of settings found on a node or slot configuration.
    void CollectDynamicNodeSettings(
        const DynamicNodeSettingsMap& settings, const AZStd::string& settingName, AZStd::vector<AZStd::string>& container);

    // Convenience function to get a list of all currently registered slot data type names.
    AZStd::vector<AZStd::string> GetRegisteredDataTypeNames();

    // Search the settings map and the dynamic node manager for dynamic edit data for the setting mapped to elementPtr
    const AZ::Edit::ElementData* FindDynamicEditDataForSetting(const DynamicNodeSettingsMap& settings, const void* elementPtr);

    // Add a new attribute to dynamic edit data for dynamic node settings
    template<typename AttributeValueType>
    void AddEditDataAttribute(AZ::Edit::ElementData& editData, const AZ::Crc32& crc, const AttributeValueType& attribute)
    {
        editData.m_attributes.push_back(AZ::Edit::AttributePair(crc, aznew AZ::AttributeContainerType<AttributeValueType>(attribute)));
    }
} // namespace AtomToolsFramework
