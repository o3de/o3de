/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_EDITOR_KEYBOARD_CUSTOMIZATION_SETTINGS_H
#define CRYINCLUDE_EDITOR_KEYBOARD_CUSTOMIZATION_SETTINGS_H
#pragma once

#include <QWidget>
#include <QHash>
#include <QVector>

// TODO: Move to an utils class or at least namespace
QString RemoveAcceleratorAmpersands(const QString& original);

class SANDBOX_API KeyboardCustomizationSettings
{
public:

    struct Shortcut
    {
        QString text;
        QList<QKeySequence> keySequence;
    };

    using Snapshot = QHash<const QAction*, Shortcut>;

    KeyboardCustomizationSettings(const QString& group, QWidget* parent);
    virtual ~KeyboardCustomizationSettings();

    // Iterates over all KeyboardCustomizationSettings instances and calls EnableShortcuts()
    static void EnableShortcutsGlobally(bool);
    static void LoadDefaultsGlobally();
    static void SaveGlobally();

    // Enables or disables shortcuts. Disabling shortcuts is used when in game mode.
    void EnableShortcuts(bool);

    void Load();
    void Load(const Snapshot& snapshot);
    void LoadDefaults();
    void Save();

    Snapshot CreateSnapshot();

    QAction* FindActionForShortcut(QKeySequence) const;

public:
    static void ExportToFile(QWidget* parentWindow);
    static void ImportFromFile(QWidget* parentWindow);

private:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    const QWidget* m_parent;
    const QString m_group;
    const Snapshot m_defaults;
    bool m_shortcutsEnabled;
    Snapshot m_lastEnabledShortcuts; // just to avoid load/save IO from/to disk
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    void LoadFromSnapshot(const Snapshot& snapshot);

    QJsonObject ExportGroup();
    void ImportGroup(const QJsonObject& group);
    void ClearShortcutsAndAccelerators();
private:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    static QVector<KeyboardCustomizationSettings*> m_instances;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif //CRYINCLUDE_EDITOR_KEYBOARD_CUSTOMIZATION_SETTINGS_H
