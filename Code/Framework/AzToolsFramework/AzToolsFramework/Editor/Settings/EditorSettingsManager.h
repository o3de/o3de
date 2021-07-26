/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <AzToolsFramework/Editor/Settings/Dialog/EditorSettingsDialog.h>
#include <AzToolsFramework/Editor/Settings/EditorSettingsInterface.h>

namespace AzToolsFramework::Editor
{
    class EditorSettingsManager
        : EditorSettingsInterface
    {
    public:
        EditorSettingsManager();
        virtual ~EditorSettingsManager();

        void Start();

        // EditorSettingsInterface
        void OpenEditorSettingsDialog() override;
        AZStd::string GetTestSettingsList() override;

    private:
        EditorSettingsDialog* m_settingsDialog = nullptr;
    };
} // namespace AzToolsFramework::Editor
