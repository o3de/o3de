/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QTranslator>
#include <QMimeData>
#include <QToolButton>
#include <QWidget>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>

#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Components/ToastNotification.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <GraphCanvas/Widgets/ConstructPresetDialog/ConstructPresetDialog.h>
#include <GraphCanvas/Styling/StyleManager.h>

#include <Core/Core.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Debugger/ClientTransceiver.h>
#include <Editor/Undo/ScriptCanvasGraphCommand.h>
#include <Editor/Utilities/RecentFiles.h>
#include <Editor/View/Dialogs/SettingsDialog.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <Editor/View/Widgets/AssetGraphSceneDataBus.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Controller.h>
#include <Editor/View/Windows/Tools/UpgradeTool/FileSaver.h>

#if SCRIPTCANVAS_EDITOR
#include <Include/EditorCoreAPI.h>
#endif//#if SCRIPTCANVAS_EDITOR

#endif//#if !defined(Q_MOC_RUN)

namespace GraphCanvas
{
    class AssetEditorToolbar;
    class GraphCanvasMimeEvent;
    class GraphCanvasEditorEmptyDockWidget;
    class MiniMapDockWidget;

    namespace Styling
    {
        class StyleSheet;
    }
}

namespace Ui
{
    class MainWindow;
}

namespace AzQtComponents
{
    class TabWidget;
}

class QDir;
class QFile;
class QProgressDialog;

namespace ScriptCanvasEditor
{
    class MainWindow;
    class InterpreterWidget;

    class ScriptCanvasAssetBrowserModel
        : public AzToolsFramework::AssetBrowser::AssetBrowserFilterModel
        , private UpgradeNotificationsBus::Handler
    {
    public:

        explicit ScriptCanvasAssetBrowserModel(QObject* parent = nullptr)
            : AzToolsFramework::AssetBrowser::AssetBrowserFilterModel(parent)
        {
            UpgradeNotificationsBus::Handler::BusConnect();
        }

        ~ScriptCanvasAssetBrowserModel() override
        {
            UpgradeNotificationsBus::Handler::BusDisconnect();
        }

        void OnUpgradeStart() override
        {
            AzToolsFramework::AssetBrowser::AssetBrowserComponentNotificationBus::Handler::BusDisconnect();
        }
    };

    class OnSaveToast
    {
    public:
        OnSaveToast(AZStd::string_view tabName, AZ::EntityId graphCanvasGraphId, bool saveSuccessful)
        {
            AzQtComponents::ToastType toastType = AzQtComponents::ToastType::Information;
            AZStd::string titleLabel = "Notification";

            AZStd::string description;

            if (saveSuccessful)
            {
                description = AZStd::string::format("%s Saved", tabName.data());
            }
            else
            {
                description = AZStd::string::format("Failed to save %s", tabName.data());
                toastType = AzQtComponents::ToastType::Error;
            }

            AzQtComponents::ToastConfiguration toastConfiguration(toastType, titleLabel.c_str(), description.c_str());

            AzToolsFramework::ToastId validationToastId;

            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);
            if (viewId.IsValid())
            {
                GraphCanvas::ViewRequestBus::EventResult(validationToastId, viewId, &GraphCanvas::ViewRequests::ShowToastNotification, toastConfiguration);
            }
        }
    };

    //! Manages the Save/Restore operations of the user's last opened and focused graphs
    class Workspace
    {
    public:

        Workspace(MainWindow* mainWindow)
            : m_mainWindow(mainWindow)
        {
            auto userSettings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
            m_rememberOpenCanvases = (userSettings && userSettings->m_rememberOpenCanvases);
        }

        //! Saves the current state of the workspace (which assets are open, which asset is in focus)
        void Save();

        //! Restores the workspace to the previously saved state (opens all previously opened assets and sets focus on what was in focus)
        void Restore();

    private:

        void SignalAssetComplete(const ScriptCanvasEditor::SourceHandle& fileAssetId);

        ScriptCanvasEditor::SourceHandle GetSourceAssetId(const ScriptCanvasEditor::SourceHandle& memoryAssetId) const;

        bool m_rememberOpenCanvases;
        MainWindow* m_mainWindow;

        //! Setting focus is problematic unless it is done until after all currently loading graphs have finished loading
        //! This vector is used to track the list of graphs being opened to restore the workspace and as assets are fully
        //! ready and activated they are removed from this list.
        AZStd::vector<ScriptCanvasEditor::SourceHandle> m_loadingAssets;

        //! During restore we queue the asset Id to focus in order to do it last
        ScriptCanvasEditor::SourceHandle m_queuedAssetFocus;
    };

    enum class UnsavedChangesOptions;
    namespace Widget
    {
        class BusSenderTree;
        class CommandLine;
        class GraphTabBar;
        class NodePaletteDockWidget;
        class NodeProperties;
        class PropertyGrid;
        class LogPanelWidget;
    }

    class EBusHandlerActionMenu;
    class VariableDockWidget;
    class UnitTestDockWidget;
    class BatchOperatorTool;
    class ScriptCanvasBatchConverter;    
    class LoggingWindow;
    class GraphValidationDockWidget;
    class MainWindowStatusWidget;
    class StatisticsDialog;
    class VariableConfigurationWidget;

    // Custom Context Menus
    class SceneContextMenu;
    class ConnectionContextMenu;
    class RenameFunctionDefinitionNodeAction;

    class MainWindow
        : public QMainWindow
        , private UIRequestBus::Handler
        , private GeneralRequestBus::Handler
        , private UndoNotificationBus::Handler
        , private GraphCanvas::SceneNotificationBus::MultiHandler
        , private GraphCanvas::AssetEditorSettingsRequestBus::Handler
        , private GraphCanvas::AssetEditorRequestBus::Handler
        , private VariablePaletteRequestBus::Handler
        , private ScriptCanvas::BatchOperationNotificationBus::Handler
        , private AssetGraphSceneBus::Handler
#if SCRIPTCANVAS_EDITOR
        //, public IEditorNotifyListener
#endif
        , private AutomationRequestBus::Handler
        , private GraphCanvas::AssetEditorAutomationRequestBus::Handler
        , private GraphCanvas::ViewNotificationBus::Handler
        , public AZ::SystemTickBus::Handler
        , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
        , private AzToolsFramework::AssetSystemBus::Handler
        , private ScriptCanvas::ScriptCanvasSettingsRequestBus::Handler
    {
        Q_OBJECT
    private:
        friend class BatchOperatorTool;
        friend class ScriptCanvasBatchConverter;
        friend class Workspace;

        enum SystemTickActionFlag
        {
            RefreshPropertyGrid = 1,
            CloseWindow = 1 << 1,
            UpdateSaveMenuState = 1 << 2,
            CloseCurrentGraph = 1 << 3,
            CloseNextTabAction = 1 << 4
        };

    public:

        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

    private:
        // UIRequestBus
        QMainWindow* GetMainWindow() override { return qobject_cast<QMainWindow*>(this); }
        void OpenValidationPanel() override;
        //

        // Undo Handlers
        void PostUndoPoint(ScriptCanvas::ScriptCanvasId scriptCanvasId) override;
        void SignalSceneDirty(ScriptCanvasEditor::SourceHandle assetId) override;

        void PushPreventUndoStateUpdate() override;
        void PopPreventUndoStateUpdate() override;
        void ClearPreventUndoStateUpdate() override;
        void TriggerUndo() override;
        void TriggerRedo() override;

        // VariablePaletteRequestBus
        void RegisterVariableType(const ScriptCanvas::Data::Type& variableType) override;
        bool IsValidVariableType(const ScriptCanvas::Data::Type& dataType) const override;
        VariablePaletteRequests::VariableConfigurationOutput ShowVariableConfigurationWidget(
            const VariablePaletteRequests::VariableConfigurationInput& input, const QPoint& scenePosition) override;
        ////

        // GraphCanvas::AssetEditorRequestBus
        void OnSelectionManipulationBegin() override;
        void OnSelectionManipulationEnd() override;

        AZ::EntityId CreateNewGraph() override;
        bool ContainsGraph(const GraphCanvas::GraphId& graphId) const override;
        bool CloseGraph(const GraphCanvas::GraphId& graphId) override;

        void CustomizeConnectionEntity(AZ::Entity* connectionEntity) override;

        void ShowAssetPresetsMenu(GraphCanvas::ConstructType constructType) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowSceneContextMenuWithGroup(const QPoint& screenPoint, const QPointF& scenePoint, AZ::EntityId groupTarget) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowNodeContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowCommentContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowNodeGroupContextMenu(const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        GraphCanvas::ContextMenuAction::SceneReaction ShowCollapsedNodeGroupContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowBookmarkContextMenu(const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowConnectionContextMenuWithGroup(const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint, AZ::EntityId groupTarget) override;

        GraphCanvas::ContextMenuAction::SceneReaction ShowSlotContextMenu(const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint) override;

        GraphCanvas::Endpoint CreateNodeForProposalWithGroup(const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint, AZ::EntityId groupTarget) override;
        void OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint) override;
        ////

        // SystemTickBus
        void OnSystemTick() override;
        ////

        //! ScriptCanvas::BatchOperationsNotificationBus
        void OnCommandStarted(AZ::Crc32 commandTag) override;
        void OnCommandFinished(AZ::Crc32 commandTag) override;

        // File menu
        void OnFileNew();

        bool OnFileSave();
        bool OnFileSaveAs();
        bool OnFileSaveCaller(){return OnFileSave();};
        bool OnFileSaveAsCaller(){return OnFileSaveAs();};        
        enum class Save
        {
            InPlace,
            As
        };
        bool SaveAssetImpl(const ScriptCanvasEditor::SourceHandle& assetId, Save save);
        void OnFileOpen();        

        // Edit menu
        void SetupEditMenu();
        void OnEditMenuShow();
        void RefreshPasteAction();
        void RefreshGraphPreferencesAction();
        void OnEditCut();
        void OnEditCopy();
        void OnEditPaste();
        void OnEditDuplicate();
        void OnEditDelete();
        void OnRemoveUnusedVariables();
        void OnRemoveUnusedNodes();
        void OnRemoveUnusedElements();
        void OnScreenshot();
        void OnSelectAll();
        void OnSelectInputs();
        void OnSelectOutputs();
        void OnSelectConnected();
        void OnClearSelection();
        void OnEnableSelection();
        void OnDisableSelection();
        void OnAlignTop();
        void OnAlignBottom();
        void OnAlignLeft();
        void OnAlignRight();

        void AlignSelected(const GraphCanvas::AlignConfig& alignConfig);

        // View Menu
        void OnShowEntireGraph();
        void OnZoomIn();
        void OnZoomOut();
        void OnZoomToSelection();

        void OnGotoStartOfChain();
        void OnGotoEndOfChain();

        // Tools menu
        void OnViewNodePalette();
        void OnViewProperties();
        void OnViewDebugger();
        void OnViewCommandLine();
        void OnViewLog();
        void OnBookmarks();
        void OnVariableManager();        
        void OnViewMiniMap();
        void OnViewLogWindow();
        void OnViewGraphValidation();
        void OnViewDebuggingWindow();
        void OnViewUnitTestManager();
        void OnViewStatisticsPanel();
        void OnViewPresetsEditor();
        void OnRestoreDefaultLayout();

        void UpdateViewMenu();
        /////////////////////////////////////////////////////////////////////////////////////////////

        //SceneNotificationBus
        void OnNodeAdded(const AZ::EntityId& nodeId, bool isPaste) override;
        void OnSelectionChanged() override;
        /////////////////////////////////////////////////////////////////////////////////////////////
        
        void OnVariableSelectionChanged(const AZStd::vector<AZ::EntityId>& variablePropertyIds);
        void QueuePropertyGridUpdate();
        void DequeuePropertyGridUpdate();

        void SetDefaultLayout();

        void RefreshSelection();
        void Clear();

        void OnTabCloseRequest(int index);
        void OnTabCloseButtonPressed(int index);

        void SaveTab(int index);
        void CloseAllTabs();
        void CloseAllTabsBut(int index);
        void CopyPathToClipboard(int index);
        void OnActiveFileStateChanged();

        void CloseNextTab();

        bool IsTabOpen(const SourceHandle& assetId, int& outTabIndex) const;
        QVariant GetTabData(const SourceHandle& assetId);

        //! GeneralRequestBus
        AZ::Outcome<int, AZStd::string> OpenScriptCanvasAssetId(const SourceHandle& assetId, Tracker::ScriptCanvasFileState fileState) override;
        AZ::Outcome<int, AZStd::string> OpenScriptCanvasAsset(SourceHandle scriptCanvasAssetId, Tracker::ScriptCanvasFileState fileState, int tabIndex = -1) override;
        AZ::Outcome<int, AZStd::string> OpenScriptCanvasAssetImplementation(const SourceHandle& sourceHandle, Tracker::ScriptCanvasFileState fileState, int tabIndex = -1);
        int CloseScriptCanvasAsset(const SourceHandle& assetId) override;
        bool CreateScriptCanvasAssetFor(const TypeDefs::EntityComponentId& requestingEntityId) override;

        bool IsScriptCanvasAssetOpen(const SourceHandle& assetId) const override;

        const CategoryInformation* FindNodePaletteCategoryInformation(AZStd::string_view categoryPath) const override;
        const NodePaletteModelInformation* FindNodePaletteModelInformation(const ScriptCanvas::NodeTypeIdentifier& nodeType) const override;
        ////

        AZ::Outcome<int, AZStd::string> CreateScriptCanvasAsset(AZStd::string_view assetPath, int tabIndex = -1);
        
        void RemoveScriptCanvasAsset(const ScriptCanvasEditor::SourceHandle& assetId);
        void OnChangeActiveGraphTab(ScriptCanvasEditor::SourceHandle) override;

        void CreateNewRuntimeAsset() override { OnFileNew(); }

        GraphCanvas::GraphId GetActiveGraphCanvasGraphId() const override;
        ScriptCanvas::ScriptCanvasId GetScriptCanvasId(const GraphCanvas::GraphId& graphCanvasGraphId) const override;
        ScriptCanvas::ScriptCanvasId GetActiveScriptCanvasId() const override;

        GraphCanvas::GraphId GetGraphCanvasGraphId(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const override;

        GraphCanvas::GraphId FindGraphCanvasGraphIdByAssetId(const ScriptCanvasEditor::SourceHandle& assetId) const override;
        ScriptCanvas::ScriptCanvasId FindScriptCanvasIdByAssetId(const ScriptCanvasEditor::SourceHandle& assetId) const override;

        bool IsInUndoRedo(const AZ::EntityId& graphCanvasGraphId) const override;
        bool IsScriptCanvasInUndoRedo(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const override;
        bool IsActiveGraphInUndoRedo() const override;
        ////////////////////////////

        // GraphCanvasSettingsRequestBus
        double GetSnapDistance() const override;

        bool IsGroupDoubleClickCollapseEnabled() const override;

        bool IsBookmarkViewportControlEnabled() const override;

        bool IsDragNodeCouplingEnabled() const override;
        AZStd::chrono::milliseconds GetDragCouplingTime() const override;

        bool IsDragConnectionSpliceEnabled() const override;
        AZStd::chrono::milliseconds GetDragConnectionSpliceTime() const override;

        bool IsDropConnectionSpliceEnabled() const override;
        AZStd::chrono::milliseconds GetDropConnectionSpliceTime() const override;

        bool IsNodeNudgingEnabled() const override;

        bool IsShakeToDespliceEnabled() const override;
        int GetShakesToDesplice() const override;
        float GetMinimumShakePercent() const override;
        float GetShakeDeadZonePercent() const override;
        float GetShakeStraightnessPercent() const override;
        AZStd::chrono::milliseconds GetMaximumShakeDuration() const override;

        AZStd::chrono::milliseconds GetAlignmentTime() const override;

        float GetMaxZoom() const override;

        float GetEdgePanningPercentage() const override;
        float GetEdgePanningScrollSpeed() const override;        

        GraphCanvas::EditorConstructPresets* GetConstructPresets() const override;
        const GraphCanvas::ConstructTypePresetBucket* GetConstructTypePresetBucket(GraphCanvas::ConstructType constructType) const override;

        GraphCanvas::Styling::ConnectionCurveType GetConnectionCurveType() const override;
        GraphCanvas::Styling::ConnectionCurveType GetDataConnectionCurveType() const override;

        bool AllowNodeDisabling() const override;
        bool AllowDataReferenceSlots() const override;
        ////

        // AutomationRequestBus
        NodeIdPair ProcessCreateNodeMimeEvent(GraphCanvas::GraphCanvasMimeEvent* mimeEvent, const AZ::EntityId& graphCanvasGraphId, AZ::Vector2 nodeCreationPos) override;
        const GraphCanvas::GraphCanvasTreeItem* GetNodePaletteRoot() const override;

        void SignalAutomationBegin() override;
        void SignalAutomationEnd() override;

        void ForceCloseActiveAsset() override;        
        ////

        // AssetEditorAutomationRequestBus
        bool RegisterObject(AZ::Crc32 elementId, QObject* object) override;
        bool UnregisterObject(AZ::Crc32 elementId) override;

        QObject* FindObject(AZ::Crc32 elementId) override;

        QObject* FindElementByName(QString elementName) override;
        ////

        AZ::EntityId FindEditorNodeIdByAssetNodeId(const ScriptCanvasEditor::SourceHandle& assetId, AZ::EntityId assetNodeId) const override;
        AZ::EntityId FindAssetNodeIdByEditorNodeId(const ScriptCanvasEditor::SourceHandle& assetId, AZ::EntityId editorNodeId) const override;
        
    private:
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void SourceFileRemoved(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        
        void DeleteNodes(const AZ::EntityId& sceneId, const AZStd::vector<AZ::EntityId>& nodes) override;
        void DeleteConnections(const AZ::EntityId& sceneId, const AZStd::vector<AZ::EntityId>& connections) override;
        void DisconnectEndpoints(const AZ::EntityId& sceneId, const AZStd::vector<GraphCanvas::Endpoint>& endpoints) override;
        /////////////////////////////////////////////////////////////////////////////////////////////        

        GraphCanvas::Endpoint HandleProposedConnection(const GraphCanvas::GraphId& graphId, const GraphCanvas::ConnectionId& connectionId, const GraphCanvas::Endpoint& endpoint, const GraphCanvas::NodeId& proposedNode, const QPoint& screenPoint);

        //! UndoNotificationBus
        void OnCanUndoChanged(bool canUndo) override;
        void OnCanRedoChanged(bool canRedo) override;
        ////

        //! ScriptCanvasSettingsRequestBus
        bool CanShowNetworkSettings() override;
        ////

        GraphCanvas::ContextMenuAction::SceneReaction HandleContextMenu(GraphCanvas::EditorContextMenu& editorContextMenu, const AZ::EntityId& memberId, const QPoint& screenPoint, const QPointF& scenePoint) const;

        void OnAutoSave();

        void UpdateFileState(const ScriptCanvasEditor::SourceHandle& assetId, Tracker::ScriptCanvasFileState fileState);

        // QMainWindow
        void closeEvent(QCloseEvent *event) override;
        UnsavedChangesOptions ShowSaveDialog(const QString& filename);
        
        bool ActivateAndSaveAsset(const ScriptCanvasEditor::SourceHandle& unsavedAssetId);

        void SaveAs(AZStd::string_view path, ScriptCanvasEditor::SourceHandle assetId);

        void OpenFile(const char* fullPath);
        void CreateMenus();

        void SignalActiveSceneChanged(const ScriptCanvasEditor::SourceHandle assetId);

        void RunUpgradeTool();
        void ShowInterpreter();

        void OnShowValidationErrors();
        void OnShowValidationWarnings();
        void OnValidateCurrentGraph();

        void RunGraphValidation(bool displayToastNotification);

        // ViewNotificationBus
        void OnViewParamsChanged(const GraphCanvas::ViewParams& viewParams) override;
        void OnZoomChanged(qreal zoomLevel) override;
        ////

        // ToolsNotificationBus
        void AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList& newlySelectedEntities, const AzToolsFramework::EntityIdList& newlyDeselectedEntities) override;
        ////

    public slots:
        void UpdateRecentMenu();
        void OnViewVisibilityChanged(bool visibility);

    private:

        void UpdateMenuState(bool enabledState);

        void OnWorkspaceRestoreStart();
        void OnWorkspaceRestoreEnd(ScriptCanvasEditor::SourceHandle lastFocusAsset);

        void UpdateAssignToSelectionState();
        void UpdateUndoRedoState();
        void UpdateSaveState(bool enabled);

        void CreateFunctionInput();
        void CreateFunctionOutput();

        void CreateFunctionDefinitionNode(int positionOffset);

        int CreateAssetTab(const ScriptCanvasEditor::SourceHandle& assetId, Tracker::ScriptCanvasFileState fileState, int tabIndex = -1);

        //! \param asset The AssetId of the ScriptCanvas Asset.
        void SetActiveAsset(const ScriptCanvasEditor::SourceHandle& assetId);
        void RefreshActiveAsset();

        void ReconnectSceneBuses(ScriptCanvasEditor::SourceHandle previousAssetId, ScriptCanvasEditor::SourceHandle nextAssetId);

        void PrepareActiveAssetForSave();
        void PrepareAssetForSave(const ScriptCanvasEditor::SourceHandle& asssetId);

        void RestartAutoTimerSave(bool forceTimer = false);

        // Assign to selected entities Menus
        void OnSelectedEntitiesAboutToShow();
        void OnAssignToSelectedEntities();
        void OnAssignToEntity(const AZ::EntityId& entityId);
        void AssignGraphToEntityImpl(const AZ::EntityId& entityId);
        ////

        Tracker::ScriptCanvasFileState GetAssetFileState(ScriptCanvasEditor::SourceHandle assetId) const;

        ScriptCanvasEditor::SourceHandle GetSourceAssetId(const ScriptCanvasEditor::SourceHandle& memoryAssetId) const
        {
            return memoryAssetId;
        }

        int InsertTabForAsset(AZStd::string_view assetPath, ScriptCanvasEditor::SourceHandle assetId, int tabIndex = -1);

        void UpdateUndoCache(ScriptCanvasEditor::SourceHandle assetId);


        bool HasSystemTickAction(SystemTickActionFlag action);
        void RemoveSystemTickAction(SystemTickActionFlag action);
        void AddSystemTickAction(SystemTickActionFlag action);

        void BlockCloseRequests();
        void UnblockCloseRequests();

        void OpenNextFile();


        void DisableAssetView(const ScriptCanvasEditor::SourceHandle& memoryAssetId);
        void EnableAssetView(const ScriptCanvasEditor::SourceHandle& memoryAssetId);


        QWidget* m_host = nullptr;

        ScriptCanvasAssetBrowserModel*      m_scriptEventsAssetModel;
        ScriptCanvasAssetBrowserModel*      m_scriptCanvasAssetModel;

        AzQtComponents::TabWidget*          m_tabWidget = nullptr;
        Widget::GraphTabBar*                m_tabBar = nullptr;
        GraphCanvas::AssetEditorToolbar*    m_editorToolbar = nullptr;

        QToolButton*                        m_validateGraphToolButton = nullptr;
        QToolButton*                        m_assignToSelectedEntity = nullptr;

        QToolButton*                        m_createFunctionInput = nullptr;
        QToolButton*                        m_createFunctionOutput = nullptr;
        QToolButton*                        m_takeScreenshot = nullptr;

        QToolButton*                        m_createScriptCanvas = nullptr;

        QMenu*                              m_selectedEntityMenu = nullptr;

        VariableDockWidget*                 m_variableDockWidget = nullptr;
        GraphValidationDockWidget*          m_validationDockWidget = nullptr;
        UnitTestDockWidget*                 m_unitTestDockWidget = nullptr;
        StatisticsDialog*                   m_statisticsDialog = nullptr;
        Widget::NodePaletteDockWidget*      m_nodePalette = nullptr;
        Widget::LogPanelWidget*             m_logPanel = nullptr;
        Widget::PropertyGrid*               m_propertyGrid = nullptr;
        Widget::CommandLine*                m_commandLine = nullptr;
        GraphCanvas::BookmarkDockWidget*    m_bookmarkDockWidget = nullptr;
        GraphCanvas::MiniMapDockWidget*     m_minimap = nullptr;
        LoggingWindow*                      m_loggingWindow = nullptr;
        VariableConfigurationWidget*             m_slotTypeSelector = nullptr;

        AZStd::unique_ptr<InterpreterWidget> m_interpreterWidget;

        AzQtComponents::WindowDecorationWrapper*    m_presetWrapper = nullptr;
        GraphCanvas::ConstructPresetDialog*         m_presetEditor = nullptr;

        MainWindowStatusWidget*             m_statusWidget = nullptr;

        NodePaletteModel                    m_nodePaletteModel;

        QStringList                         m_filesToOpen;

        void CreateUnitTestWidget();

        // Reusable context menu for the context menu's that have a node palette.
        SceneContextMenu* m_sceneContextMenu;
        ConnectionContextMenu* m_connectionContextMenu;

        AZ::EntityId m_entityMimeDelegateId;

        // Reusable context menu for adding/removing ebus events from a wrapper node
        EBusHandlerActionMenu* m_ebusHandlerActionMenu;

        GraphCanvas::GraphCanvasEditorEmptyDockWidget* m_emptyCanvas; // Displayed when there is no open graph
        QVBoxLayout* m_layout;

        ScriptCanvasEditor::SourceHandle m_activeGraph;
        
        bool                  m_loadingNewlySavedFile;
        AZStd::string         m_newlySavedFile;

        AZStd::string         m_errorFilePath;

        bool m_isClosingTabs;
        ScriptCanvasEditor::SourceHandle m_skipTabOnClose;

        bool m_enterState;
        bool m_ignoreSelection;
        AZ::s32 m_preventUndoStateUpdateCount;

        bool m_isRestoringWorkspace;
        ScriptCanvasEditor::SourceHandle m_queuedFocusOverride;

        Ui::MainWindow* ui;
        AZStd::array<AZStd::pair<QAction*, QMetaObject::Connection>, c_scriptCanvasEditorSettingsRecentFilesCountMax> m_recentActions;

        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> m_userSettings;

        bool m_queueCloseRequest;
        bool m_hasQueuedClose;

        bool m_isInAutomation;

        bool m_allowAutoSave;
        bool m_showUpgradeTool;
        QTimer m_autoSaveTimer;

        QByteArray m_defaultLayout;
        QTranslator m_translator;
        
        AZStd::vector<AZ::EntityId> m_selectedVariableIds;

        AZ::u32                                   m_systemTickActions;
        AZStd::unordered_set< ScriptCanvasEditor::SourceHandle > m_processedClosedAssetIds;

        AZStd::unordered_set< ScriptCanvasEditor::SourceHandle > m_loadingWorkspaceAssets;
        AZStd::unordered_set< ScriptCanvasEditor::SourceHandle > m_loadingAssets;
        AZStd::unordered_set< AZ::Uuid > m_variablePaletteTypes;

        AZStd::unordered_map< AZ::Crc32, QObject* > m_automationLookUpMap;

        bool m_closeCurrentGraphAfterSave;

        AZStd::unordered_map< ScriptCanvasEditor::SourceHandle, TypeDefs::EntityComponentId > m_assetCreationRequests;

        ScriptCanvas::Debugger::ClientTransceiver m_clientTRX;
        GraphCanvas::StyleManager m_styleManager;

        //! The workspace stores in the user settings which assets were open and which asset was in focus
        //! this object manages the Save/Restore operations
        Workspace* m_workspace;

        AZStd::unique_ptr<VersionExplorer::FileSaver> m_fileSaver;
        VersionExplorer::FileSaveResult m_fileSaveResult;
        void OnSaveCallBack(const VersionExplorer::FileSaveResult& result);

        void ClearStaleSaves();
        bool IsRecentSave(const SourceHandle& handle) const;
        void MarkRecentSave(const SourceHandle& handle);
        AZStd::recursive_mutex m_mutex; 
        AZStd::unordered_map <AZStd::string, AZStd::chrono::system_clock::time_point> m_saves;
    };
}
