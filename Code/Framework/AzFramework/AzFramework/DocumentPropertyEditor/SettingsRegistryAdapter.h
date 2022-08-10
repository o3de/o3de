/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryOriginTracker.h>

namespace AZ::DocumentPropertyEditor
{
    class SettingsRegistryAdapter : public DocumentAdapter
    {
    public:
        //! Default constructs a Settings Registry adapter that uses the globally registered
        //! SettingsRegistryOriginTracker instance
        SettingsRegistryAdapter();

        //! Constructs a Settings Registry adapter using the supplied SettingsRegistry Origin Tracker instance
        explicit SettingsRegistryAdapter(AZ::SettingsRegistryOriginTracker* originTracker);
        ~SettingsRegistryAdapter();

        // Recurses through the Settings Registry associated with the supplied Settings Registry Origin Tracker
        // populates PropertyEditorNodes
        bool BuildField(AZStd::string_view path, AZStd::string_view fieldName, AZ::SettingsRegistryInterface::Type type);

        SettingsRegistryOriginTracker* GetSettingsRegistryOriginTracker();
        const SettingsRegistryOriginTracker* GetSettingsRegistryOriginTracker() const;

    protected:
        Dom::Value GenerateContents() override;

    private:
        struct SettingsNotificationHandler
        {
            SettingsNotificationHandler(SettingsRegistryAdapter& adapter);
            ~SettingsNotificationHandler();

            void operator()(const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs);

        private:
            SettingsRegistryAdapter& m_settingsRegistryAdapter;
        };

        AdapterBuilder m_builder;
        AZ::SettingsRegistryOriginTracker* m_originTracker = nullptr;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_notifyHandler;
    };
}
