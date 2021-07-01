/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    void DeleteThis() { delete this; };
    IEditorClassFactory* GetClassFactory();
    CEditorCommandManager* GetCommandManager() { return m_pCommandManager; };
    ICommandManager* GetICommandManager() { return m_pCommandManager; }
    void ExecuteCommand(const char* sCommand, ...);
    void ExecuteCommand(const QString& command);
    void SetDocument(CCryEditDoc* pDoc);
    CCryEditDoc* GetDocument() const;
    bool IsLevelLoaded() const override;
    void SetModifiedFlag(bool modified = true);
    void SetModifiedModule(EModifiedModule  eModifiedModule, bool boSet = true);
    bool IsLevelExported() const;
    bool SetLevelExported(bool boExported = true);
    void InitFinished();
    bool IsModified();
    bool IsInitialized() const{ return m_bInitialized; }
    bool SaveDocument();
    ISystem*    GetSystem();
    void WriteToConsole(const char* string) { CLogFile::WriteLine(string); };
    void WriteToConsole(const QString& string) { CLogFile::WriteLine(string); };
    // Change the message in the status bar
    void SetStatusText(const QString& pszString);
    virtual IMainStatusBar* GetMainStatusBar() override;
    bool ShowConsole([[maybe_unused]] bool show)
    {
        //if (AfxGetMainWnd())return ((CMainFrame *) (AfxGetMainWnd()))->ShowConsole(show);
        return false;
    }

    void SetConsoleVar(const char* var, float value);
    float GetConsoleVar(const char* var);
    //! Query main window of the editor
    QMainWindow* GetEditorMainWindow() const override
    {
        return MainWindow::instance();
    };

    QString GetPrimaryCDFolder();
    QString GetLevelName() override;
    QString GetLevelFolder();
    QString GetLevelDataFolder();
    QString GetSearchPath(EEditorPathName path);
    QString GetResolvedUserFolder();
    bool ExecuteConsoleApp(const QString& CommandLine, QString& OutputText, bool bNoTimeOut = false, bool bShowWindow = false);
    virtual bool IsInGameMode() override;
    virtual void SetInGameMode(bool inGame) override;
    virtual bool IsInSimulationMode() override;
    virtual bool IsInTestMode() override;
    virtual bool IsInPreviewMode() override;
    virtual bool IsInConsolewMode() override;
    virtual bool IsInLevelLoadTestMode() override;
    virtual bool IsInMatEditMode() override { return m_bMatEditMode; }

    //! Enables/Disable updates of editor.
    void EnableUpdate(bool enable) { m_bUpdates = enable; };
    //! Enable/Disable accelerator table, (Enabled by default).
    void EnableAcceleratos(bool bEnable);
    CGameEngine* GetGameEngine() { return m_pGameEngine; };
    CDisplaySettings*   GetDisplaySettings() { return m_pDisplaySettings; };
    const SGizmoParameters& GetGlobalGizmoParameters();
    CBaseObject* NewObject(const char* typeName, const char* fileName = "", const char* name = "", float x = 0.0f, float y = 0.0f, float z = 0.0f, bool modifyDoc = true);
    void DeleteObject(CBaseObject* obj);
    CBaseObject* CloneObject(CBaseObject* obj);
    IObjectManager* GetObjectManager();
    // This will return a null pointer if CrySystem is not loaded before
    // Global Sandbox Settings are loaded from the registry before CrySystem
    // At that stage GetSettingsManager will return null and xml node in
    // memory will not be populated with Sandbox Settings.
    // After m_IEditor is created and CrySystem loaded, it is possible
    // to feed memory node with all necessary data needed for export
    // (gSettings.Load() and CXTPDockingPaneManager/CXTPDockingPaneLayout Sandbox layout management)
    CSettingsManager* GetSettingsManager();
    CSelectionGroup*    GetSelection();
    int ClearSelection();
    CBaseObject* GetSelectedObject();
    void SelectObject(CBaseObject* obj);
    void LockSelection(bool bLock);
    bool IsSelectionLocked();

    IDataBaseManager* GetDBItemManager(EDataBaseItemType itemType);
    CMusicManager* GetMusicManager() { return m_pMusicManager; };

    IEditorFileMonitor* GetFileMonitor() override;
    void RegisterEventLoopHook(IEventLoopHook* pHook) override;
    void UnregisterEventLoopHook(IEventLoopHook* pHook) override;
    IIconManager* GetIconManager();
    float GetTerrainElevation(float x, float y);
    Editor::EditorQtApplication* GetEditorQtApplication() { return m_QtApplication; }
    const QColor& GetColorByName(const QString& name) override;

    //////////////////////////////////////////////////////////////////////////
    IMovieSystem* GetMovieSystem()
    {
        if (m_pSystem)
        {
            return m_pSystem->GetIMovieSystem();
        }
        return NULL;
    };

    CPluginManager* GetPluginManager() { return m_pPluginManager; }
    CViewManager* GetViewManager();
    CViewport* GetActiveView();
    void SetActiveView(CViewport* viewport);

    CLevelIndependentFileMan* GetLevelIndependentFileMan() { return m_pLevelIndependentFileMan; }

    void UpdateViews(int flags, const AABB* updateRegion);
    void ResetViews();
    void ReloadTrackView();
    Vec3 GetMarkerPosition() { return m_marker; };
    void SetMarkerPosition(const Vec3& pos) { m_marker = pos; };
    void    SetSelectedRegion(const AABB& box);
    void    GetSelectedRegion(AABB& box);
    bool AddToolbarItem(uint8 iId, IUIEvent* pIHandler);
    void SetDataModified();
    void SetOperationMode(EOperationMode mode);
    EOperationMode GetOperationMode();

    ITransformManipulator* ShowTransformManipulator(bool bShow);
    ITransformManipulator* GetTransformManipulator();
    void SetAxisConstraints(AxisConstrains axis);
    AxisConstrains GetAxisConstrains();
    void SetAxisVectorLock(bool bAxisVectorLock) { m_bAxisVectorLock = bAxisVectorLock; }
    bool IsAxisVectorLocked() { return m_bAxisVectorLock; }
    void SetTerrainAxisIgnoreObjects(bool bIgnore);
    bool IsTerrainAxisIgnoreObjects();
    void SetReferenceCoordSys(RefCoordSys refCoords);
    RefCoordSys GetReferenceCoordSys();
    XmlNodeRef FindTemplate(const QString& templateName);
    void AddTemplate(const QString& templateName, XmlNodeRef& tmpl);
   
    const QtViewPane* OpenView(QString sViewClassName, bool reuseOpened = true) override;

    /**
     * Returns the top level widget which is showing the view pane with the specified name.
     * To access the child widget which actually implements the view pane use this instead:
     * QtViewPaneManager::FindViewPane<MyDialog>(name);
     */
    QWidget* FindView(QString viewClassName) override;

    bool CloseView(const char* sViewClassName);
    bool SetViewFocus(const char* sViewClassName);

    virtual QWidget* OpenWinWidget(WinWidgetId openId) override;
    virtual WinWidget::WinWidgetManager* GetWinWidgetManager() const override;

    // close ALL panels related to classId, used when unloading plugins.
    void CloseView(const GUID& classId);
    bool SelectColor(QColor &color, QWidget *parent = 0) override;
    void Update();
    SFileVersion GetFileVersion() { return m_fileVersion; };
    SFileVersion GetProductVersion() { return m_productVersion; };
    //! Get shader enumerator.
    CUndoManager* GetUndoManager() { return m_pUndoManager; };
    void BeginUndo();
    void RestoreUndo(bool undo);
    void AcceptUndo(const QString& name);
    void CancelUndo();
    void SuperBeginUndo();
    void SuperAcceptUndo(const QString& name);
    void SuperCancelUndo();
    void SuspendUndo();
    void ResumeUndo();
    void Undo();
    void Redo();
    bool IsUndoRecording();
    bool IsUndoSuspended();
    void RecordUndo(IUndoObject* obj);
    bool FlushUndo(bool isShowMessage = false);
    bool ClearLastUndoSteps(int steps);
    bool ClearRedoStack();
    //! Retrieve current animation context.
    CAnimationContext* GetAnimation();
    CTrackViewSequenceManager* GetSequenceManager() override;
    ITrackViewSequenceManager* GetSequenceManagerInterface() override;

    CToolBoxManager* GetToolBoxManager() { return m_pToolBoxManager; };
    IErrorReport* GetErrorReport() { return m_pErrorReport; }
    IErrorReport* GetLastLoadedLevelErrorReport() { return m_pLasLoadedLevelErrorReport; }
    void StartLevelErrorReportRecording() override;
    void CommitLevelErrorReport() {SAFE_DELETE(m_pLasLoadedLevelErrorReport); m_pLasLoadedLevelErrorReport = new CErrorReport(*m_pErrorReport); }
    virtual IFileUtil* GetFileUtil() override { return m_pFileUtil;  }
    void Notify(EEditorNotifyEvent event);
    void NotifyExcept(EEditorNotifyEvent event, IEditorNotifyListener* listener);
    void RegisterNotifyListener(IEditorNotifyListener* listener);
    void UnregisterNotifyListener(IEditorNotifyListener* listener);
    //! Register document notifications listener.
    void RegisterDocListener(IDocListener* listener);
    //! Unregister document notifications listener.
    void UnregisterDocListener(IDocListener* listener);
    //! Retrieve interface to the source control.
    ISourceControl* GetSourceControl();
    //! Retrieve true if source control is provided and enabled in settings
    bool IsSourceControlAvailable() override;
    //! Only returns true if source control is both available AND currently connected and functioning
    bool IsSourceControlConnected() override;
    //! Setup Material Editor mode
    void SetMatEditMode(bool bIsMatEditMode);
    CUIEnumsDatabase* GetUIEnumsDatabase() { return m_pUIEnumsDatabase; };
    void AddUIEnums();
    void GetMemoryUsage(ICrySizer* pSizer);
    void ReduceMemory();
    // Get Export manager
    IExportManager* GetExportManager();
    // Set current configuration spec of the editor.
    void SetEditorConfigSpec(ESystemConfigSpec spec, ESystemConfigPlatform platform);
    ESystemConfigSpec GetEditorConfigSpec() const;
    ESystemConfigPlatform GetEditorConfigPlatform() const;
    void ReloadTemplates();
    void AddErrorMessage(const QString& text, const QString& caption);
    IResourceSelectorHost* GetResourceSelectorHost() { return m_pResourceSelectorHost.get(); }
    virtual void ShowStatusText(bool bEnable);

    void OnObjectContextMenuOpened(QMenu* pMenu, const CBaseObject* pObject);
    virtual void RegisterObjectContextMenuExtension(TContextMenuExtensionFunc func) override;

    virtual SSystemGlobalEnvironment* GetEnv() override;
    virtual IBaseLibraryManager* GetMaterialManagerLibrary() override; // Vladimir@Conffx
    virtual IEditorMaterialManager* GetIEditorMaterialManager() override; // Vladimir@Conffx
    virtual IImageUtil* GetImageUtil() override;  // Vladimir@conffx
    virtual SEditorSettings* GetEditorSettings() override;
    virtual IEditorPanelUtils* GetEditorPanelUtils() override;
    virtual ILogFile* GetLogFile() override { return m_pLogFile; }

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
    std::unique_ptr<IResourceSelectorHost> m_pResourceSelectorHost;
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

    CryMutex m_pluginMutex; // protect any pointers that come from plugins, such as the source control cached pointer.
    static const char* m_crashLogFileName;
};

