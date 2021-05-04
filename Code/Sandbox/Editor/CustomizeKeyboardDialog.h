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
#ifndef CRYINCLUDE_EDITOR_CUSTOMIZE_KEYBOARD_DIALOG_H
#define CRYINCLUDE_EDITOR_CUSTOMIZE_KEYBOARD_DIALOG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include "KeyboardCustomizationSettings.h"
#endif

namespace Ui
{
    class CustomizeKeyboardDialog;
}

class QAbstractButton;
class NestedQAction;
class MenuActionsModel;
class ActionShortcutsModel;

class CustomizeKeyboardDialog
    : public QDialog
{
    Q_OBJECT
public:
    CustomizeKeyboardDialog(KeyboardCustomizationSettings& settings, QWidget* parent = nullptr);
    virtual ~CustomizeKeyboardDialog();

private slots:
    void CommandSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

    void CategoryChanged(const QString& category);
    void DialogButtonClicked(const QAbstractButton* button);
    void KeySequenceEditingFinished();
    void AssignButtonClicked();
    void ShortcutsViewSelectionChanged(const QModelIndex& current, const QModelIndex& previous);
    void ShortcutsViewDataChanged();
    void ShortcutRemoved();

private:
    QScopedPointer<Ui::CustomizeKeyboardDialog> m_ui;
    QHash<QString, QVector<NestedQAction> > m_menuActions;

    KeyboardCustomizationSettings& m_settings;
    const KeyboardCustomizationSettings::Snapshot m_settingsSnapshot;

    MenuActionsModel* m_menuActionsModel;
    ActionShortcutsModel* m_actionShortcutsModel;

    QStringList BuildModels(QWidget* parent);
};

#endif //CRYINCLUDE_EDITOR_CUSTOMIZE_KEYBOARD_DIALOG_H
