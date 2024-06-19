/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PerforceSettings.h"
#include <AzToolsFramework/SourceControl/ui_PerforceSettings.h>

#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>

#include <QTimer>
#include <AzCore/std/bind/bind.h>

namespace AzToolsFramework
{
    const char* const PerforceUser = "P4USER";
    const char* const PerforceServer = "P4PORT";
    const char* const PerforceClient = "P4CLIENT";
    const char* const PerforceCharset = "P4CHARSET";

    PerforceSettings::PerforceSettings(QWidget* parent)
        : QDialog(parent)
    {
        this->setWindowFlags(Qt::Dialog
            | Qt::MSWindowsFixedSizeDialogHint
            | Qt::WindowStaysOnTopHint);
        m_ui = new Ui::P4SettingsDialog();
        m_ui->setupUi(this);

        {
            SourceControlState state = SourceControlState::Disabled;
            SourceControlConnectionRequestBus::BroadcastResult(state, &SourceControlConnectionRequestBus::Events::GetSourceControlState);

            bool onlineMode = state == SourceControlState::Disabled ? false : true;
            AzQtComponents::CheckBox::applyToggleSwitchStyle(m_ui->workOnlineCheckbox);
            m_ui->workOnlineCheckbox->setChecked(onlineMode);
        }
    }

    void PerforceSettings::RetrieveSettings()
    {
        m_retrievedSettings.insert_key(PerforceClient);
        m_retrievedSettings.insert_key(PerforceUser);
        m_retrievedSettings.insert_key(PerforceServer);
        m_retrievedSettings.insert_key(PerforceCharset);

        setEnabled(false);

        int numSettingsToGet = static_cast<int>(m_retrievedSettings.size());

        auto applySettingResultFunction = [this, &numSettingsToGet](AZStd::string setting, const SourceControlSettingInfo& info) -> void
        {
            m_retrievedSettings[setting] = info;
            --numSettingsToGet;
        };

        using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
        for (const auto& kvp : m_retrievedSettings)
        {
            auto boundLocal = AZStd::bind<void>(applySettingResultFunction, kvp.first.c_str(), AZStd::placeholders::_1);
            SCRequestBus::Broadcast(&SCRequestBus::Events::GetConnectionSetting, kvp.first.c_str(), boundLocal);
        }

        auto waitForAllSettings = [&numSettingsToGet](int&, int&)
        {
            return numSettingsToGet == 0;
        };
        // Wait for completion
        ProgressShield::LegacyShowAndWait(this, tr("Getting settings"), waitForAllSettings);

        // wrinkle in the plan - charset might have (SERVER)charset applied to it.  So ask for one more.
        m_charsetKey = PerforceCharset;
        if (m_retrievedSettings[PerforceServer].IsAvailable())
        {
            AZStd::string charsetNameWithServerAddress = AZStd::string::format("P4_%s_CHARSET", m_retrievedSettings[PerforceServer].m_value.c_str());
            // ask for one more
            numSettingsToGet = 1;
            auto boundLocal = AZStd::bind<void>(applySettingResultFunction, charsetNameWithServerAddress.c_str(), AZStd::placeholders::_1);
            SCRequestBus::Broadcast(&SCRequestBus::Events::GetConnectionSetting, charsetNameWithServerAddress.c_str(), boundLocal);
            ProgressShield::LegacyShowAndWait(this, tr("Getting settings"), waitForAllSettings);

            // did we get a value?
            if (m_retrievedSettings[charsetNameWithServerAddress].IsAvailable())
            {
                m_charsetKey = charsetNameWithServerAddress;
            }
        }

        // which CHARSET do we pick?  we prefer (servername) charset, but will fall back to just P4CHARSET
        for (const auto& value : m_retrievedSettings)
        {
            const AZStd::string& settingName = value.first;
            const SourceControlSettingInfo& info = value.second;
            ApplyValueToControl(GetControlForSetting(settingName), info);
        }

        setEnabled(true);
    }

    QLineEdit* PerforceSettings::GetControlForSetting(const AZStd::string& settingName) const
    {
        if (settingName == PerforceClient)
        {
            return m_ui->workspaceEdit;
        }
        else if (settingName == PerforceUser)
        {
            return m_ui->userEdit;
        }
        else if (settingName == PerforceServer)
        {
            return m_ui->serverEdit;
        }
        else if (settingName == m_charsetKey)
        {
            return m_ui->charsetEdit;
        }
        return nullptr;
    }

    PerforceSettings::~PerforceSettings()
    {
        delete m_ui;
    }

    void PerforceSettings::ApplyValueToControl(QLineEdit* targetControl, const SourceControlSettingInfo& value)
    {
        if (targetControl)
        {
            if (value.IsAvailable())
            {
                targetControl->setText(QString::fromUtf8(value.m_value.c_str()));
            }
            else
            {
                targetControl->setText(QString(""));
            }

            if (value.IsSettable())
            {
                targetControl->setReadOnly(false);
                targetControl->setEnabled(true);
                targetControl->setToolTip("");
            }
            else
            {
                targetControl->setReadOnly(true);
                targetControl->setEnabled(false);
                QString toolTip;
                QString ttDescStart("Cannot change this value - it is currently being overridden by");
                if (value.m_status == SourceControlSettingStatus::Config)
                {
                    if (!value.m_context.empty())
                    {
                        toolTip = tr("%1 config file: %2.").arg(ttDescStart).arg(QString::fromUtf8(value.m_context.c_str()));
                    }
                    else
                    {
                        toolTip = tr("%1 a config file.").arg(ttDescStart);
                    }
                }
                else
                {
                    toolTip = tr("%1 your system environment.  Please check your environment variables in your system's control panel").arg(ttDescStart);
                }
                targetControl->setToolTip(toolTip);
            }
        }
    }

    void PerforceSettings::on_workOnlineCheckbox_toggled(bool newState)
    {
        m_ui->workspaceEdit->setEnabled(false);
        m_ui->userEdit->setEnabled(false);
        m_ui->serverEdit->setEnabled(false);
        m_ui->charsetEdit->setEnabled(false);

        if (newState)
        {
            // we only fetch settings if we are working online.
            QTimer::singleShot(0, this, &PerforceSettings::RetrieveSettings);
        }
        else
        {
            m_ui->workspaceEdit->setText(tr("(offline)"));
            m_ui->userEdit->setText(tr("(offline)"));
            m_ui->serverEdit->setText(tr("(offline)"));
            m_ui->charsetEdit->setText(tr("(offline)"));
        }
    }

    void PerforceSettings::Apply()
    {
        bool onlineMode = m_ui->workOnlineCheckbox->isChecked();
        {
            using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
            SCRequestBus::Broadcast(&SCRequestBus::Events::EnableSourceControl, onlineMode);
        }

        if (onlineMode)
        {
            // work online!
            // apply all the settings.
            // you may only apply some of the settings - the ones that are either 'set' or 'unset'
            // only set values that are not already that value
            // and charset is special in that you must set it as eitehr CHARSET or override the server.
            ApplySetting(PerforceUser);
            ApplySetting(PerforceServer);
            ApplySetting(PerforceClient);
            ApplySetting(m_charsetKey.c_str());
        }
    }

    bool PerforceSettings::ApplySetting(const char* key)
    {
        if (!m_retrievedSettings[key].IsSettable())
        {
            return false;
        }

        QLineEdit* source = GetControlForSetting(key);
        if (!source)
        {
            return false;
        }

        AZStd::string newValue = source->text().toUtf8().data();
        if (newValue.empty())
        {
            return false;
        }

        if (newValue == m_retrievedSettings[key].m_value)
        {
            return false;
        }

        // again, charset is special in that it can under certain circumstances take the form P4_SERVERNAME_CHARSET instead of just P4CHARSET

        if (m_charsetKey == key)
        {
            // if its already P4CHARSET then we don't care.
            if (m_charsetKey != PerforceCharset)
            {
                QLineEdit* hostSource = GetControlForSetting(PerforceServer);
                if (!hostSource)
                {
                    return false;
                }
                m_charsetKey = AZStd::string::format("P4_%s_CHARSET", hostSource->text().toUtf8().data());
                key = m_charsetKey.c_str();

            }
        }

        bool succeeded = false;
        bool complete = false;
        auto respCallback = [&succeeded, &complete, newValue](const SourceControlSettingInfo& info)
        {
            succeeded = (info.m_value == newValue);
            complete = true;
        };

        auto waitForDone = [&complete](int&, int&)
        {
            return complete;
        };

        using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
        SCRequestBus::Broadcast(&SCRequestBus::Events::SetConnectionSetting, key, newValue.c_str(), respCallback);
        ProgressShield::LegacyShowAndWait(this, tr("Applying settings"), waitForDone);

        return succeeded;
    }

    bool OpenPasswordDlg()
    {
        PerforceSettings dialog;

        if (dialog.exec() == QDialog::Accepted)
        {
            dialog.Apply();
            return true;
        }
        return false;
    }
} // namespace PerforceConnection


