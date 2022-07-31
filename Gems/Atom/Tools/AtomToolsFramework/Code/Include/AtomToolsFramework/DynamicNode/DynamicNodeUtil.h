/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/DynamicNode/DynamicNodeConfig.h>

namespace AtomToolsFramework
{
    using SettingsVisitorFn = AZStd::function<void(const DynamicNodeSettingsMap&)>;

    void VisitDynamicNodeSettings(const DynamicNodeConfig& nodeConfig, const SettingsVisitorFn& visitorFn);

    // Build a unique set of settings found on a node or slot configuration.
    void CollectDynamicNodeSettings(
        const DynamicNodeSettingsMap& settings, const AZStd::string& settingName, AZStd::set<AZStd::string>& container);

    // Build an accumulated list of settings found on a node or slot configuration.
    void CollectDynamicNodeSettings(
        const DynamicNodeSettingsMap& settings, const AZStd::string& settingName, AZStd::vector<AZStd::string>& container);
} // namespace AtomToolsFramework
