/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#ifndef LEVELEDITORMENUHANDLER_H
#define LEVELEDITORMENUHANDLER_H

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QString>
#include <QMenu>
#include <QPointer>
#include "ActionManager.h"
#include "QtViewPaneManager.h"
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#endif

class MainWindow;
class QtViewPaneManager;
class QSettings;
struct QtViewPane;

class LevelEditorMenuHandler
    : public QObject
    , private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
    , private AzToolsFramework::EditorMenuRequestBus::Handler
{
    Q_OBJECT
public:
    LevelEditorMenuHandler(MainWindow* mainWindow, QtViewPaneManager* const viewPaneManager);
    ~LevelEditorMenuHandler();

    void Initialize();

    bool MRUEntryIsValid(const QString& entry, const QString& gameFolderPath);

    void IncrementViewPaneVersion();
    int GetViewPaneVersion() const;

    void UpdateViewLayoutsMenu(ActionManager::MenuWrapper& layoutsMenu);

    void ResetToolsMenus();

    QAction* CreateViewPaneAction(const QtViewPane* view);

    // It's used when users update the Tool Box Macro list in the Configure Tool Box Macro dialog
    void UpdateMacrosMenu();

Q_SIGNALS:
    void ActivateAssetImporter();

private:
    QMenu* CreateFileMenu();
    QMenu* CreateEditMenu();
    void PopulateEditMenu(ActionManager::MenuWrapper& editMenu);
    QMenu* CreateGameMenu();
    QMenu* CreateToolsMenu();
    QMenu* CreateViewMenu();
    QMenu* CreateHelpMenu();

    void checkOrOpenView();

    QMap<QString, QList<QtViewPane*>> CreateMenuMap(QMap<QString, QList<QtViewPane*>>& menuMap, QtViewPanes& allRegisteredViewPanes);
    void CreateMenuOptions(QMap<QString, QList<QtViewPane*>>* menuMap, ActionManager::MenuWrapper& menu, const char* category);

    void CreateDebuggingSubMenu(ActionManager::MenuWrapper gameMenu);

    void UpdateMRUFiles();

    void ClearAll();
    void OnUpdateOpenRecent();

    void OnUpdateMacrosMenu();

    void UpdateOpenViewPaneMenu();

    QAction* CreateViewPaneMenuItem(ActionManager* actionManager, ActionManager::MenuWrapper& menu, const QtViewPane* view);

    void InitializeViewPaneMenu(ActionManager* actionManager, ActionManager::MenuWrapper& menu, AZStd::function < bool(const QtViewPane& view)> functor);

    void LoadComponentLayout();

    void AddDisableActionInSimModeListener(QAction* action);

    // ViewportEditorModeNotificationsBus overrides ...
    void OnEditorModeActivated(
        const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;
    void OnEditorModeDeactivated(
        const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;

    // EditorMenuRequestBus
    void AddEditMenuAction(QAction* action) override;
    void AddMenuAction(AZStd::string_view categoryId, QAction* action, bool addToToolsToolbar) override;
    void RestoreEditMenuToDefault() override;

    MainWindow* m_mainWindow;
    ActionManager* m_actionManager;
    QtViewPaneManager* m_viewPaneManager;

    QPointer<QMenu> m_viewportViewsMenu;

    ActionManager::MenuWrapper m_toolsMenu;

    QMenu* m_mostRecentLevelsMenu = nullptr;
    QMenu* m_editmenu = nullptr;

    ActionManager::MenuWrapper m_viewPanesMenu;
    ActionManager::MenuWrapper m_layoutsMenu;
    ActionManager::MenuWrapper m_macrosMenu;

    const char* m_levelExtension = nullptr;
    int m_viewPaneVersion = 0;

    QList<QMenu*> m_topLevelMenus;
};

#endif // LEVELEDITORMENUHANDLER_H
