/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Editor/Settings/EditorSettingsManager.h>

#include <AzCore/Component/ComponentApplication.h>

#include <AzToolsFramework/Editor/Settings/EditorSettingsContext.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

namespace AzToolsFramework::Editor
{
    EditorSettingsManager::EditorSettingsManager()
    {
        AZ::Interface<EditorSettingsInterface>::Register(this);
    }

    void EditorSettingsManager::Start()
    {
        AZ::ReflectionEnvironment::GetReflectionManager()->AddReflectContext<EditorSettingsContext>();
    }

    EditorSettingsManager::~EditorSettingsManager()
    {
        if (m_settingsDialog != nullptr)
        {
            delete m_settingsDialog;
        }

        AZ::Interface<EditorSettingsInterface>::Unregister(this);
        AZ::ReflectionEnvironment::GetReflectionManager()->RemoveReflectContext<EditorSettingsContext>();
    }

    void EditorSettingsManager::OpenEditorSettingsDialog()
    {
        if (m_settingsDialog == nullptr)
        {
            m_settingsDialog = new EditorSettingsDialog(AzToolsFramework::GetActiveWindow());
        }
        m_settingsDialog->exec();
    }

    AZStd::string EditorSettingsManager::GetTestSettingsList()
    {
        AZStd::string result;

        auto context = AZ::ReflectionEnvironment::GetReflectionManager()->GetReflectContext<EditorSettingsContext>();
        for (auto elem : context->GetSettingsArray())
        {
            result += AZStd::string::format("%s - %s - %s\n",
                elem.GetCategory().data(), elem.GetSubCategory().data(), elem.GetName().data());
        }

        return result;
    }
} // namespace AzToolsFramework::Editor
