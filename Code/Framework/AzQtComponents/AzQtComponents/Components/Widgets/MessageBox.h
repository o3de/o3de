/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/Components/StyledDialog.h>
#include <QMessageBox>


namespace AzQtComponents
{
    class Style;

    /**
     * Open 3D Engine specific wrapper to do MessageBox type stuff, specifically to automatically handle
     * checking/writing "do not ask this question again" checkbox/settings.
     *
     * Called AzMessageBox instead of MessageBox because for windows.h does #define MessageBox MessageBoxA / MessageBoxW
     *
     */
    class AZ_QT_COMPONENTS_API AzMessageBox
    {
    public:

        /**
          This is a variant of QMessageBox::question() that will first check if there is a QSetting saved
          already under the settingKey parameter.
          If there is, that value will be returned.
          If there isn't, the dialog will be shown to the user modally, the response will be saved in the QSettings
          and then returned.
         */
        static int questionIfNecessary(QWidget* parent, const QString& settingKey, const QString& title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons(QMessageBox::No | QMessageBox::Yes), QMessageBox::StandardButton defaultButton = QMessageBox::No, const QString& autoHideCheckBoxText = QString());

        /**
          This is a variant of QMessageBox::critical() that will first check if there is a QSetting saved
          already under the settingKey parameter.
          If there is, that value will be returned.
          If there isn't, the dialog will be shown to the user modally, the response will be saved in the QSettings
          and then returned.
         */
        static int criticalIfNecessary(QWidget* parent, const QString& settingKey, const QString& title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::No, const QString& autoHideCheckBoxText = QString());

        /**
          This is a variant of QMessageBox::information() will first check if there is a QSetting saved
          already under the settingKey parameter.
          If there is, that value will be returned.
          If there isn't, the dialog will be shown to the user modally, the response will be saved in the QSettings
          and then returned.
         */
        static int informationIfNecessary(QWidget* parent, const QString& settingKey, const QString& title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::No, const QString& autoHideCheckBoxText = QString());

        /**
          This is a variant of QMessageBox::warning() will first check if there is a QSetting saved
          already under the settingKey parameter.
          If there is, that value will be returned.
          If there isn't, the dialog will be shown to the user modally, the response will be saved in the QSettings
          and then returned.
         */
        static int warningIfNecessary(QWidget* parent, const QString& settingKey, const QString& title, const QString& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::No, const QString& autoHideCheckBoxText = QString());

    private:
        static int execIfNecessary(QMessageBox::Icon icon, QWidget* parent, const QString& settingKey, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton, const QString& autoHideCheckBoxText);
    };

    class AZ_QT_COMPONENTS_API FixedWidthMessageBox : public QMessageBox
    {
        Q_OBJECT
    public:
        FixedWidthMessageBox(
            int width,
            const QString& title,
            const QString& text,
            const QString& informativeText,
            const QString& detailedText,
            QMessageBox::Icon icon,
            QMessageBox::StandardButton standardButton,
            QMessageBox::StandardButton defaultButton,
            QWidget* parent = nullptr);

    private:
        void SetWidth(int width);
    };


} // namespace AzQtComponents
