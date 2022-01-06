/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

namespace AzToolsFramework
{
    struct EditorSettingsOriginTracker
    {
        explicit EditorSettingsOriginTracker(AZ::SettingsRegistryInterface& registry);

        struct SettingsNotificationHandler
        {
            SettingsNotificationHandler(AZ::SettingsRegistryInterface& registry);
            ~SettingsNotificationHandler();

            AZ::JsonSerializationResult::ResultCode operator()(
                AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path);
            void operator()(AZStd::string_view path, AZ::SettingsRegistryInterface::Type type);

        private:
            AZ::SettingsRegistryInterface& m_settingsRegistry;
            AZ::JsonSerializationResult::JsonIssueCallback m_prevReportingCallback;
        };

    private:
        AZ::SettingsRegistryInterface& m_settingsRegistry;
        AZ::SettingsRegistryInterface::PreMergeEventHandler m_preMergeEventHandler;
        AZ::SettingsRegistryInterface::PostMergeEventHandler m_postMergeEventHandler;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_notifyHandler;
    };
} // namespace AZ