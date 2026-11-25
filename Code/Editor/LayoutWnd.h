/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// LayoutWnd.h : header file
//

#include "Viewport.h"

#include <AzQtComponents/Components/DockMainWindow.h>
#include <QSet>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

namespace AzToolsFramework
{
    class ActionManagerInterface;
    class MenuManagerInterface;
    class ToolBarManagerInterface;
}

class CLayoutViewPane;
class CViewport;

class QSettings;

/////////////////////////////////////////////////////////////////////////////
// CLayoutWnd window

enum EViewLayout
{
    ET_Layout0,
    ET_Layout1,
    ET_Layout2,
    ET_Layout3,
    ET_Layout4,
    ET_Layout5,
    ET_Layout6,
    ET_Layout7,
    ET_Layout8
};

class QDockWidget;

class CLayoutWnd : public AzQtComponents::DockMainWindow
    , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
{
    Q_OBJECT
public:
    explicit CLayoutWnd(QWidget* parent = nullptr);
    ~CLayoutWnd() override;

    CLayoutViewPane* GetViewPaneByIndex(unsigned int index) const;
    unsigned int GetViewPaneCount() const;
    void MaximizeViewport(int paneId);
    void CreateLayout(EViewLayout layout, bool bindViewports = true, EViewportType defaultView = ET_ViewportCamera);
    EViewLayout GetLayout() const { return m_layout; }

    void SaveConfig();
    bool LoadConfig();
    static const char* GetConfigGroupName();
    void ResetLayout();
    void Cycle2DViewport();
    CLayoutViewPane* FindViewByClass(const QString& viewClassName);
    QString ViewportTypeToClassName(EViewportType viewType);

protected:
    bool eventFilter(QObject* object, QEvent* event) override;

private:
    void OnMenuRegistrationHook() override;
    void OnToolBarRegistrationHook() override;
    void OnActionRegistrationHook() override;
    void OnMenuBindingHook() override;
    void OnToolBarBindingHook() override;

    bool CycleViewport(EViewportType from, EViewportType to);
    void SelectPaneForWidget(QWidget* widget);
    void UpdateDockFeatures();
    CLayoutViewPane* Pane(int slot) const;

    QList<QDockWidget*> m_docks;
    EViewLayout m_layout = ET_Layout0;
    QSet<QDockWidget*> m_closedDocks;
    QList<QDockWidget*> m_hiddenByMaximize;
    int m_maximizedSlot = -1;

    AzToolsFramework::ActionManagerInterface* m_actionManagerInterface;
    AzToolsFramework::MenuManagerInterface* m_menuManagerInterface;
    AzToolsFramework::ToolBarManagerInterface* m_toolBarManagerInterface;
};
