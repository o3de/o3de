/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/MessageBox.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QSettings>

namespace AzQtComponents
{
    int AzMessageBox::questionIfNecessary(QWidget* parent, const QString& settingKey, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton, const QString& autoHideCheckBoxText)
    {
        return execIfNecessary(QMessageBox::Question, parent, settingKey, title, text, buttons, defaultButton, autoHideCheckBoxText);
    }

    int AzMessageBox::criticalIfNecessary(QWidget* parent, const QString& settingKey, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton, const QString& autoHideCheckBoxText)
    {
        return execIfNecessary(QMessageBox::Critical, parent, settingKey, title, text, buttons, defaultButton, autoHideCheckBoxText);
    }

    int AzMessageBox::informationIfNecessary(QWidget* parent, const QString& settingKey, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton, const QString& autoHideCheckBoxText)
    {
        return execIfNecessary(QMessageBox::Information, parent, settingKey, title, text, buttons, defaultButton, autoHideCheckBoxText);
    }

    int AzMessageBox::warningIfNecessary(QWidget* parent, const QString& settingKey, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton, const QString& autoHideCheckBoxText)
    {
        return execIfNecessary(QMessageBox::Warning, parent, settingKey, title, text, buttons, defaultButton, autoHideCheckBoxText);
    }

    int AzMessageBox::execIfNecessary(QMessageBox::Icon icon, QWidget* parent, const QString& settingKey, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton, const QString& autoHideCheckBoxText)
    {
        QSettings settings;

        QVariant autoSavedSetting = settings.value(settingKey);
        if (!autoSavedSetting.isNull())
        {
            return autoSavedSetting.toInt();
        }

        QString checkBoxText = autoHideCheckBoxText.isEmpty() ? QStringLiteral("Do not show this message again") : autoHideCheckBoxText;

        QMessageBox messageBox(icon, title, text, buttons, parent);
        messageBox.setDefaultButton(defaultButton);

        QCheckBox* checkBox = new QCheckBox(checkBoxText);
        messageBox.setCheckBox(checkBox);

        int ret = messageBox.exec();

        if (checkBox->isChecked())
        {
            settings.setValue(settingKey, ret);
        }

        return ret;
    }
} // namespace AzQtComponents
