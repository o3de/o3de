/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
