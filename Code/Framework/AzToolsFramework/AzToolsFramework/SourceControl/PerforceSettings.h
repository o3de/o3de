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
#include <QString>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h> // for SouceControlSettingInfo
#endif

class QLineEdit;

namespace Ui {
    class P4SettingsDialog;
} // namespace Ui

namespace AzToolsFramework
{
    class PerforceSettings
        : public QDialog
    {
        Q_OBJECT
    public:
        PerforceSettings(QWidget* pParent = nullptr);
        virtual ~PerforceSettings();

        bool GetWorkOffline() const;
        void SetWorkOffline(bool value);

    public Q_SLOTS:
        void RetrieveSettings();
        // auto-bound slots
        void on_workOnlineCheckbox_toggled(bool newState);

        void Apply();

    private:
        Ui::P4SettingsDialog * m_ui;

        AZStd::unordered_map<AZStd::string, AzToolsFramework::SourceControlSettingInfo> m_retrievedSettings;
        AZStd::string m_charsetKey;

        void ApplyValueToControl(QLineEdit* targetControl, const AzToolsFramework::SourceControlSettingInfo& value);
        bool ApplySetting(const char* key);
        QLineEdit* GetControlForSetting(const AZStd::string& settingName) const;
    };

    bool OpenPasswordDlg();
} // namespace PerforceConnection

