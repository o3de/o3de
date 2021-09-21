/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>
#include <IEditor.h>
#include <RenderHelpers/AxisHelper.h>

class CEditorMock
    : public IEditor
{
public:
    // GMock does not work with a variadic function
    void ExecuteCommand(const char* sCommand, ...) override
    {
        va_list args;
        va_start(args, sCommand);
        ExecuteCommand(QString::asprintf(sCommand, args));
        va_end(args);
    }

public:
    virtual ~CEditorMock() = default;

    MOCK_METHOD0(DeleteThis, void());
    MOCK_METHOD0(GetSystem, ISystem*());
    MOCK_METHOD0(GetClassFactory, IEditorClassFactory* ());
    MOCK_METHOD0(GetCommandManager, CEditorCommandManager*());
    MOCK_METHOD0(GetICommandManager, ICommandManager* ());
    MOCK_METHOD1(ExecuteCommand, void(const QString& sCommand));
    MOCK_METHOD1(SetDocument, void(CCryEditDoc* pDoc));
    MOCK_CONST_METHOD0(GetDocument, CCryEditDoc* ());
    MOCK_CONST_METHOD0(IsLevelLoaded, bool());
    MOCK_METHOD1(SetModifiedFlag, void(bool));
    MOCK_METHOD2(SetModifiedModule, void(EModifiedModule, bool));
    MOCK_CONST_METHOD0(IsLevelExported, bool());
    MOCK_METHOD1(SetLevelExported, bool(bool));
    MOCK_METHOD0(IsModified, bool());
    MOCK_METHOD0(SaveDocument, bool());
    MOCK_METHOD1(WriteToConsole, void(const char* ));
    MOCK_METHOD1(WriteToConsole, void(const QString& ));
    MOCK_METHOD2(SetConsoleVar, void(const char* var, float ));
    MOCK_METHOD1(GetConsoleVar, float(const char* var));
    MOCK_METHOD1(ShowConsole, bool(bool show));
    MOCK_METHOD0(GetMainStatusBar, IMainStatusBar*());
    MOCK_METHOD1(SetStatusText, void(const QString& ));
    MOCK_CONST_METHOD0(GetEditorMainWindow, QMainWindow* ());
    MOCK_METHOD0(GetPrimaryCDFolder, QString());
    MOCK_METHOD0(GetLevelName, QString());
    MOCK_METHOD0(GetLevelFolder, QString());
    MOCK_METHOD0(GetLevelDataFolder, QString());
    MOCK_METHOD1(GetPrimaryCDFolder, QString(EEditorPathName));
    MOCK_METHOD0(GetResolvedUserFolder, QString());
    MOCK_METHOD4(ExecuteConsoleApp, bool(const QString&,QString&,bool,bool));
    MOCK_METHOD0(SetDataModified, void());
    MOCK_CONST_METHOD0(IsInitialized, bool());
    MOCK_METHOD0(IsInGameMode, bool());
    MOCK_METHOD0(IsInSimulationMode, bool());
    MOCK_METHOD1(SetInGameMode, void(bool));
    MOCK_METHOD0(IsInTestMode, bool());
    MOCK_METHOD0(IsInPreviewMode, bool());
    MOCK_METHOD0(IsInConsolewMode, bool());
    MOCK_METHOD0(IsInLevelLoadTestMode, bool());
    MOCK_METHOD0(IsInMatEditMode, bool());
    MOCK_METHOD1(EnableUpdate, void(bool));
    MOCK_METHOD1(EnableAcceleratos, void(bool));
    MOCK_METHOD0(GetFileVersion, SFileVersion());
    MOCK_METHOD0(GetProductVersion, SFileVersion());
    MOCK_METHOD0(GetGameEngine , CGameEngine* ());
    MOCK_METHOD0(GetDisplaySettings, CDisplaySettings*());
    MOCK_METHOD0(GetGlobalGizmoParameters, const SGizmoParameters& ());
    MOCK_METHOD7(NewObject, CBaseObject* (const char*, const char*, const char*, float, float, float, bool));
    MOCK_METHOD1(DeleteObject, void(CBaseObject* obj));
    MOCK_METHOD1(CloneObject, CBaseObject* (CBaseObject* ));
    MOCK_METHOD0(GetSelection, CSelectionGroup* ());
    MOCK_METHOD0(GetSelectedObject, CBaseObject* ());
    MOCK_METHOD0(ClearSelection, int());
    MOCK_METHOD1(SelectObject, void(CBaseObject*));
    MOCK_METHOD1(LockSelection, void(bool));
    MOCK_METHOD0(IsSelectionLocked, bool());
    MOCK_METHOD0(GetObjectManager, struct IObjectManager* ());
    MOCK_METHOD0(GetSettingsManager, CSettingsManager* ());
    MOCK_METHOD1(GetDBItemManager, IDataBaseManager* (EDataBaseItemType));
    MOCK_METHOD0(GetMaterialManagerLibrary, IBaseLibraryManager* ());
    MOCK_METHOD0(GetIEditorMaterialManager, IEditorMaterialManager* ());
    MOCK_METHOD0(GetIconManager, IIconManager* ());
    MOCK_METHOD0(GetMusicManager, CMusicManager* ());
    MOCK_METHOD2(GetTerrainElevation, float(float , float ));
    MOCK_METHOD0(GetVegetationMap, class CVegetationMap* ());
    MOCK_METHOD0(GetEditorQtApplication, Editor::EditorQtApplication* ());
    MOCK_METHOD1(GetColorByName, const QColor& (const QString&));
    MOCK_METHOD0(GetMovieSystem, struct IMovieSystem* ());
    MOCK_METHOD0(GetPluginManager, class CPluginManager*());
    MOCK_METHOD0(GetViewManager, class CViewManager* ());
    MOCK_METHOD0(GetActiveView, class CViewport* ());
    MOCK_METHOD1(SetActiveView, void(CViewport*));
    MOCK_METHOD0(GetFileMonitor, struct IEditorFileMonitor* ());
    MOCK_METHOD1(RegisterEventLoopHook, void(IEventLoopHook* ));
    MOCK_METHOD1(UnregisterEventLoopHook, void(IEventLoopHook* ));
    MOCK_CONST_METHOD0(CreateQMimeData, QMimeData* ());
    MOCK_CONST_METHOD1(DestroyQMimeData, void(QMimeData*));
    MOCK_METHOD0(GetLevelIndependentFileMan, class CLevelIndependentFileMan* ());
    MOCK_METHOD2(UpdateViews, void(int , const AABB* ));
    MOCK_METHOD0(ResetViews, void());
    MOCK_METHOD0(ReloadTrackView, void());
    MOCK_METHOD0(GetMarkerPosition, Vec3());
    MOCK_METHOD1(SetMarkerPosition, void(const Vec3&));
    MOCK_METHOD1(SetSelectedRegion, void(const AABB& box));
    MOCK_METHOD1(GetSelectedRegion, void(AABB& box));
    MOCK_METHOD1(SetOperationMode, void(EOperationMode ));
    MOCK_METHOD0(GetOperationMode, EOperationMode());
    MOCK_METHOD1(ShowTransformManipulator, ITransformManipulator* (bool));
    MOCK_METHOD0(GetTransformManipulator, ITransformManipulator* ());
    MOCK_METHOD1(SetAxisConstraints, void(AxisConstrains ));
    MOCK_METHOD0(GetAxisConstrains, AxisConstrains());
    MOCK_METHOD1(SetAxisVectorLock, void(bool));
    MOCK_METHOD0(IsAxisVectorLocked, bool());
    MOCK_METHOD1(SetTerrainAxisIgnoreObjects, void(bool));
    MOCK_METHOD0(IsTerrainAxisIgnoreObjects, bool());
    MOCK_METHOD1(SetReferenceCoordSys, void(RefCoordSys ));
    MOCK_METHOD0(GetReferenceCoordSys, RefCoordSys());
    MOCK_METHOD1(FindTemplate, XmlNodeRef(const QString& ));
    MOCK_METHOD2(AddTemplate, void(const QString& , XmlNodeRef& ));
    MOCK_METHOD2(OpenView, const QtViewPane* (QString , bool ));
    MOCK_METHOD1(FindView, QWidget* (QString ));
    MOCK_METHOD1(CloseView, bool(const char* ));
    MOCK_METHOD1(SetViewFocus, bool(const char* ));
    MOCK_METHOD1(CloseView, void(const GUID& ));
    MOCK_METHOD1(OpenWinWidget, QWidget* (WinWidgetId ));
    MOCK_CONST_METHOD0(GetWinWidgetManager, WinWidget::WinWidgetManager* ());
    MOCK_METHOD2(SelectColor, bool(QColor &, QWidget *));
    MOCK_METHOD0(GetUndoManager, class CUndoManager* ());
    MOCK_METHOD0(BeginUndo, void());
    MOCK_METHOD1(RestoreUndo, void(bool));
    MOCK_METHOD1(AcceptUndo, void(const QString& ));
    MOCK_METHOD0(CancelUndo, void());
    MOCK_METHOD0(SuperBeginUndo, void());
    MOCK_METHOD1(SuperAcceptUndo, void(const QString&));
    MOCK_METHOD0(SuperCancelUndo, void());
    MOCK_METHOD0(SuspendUndo, void());
    MOCK_METHOD0(ResumeUndo, void());
    MOCK_METHOD0(Undo, void());
    MOCK_METHOD0(Redo, void());
    MOCK_METHOD0(IsUndoRecording, bool());
    MOCK_METHOD0(IsUndoSuspended, bool());
    MOCK_METHOD1(RecordUndo, void(struct IUndoObject* ));
    MOCK_METHOD1(FlushUndo, bool(bool isShowMessage));
    MOCK_METHOD1(ClearLastUndoSteps, bool(int ));
    MOCK_METHOD0(ClearRedoStack, bool());
    MOCK_METHOD0(GetAnimation, CAnimationContext* ());
    MOCK_METHOD0(GetSequenceManager, CTrackViewSequenceManager* ());
    MOCK_METHOD0(GetSequenceManagerInterface, ITrackViewSequenceManager* ());
    MOCK_METHOD0(GetToolBoxManager, CToolBoxManager* ());
    MOCK_METHOD0(GetErrorReport, IErrorReport* ());
    MOCK_METHOD0(GetLastLoadedLevelErrorReport, IErrorReport* ());
    MOCK_METHOD0(StartLevelErrorReportRecording, void());
    MOCK_METHOD0(CommitLevelErrorReport, void());
    MOCK_METHOD0(GetFileUtil, IFileUtil* ());
    MOCK_METHOD1(Notify, void(EEditorNotifyEvent));
    MOCK_METHOD2(NotifyExcept, void(EEditorNotifyEvent , IEditorNotifyListener* ));
    MOCK_METHOD1(RegisterNotifyListener, void(IEditorNotifyListener* ));
    MOCK_METHOD1(UnregisterNotifyListener, void(IEditorNotifyListener* ));
    MOCK_METHOD1(RegisterDocListener, void(IDocListener* ));
    MOCK_METHOD1(UnregisterDocListener, void(IDocListener* ));
    MOCK_METHOD0(GetSourceControl, ISourceControl* ());
    MOCK_METHOD0(IsSourceControlAvailable, bool());
    MOCK_METHOD0(IsSourceControlConnected, bool());
    MOCK_METHOD0(GetUIEnumsDatabase, CUIEnumsDatabase* ());
    MOCK_METHOD0(AddUIEnums, void());
    MOCK_METHOD0(ReduceMemory, void());
    MOCK_METHOD0(GetExportManager, IExportManager* ());
    MOCK_METHOD2(SetEditorConfigSpec, void(ESystemConfigSpec , ESystemConfigPlatform ));
    MOCK_CONST_METHOD0(GetEditorConfigSpec, ESystemConfigSpec());
    MOCK_CONST_METHOD0(GetEditorConfigPlatform, ESystemConfigPlatform());
    MOCK_METHOD0(ReloadTemplates, void());
    MOCK_METHOD1(ShowStatusText, void(bool ));
    MOCK_METHOD1(RegisterObjectContextMenuExtension, void(TContextMenuExtensionFunc ));
    MOCK_METHOD0(GetEnv, SSystemGlobalEnvironment* ());
    MOCK_METHOD0(GetImageUtil, IImageUtil* ());
    MOCK_METHOD0(GetEditorSettings, SEditorSettings* ());
    MOCK_METHOD0(GetLogFile, ILogFile* ());
    MOCK_METHOD0(UnloadPlugins, void());
    MOCK_METHOD0(LoadPlugins, void());
    MOCK_METHOD1(GetSearchPath, QString(EEditorPathName));
    MOCK_METHOD0(GetEditorPanelUtils, IEditorPanelUtils* ());

};
