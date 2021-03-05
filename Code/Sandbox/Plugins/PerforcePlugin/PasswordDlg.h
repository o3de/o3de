/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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

namespace PerforceConnection
{
    class PerforceConfigDialog
        : public QDialog
    {
        Q_OBJECT
    public:
        PerforceConfigDialog(QWidget* pParent = nullptr);
        virtual ~PerforceConfigDialog();

        bool GetWorkOffline() const;
        void SetWorkOffline(bool value);

    public Q_SLOTS:
        void RetrieveSettings();
        // auto-bound slots
        void on_workOnlineCheckbox_toggled(bool newState);

        void Apply();

    private:
        Ui::P4SettingsDialog* m_ui;

        AZStd::unordered_map<AZStd::string, AzToolsFramework::SourceControlSettingInfo> m_retrievedSettings;
        AZStd::string m_charsetKey;

        void ApplyValueToControl(QLineEdit* targetControl, const AzToolsFramework::SourceControlSettingInfo& value);
        bool ApplySetting(const char* key);
        QLineEdit* GetControlForSetting(const AZStd::string& settingName) const;
    };

    bool OpenPasswordDlg();
} // namespace PerforceConnection
