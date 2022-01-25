/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "KeyboardCustomizationSettings.h"

// Qt
#include <QMenu>
#include <QMenuBar>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>


QString RemoveAcceleratorAmpersands(const QString& original)
{
    static const QRegularExpression expression(QStringLiteral("&(.)"));
    QString updated = original;
    updated.replace(expression, QStringLiteral("\\1"));

    return updated;
}

QVector<QAction*> GetAllActionsForMenu(const QMenu* menu)
{
    QList<QAction*> menuActions = menu->actions();
    QVector<QAction*> actions;
    actions.reserve(menuActions.size());
    foreach(QAction * action, menuActions)
    {
        if (action->menu() != nullptr)
        {
            QVector<QAction*> subMenuActions = GetAllActionsForMenu(action->menu());
            actions.reserve(actions.size() + subMenuActions.size());
            actions += subMenuActions;
        }
        else if (!action->isSeparator())
        {
            actions.push_back(action);
        }
    }

    return actions;
}

void ProcessAllActions(const QWidget* parent, const std::function<bool(QAction*)>& processor)
{
    QMenuBar* menuBar = parent->findChild<QMenuBar*>();
    QList<QAction*> menuBarActions = menuBar->actions();

    for(QAction* menuAction : menuBarActions)
    {
        processor(menuAction);
        auto actions = GetAllActionsForMenu(menuAction->menu());
        for(QAction* action : actions)
        {
            if (Q_UNLIKELY(processor(action)))
            {
                return;
            }
        }
    }
}

QString GetName(const QAction* action)
{
    if (action->data().isValid())
    {
        return action->data().toString();
    }
    else
    {
        return action->objectName();
    }
}

QVector<KeyboardCustomizationSettings*> KeyboardCustomizationSettings::m_instances;

KeyboardCustomizationSettings::KeyboardCustomizationSettings(const QString& group, QWidget* parent)
    : m_parent(parent)
    , m_group(group)
    , m_defaults(CreateSnapshot())
    , m_shortcutsEnabled(true)
{
    m_instances.append(this);
    Load();
}

KeyboardCustomizationSettings::~KeyboardCustomizationSettings()
{
    m_instances.remove(m_instances.indexOf(this));
}

void KeyboardCustomizationSettings::LoadDefaults()
{
    LoadFromSnapshot(m_defaults);
}

void KeyboardCustomizationSettings::Load()
{
    QSettings settings(QStringLiteral("O3DE"), QStringLiteral("O3DE"));
    settings.beginGroup(QStringLiteral("Keyboard Shortcuts"));
    settings.beginGroup(m_group);

    QStringList groups = settings.childGroups();

    ProcessAllActions(m_parent, [&](QAction* action)
        {
            auto groupName = GetName(action);
            if (groups.contains(groupName))
            {
                settings.beginGroup(groupName);

                auto sequence = QKeySequence::listFromString(settings.value("shortcuts").toString());
                action->setShortcuts(sequence);

                settings.endGroup();
            }
            return false;
        });

    settings.endGroup(); // m_group
    settings.endGroup(); // Keyboard Shortcuts
}

void KeyboardCustomizationSettings::Load(const Snapshot& snapshot)
{
    LoadFromSnapshot(snapshot);
}

void KeyboardCustomizationSettings::LoadFromSnapshot(const Snapshot& snapshot)
{
    ProcessAllActions(m_parent, [&](QAction* action)
        {
            auto entry = snapshot.constFind(action);
            if (entry != snapshot.constEnd())
            {
                const Shortcut& s = entry.value();
                action->setText(s.text);
                action->setShortcuts(s.keySequence);
            }
            return false;
        });
}

void KeyboardCustomizationSettings::Save()
{
    QSettings settings(QStringLiteral("O3DE"), QStringLiteral("O3DE"));
    settings.beginGroup(QStringLiteral("Keyboard Shortcuts"));
    settings.beginGroup(m_group);

    ProcessAllActions(m_parent, [&](QAction* action)
        {
            auto groupName = GetName(action);
            settings.beginGroup(groupName);

            settings.setValue("shortcuts", QKeySequence::listToString(action->shortcuts()));

            settings.endGroup();
            return false;
        });

    settings.endGroup(); // m_group
    settings.endGroup(); // Keyboard Shortcuts
}

KeyboardCustomizationSettings::Snapshot KeyboardCustomizationSettings::CreateSnapshot()
{
    Snapshot result;
    ProcessAllActions(m_parent, [&](QAction* action)
        {
            Shortcut s = { action->text(), action->shortcuts() };
            result[action] = s;
            return false;
        });
    return result;
}

void KeyboardCustomizationSettings::ExportToFile(QWidget* parent)
{
    QString fileName = QFileDialog::getSaveFileName(parent, QObject::tr("Export Keyboard Shortcuts"), QStringLiteral("o3de.keys"), QObject::tr("Keyboard Settings (*.keys)"));
    if (fileName.isEmpty())
    {
        return;
    }

    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(parent, QObject::tr("Shortcut Export Error"), QObject::tr("Couldn't open \"%1\": %2").arg(fileName).arg(file.errorString()));
        return;
    }

    QJsonObject store;
    store.insert("version", "1.0");
    store.insert("Content-Type", "application/x-o3de-sdk-keyboard-settings+json");
    QJsonObject groups;

    for (auto instance = m_instances.constBegin(); instance != m_instances.constEnd(); instance++)
    {
        groups.insert((*instance)->m_group, QJsonValue((*instance)->ExportGroup()));
    }

    store.insert("groups", groups);

    QJsonDocument exported(store);
    if (-1 == file.write(exported.toJson()))
    {
        QMessageBox::critical(parent, QObject::tr("Shortcut Export Error"), QObject::tr("Couldn't write settings to \"%1\": %2").arg(fileName).arg(file.errorString()));
        return;
    }

    file.close();
}

QJsonObject KeyboardCustomizationSettings::ExportGroup()
{
    QJsonObject group;
    group.insert("name", m_group);

    ProcessAllActions(m_parent, [&](QAction* action)
        {
            QJsonObject entry;
            entry.insert("label", RemoveAcceleratorAmpersands(action->text()));
            entry.insert("shortcuts", QKeySequence::listToString(action->shortcuts()));
            group.insert(GetName(action), QJsonValue(entry));
            return false;
        });

    return group;
}

void KeyboardCustomizationSettings::ImportFromFile(QWidget* parent)
{
    QString fileName = QFileDialog::getOpenFileName(parent, QObject::tr("Export Keyboard Shortcuts"), QString(), QObject::tr("Keyboard Settings (*.keys)"));
    if (fileName.isEmpty())
    {
        return;
    }

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(parent, QObject::tr("Shortcut Import Error"), QObject::tr("Couldn't open \"%1\": %2").arg(fileName).arg(file.errorString()));
        return;
    }

    QByteArray rawData = file.readAll();
    QJsonDocument imported(QJsonDocument::fromJson(rawData));
    QJsonObject store = imported.object();

    if (store.value("Content-Type") != "application/x-o3de-sdk-keyboard-settings+json" || store.value("version") != "1.0")
    {
        QMessageBox::critical(parent, QObject::tr("Shortcut Import Error"), QObject::tr("\"%1\" doesn't appear to contain keyboard settings").arg(fileName));
        return;
    }

    QJsonObject groups = store.value("groups").toObject();
    if (QJsonObject() == groups)
    {
        QMessageBox::critical(parent, QObject::tr("Shortcut Import Error"), QObject::tr("\"%1\" contains no keyboard settings").arg(fileName));
        return;
    }

    for (auto instance = m_instances.begin(); instance != m_instances.end(); instance++)
    {
        QJsonValue rawGroup = groups.value((*instance)->m_group);
        if (!rawGroup.isUndefined())
        {
            QJsonObject group = rawGroup.toObject();
            if (QJsonObject() != group && group.value("name").toString() == (*instance)->m_group)
            {
                (*instance)->ImportGroup(group);
            }
        }
    }

    file.close();
}

void KeyboardCustomizationSettings::ImportGroup(const QJsonObject& group)
{
    ProcessAllActions(m_parent, [&](QAction* action)
        {
            auto position = group.constFind(GetName(action));
            if (position != group.constEnd())
            {
                QJsonObject entry = position.value().toObject();
                if (QJsonObject() != entry)
                {
                    QString value = entry.value("shortcuts").toString();
                    action->setShortcuts(QKeySequence::listFromString(value));
                }
                else
                {
                    action->setShortcuts({});
                }
            }
            return false;
        });
}


QAction* KeyboardCustomizationSettings::FindActionForShortcut(QKeySequence shortcut) const
{
    QAction* result {
        nullptr
    };
    ProcessAllActions(m_parent, [&](QAction* action)
        {
            bool found { false };
            if (action->shortcuts().contains(shortcut))
            {
                result = action;
                found = true;
            }
            return found;
        });

    return result;
}

void KeyboardCustomizationSettings::ClearShortcutsAndAccelerators()
{
    ProcessAllActions(m_parent, [&](QAction* action)
        {
            action->setText(RemoveAcceleratorAmpersands(action->text()));
            action->setShortcut({});
            return false;
        });
}

/** static */
void KeyboardCustomizationSettings::EnableShortcutsGlobally(bool enable)
{
    for (auto it : m_instances)
    {
        it->EnableShortcuts(enable);
    }
}

void KeyboardCustomizationSettings::EnableShortcuts(bool enabled)
{
    if (enabled != m_shortcutsEnabled)
    {
        m_shortcutsEnabled = enabled;
        if (enabled)
        {
            LoadFromSnapshot(m_lastEnabledShortcuts);
            CLogFile::WriteLine("Enable Accelerators");
        }
        else
        {
            m_lastEnabledShortcuts = CreateSnapshot();
            ClearShortcutsAndAccelerators();
            CLogFile::WriteLine("Disable Accelerators");
        }
    }
}

/** static */
void KeyboardCustomizationSettings::LoadDefaultsGlobally()
{
    for (auto it : m_instances)
    {
        it->LoadDefaults();
    }
}

/** static */
void KeyboardCustomizationSettings::SaveGlobally()
{
    for (auto it : m_instances)
    {
        it->Save();
    }
}
