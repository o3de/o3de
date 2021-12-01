/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#ifdef PLUGIN_EXPORTS
#define PLUGIN_API DLL_EXPORT
#else
#define PLUGIN_API DLL_IMPORT
#endif

#include <ISystem.h>
#include "Include/SandboxAPI.h"
#include "Util/UndoUtil.h"
#include <CryVersion.h>

#include <WinWidgetId.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Debug/Budget.h>

class QMenu;

struct CRuntimeClass;
struct QtViewPane;
class QMainWindow;
struct QMetaObject;

class CBaseObject;
class CCryEditDoc;
class CSelectionGroup;
class CAnimationContext;
class CTrackViewSequenceManager;
class CGameEngine;
struct IIconManager;
class CToolBoxManager;
class CClassFactory;
class CMusicManager;
struct IEditorParticleManager;
class CEAXPresetManager;
class CErrorReport;
class CBaseLibraryItem;
class ICommandManager;
class CEditorCommandManager;
class CHyperGraphManager;
class CConsoleSynchronization;
class CUIEnumsDatabase;
struct ISourceControl;
struct IEditorClassFactory;
struct IDataBaseItem;
struct ITransformManipulator;
struct IDataBaseManager;
class IFacialEditor;
class CDialog;
#if defined(AZ_PLATFORM_WINDOWS)
class C3DConnexionDriver;
#endif
class CSettingsManager;
struct IExportManager;
class CDisplaySettings;
struct SGizmoParameters;
class CLevelIndependentFileMan;
class CSelectionTreeManager;
struct SEditorSettings;
class CGameExporter;
class IAWSResourceManager;
struct IEditorPanelUtils;

namespace WinWidget
{
    class WinWidgetManager;
}

struct ISystem;
struct IRenderer;
struct AABB;
struct IEventLoopHook;
struct IErrorReport; // Vladimir@conffx
struct IFileUtil;  // Vladimir@conffx
struct IEditorLog;  // Vladimir@conffx
struct IEditorMaterialManager;  // Vladimir@conffx
struct IBaseLibraryManager;  // Vladimir@conffx
struct IImageUtil;  // Vladimir@conffx
struct IEditorParticleUtils;  // Leroy@conffx
struct ILogFile; // Vladimir@conffx

// Qt

class QWidget;
class QMimeData;
class QString;
class QColor;
class QPixmap;

#if !AZ_TRAIT_OS_PLATFORM_APPLE && !defined(AZ_PLATFORM_LINUX)
typedef void* HANDLE;
struct HWND__;
typedef HWND__* HWND;
#endif

namespace Editor
{
    class EditorQtApplication;
}


// Global editor notify events.
enum EEditorNotifyEvent
{
    // Global events.
    eNotify_OnInit = 10,               // Sent after editor fully initialized.
    eNotify_OnQuit,                    // Sent before editor quits.
    eNotify_OnIdleUpdate,              // Sent every frame while editor is idle.

    // Document events.
    eNotify_OnBeginNewScene,           // Sent when the document is begin to be cleared.
    eNotify_OnEndNewScene,             // Sent after the document have been cleared.
    eNotify_OnBeginSceneOpen,          // Sent when document is about to be opened.
    eNotify_OnEndSceneOpen,            // Sent after document have been opened.
    eNotify_OnBeginSceneSave,          // Sent when document is about to be saved.
    eNotify_OnEndSceneSave,            // Sent after document have been saved.
    eNotify_OnBeginLayerExport,        // Sent when a layer is about to be exported.
    eNotify_OnEndLayerExport,          // Sent after a layer have been exported.
    eNotify_OnCloseScene,              // Send when the document is about to close.
    eNotify_OnSceneClosed,             // Send when the document is closed.
    eNotify_OnBeginLoad,               // Sent when the document is start to load.
    eNotify_OnEndLoad,                 // Sent when the document loading is finished

    // Editing events.
    eNotify_OnEditModeChange,          // Sent when editing mode change (move,rotate,scale,....)
    eNotify_OnEditToolChange,          // Sent when edit tool is changed (ObjectMode,TerrainModify,....)

    // Deferred terrain create event.
    eNotify_OnBeginTerrainCreate,      // Sent when terrain is created later (and not during level creation)
    eNotify_OnEndTerrainCreate,        // Sent when terrain is created later (and not during level creation)

    // Game related events.
    eNotify_OnBeginGameMode,           // Sent when editor goes to game mode.
    eNotify_OnEndGameMode,             // Sent when editor goes out of game mode.

    // AI/Physics simulation related events.
    eNotify_OnBeginSimulationMode,     // Sent when simulation mode is started.
    eNotify_OnEndSimulationMode,       // Sent when editor goes out of simulation mode.

    // UI events.
    eNotify_OnUpdateViewports,         // Sent when editor needs to update data in the viewports.
    eNotify_OnReloadTrackView,         // Sent when editor needs to update the track view.
    eNotify_OnSplashScreenCreated,     // Sent when the editor splash screen was created.
    eNotify_OnSplashScreenDestroyed,   // Sent when the editor splash screen was destroyed.

    eNotify_OnInvalidateControls,      // Sent when editor needs to update some of the data that can be cached by controls like combo boxes.
    eNotify_OnStyleChanged,            // Sent when UI color theme was changed

    // Object events.
    eNotify_OnSelectionChange,         // Sent when object selection change.
    eNotify_OnPlaySequence,            // Sent when editor start playing animation sequence.
    eNotify_OnStopSequence,            // Sent when editor stop playing animation sequence.

    // Task specific events.
    eNotify_OnTerrainRebuild,          // Sent when terrain was rebuilt (resized,...)
    eNotify_OnBeginTerrainRebuild,     // Sent when terrain begin rebuilt (resized,...)
    eNotify_OnEndTerrainRebuild,       // Sent when terrain end rebuilt (resized,...)
    eNotify_OnVegetationObjectSelection, // When vegetation objects selection change.
    eNotify_OnVegetationPanelUpdate,   // When vegetation objects selection change.

    eNotify_OnDisplayRenderUpdate,     // Sent when editor finish terrain texture generation.

    eNotify_OnDataBaseUpdate,          // DataBase Library was modified.

    eNotify_OnLayerImportBegin,         //layer import was started
    eNotify_OnLayerImportEnd,           //layer import completed

    eNotify_OnBeginSWNewScene,          // Sent when SW document is begin to be cleared.
    eNotify_OnEndSWNewScene,                // Sent after SW document have been cleared.
    eNotify_OnBeginSWMoveTo,                // moveto operation was started
    eNotify_OnEndSWMoveTo,          // moveto operation completed
    eNotify_OnSWLockUnlock,             // Sent when commit, rollback or getting lock from segmented world
    eNotify_OnSWVegetationStatusChange, // When changed segmented world status of vegetation map

    eNotify_OnBeginUndoRedo,
    eNotify_OnEndUndoRedo,
    eNotify_CameraChanged, // When the active viewport camera was changed

    eNotify_OnTextureLayerChange,      // Sent when texture layer was added, removed or moved

    eNotify_OnSplatmapImport, // Sent when splatmaps get imported

    eNotify_OnParticleUpdate,          // A particle effect was modified.
    eNotify_OnAddAWSProfile,           // An AWS profile was added
    eNotify_OnSwitchAWSProfile,        // The AWS profile was switched
    eNotify_OnSwitchAWSDeployment,     // The AWS deployment was switched
    eNotify_OnFirstAWSUse,             // This should only be emitted once

    eNotify_OnRefCoordSysChange,

    // Entity selection events.
    eNotify_OnEntitiesSelected,
    eNotify_OnEntitiesDeselected,

    // More document events - added here in case enum values matter to any event consumers, metrics reporters, etc.
    eNotify_OnBeginCreate,               // Sent when the document is starting to be created.
    eNotify_OnEndCreate,                 // Sent when the document creation is finished.

};

// UI event handler
struct IUIEvent
{
    virtual void OnClick(DWORD dwId) = 0;
    virtual bool IsEnabled(DWORD dwId) = 0;
    virtual bool IsChecked(DWORD dwId) = 0;
    virtual const char* GetUIElementName(DWORD dwId) = 0;
};

//! Add object that implements this interface to Load listeners of IEditor
//! To receive notifications when new document is loaded.
struct IDocListener
{
    virtual ~IDocListener() = default;

    //! Called after new level is created.
    virtual void OnNewDocument() = 0;
    //! Called after level have been loaded.
    virtual void OnLoadDocument() = 0;
    //! Called when document is being closed.
    virtual void OnCloseDocument() = 0;
};

//! Derive from this class if you want to register for getting global editor notifications.
struct IEditorNotifyListener
{
    bool m_bIsRegistered;

    IEditorNotifyListener()
        : m_bIsRegistered(false)
    {
    }
    virtual ~IEditorNotifyListener()
    {
        if (m_bIsRegistered)
        {
            CryFatalError("Destroying registered IEditorNotifyListener");
        }
    }

    //! called by the editor to notify the listener about the specified event.
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) = 0;
};

//! Axis constrains value.
enum AxisConstrains
{
    AXIS_NONE = 0,
    AXIS_X,
    AXIS_Y,
    AXIS_Z,
    AXIS_XY,
    AXIS_YZ,
    AXIS_XZ,
    AXIS_XYZ,
    //! Follow terrain constrain
    AXIS_TERRAIN,
};

//! Reference coordinate system values
enum RefCoordSys
{ // Don't change this order. Should be in the same order as MainWindow::CreateRefCoordComboBox()
    COORDS_VIEW = 0,
    COORDS_LOCAL,
    COORDS_PARENT,
    COORDS_WORLD,
    COORDS_USERDEFINED,
    LAST_COORD_SYSTEM, // Must always be the last member
};

// Insert locations for menu items
enum EMenuInsertLocation
{
    // Custom menu of the plugin
    eMenuPlugin,
    // Predefined editor menus
    eMenuEdit,
    eMenuFile,
    eMenuInsert,
    eMenuGenerators,
    eMenuScript,
    eMenuView,
    eMenuHelp
};

//! Global editor operation mode
enum EOperationMode
{
    eOperationModeNone = 0, // None
    eCompositingMode, // Normal operation mode where objects are composited in the scene
    eModellingMode // Geometry modeling mode
};

//! Mouse events that viewport can send
enum EMouseEvent
{
    eMouseMove,
    eMouseLDown,
    eMouseLUp,
    eMouseLDblClick,
    eMouseRDown,
    eMouseRUp,
    eMouseRDblClick,
    eMouseMDown,
    eMouseMUp,
    eMouseMDblClick,
    eMouseWheel,
    eMouseLeave,
};

//! Viewports update flags
enum UpdateConentFlags
{
    eUpdateHeightmap = 0x01,
    eUpdateStatObj = 0x02,
    eUpdateObjects = 0x04, //! Update objects in viewport.
    eRedrawViewports = 0x08 //! Just redraw viewports..
};

enum MouseCallbackFlags
{
    MK_CALLBACK_FLAGS = 0x100
};

//! Types of database items
enum EDataBaseItemType
{
    EDB_TYPE_MATERIAL,
    EDB_TYPE_PARTICLE,
    EDB_TYPE_MUSIC,
    EDB_TYPE_EAXPRESET,
    EDB_TYPE_SOUNDMOOD,
    EDB_TYPE_FLARE
};

enum EEditorPathName
{
    EDITOR_PATH_OBJECTS,
    EDITOR_PATH_TEXTURES,
    EDITOR_PATH_SOUNDS,
    EDITOR_PATH_MATERIALS,
    EDITOR_PATH_UI_ICONS,
    EDITOR_PATH_LAST
};

enum EModifiedModule
{
    eModifiedNothing = 0x0,
    eModifiedTerrain = BIT(0),
    eModifiedBrushes = BIT(1),
    eModifiedEntities = BIT(2),
    eModifiedAll = -1
};

//! Class provided by editor for various registration functions.
struct CRegistrationContext
{
    CEditorCommandManager* pCommandManager;
    CClassFactory* pClassFactory;
};

//! Interface provided by editor to reach status bar functionality.
struct IMainStatusBar
{
    virtual void SetStatusText(const QString& text) = 0;
    virtual QWidget* SetItem(QString indicatorName, QString text, QString tip, int iconId) = 0;
    virtual QWidget* SetItem(QString indicatorName, QString text, QString tip, const QPixmap& icon) = 0;

    virtual QWidget* GetItem(QString indicatorName) = 0;
};

// forward declaration
struct IAnimSequence;
class CTrackViewSequence;

//! Interface to expose TrackViewSequenceManager functionality to SequenceComponent
struct ITrackViewSequenceManager
{
    virtual IAnimSequence* OnCreateSequenceObject(QString name, bool isLegacySequence = true, AZ::EntityId entityId = AZ::EntityId()) = 0;

    //! Notifies of the delete of a sequence entity OR legacy sequence object
    //! @param entityId The Sequence Component Entity Id OR the legacy sequence object Id packed in the lower 32-bits, as returned from IAnimSequence::GetSequenceEntityId()
    virtual void OnDeleteSequenceEntity(const AZ::EntityId& entityId) = 0;

    //! Get the first sequence with the given name. They may be more than one sequence with this name.
    //! Only intended for use with scripting or other cases where a user provides a name.
    virtual CTrackViewSequence* GetSequenceByName(QString name) const = 0;

    //! Get the sequence with the given EntityId. For legacy support, legacy sequences can be found by giving
    //! the sequence ID in the lower 32 bits of the EntityId.
    virtual CTrackViewSequence* GetSequenceByEntityId(const AZ::EntityId& entityId) const = 0;

    virtual void OnCreateSequenceComponent(AZStd::intrusive_ptr<IAnimSequence>& sequence) = 0;

    virtual void OnSequenceActivated(const AZ::EntityId& entityId) = 0;
};

//! Interface to expose TrackViewSequence functionality to SequenceComponent
struct ITrackViewSequence
{
    virtual void Load() = 0;
};

//! Interface to permit usage of editor functionality inside the plugin
struct IEditor
{
    virtual void DeleteThis() = 0;
    //! Access to Editor ISystem interface.
    virtual ISystem* GetSystem() = 0;
    //! Access to class factory.
    virtual IEditorClassFactory* GetClassFactory() = 0;
    //! Access to commands manager.
    virtual CEditorCommandManager* GetCommandManager() = 0;
    virtual ICommandManager* GetICommandManager() = 0;
    // Executes an Editor command.
    virtual void ExecuteCommand(const char* sCommand, ...) = 0;
    virtual void ExecuteCommand(const QString& sCommand) = 0;
    virtual void SetDocument(CCryEditDoc* pDoc) = 0;
    //! Get active document
    virtual CCryEditDoc* GetDocument() const = 0;
    //! Check if there is a level loaded
    virtual bool IsLevelLoaded() const = 0;
    //! Set document modified flag.
    virtual void SetModifiedFlag(bool modified = true) = 0;
    virtual void SetModifiedModule(EModifiedModule eModifiedModule, bool boSet = true) = 0;
    virtual bool IsLevelExported() const = 0;
    virtual bool SetLevelExported(bool boExported = true) = 0;
    //! Check if active document is modified.
    virtual bool IsModified() = 0;
    //! Save current document.
    virtual bool SaveDocument() = 0;
    //! Legacy version of WriteToConsole; don't use.
    virtual void WriteToConsole(const char* string) = 0;
    //! Write the passed string to the editors console
    virtual void WriteToConsole(const QString& string) = 0;
    //! Set value of console variable.
    virtual void SetConsoleVar(const char* var, float value) = 0;
    //! Get value of console variable.
    virtual float GetConsoleVar(const char* var) = 0;
    //! Shows or Hides console window.
    //! @return Previous visibility flag of console.
    virtual bool ShowConsole(bool show) = 0;
    // Get Main window status bar
    virtual IMainStatusBar* GetMainStatusBar() = 0;
    //! Change the message in the status bar
    virtual void SetStatusText(const QString& pszString) = 0;
    //! Query main window of the editor
    virtual QMainWindow* GetEditorMainWindow() const = 0;
    //! Returns the path of the editors Primary CD folder
    virtual QString GetPrimaryCDFolder() = 0;
    //! Get current level name (name only)
    virtual QString GetLevelName() = 0;
    //! Get path to folder of current level (Absolute, contains slash)
    virtual QString GetLevelFolder() = 0;
    //! Get path to folder of current level (absolute)
    virtual QString GetLevelDataFolder() = 0;
    //! Get path to folder of current level.
    virtual QString GetSearchPath(EEditorPathName path) = 0;
    //! This folder is supposed to store Sandbox user settings and state
    virtual QString GetResolvedUserFolder() = 0;
    //! Execute application and get console output.
    virtual bool ExecuteConsoleApp(
        const QString& CommandLine,
        QString& OutputText,
        bool bNoTimeOut = false,
        bool bShowWindow = false) = 0;
    //! Sets the document modified flag in the editor
    virtual void SetDataModified() = 0;
    //! Tells if editor startup is finished
    virtual bool IsInitialized() const = 0;
    //! Check if editor running in gaming mode.
    virtual bool IsInGameMode() = 0;
    //! Check if editor running in AI/Physics mode.
    virtual bool IsInSimulationMode() = 0;
    //! Set game mode of editor.
    virtual void SetInGameMode(bool inGame) = 0;
    //! Return true if Editor runs in the testing mode.
    virtual bool IsInTestMode() = 0;
    //! Return true if Editor runs in the preview mode.
    virtual bool IsInPreviewMode() = 0;
    //! Return true if Editor runs in the console only mode.
    virtual bool IsInConsolewMode() = 0;
    //! return true if editor is running the level load tests mode.
    virtual bool IsInLevelLoadTestMode() = 0;
    //! Return true if Editor runs in the material editing mode.
    virtual bool IsInMatEditMode() = 0;
    //! Enable/Disable updates of editor.
    virtual void EnableUpdate(bool enable) = 0;
    //! Enable/Disable accelerator table, (Enabled by default).
    virtual void EnableAcceleratos(bool bEnable) = 0;
    virtual SFileVersion GetFileVersion() = 0;
    virtual SFileVersion GetProductVersion() = 0;
    //! Retrieve pointer to game engine instance
    virtual CGameEngine* GetGameEngine() = 0;
    virtual CDisplaySettings* GetDisplaySettings() = 0;
    virtual const SGizmoParameters& GetGlobalGizmoParameters() = 0;
    //! Create new object
    virtual CBaseObject* NewObject(const char* typeName, const char* fileName = "", const char* name = "", float x = 0.0f, float y = 0.0f, float z = 0.0f, bool modifyDoc = true) = 0;
    //! Delete object
    virtual void DeleteObject(CBaseObject* obj) = 0;
    //! Get current selection group
    virtual CSelectionGroup* GetSelection() = 0;
    virtual CBaseObject* GetSelectedObject() = 0;
    virtual int ClearSelection() = 0;
    //! Select object
    virtual void SelectObject(CBaseObject* obj) = 0;
    //! Lock current objects selection
    //! While selection locked, other objects cannot be selected or unselected
    virtual void LockSelection(bool bLock) = 0;
    //! Check if selection is currently locked
    virtual bool IsSelectionLocked() = 0;
    //! Get access to object manager.
    virtual struct IObjectManager* GetObjectManager() = 0;
    virtual CSettingsManager* GetSettingsManager() = 0;
    //! Get DB manager that own items of specified type.
    virtual IDataBaseManager* GetDBItemManager(EDataBaseItemType itemType) = 0;
    virtual IBaseLibraryManager* GetMaterialManagerLibrary() = 0; // Vladimir@conffx
    virtual IEditorMaterialManager* GetIEditorMaterialManager() = 0; // Vladimir@Conffx
    //! Returns IconManager.
    virtual IIconManager* GetIconManager() = 0;
    //! Get Panel Editor Utilities
    virtual IEditorPanelUtils* GetEditorPanelUtils() = 0;
    //! Get Music Manager.
    virtual CMusicManager* GetMusicManager() = 0;
    virtual float GetTerrainElevation(float x, float y) = 0;
    virtual Editor::EditorQtApplication* GetEditorQtApplication() = 0;
    virtual const QColor& GetColorByName(const QString& name) = 0;

    virtual struct IMovieSystem* GetMovieSystem() = 0;
    virtual class CPluginManager* GetPluginManager() = 0;
    virtual class CViewManager* GetViewManager() = 0;
    virtual class CViewport* GetActiveView() = 0;
    virtual void SetActiveView(CViewport* viewport) = 0;
    virtual struct IEditorFileMonitor* GetFileMonitor() = 0;

    // These are needed for Qt integration:
    virtual void RegisterEventLoopHook(IEventLoopHook* pHook) = 0;
    virtual void UnregisterEventLoopHook(IEventLoopHook* pHook) = 0;
    // ^^^

    //! QMimeData is used by the Qt clipboard.
    //! IMPORTANT: Any QMimeData allocated for the clipboard will be deleted
    //! when the editor exists. If a QMimeData is allocated by a different
    //! memory allocator (for example, in a different DLL) than the one used
    //! by the main editor, a crash will occur on exit, if data is left in
    //! the clipboard. The solution is to enfore all allocations of QMimeData
    //! using CreateQMimeData().
    virtual QMimeData* CreateQMimeData() const = 0;
    virtual void DestroyQMimeData(QMimeData* data) const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Access for CLevelIndependentFileMan
    // Manager can be used to register as an module that is asked before editor quits / loads level / creates level
    // This gives the module the change to save changes or cancel the process
    //////////////////////////////////////////////////////////////////////////
    virtual class CLevelIndependentFileMan* GetLevelIndependentFileMan() = 0;
    //! Notify all views that data is changed.
    virtual void UpdateViews(int flags = 0xFFFFFFFF, const AABB* updateRegion = nullptr) = 0;
    virtual void ResetViews() = 0;
    //! Update information in track view dialog.
    virtual void ReloadTrackView() = 0;
    //! Current position marker
    virtual Vec3 GetMarkerPosition() = 0;
    //! Set current position marker.
    virtual void    SetMarkerPosition(const Vec3& pos) = 0;
    //! Set current selected region.
    virtual void    SetSelectedRegion(const AABB& box) = 0;
    //! Get currently selected region.
    virtual void    GetSelectedRegion(AABB& box) = 0;

    virtual void SetOperationMode(EOperationMode mode) = 0;
    virtual EOperationMode GetOperationMode() = 0;
    //! Shows/Hides transformation manipulator.
    //! if bShow is true also returns a valid ITransformManipulator pointer.
    virtual ITransformManipulator* ShowTransformManipulator(bool bShow) = 0;
    //! Return a pointer to a ITransformManipulator pointer if shown.
    //! nullptr if manipulator is not shown.
    virtual ITransformManipulator* GetTransformManipulator() = 0;
    //! Set constrain on specified axis for objects construction and modifications.
    //! @param axis one of AxisConstrains enumerations.
    virtual void SetAxisConstraints(AxisConstrains axis) = 0;
    //! Get axis constrain for objects construction and modifications.
    virtual AxisConstrains GetAxisConstrains() = 0;
    //! Set whether axes are forced to the same value when they are changed (x = y = z).
    virtual void SetAxisVectorLock(bool bAxisVectorLock) = 0;
    //! Get whether axes are forced to the same value when they are changed (x = y = z).
    virtual bool IsAxisVectorLocked() = 0;
    //! If set, when axis terrain constrain is selected, snapping only to terrain.
    virtual void SetTerrainAxisIgnoreObjects(bool bIgnore) = 0;
    virtual bool IsTerrainAxisIgnoreObjects() = 0;
    //! Set current reference coordinate system used when constructing/modifying objects.
    virtual void SetReferenceCoordSys(RefCoordSys refCoords) = 0;
    //! Get current reference coordinate system used when constructing/modifying objects.
    virtual RefCoordSys GetReferenceCoordSys() = 0;
    virtual XmlNodeRef FindTemplate(const QString& templateName) = 0;
    virtual void AddTemplate(const QString& templateName, XmlNodeRef& tmpl) = 0;

    virtual const QtViewPane* OpenView(QString sViewClassName, bool reuseOpen = true) = 0;
    virtual QWidget* FindView(QString viewClassName) = 0;

    virtual bool CloseView(const char* sViewClassName) = 0;
    virtual bool SetViewFocus(const char* sViewClassName) = 0;
    virtual void CloseView(const GUID& classId) = 0; // close ALL panels related to classId, used when unloading plugins.

    // We want to open a view object but not wrap it in a view pane)
    virtual QWidget* OpenWinWidget(WinWidgetId openId) = 0;
    virtual WinWidget::WinWidgetManager* GetWinWidgetManager() const = 0;

    //! Opens standard color selection dialog.
    //! Initialized with the color specified in color parameter.
    //! Returns true if selection is made and false if selection is canceled.
    virtual bool SelectColor(QColor& color, QWidget* parent = 0) = 0;
    //! Get shader enumerator.
    virtual class CUndoManager* GetUndoManager() = 0;
    //! Begin operation requiring undo
    //! Undo manager enters holding state.
    virtual void BeginUndo() = 0;
    //! Restore all undo objects registered since last BeginUndo call.
    //! @param bUndo if true all Undo object registered since BeginUpdate call up to this point will be undone.
    virtual void RestoreUndo(bool undo = true) = 0;
    //! Accept changes and registers an undo object with the undo manager.
    //! This will allow the user to undo the operation.
    virtual void AcceptUndo(const QString& name) = 0;
    //! Cancel changes and restore undo objects.
    virtual void CancelUndo() = 0;
    //! Normally this is NOT needed but in special cases this can be useful.
    //! This allows to group a set of Begin()/Accept() sequences to be undone in one operation.
    virtual void SuperBeginUndo() = 0;
    //! When a SuperBegin() used, this method is used to Accept.
    //! This leaves the undo database in its modified state and registers the IUndoObjects with the undo system.
    //! This will allow the user to undo the operation.
    virtual void SuperAcceptUndo(const QString& name) = 0;
    //! Cancel changes and restore undo objects.
    virtual void SuperCancelUndo() = 0;
    //! Suspend undo recording.
    virtual void SuspendUndo() = 0;
    //! Resume undo recording.
    virtual void ResumeUndo() = 0;
    // Undo last operation.
    virtual void Undo() = 0;
    //! Redo last undo.
    virtual void Redo() = 0;
    //! Check if undo information is recording now.
    virtual bool IsUndoRecording() = 0;
    //! Check if undo information is suspzended now.
    virtual bool IsUndoSuspended() = 0;
    //! Put new undo object, must be called between Begin and Accept/Cancel methods.
    virtual void RecordUndo(struct IUndoObject* obj) = 0;
    //! Completely flush all Undo and redo buffers.
    //! Must be done on level reloads or global Fetch operation.
    virtual bool FlushUndo(bool isShowMessage = false) = 0;
    //! Clear the last N number of steps in the undo stack
    virtual bool ClearLastUndoSteps(int steps) = 0;
    //! Clear all current Redo steps in the undo stack
    virtual bool ClearRedoStack() = 0;
    //! Retrieve current animation context.
    virtual CAnimationContext* GetAnimation() = 0;
    //! Retrieve sequence manager
    virtual CTrackViewSequenceManager* GetSequenceManager() = 0;
    virtual ITrackViewSequenceManager* GetSequenceManagerInterface() = 0;

    //! Returns external tools manager.
    virtual CToolBoxManager* GetToolBoxManager() = 0;
    //! Get global Error Report instance.
    virtual IErrorReport* GetErrorReport() = 0;
    virtual IErrorReport* GetLastLoadedLevelErrorReport() = 0;
    virtual void StartLevelErrorReportRecording() = 0;
    virtual void CommitLevelErrorReport() = 0;
    // Retrieve interface to FileUtil
    virtual IFileUtil* GetFileUtil() = 0;
    // Notify all listeners about the specified event.
    virtual void Notify(EEditorNotifyEvent event) = 0;
    // Notify all listeners about the specified event, except for one.
    virtual void NotifyExcept(EEditorNotifyEvent event, IEditorNotifyListener* listener) = 0;
    //! Register Editor notifications listener.
    virtual void RegisterNotifyListener(IEditorNotifyListener* listener) = 0;
    //! Unregister Editor notifications listener.
    virtual void UnregisterNotifyListener(IEditorNotifyListener* listener) = 0;
    //! Register document notifications listener.
    virtual void RegisterDocListener(IDocListener* listener) = 0;
    //! Unregister document notifications listener.
    virtual void UnregisterDocListener(IDocListener* listener) = 0;
    //! Retrieve interface to the source control.
    virtual ISourceControl* GetSourceControl() = 0;
    //! Retrieve true if source control is provided and enabled in settings
    virtual bool IsSourceControlAvailable() = 0;
    //! Only returns true if source control is both available AND currently connected and functioning
    virtual bool IsSourceControlConnected() = 0;

    virtual CUIEnumsDatabase* GetUIEnumsDatabase() = 0;
    virtual void AddUIEnums() = 0;
    virtual void ReduceMemory() = 0;

    //! Export manager for exporting objects and a terrain from the game to DCC tools
    virtual IExportManager* GetExportManager() = 0;
    //! Set current configuration spec of the editor.
    virtual void SetEditorConfigSpec(ESystemConfigSpec spec, ESystemConfigPlatform platform) = 0;
    virtual ESystemConfigSpec GetEditorConfigSpec() const = 0;
    virtual ESystemConfigPlatform GetEditorConfigPlatform() const = 0;
    virtual void ReloadTemplates() = 0;
    virtual void ShowStatusText(bool bEnable) = 0;

    // Provides a way to extend the context menu of an object. The function gets called every time the menu is opened.
    typedef AZStd::function<void(QMenu*, const CBaseObject*)> TContextMenuExtensionFunc;
    virtual void RegisterObjectContextMenuExtension(TContextMenuExtensionFunc func) = 0;

    virtual SSystemGlobalEnvironment* GetEnv() = 0;
    virtual IImageUtil* GetImageUtil() = 0;  // Vladimir@conffx
    virtual SEditorSettings* GetEditorSettings() = 0;

    virtual ILogFile* GetLogFile() = 0;  // Vladimir@conffx

    // unload all plugins
    virtual void UnloadPlugins() = 0;

    // reloads the plugins
    virtual void LoadPlugins() = 0;
};

//! Callback used by editor when initializing for info in UI dialogs
struct IInitializeUIInfo
{
    virtual void SetInfoText(const char* text) = 0;
};

AZ_DECLARE_BUDGET(Editor);

