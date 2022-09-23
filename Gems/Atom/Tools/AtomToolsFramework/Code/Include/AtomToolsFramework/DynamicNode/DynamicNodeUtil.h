/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/DynamicNode/DynamicNodeConfig.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    using SlotConfigVisitorFn = AZStd::function<void(const DynamicNodeSlotConfig&)>;

    // Visit the dynamic node and all of its slot configurations calling the visitor function.
    void VisitDynamicNodeSlotConfigs(const DynamicNodeConfig& nodeConfig, const SlotConfigVisitorFn& visitorFn);

    using SettingsVisitorFn = AZStd::function<void(const DynamicNodeSettingsMap&)>;

    // Visit the dynamic node and all of its slot configurations calling the visitor function for their settings maps.
    void VisitDynamicNodeSettings(const DynamicNodeConfig& nodeConfig, const SettingsVisitorFn& visitorFn);

    // Build a unique set of settings found on a node or slot configuration.
    void CollectDynamicNodeSettings(
        const DynamicNodeSettingsMap& settings, const AZStd::string& settingName, AZStd::set<AZStd::string>& container);

    // Build an accumulated list of settings found on a node or slot configuration.
    void CollectDynamicNodeSettings(
        const DynamicNodeSettingsMap& settings, const AZStd::string& settingName, AZStd::vector<AZStd::string>& container);
} // namespace AtomToolsFramework
