/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_LUAEDITORMAINWINDOW_H
#define LUAEDITOR_LUAEDITORMAINWINDOW_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/map.h>

#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/UI/LegacyFramework/UIFrameworkAPI.h>
#include <AzToolsFramework/UI/LegacyFramework/MainWindowSavedState.h>
#include <AzToolsFramework/UI/Logging/LogPanel_Panel.h>

#include <Source/LUA/LUAEditorFindResults.hxx>

#include "LUAEditorContextInterface.h"
#include "LUAEditorViewMessages.h"
#include "LUAEditorView.hxx"
#include "LUAEditorFindDialog.hxx"

#pragma once

class QMenu;
class QAction;
class QToolbar;
class QDockWidget;
class QSettings;
class QStandardItemModel;

namespace Ui
{
    class LUAEditorMainWindow;
}

namespace AzToolsFramework
{
    class TargetSelectorButtonAction;

    namespace AssetBrowser
    {
        class AssetBrowserFilterModel;
    }
}

namespace LUA
{
    class TargetContextButtonAction;
}
class ClassReferenceFilterModel;

#include <QtWidgets/QMainWindow>
#endif

namespace LUAEditor
{
    // Structure used for storing/retrieving compilation error data
    struct CompilationErrorData
    {
        AZStd::string m_filename;
        int m_lineNumber = 0;
    };

    //forwards
    class LUAViewWidget;
    class LUAEditorSettingsDialog;
    class DebugAttachmentButtonAction;
    class AssetDatabaseLocationListener;

    //////////////////////////////////////////////////////////////////////////
    //Main Window
    class LUAEditorMainWindow
        : public QMainWindow
        , public LUAEditorMainWindowMessages::Handler
        , private LUAEditor::LUABreakpointTrackerMessages::Bus::Handler
        , public LUAViewMessages::Handler
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(LUAEditorMainWindow,AZ::SystemAllocator,0);
        LUAEditorMainWindow(QStandardItemModel* dataModel, bool connectedState, QWidget* parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~LUAEditorMainWindow();

        bool OnGetPermissionToShutDown();

        //////////////////////////////////////////////////////////////////////////
        // Qt Events
    private:
        void closeEvent(QCloseEvent* event) override;

        Ui::LUAEditorMainWindow* m_gui;
        AzToolsFramework::TargetSelectorButtonAction* m_pTargetButton;
        LUA::TargetContextButtonAction* m_pContextButton;
        DebugAttachmentButtonAction* m_pDebugAttachmentButton;
        bool m_bAutocompleteEnabled;
        int m_SkinChoice;

    Q_SIGNALS:
        void OnReferenceDataChanged();

        //////////////////////////////////////////////////////////////////////////
        // Qt Slots
    public slots:

        void RestoreWindowState();
        void OnMenuCloseCurrentWindow();

        //file menu
        void assetBrowserPressed();
        void OnFileMenuNew();
        void OnFileMenuSave();
        void OnFileMenuSaveAs();
        void OnFileMenuSaveAll();
        void OnFileMenuReload();
        void OnFileMenuClose();
        void OnFileMenuCloseAll();
        void OnFileMenuCloseAllExcept();

        //edit menu
        void OnEditMenuUndo();
        void OnEditMenuRedo();
        void OnEditMenuCut();
        void OnEditMenuCopy();
        void OnEditMenuPaste();
        void OnEditMenuFind();
        void OnEditMenuFindNext();
        void OnEditMenuFindInAllOpen();
        void OnEditMenuFindLocal();
        void OnEditMenuFindLocalReverse();
        void OnEditMenuGoToLine();
        void OnEditMenuFoldAll();
        void OnEditMenuUnfoldAll();
        void OnEditMenuReplace();
        void OnEditMenuReplaceInAllOpen();
        void OnEditMenuSelectAll();
        void OnEditMenuSelectToBrace();
        void OnCommentSelectedBlock();
        void OnUnCommentSelectedBlock();

        void OnEditMenuTransposeUp();
        void OnEditMenuTransposeDn();

        void OnTabForwards();
        void OnTabBackwards();

        //view menu
        void OnViewMenuBreakpoints();
        void OnViewMenuStack();
        void OnViewMenuLocals();
        void OnViewMenuWatch();
        void OnViewMenuReference();
        void OnViewMenuFind1();
        void OnViewMenuFind2();
        void OnViewMenuFind3();
        void OnViewMenuFind4();
        void OnViewMenuResetZoom();

        // options Menu
        void OnAutocompleteChanged(bool change);
        void OnSettings();

        // help menu
        void OnLuaDocumentation();

        void OnDebugExecute();
        void OnDebugExecuteOnTarget();

        // execution control
        void OnDebugToggleBreakpoint();
        void OnDebugContinueRunning();
        void OnDebugStepOver();
        void OnDebugStepIn();
        void OnDebugStepOut();

        //source control
        void OnSourceControlMenuCheckOut();

        //tools menu

        // panels
        void luaClassFilterTextChanged( const QString& );

        //find
        void OnFindResultClicked(FindResultsBlockInfo result);

        void showTabContextMenu(const AZStd::string& assetId, const QPoint&);
        void closeAllTabsExceptThisTabContextMenu();

        // user closed all tabs in the log control
        void OnLogTabsReset();

        void AddMessageToLog(AzToolsFramework::Logging::LogLine::LogType type, const char* window, const char* message, void* data);

    private:
        AZStd::string m_currentTabContextMenuUUID;

        AZStd::string m_lastOpenFilePath;

        AZStd::vector<FindResultsBlockInfo> m_dProcessFindListClicked;
        void OnDataLoadedAndSet(const DocumentInfo& info, LUAViewWidget* pLUAViewWidget) override;

        AzToolsFramework::AssetBrowser::AssetBrowserFilterModel* m_filterModel;
        QSharedPointer<AzToolsFramework::AssetBrowser::CompositeFilter> CreateFilter();

        void LogLineSelectionChanged(const AzToolsFramework::Logging::LogLine& logLine);

        void OnOptionsMenuRequested();

    public:

        void SetupLuaFilesPanel();

        void ResetSearchClicks();
        bool NeedsCheckout();

        //////////////////////////////////////////////////////////////////////////
        //ClientInterface Messages
    private:
        //////////////////////////////////////////////////////////////////////////

        AssetDatabaseLocationListener* m_assetDatabaseListener = nullptr;

    public:
        void OnOpenLUAView(const DocumentInfo& docInfo);
        void OnOpenWatchView();
        void OnOpenBreakpointsView();
        void OnOpenStackView();
        void OnOpenLocalsView();
        void OnOpenFindView(int watchIndex); //this is not the page index this is the watch window index
        void OnOpenReferenceView();

        void MoveProgramCursor(const AZStd::string& assetId, int lineNumber);
        void MoveEditCursor(const AZStd::string& assetId, int lineNumber, bool withSelection = false);

        //////////////////////////////////////////////////////////////////////////
        // LUAEditorMainWindow Messages.
    public:
        virtual void OnCloseView(const AZStd::string& assetId);
        void OnFocusInEvent(const AZStd::string& assetId) override;
        void OnFocusOutEvent(const AZStd::string& assetId) override;
        void OnRequestCheckOut(const AZStd::string& assetId) override;
        void OnConnectedToTarget() override;
        void OnDisconnectedFromTarget() override;
        void OnConnectedToDebugger() override;
        void OnDisconnectedFromDebugger() override;
        void Repaint() override;
        //////////////////////////////////////////////////////////////////////////

        void dragEnterEvent(QDragEnterEvent *pEvent) override;
        void dropEvent(QDropEvent *pEvent) override;

        void IgnoreFocusEvents(bool ignore) { m_bIgnoreFocusRequests = ignore; }

        // specifically pass by value
        bool RequestCloseDocument(const AZStd::string assetId);

        void OnDockWidgetLocationChanged(const AZStd::string assetId);

        void OnDocumentInfoUpdated(const DocumentInfo& newInfo);
        bool OnRequestFocusView(const AZStd::string& assetId);

        bool OnFileSaveDialog(const AZStd::string& assetName, AZStd::string& newAssetName);
        bool OnFileSaveAsDialog(const AZStd::string& assetName, AZStd::string& newAssetName);

        //window state
        void SaveWindowState();

        // send new data to context.
        bool SyncDocumentToContext(const AZStd::string& assetId);

    private:
        //////////////////////////////////////////////////////////////////////////
        // Find Dialog
        LUAEditorFindDialog *m_ptrFindDialog;
        //////////////////////////////////////////////////////////////////////////

        LUAEditorSettingsDialog* m_settingsDialog;

        QTabBar*  ResolveDockWidgetToTabBarAndIndex(LUADockWidget* pDockWidget, int& tabIndex);
        AZ::Uuid ResolveTabBarAndIndexToDocumentID(QTabBar* pBar, int tabIndex);
        QTabBar*  FindMyDock(QWidget *source);

        //////////////////////////////////////////////////////////////////////////
        //View Tracking
    private:
        //LUA Views
        class TrackedLUAView
        {
        public:
            TrackedLUAView(    LUADockWidget* pLUADockWidget,
                            LUAViewWidget* pLUAViewWidget,
                            AZStd::string assetId)
                        : m_pLUADockWidget(pLUADockWidget)
                        , m_pLUAViewWidget(pLUAViewWidget)
                        , m_assetId(assetId){}
            ~TrackedLUAView(){};

            LUADockWidget* luaDockWidget(){ return m_pLUADockWidget;}
            LUAViewWidget* luaViewWidget(){ return m_pLUAViewWidget;}
            AZStd::string assetId(){ return m_assetId; }

        private:
            LUADockWidget* m_pLUADockWidget;
            LUAViewWidget* m_pLUAViewWidget;
            AZStd::string m_assetId;
        };//class TrackedLUAView


        typedef AZStd::unordered_map<AZStd::string, TrackedLUAView> TrackedLUAViewMap;
        TrackedLUAViewMap m_dOpenLUAView;

        //this will track the last focused document
        //this will get set on the focus event
        //Note this is only valid if the m_dOpenLUAView is not empty
        AZStd::string m_lastFocusedAssetId;
        QAction *actionTabForwards;
        QAction *actionTabBackwards;
        QLabel *m_ptrPerforceStatusWidget;

        // support for windows-ish Ctrl+Tab cycling through documents via the above Tab actions
        typedef AZStd::list<AZStd::string> TrackedLUACtrlTabOrder;
        TrackedLUACtrlTabOrder m_CtrlTabOrder;
        bool eventFilter(QObject *obj, QEvent *event) override;
        AZStd::string m_StoredTabAssetId;

        bool m_bIgnoreFocusRequests;

        // this tracks where the program cursor was last:
        AZStd::string m_lastProgramCounterAssetId;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //Debugger Messages, from the LUAEditor::LUABreakpointTrackerMessages::Bus
        void BreakpointsUpdate(const LUAEditor::BreakpointMap& uniqueBreakpoints) override;
        void BreakpointHit(const LUAEditor::Breakpoint& breakpoint) override;
        void BreakpointResume() override;
        void OnExecuteScriptResult(bool success) override;

        //////////////////////////////////////////////////////////////////////////
        // track activity and synchronize the appropriate widgets' states to match
        void SetDebugControlsToInitial();
        void SetDebugControlsToRunning();
        void SetDebugControlsToAtBreak();
        void SetEditContolsToNoFilesOpen();
        void SetEditContolsToAtLeastOneFileOpen();

        //////////////////////////////////////////////////////////////////////////
        // LUA class reference panel support
        ClassReferenceFilterModel *m_ClassReferenceFilter;

        //////////////////////////////////////////////////////////////////////////
        // Internal state tracking to consolidate button/GUI handling
        struct StateTrack
        {
            StateTrack()
                : targetConnected(false)
                , debuggerAttached(false)
                , atLeastOneFileOpen(false)
                , scriptRunning(false)
                , atBreak(false)
                , hasExecuted(false)
            {}
            void Init()
            {
                targetConnected = false;
                debuggerAttached = false;
                atLeastOneFileOpen = false;
                scriptRunning = false;
                atBreak = false;
                hasExecuted = false;
            }
            bool targetConnected;
            bool debuggerAttached;
            bool atLeastOneFileOpen;
            bool scriptRunning;
            bool atBreak;
            bool hasExecuted;
        } m_StateTrack;
        void SetGUIToMatch( StateTrack & );

        //////////////////////////////////////////////////////////////////////////
        // Debugging
    private:
        void ExecuteScript(bool locally);

        //////////////////////////////////////////////////////////////////////////
        //finding

    public:
        QTabWidget* GetFindTabWidget();
        FindResults* GetFindResultsWidget(int index);
        void SetCurrentFindListWidget(int index);
        LUAViewWidget* GetCurrentView();
        AZStd::vector<LUAViewWidget*> GetAllViews();
    };

    class LUAEditorMainWindowLayout : public QLayout
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(LUAEditorMainWindowLayout, AZ::SystemAllocator, 0);
        LUAEditorMainWindowLayout(QWidget *pParent);
        virtual ~LUAEditorMainWindowLayout();
        virtual void addItem(QLayoutItem *);
        virtual QLayoutItem *itemAt(int index) const ;
        virtual QLayoutItem *takeAt(int index) ;
        virtual int count() const;
        virtual void setGeometry ( const QRect & r );
        virtual QSize sizeHint() const;
        virtual QSize minimumSize() const;

        // this layout can only have two children:

        AZStd::vector<QLayoutItem *> children;
        virtual Qt::Orientations expandingDirections() const;
    };

    class LUAEditorMainWindowSavedState : public AzToolsFramework::MainWindowSavedState
    {
    public:
        AZ_RTTI(LUAEditorMainWindowSavedState, "{AEB8E5D6-4F2F-49A2-BB09-795614ABAAFF}", AzToolsFramework::MainWindowSavedState);
        AZ_CLASS_ALLOCATOR(LUAEditorMainWindowSavedState, AZ::SystemAllocator, 0);

        AZStd::vector<AZStd::string> m_openAssetIds;
        bool m_bAutocompleteEnabled;
        bool m_bAutoReloadUnmodifiedFiles = false;

        LUAEditorMainWindowSavedState() {}
        void Init(const QByteArray& windowState,const QByteArray& windowGeom) override
        {
             AzToolsFramework::MainWindowSavedState::Init(windowState, windowGeom);
        }

        static void Reflect(AZ::ReflectContext* reflection);

    };

}

#endif //LUAEDITOR_LUAEDITORMAINWINDOW_H
