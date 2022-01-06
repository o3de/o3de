/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Editor/Settings/EditorSettingsOriginTracker.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Utils/Utils.h>

namespace AzToolsFramework
{
    EditorSettingsOriginTracker::SettingsNotificationHandler::SettingsNotificationHandler(
        AZ::SettingsRegistryInterface& registry)
        : m_settingsRegistry(registry)
    {
        AZ::JsonApplyPatchSettings applyPatchSettings;
        m_settingsRegistry.GetApplyPatchSettings(applyPatchSettings);
        // Wrap any existing callbacks into the reporting callback, so that both this struct
        // reporting callbacks and the existing callbacks can be invoked
        m_prevReportingCallback = applyPatchSettings.m_reporting;
        applyPatchSettings.m_reporting = [this](
            AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result,
            AZStd::string_view path) -> AZ::JsonSerializationResult::ResultCode
        {
            m_prevReportingCallback(message, result, path);
            (*this)(message, result, path);
            return result;
        };
        m_settingsRegistry.SetApplyPatchSettings(applyPatchSettings);
    }

    EditorSettingsOriginTracker::SettingsNotificationHandler::~SettingsNotificationHandler()
    {
        // Restore previous reporting callback
        AZ::JsonApplyPatchSettings applyPatchSettings;
        m_settingsRegistry.GetApplyPatchSettings(applyPatchSettings);
        applyPatchSettings.m_reporting = m_prevReportingCallback;
        m_settingsRegistry.SetApplyPatchSettings(applyPatchSettings);
    }

    // Use the Json Serialization Issue Callback system
    // to determine when a merge option modifies a value
    AZ::JsonSerializationResult::ResultCode EditorSettingsOriginTracker::SettingsNotificationHandler::operator()(
        AZStd::string_view /*message*/, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

        AZ::IO::PathView preferencesRootKey{ "/Amazon/Preferences", AZ::IO::PosixPathSeparator };
        AZ::IO::PathView inputKey{ path, AZ::IO::PosixPathSeparator };

        // Delegate to the Notifier Handler callable below
        if (result.GetTask() == AZ::JsonSerializationResult::Tasks::Merge &&
            result.GetProcessing() == AZ::JsonSerializationResult::Processing::Completed && inputKey.IsRelativeTo(preferencesRootKey))
        {
            if (auto type = m_settingsRegistry.GetType(path); type != AZ::SettingsRegistryInterface::Type::NoType)
            {
                operator()(path, type);
            }
        }

        return result;
    }

    void EditorSettingsOriginTracker::SettingsNotificationHandler::operator()(
        AZStd::string_view path, AZ::SettingsRegistryInterface::Type /*type*/)
    {
        constexpr AZ::IO::PathView preferencesRootKey{ "/Amazon/Preferences", AZ::IO::PosixPathSeparator };
        AZ::IO::PathView inputKey{ path, AZ::IO::PosixPathSeparator };
        if (inputKey.IsRelativeTo(preferencesRootKey))
        {
            // Do stuff with key
        }
    }

    EditorSettingsOriginTracker::EditorSettingsOriginTracker(AZ::SettingsRegistryInterface& registry)
        : m_settingsRegistry(registry)
    {
        auto PreMergeEvent = [this](AZStd::string_view filePath, AZStd::string_view /*rootKey*/)
        {
            AZ::IO::FixedMaxPath editorPreferencesPath = AZ::Utils::GetProjectPath();
            editorPreferencesPath = editorPreferencesPath / "user" / "Registry" / "editorpreferences.setreg";

            if (AZ::IO::PathView(filePath) == editorPreferencesPath)
            {
                m_notifyHandler = m_settingsRegistry.RegisterNotifier(SettingsNotificationHandler(m_settingsRegistry));
            }
        };

        auto PostMergeEvent = [this](AZStd::string_view /*filePath*/, AZStd::string_view /*rootKey*/)
        {
            // Clear the notification handler so that it goes out of scope
            // and this tracker instance no handles settings updates
            m_notifyHandler = {};
        };

        m_preMergeEventHandler = m_settingsRegistry.RegisterPreMergeEvent(PreMergeEvent);
        m_postMergeEventHandler = m_settingsRegistry.RegisterPostMergeEvent(PostMergeEvent);
    }
}