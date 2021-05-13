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
