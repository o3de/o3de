/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <AzToolsFramework/Editor/Settings/EditorSettingsBlock.h>
#include <AzToolsFramework/Editor/Settings/EditorSettingsOriginTracker.h>
#include <AzToolsFramework/Editor/Settings/Dialog/EditorSettingsDialog.h>
#include <AzToolsFramework/Editor/Settings/EditorSettingsInterface.h>

namespace AzToolsFramework
{
    class EditorSettingsManager
        : EditorSettingsInterface
    {
    public:
        EditorSettingsManager();
        virtual ~EditorSettingsManager();

        static void Reflect(AZ::ReflectContext* context);
        void Start();

        // EditorSettingsInterface
        void OpenEditorSettingsDialog() override;
        CategoryMap* GetSettingsBlocks() override;

    private:
        void SetupSettings();

        bool m_isSetup = false;
        CategoryMap m_settingItems;

        AZ::SettingsRegistryInterface* m_settingsRegistry;
        EditorSettingsOriginTracker* m_settingsOriginTracker = nullptr;
        EditorSettingsDialog* m_settingsDialog = nullptr;
    };
} // namespace AzToolsFramework::Editor
