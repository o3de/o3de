/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MainStatusBar.h>
#include <IRenderer.h>
#include <QMutex>
#include <QString>
#include <QSet>

#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

class GeneralStatusItem
    : public StatusBarItem
{
    Q_OBJECT
public:
    GeneralStatusItem(QString name, MainStatusBar* parent);

protected:
    QString CurrentText() const override;
};


class SourceControlItem
    : public StatusBarItem
    , private AzToolsFramework::SourceControlNotificationBus::Handler
{
    Q_OBJECT
public:
    SourceControlItem(QString name, MainStatusBar* parent);
    ~SourceControlItem();

    void ConnectivityStateChanged(const AzToolsFramework::SourceControlState state) override;

private:
    void InitMenu();
    void UpdateMenuItems();
    void SetSourceControlEnabledState(bool state);
    void UpdateAndShowMenu();
private Q_SLOTS:
    void OnOpenSettings();

private:
    std::unique_ptr<QMenu> m_menu;
    QAction* m_settingsAction;
    QWidgetAction* m_enableAction;
    QCheckBox* m_checkBox;

    QIcon m_scIconOk;
    QIcon m_scIconError;
    QIcon m_scIconWarning;
    QIcon m_scIconDisabled;
    bool m_sourceControlAvailable;
    AzToolsFramework::SourceControlState m_SourceControlState;
};


class MemoryStatusItem
    : public StatusBarItem
{
    Q_OBJECT
public:
    MemoryStatusItem(QString name, MainStatusBar* parent);
    ~MemoryStatusItem();

private:
    void updateStatus();
};

class GameInfoItem
    : public StatusBarItem
{
    Q_OBJECT
public:
    GameInfoItem(QString name, MainStatusBar* parent);

private Q_SLOTS:
    void OnShowContextMenu(const QPoint& pos);

private:
    QString m_projectPath;
};
