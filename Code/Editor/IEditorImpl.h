/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : IEditor interface implementation.

#pragma once

#include "IEditor.h"
#include "MainWindow.h"

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <memory> // for shared_ptr
#include <QMap>
#include <QApplication>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <AzCore/std/string/string.h>

#include "Commands/CommandManager.h"

#include "Include/IErrorReport.h"
#include "ErrorReport.h"

class QMenu;


#define GET_PLUGIN_ID_FROM_MENU_ID(ID) (((ID) & 0x000000FF))
#define GET_UI_ELEMENT_ID_FROM_MENU_ID(ID) ((((ID) & 0x0000FF00) >> 8))

class CUndoManager;
class CGameEngine;
class CErrorsDlg;
class CTrackViewSequenceManager;
class CEditorFileMonitor;
class AzAssetWindow;
class AzAssetBrowserRequestHandler;
class AssetEditorRequestsHandler;
struct IEditorFileMonitor;
class CVegetationMap;


namespace Editor
{
    class EditorQtApplication;
}

namespace AssetDatabase
{
    class AssetDatabaseLocationListener;
}

class CEditorImpl
    : public IEditor
{
    Q_DECLARE_TR_FUNCTIONS(CEditorImpl)

public:
    CEditorImpl();
    virtual ~CEditorImpl();

    void Initialize();
    void OnBeginShutdownSequence();
    void OnEarlyExitShutdownSequence();
    void Uninitialize();

    void SetGameEngine(CGameEngine* ge);
    void DeleteThis() override { delete this; };
    CEditorCommandManager* GetCommandManager() override { return m_pCommandManager; };
    ICommandManager* GetICommandManager() override { return m_pCommandManager; }
    void ExecuteCommand(const char* sCommand, ...) override;
    void ExecuteCommand(const QString& command) override;
    void SetDocument(CCryEditDoc* pDoc) override;
    CCryEditDoc* GetDocument() const override;
    bool IsLevelLoaded() const override;
    void SetModifiedFlag(bool modified = true) override;
    void SetModifiedModule(EModifiedModule  eModifiedModule, bool boSet = true) override;
    bool IsLevelExported() const override;
    bool SetLevelExported(bool boExported = true) override;
    void InitFinished();
    bool IsModified() override;
    bool IsInitialized() const override{ return m_bInitialized; }
    bool SaveDocument() override;
    ISystem* GetSystem() override;
    void WriteToConsole(const char* string) override { CLogFile::WriteLine(string); };
    void WriteToConsole(const QString& string) override { CLogFile::WriteLine(string); };
    // Change the message in the status bar
    void SetStatusText(const QString& pszString) override;
    IMainStatusBar* GetMainStatusBar() override;
    bool ShowConsole([[maybe_unused]] bool show) override
    {
        //if (AfxGetMainWnd())return ((CMainFrame *) (AfxGetMainWnd()))->ShowConsole(show);
        return false;
    }

    void SetConsoleVar(const char* var, float value) override;
    float GetConsoleVar(const char* var) override;
    //! Query main window of the editor
    QMainWindow* GetEditorMainWindow() const override
    {
        return MainWindow::instance();
    };

    QString GetPrimaryCDFolder() override;
    QString GetLevelName() override;
    QString GetLevelFolder() override;
    QString GetLevelDataFolder() override;
    QString GetSearchPath(EEditorPathName path) override;
    QString GetResolvedUserFolder() override;
    bool ExecuteConsoleApp(const QString& CommandLine, QString& OutputText, bool bNoTimeOut = false, bool bShowWindow = false) override;
    bool IsInGameMode() override;
    void SetInGameMode(bool inGame) override;
    bool IsInSimulationMode() override;
    bool IsInTestMode() override;
    bool IsInPreviewMode() override;
    bool IsInConsolewMode() override;
    bool IsInLevelLoadTestMode() override;

    //! Enables/Disable updates of editor.
    void EnableUpdate(bool enable) override { m_bUpdates = enable; };
    CGameEngine* GetGameEngine() override { return m_pGameEngine; };
    CDisplaySettings* GetDisplaySettings() override { return m_pDisplaySettings; };
    // This will return a null pointer if CrySystem is not loaded before
    // Global Sandbox Settings are loaded from the registry before CrySystem
    // At that stage GetSettingsManager will return null and xml node in
    // memory will not be populated with Sandbox Settings.
    // After m_IEditor is created and CrySystem loaded, it is possible
    // to feed memory node with all necessary data needed for export
    // (gSettings.Load() and CXTPDockingPaneManager/CXTPDockingPaneLayout Sandbox layout management)
    CSettingsManager* GetSettingsManager() override;

    CMusicManager* GetMusicManager() override { return m_pMusicManager; };

    IEditorFileMonitor* GetFileMonitor() override;
    float GetTerrainElevation(float x, float y) override;
    Editor::EditorQtApplication* GetEditorQtApplication() override { return m_QtApplication; }
    const QColor& GetColorByName(const QString& name) override;

    //////////////////////////////////////////////////////////////////////////
    CPluginManager* GetPluginManager() override { return m_pPluginManager; }
    CViewManager* GetViewManager() override;
    CViewport* GetActiveView() override;
    void SetActiveView(CViewport* viewport) override;

    CLevelIndependentFileMan* GetLevelIndependentFileMan() override { return m_pLevelIndependentFileMan; }

    void UpdateViews(int flags, const AABB* updateRegion) override;
    void ResetViews() override;
    void ReloadTrackView() override;
    bool AddToolbarItem(uint8 iId, IUIEvent* pIHandler);
    void SetDataModified() override;

    void SetAxisConstraints(AxisConstrains axis) override;
    AxisConstrains GetAxisConstrains() override;
    void SetTerrainAxisIgnoreObjects(bool bIgnore) override;
    bool IsTerrainAxisIgnoreObjects() override;
    XmlNodeRef FindTemplate(const QString& templateName) override;
    void AddTemplate(const QString& templateName, XmlNodeRef& tmpl) override;

    const QtViewPane* OpenView(QString sViewClassName, bool reuseOpened = true) override;

    /**
     * Returns the top level widget which is showing the view pane with the specified name.
     * To access the child widget which actually implements the view pane use this instead:
     * QtViewPaneManager::FindViewPane<MyDialog>(name);
     */
    QWidget* FindView(QString viewClassName) override;

    bool SetViewFocus(const char* sViewClassName) override;

    // close ALL panels related to classId, used when unloading plugins.
    bool SelectColor(QColor &color, QWidget *parent = 0) override;
    void Update();
    SFileVersion GetFileVersion() override { return m_fileVersion; };
    SFileVersion GetProductVersion() override { return m_productVersion; };
    //! Get shader enumerator.
    CUndoManager* GetUndoManager() override { return m_pUndoManager; };
    void BeginUndo() override;
    void RestoreUndo(bool undo) override;
    void AcceptUndo(const QString& name) override;
    void CancelUndo() override;
    void SuperBeginUndo() override;
    void SuperAcceptUndo(const QString& name) override;
    void SuperCancelUndo() override;
    void SuspendUndo() override;
    void ResumeUndo() override;
    void Undo() override;
    void Redo() override;
    bool IsUndoRecording() override;
    bool IsUndoSuspended() override;
    void RecordUndo(IUndoObject* obj) override;
    bool FlushUndo(bool isShowMessage = false) override;
    bool ClearLastUndoSteps(int steps) override;
    bool ClearRedoStack() override;
    //! Retrieve current animation context.
    CAnimationContext* GetAnimation() override;
    CTrackViewSequenceManager* GetSequenceManager() override;
    ITrackViewSequenceManager* GetSequenceManagerInterface() override;

    CToolBoxManager* GetToolBoxManager() override { return m_pToolBoxManager; };
    IErrorReport* GetErrorReport() override { return m_pErrorReport; }
    IErrorReport* GetLastLoadedLevelErrorReport() override { return m_pLasLoadedLevelErrorReport; }
    void StartLevelErrorReportRecording() override;
    void CommitLevelErrorReport() override {SAFE_DELETE(m_pLasLoadedLevelErrorReport); m_pLasLoadedLevelErrorReport = new CErrorReport(*m_pErrorReport); }
    IFileUtil* GetFileUtil() override { return m_pFileUtil;  }
    void Notify(EEditorNotifyEvent event) override;
    void NotifyExcept(EEditorNotifyEvent event, IEditorNotifyListener* listener) override;
    void RegisterNotifyListener(IEditorNotifyListener* listener) override;
    void UnregisterNotifyListener(IEditorNotifyListener* listener) override;

    void ReduceMemory() override;
    ESystemConfigPlatform GetEditorConfigPlatform() const override;
    void ReloadTemplates() override;
    void ShowStatusText(bool bEnable) override;

    SSystemGlobalEnvironment* GetEnv() override;
    SEditorSettings* GetEditorSettings() override;

    void UnloadPlugins() override;
    void LoadPlugins() override;

protected:

    AZStd::string LoadProjectIdFromProjectData();

    void DetectVersion();
    void RegisterTools();
    void SetPrimaryCDFolder();

    //! List of all notify listeners.
    std::list<IEditorNotifyListener*> m_listeners;

    ISystem* m_pSystem;
    IFileUtil* m_pFileUtil;
    CEditorCommandManager* m_pCommandManager;
    CPluginManager* m_pPluginManager;
    CViewManager*   m_pViewManager;
    CUndoManager* m_pUndoManager;
    AxisConstrains m_selectedAxis;
    bool m_bUpdates;
    bool m_bTerrainAxisIgnoreObjects;
    SFileVersion m_fileVersion;
    SFileVersion m_productVersion;
    CXmlTemplateRegistry m_templateRegistry;
    CDisplaySettings* m_pDisplaySettings;
    QString m_primaryCDFolder;
    QString m_userFolder;
    bool m_bSelectionLocked;
    CGameEngine* m_pGameEngine;
    CAnimationContext* m_pAnimationContext;
    CTrackViewSequenceManager* m_pSequenceManager;
    CToolBoxManager* m_pToolBoxManager;
    CMusicManager* m_pMusicManager;
    CErrorReport* m_pErrorReport;
    //! Contains the error reports for the last loaded level.
    CErrorReport* m_pLasLoadedLevelErrorReport;

    CSelectionTreeManager* m_pSelectionTreeManager;

    //! CConsole Synchronization
    CConsoleSynchronization* m_pConsoleSync;
    //! Editor Settings Manager
    CSettingsManager* m_pSettingsManager;

    CLevelIndependentFileMan* m_pLevelIndependentFileMan;

    std::unique_ptr<CEditorFileMonitor> m_pEditorFileMonitor;
    QString m_selectFileBuffer;
    QString m_levelNameBuffer;

    bool m_bShowStatusText;
    bool m_bInitialized;
    bool m_bExiting;
    static void CmdPy(IConsoleCmdArgs* pArgs);

    Editor::EditorQtApplication* const m_QtApplication = nullptr;

    // This has to be absolute for the namespace since there is also a class called AssetDatabase that causes issues in unity builds
    ::AssetDatabase::AssetDatabaseLocationListener* m_pAssetDatabaseLocationListener;
    AzAssetBrowserRequestHandler* m_pAssetBrowserRequestHandler;
    AssetEditorRequestsHandler* m_assetEditorRequestsHandler;

    AZStd::mutex m_pluginMutex; // protect any pointers that come from plugins, such as the source control cached pointer.
    static const char* m_crashLogFileName;
};

