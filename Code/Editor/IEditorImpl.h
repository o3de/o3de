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

class CObjectManager;
class CUndoManager;
class CGameEngine;
class CExportManager;
class CErrorsDlg;
class CIconManager;
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

namespace WinWidget
{
    class WinWidgetManager;
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
    IEditorClassFactory* GetClassFactory() override;
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
    ISystem*    GetSystem() override;
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
    bool IsInMatEditMode() override { return m_bMatEditMode; }

    //! Enables/Disable updates of editor.
    void EnableUpdate(bool enable) override { m_bUpdates = enable; };
    //! Enable/Disable accelerator table, (Enabled by default).
    void EnableAcceleratos(bool bEnable) override;
    CGameEngine* GetGameEngine() override { return m_pGameEngine; };
    CDisplaySettings*   GetDisplaySettings() override { return m_pDisplaySettings; };
    const SGizmoParameters& GetGlobalGizmoParameters() override;
    CBaseObject* NewObject(const char* typeName, const char* fileName = "", const char* name = "", float x = 0.0f, float y = 0.0f, float z = 0.0f, bool modifyDoc = true) override;
    void DeleteObject(CBaseObject* obj) override;
    CBaseObject* CloneObject(CBaseObject* obj) override;
    IObjectManager* GetObjectManager() override;
    // This will return a null pointer if CrySystem is not loaded before
    // Global Sandbox Settings are loaded from the registry before CrySystem
    // At that stage GetSettingsManager will return null and xml node in
    // memory will not be populated with Sandbox Settings.
    // After m_IEditor is created and CrySystem loaded, it is possible
    // to feed memory node with all necessary data needed for export
    // (gSettings.Load() and CXTPDockingPaneManager/CXTPDockingPaneLayout Sandbox layout management)
    CSettingsManager* GetSettingsManager() override;
    CSelectionGroup*    GetSelection() override;
    int ClearSelection() override;
    CBaseObject* GetSelectedObject() override;
    void SelectObject(CBaseObject* obj) override;
    void LockSelection(bool bLock) override;
    bool IsSelectionLocked() override;

    IDataBaseManager* GetDBItemManager(EDataBaseItemType itemType) override;
    CMusicManager* GetMusicManager() override { return m_pMusicManager; };

    IEditorFileMonitor* GetFileMonitor() override;
    void RegisterEventLoopHook(IEventLoopHook* pHook) override;
    void UnregisterEventLoopHook(IEventLoopHook* pHook) override;
    IIconManager* GetIconManager() override;
    float GetTerrainElevation(float x, float y) override;
    Editor::EditorQtApplication* GetEditorQtApplication() override { return m_QtApplication; }
    const QColor& GetColorByName(const QString& name) override;

    //////////////////////////////////////////////////////////////////////////
    IMovieSystem* GetMovieSystem() override
    {
        if (m_pSystem)
        {
            return m_pSystem->GetIMovieSystem();
        }
        return nullptr;
    };

    CPluginManager* GetPluginManager() override { return m_pPluginManager; }
    CViewManager* GetViewManager() override;
    CViewport* GetActiveView() override;
    void SetActiveView(CViewport* viewport) override;

    CLevelIndependentFileMan* GetLevelIndependentFileMan() override { return m_pLevelIndependentFileMan; }

    void UpdateViews(int flags, const AABB* updateRegion) override;
    void ResetViews() override;
    void ReloadTrackView() override;
    Vec3 GetMarkerPosition() override { return m_marker; };
    void SetMarkerPosition(const Vec3& pos) override { m_marker = pos; };
    void    SetSelectedRegion(const AABB& box) override;
    void    GetSelectedRegion(AABB& box) override;
    bool AddToolbarItem(uint8 iId, IUIEvent* pIHandler);
    void SetDataModified() override;
    void SetOperationMode(EOperationMode mode) override;
    EOperationMode GetOperationMode() override;

    ITransformManipulator* ShowTransformManipulator(bool bShow) override;
    ITransformManipulator* GetTransformManipulator() override;
    void SetAxisConstraints(AxisConstrains axis) override;
    AxisConstrains GetAxisConstrains() override;
    void SetAxisVectorLock(bool bAxisVectorLock) override { m_bAxisVectorLock = bAxisVectorLock; }
    bool IsAxisVectorLocked() override { return m_bAxisVectorLock; }
    void SetTerrainAxisIgnoreObjects(bool bIgnore) override;
    bool IsTerrainAxisIgnoreObjects() override;
    void SetReferenceCoordSys(RefCoordSys refCoords) override;
    RefCoordSys GetReferenceCoordSys() override;
    XmlNodeRef FindTemplate(const QString& templateName) override;
    void AddTemplate(const QString& templateName, XmlNodeRef& tmpl) override;

    const QtViewPane* OpenView(QString sViewClassName, bool reuseOpened = true) override;

    /**
     * Returns the top level widget which is showing the view pane with the specified name.
     * To access the child widget which actually implements the view pane use this instead:
     * QtViewPaneManager::FindViewPane<MyDialog>(name);
     */
    QWidget* FindView(QString viewClassName) override;

    bool CloseView(const char* sViewClassName) override;
    bool SetViewFocus(const char* sViewClassName) override;

    QWidget* OpenWinWidget(WinWidgetId openId) override;
    WinWidget::WinWidgetManager* GetWinWidgetManager() const override;

    // close ALL panels related to classId, used when unloading plugins.
    void CloseView(const GUID& classId) override;
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
    //! Register document notifications listener.
    void RegisterDocListener(IDocListener* listener) override;
    //! Unregister document notifications listener.
    void UnregisterDocListener(IDocListener* listener) override;
    //! Retrieve interface to the source control.
    ISourceControl* GetSourceControl() override;
    //! Retrieve true if source control is provided and enabled in settings
    bool IsSourceControlAvailable() override;
    //! Only returns true if source control is both available AND currently connected and functioning
    bool IsSourceControlConnected() override;
    //! Setup Material Editor mode
    void SetMatEditMode(bool bIsMatEditMode);
    CUIEnumsDatabase* GetUIEnumsDatabase() override { return m_pUIEnumsDatabase; };
    void AddUIEnums() override;
    void ReduceMemory() override;
    // Get Export manager
    IExportManager* GetExportManager() override;
    // Set current configuration spec of the editor.
    void SetEditorConfigSpec(ESystemConfigSpec spec, ESystemConfigPlatform platform) override;
    ESystemConfigSpec GetEditorConfigSpec() const override;
    ESystemConfigPlatform GetEditorConfigPlatform() const override;
    void ReloadTemplates() override;
    void AddErrorMessage(const QString& text, const QString& caption);
    void ShowStatusText(bool bEnable) override;

    void OnObjectContextMenuOpened(QMenu* pMenu, const CBaseObject* pObject);
    void RegisterObjectContextMenuExtension(TContextMenuExtensionFunc func) override;

    SSystemGlobalEnvironment* GetEnv() override;
    IBaseLibraryManager* GetMaterialManagerLibrary() override; // Vladimir@Conffx
    IEditorMaterialManager* GetIEditorMaterialManager() override; // Vladimir@Conffx
    IImageUtil* GetImageUtil() override;  // Vladimir@conffx
    SEditorSettings* GetEditorSettings() override;
    IEditorPanelUtils* GetEditorPanelUtils() override;
    ILogFile* GetLogFile() override { return m_pLogFile; }

    void UnloadPlugins() override;
    void LoadPlugins() override;

    QMimeData* CreateQMimeData() const override;
    void DestroyQMimeData(QMimeData* data) const override;

protected:

    AZStd::string LoadProjectIdFromProjectData();

    void DetectVersion();
    void RegisterTools();
    void SetPrimaryCDFolder();

    //! List of all notify listeners.
    std::list<IEditorNotifyListener*> m_listeners;

    EOperationMode m_operationMode;
    ISystem* m_pSystem;
    IFileUtil* m_pFileUtil;
    CClassFactory* m_pClassFactory;
    CEditorCommandManager* m_pCommandManager;
    CObjectManager* m_pObjectManager;
    CPluginManager* m_pPluginManager;
    CViewManager*   m_pViewManager;
    CUndoManager* m_pUndoManager;
    Vec3 m_marker;
    AABB m_selectedRegion;
    AxisConstrains m_selectedAxis;
    RefCoordSys m_refCoordsSys;
    bool m_bAxisVectorLock;
    bool m_bUpdates;
    bool m_bTerrainAxisIgnoreObjects;
    SFileVersion m_fileVersion;
    SFileVersion m_productVersion;
    CXmlTemplateRegistry m_templateRegistry;
    CDisplaySettings* m_pDisplaySettings;
    CIconManager* m_pIconManager;
    std::unique_ptr<SGizmoParameters> m_pGizmoParameters;
    QString m_primaryCDFolder;
    QString m_userFolder;
    bool m_bSelectionLocked;
    class CAxisGizmo* m_pAxisGizmo;
    CGameEngine* m_pGameEngine;
    CAnimationContext* m_pAnimationContext;
    CTrackViewSequenceManager* m_pSequenceManager;
    CToolBoxManager* m_pToolBoxManager;
    CMusicManager* m_pMusicManager;
    CErrorReport* m_pErrorReport;
    //! Contains the error reports for the last loaded level.
    CErrorReport* m_pLasLoadedLevelErrorReport;
    //! Global instance of error report class.
    CErrorsDlg* m_pErrorsDlg;
    //! Source control interface.
    ISourceControl* m_pSourceControl;
    IEditorPanelUtils* m_panelEditorUtils;

    CSelectionTreeManager* m_pSelectionTreeManager;

    CUIEnumsDatabase* m_pUIEnumsDatabase;
    //! CConsole Synchronization
    CConsoleSynchronization* m_pConsoleSync;
    //! Editor Settings Manager
    CSettingsManager* m_pSettingsManager;

    CLevelIndependentFileMan* m_pLevelIndependentFileMan;

    //! Export manager for exporting objects and a terrain from the game to DCC tools
    CExportManager* m_pExportManager;
    std::unique_ptr<CEditorFileMonitor> m_pEditorFileMonitor;
    QString m_selectFileBuffer;
    QString m_levelNameBuffer;

    IAWSResourceManager* m_awsResourceManager;
    std::unique_ptr<WinWidget::WinWidgetManager> m_winWidgetManager;

    //! True if the editor is in material edit mode. Fast preview of materials.
    //! In this mode only very limited functionality is available.
    bool m_bMatEditMode;
    bool m_bShowStatusText;
    bool m_bInitialized;
    bool m_bExiting;
    static void CmdPy(IConsoleCmdArgs* pArgs);

    std::vector<TContextMenuExtensionFunc> m_objectContextMenuExtensions;

    Editor::EditorQtApplication* const m_QtApplication = nullptr;

    // This has to be absolute for the namespace since there is also a class called AssetDatabase that causes issues in unity builds
    ::AssetDatabase::AssetDatabaseLocationListener* m_pAssetDatabaseLocationListener;
    AzAssetBrowserRequestHandler* m_pAssetBrowserRequestHandler;
    AssetEditorRequestsHandler* m_assetEditorRequestsHandler;

    IImageUtil* m_pImageUtil;  // Vladimir@conffx
    ILogFile* m_pLogFile;  // Vladimir@conffx

    AZStd::mutex m_pluginMutex; // protect any pointers that come from plugins, such as the source control cached pointer.
    static const char* m_crashLogFileName;
};

