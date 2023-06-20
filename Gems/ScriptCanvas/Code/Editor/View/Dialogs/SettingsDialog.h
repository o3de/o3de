/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Component/EntityId.h>
#include <Editor/Settings.h>
#include <ScriptCanvas/Core/Core.h>
#endif

namespace Ui
{
    class SettingsDialog;
}

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace ScriptCanvasEditor
{
    class Settings
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(Settings, "{E3B5DE71-FB4E-472C-BD2A-BD180E68B9A6}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(Settings, AZ::SystemAllocator);

        Settings()
            : m_enableLogging(false)
        {}

        bool m_enableLogging;

        static void Reflect(AZ::ReflectContext* reflection);
    };


    enum class SettingsType
    {
        None,
        All,
        General,
        Graph
    };


    class SettingsDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        SettingsDialog(const QString& title, ScriptCanvas::ScriptCanvasId scriptCanvasId, QWidget* pParent = nullptr);
        ~SettingsDialog() override;

        const QString& GetText() const { return m_text; }

    protected:

        void OnOK();
        void OnCancel();
        void OnTextChanged(const QString& text);
        void ConfigurePropertyEditor(AzToolsFramework::ReflectedPropertyEditor*);

    private:
        void SetType(SettingsType settingsType);
        void SetupGeneralSettings(AZ::SerializeContext* context);
        void SetupGraphSettings(AZ::SerializeContext* context);

        void RevertSettings();

        QString m_text;
        ScriptCanvas::ScriptCanvasId m_scriptCanvasId;

        bool m_revertOnClose;

        Settings m_originalSettings;
        EditorSettings::ScriptCanvasEditorSettings m_originalEditorSettings;

        SettingsType m_settingsType = SettingsType::None;

        Ui::SettingsDialog* ui;
    };
}
