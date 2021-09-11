/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include "EditorCommon.h"

#include "Animation/UiEditorAnimationBus.h"
#include <LyShine/UiEditorDLLBus.h>
#include "UiEditorInternalBus.h"
#include "UiEditorEntityContext.h"
#include "UiSliceManager.h"
#include "ViewportInteraction.h"
#include <QList>
#include <QMetaObject>

#include <AzCore/Debug/TraceMessageBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <LyShine/Bus/UiEditorChangeNotificationBus.h>

#include <IFont.h>
#include <IFileUtil.h>
#endif

QT_FORWARD_DECLARE_CLASS(QUndoGroup)
class AssetTreeEntry;

class EditorWindow
    : public QMainWindow
    , public IEditorNotifyListener
    , public UiEditorDLLBus::Handler
    , public UiEditorChangeNotificationBus::Handler
    , public UiEditorInternalRequestBus::Handler
    , public UiEditorInternalNotificationBus::Handler
    , public AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler
    , public UiEditorEntityContextNotificationBus::Handler
    , public AzToolsFramework::EditorEvents::Bus::Handler
    , public FontNotificationBus::Handler
    , public AZ::Debug::TraceMessageBus::Handler
{
    Q_OBJECT

public: // types

    struct UiCanvasTabMetadata
    {
        AZ::EntityId m_canvasEntityId;
    };

public: // member functions

    explicit EditorWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    virtual ~EditorWindow();

    // you are required to implement this to satisfy the unregister/registerclass requirements on "RegisterQtViewPane"
    // make sure you pick a unique GUID
    static const GUID& GetClassID()
    {
        // {E72CB9F3-DCB5-4525-AEAC-541A8CC778C5}
        static const GUID guid =
        {
            0xe72cb9f3, 0xdcb5, 0x4525, { 0xae, 0xac, 0x54, 0x1a, 0x8c, 0xc7, 0x78, 0xc5 }
        };
        return guid;
    }

    void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;

    // UiEditorDLLInterface
    LyShine::EntityArray GetSelectedElements() override;
    AZ::EntityId GetActiveCanvasId() override;
    UndoStack* GetActiveUndoStack() override;
    void OpenSourceCanvasFile(QString absolutePathToFile) override;
    // ~UiEditorDLLInterface

    // UiEditorChangeNotificationBus
    void OnEditorTransformPropertiesNeedRefresh() override;
    void OnEditorPropertiesRefreshEntireTree() override;
    // ~UiEditorChangeNotificationBus

    // UiEditorInternalRequestBus
    AzToolsFramework::EntityIdList GetSelectedEntityIds() override;
    AZ::Entity::ComponentArrayType GetSelectedComponents() override;
    AZ::EntityId GetActiveCanvasEntityId() override;
    // ~UiEditorInternalRequestBus

    // UiEditorInternalNotificationBus
    void OnSelectedEntitiesPropertyChanged() override;
    void OnBeginUndoableEntitiesChange() override;
    void OnEndUndoableEntitiesChange(const AZStd::string& commandText) override;
    // ~UiEditorInternalNotificationBus

    // AssetBrowserModelNotificationBus
    void EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
    void EntryRemoved(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) override;
    // ~AssetBrowserModelNotificationBus

    // UiEditorEntityContextNotificationBus
    void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& ticket) override;
    void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& ticket) override;
    // ~UiEditorEntityContextNotificationBus
        
    // EditorEvents
    void OnEscape() override;
    // ~EditorEvents

    // FontNotifications
    void OnFontsReloaded() override;
    // ~FontNotifications
    // TraceMessageEvents
    bool OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message) override;
    bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override;
    // ~TraceMessageEvents

    AZ::EntityId GetCanvas();

    HierarchyWidget* GetHierarchy();
    ViewportWidget* GetViewport();
    PropertiesWidget* GetProperties();
    MainToolbar* GetMainToolbar();
    ModeToolbar* GetModeToolbar();
    EnterPreviewToolbar* GetEnterPreviewToolbar();
    PreviewToolbar* GetPreviewToolbar();
    NewElementToolbarSection* GetNewElementToolbarSection();
    CoordinateSystemToolbarSection* GetCoordinateSystemToolbarSection();
    CanvasSizeToolbarSection* GetCanvasSizeToolbarSection();

    const QCursor& GetEntityPickerCursor();

    bool CanExitNow();

    UndoStack* GetActiveStack();

    AssetTreeEntry* GetSliceLibraryTree();

    //! Returns the current mode of the editor (Edit or Preview)
    UiEditorMode GetEditorMode() { return m_editorMode; }

    //! Returns the UI canvas for the current mode (Edit or Preview)
    AZ::EntityId GetCanvasForCurrentEditorMode();

    //! Toggle the editor mode between Edit and Preview
    void ToggleEditorMode();

    //! Get the copy of the canvas that is used in Preview mode (will return invalid entity ID if not in preview mode)
    AZ::EntityId GetPreviewModeCanvas() { return m_previewModeCanvasEntityId; }

    //! Get the preview canvas size.  (0,0) means use viewport size
    AZ::Vector2 GetPreviewCanvasSize();

    //! Set the preview canvas size. (0,0) means use viewport size
    void SetPreviewCanvasSize(AZ::Vector2 previewCanvasSize);

    void SaveEditorWindowSettings();

    UiSliceManager* GetSliceManager();

    UiEditorEntityContext* GetEntityContext();

    void ReplaceEntityContext(UiEditorEntityContext* entityContext);

    QMenu* createPopupMenu() override;
    AZ::EntityId GetCanvasForEntityContext(const AzFramework::EntityContextId& contextId);

    //! Open a new tab and instantiate the given slice asset for editing in a special slice editing mode
    void EditSliceInNewTab(AZ::Data::AssetId sliceAssetId);

    //! Called if an asset has changed and been reloaded (used to detect if slice being edited is different to the one on disk)
    void UpdateChangedStatusOnAssetChange(const AzFramework::EntityContextId& contextId, const AZ::Data::Asset<AZ::Data::AssetData>& asset);

    //! Called when any entities have been added to or removed from the active canvas
    void EntitiesAddedOrRemoved();

    //! Called when any font texture has changed since the last render.
    //! Forces a render graph update for each loaded canvas
    void FontTextureHasChanged();

    void RefreshEditorMenu();

    void ShowEntitySearchModal();

signals:

    void EditorModeChanged(UiEditorMode mode);
    void SignalCoordinateSystemCycle();
    void SignalSnapToGridToggle();

protected:

    void paintEvent(QPaintEvent* paintEvent) override;
    void closeEvent(QCloseEvent* closeEvent) override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

public slots:
    void RestoreEditorWindowSettings();

private: // types

    struct UiCanvasEditState
    {
        UiCanvasEditState();

        // Viewport
        ViewportInteraction::TranslationAndScale m_canvasViewportMatrixProps;
        bool m_shouldScaleToFitOnViewportResize;
        ViewportInteraction::InteractionMode m_viewportInteractionMode;
        ViewportInteraction::CoordinateSystem m_viewportCoordinateSystem;

        // Hierarchy
        int m_hierarchyScrollValue;
        EntityHelpers::EntityIdList m_selectedElements;

        // Properties
        float m_propertiesScrollValue;

        // Animation
        UiEditorAnimationStateInterface::UiEditorAnimationEditState m_uiAnimationEditState;

        bool m_inited;
    };

    // Data for a loaded UI canvas
    struct UiCanvasMetadata
    {
        UiCanvasMetadata();
        ~UiCanvasMetadata();

        AZ::EntityId m_canvasEntityId;
        AZStd::string m_canvasSourceAssetPathname;
        AZStd::string m_canvasDisplayName;
        UiEditorEntityContext* m_entityContext;
        UndoStack* m_undoStack;
        //! Specifies whether this canvas was automatically loaded or loaded by the user
        bool m_autoLoaded;
        //! Specified whether there were any errors on canvas load
        bool m_errorsOnLoad;
        //! Specifies whether a canvas has been modified and saved since it was loaded/created
        bool m_canvasChangedAndSaved;
        //! State of the viewport and other panes (zoom, pan, scroll, selection, ...)
        UiCanvasEditState m_canvasEditState;
        //! This is true when the canvas tab was opened in order to edit a slice
        bool m_isSliceEditing;
        //! If m_isSliceEditing is true this is the Asset ID of the slice instance that is being edited
        AZ::Data::AssetId m_sliceAssetId;
        //! If m_isSliceEditing is true this is the entityId of the one slice instance that is being edited
        AZ::EntityId m_sliceEntityId;
    };

private: // member functions

    QUndoGroup* GetUndoGroup();

    bool GetChangesHaveBeenMade(const UiCanvasMetadata& canvasMetadata);

    //! Return true when ok.
    //! forceAskingForFilename should only be true for "Save As...", not "Save".
    bool SaveCanvasToXml(UiCanvasMetadata& canvasMetadata, bool forceAskingForFilename);

    //! Saves a slice tab to its slice with a quick push
    bool SaveSlice(UiCanvasMetadata& canvasMetadata);

    //! Check whether a canvas save should occur even though there were errors on load
    bool CanSaveWithErrors(const UiCanvasMetadata& canvasMetadata);

    // Called from menu or shortcut key events
    void NewCanvas();
    void OpenCanvas(const QString& canvasFilename);
    void OpenCanvases(const QStringList& canvasFilenames);
    void CloseCanvas(AZ::EntityId canvasEntityId);
    void CloseAllCanvases();
    void CloseAllOtherCanvases(AZ::EntityId canvasEntityId);

    bool LoadCanvas(const QString& canvasFilename, bool autoLoad, bool changeActiveCanvasToThis = true);
    bool CanUnloadCanvas(UiCanvasMetadata& canvasMetadata);
    void UnloadCanvas(AZ::EntityId canvasEntityId);
    void UnloadCanvases(const AZStd::vector<AZ::EntityId>& canvasEntityIds);

    bool CanChangeActiveCanvas();
    void SetActiveCanvas(AZ::EntityId canvasEntityId);

    void SaveActiveCanvasEditState();
    void RestoreActiveCanvasEditState();
    void RestoreActiveCanvasEditStatePostEvents();

    void OnCanvasTabCloseButtonPressed(int index);
    void OnCurrentCanvasTabChanged(int index);
    void OnCanvasTabContextMenuRequested(const QPoint &point);

    void UpdateActionsEnabledState();

    void SetupShortcuts();

    //! Check if the given toolbar should only be shown in preview mode
    bool IsPreviewModeToolbar(const QToolBar* toolBar);

    //! Check if the given dockwidget should only be shown in preview mode
    bool IsPreviewModeDockWidget(const QDockWidget* dockWidget);

    QAction* AddMenuAction(const QString& text, bool enabled, QMenu* menu,  AZStd::function<void (bool)> function);
    void AddMenu_File();
    void AddMenuItems_Edit(QMenu* menu);
    void AddMenu_Edit();
    void AddMenu_View();
    void AddMenu_View_LanguageSetting(QMenu* viewMenu);
    void AddMenu_Preview();
    void AddMenu_PreviewView();
    void AddMenu_Help();
    void EditorMenu_Open(QString optional_selectedFile);

    QAction* CreateSaveCanvasAction(AZ::EntityId canvasEntityId, bool forContextMenu = false);
    QAction* CreateSaveCanvasAsAction(AZ::EntityId canvasEntityId, bool forContextMenu = false);
    QAction* CreateSaveSliceAction(UiCanvasMetadata *canvasMetadata, bool forContextMenu = false);
    QAction* CreateSaveAllCanvasesAction(bool forContextMenu = false);
    QAction* CreateCloseCanvasAction(AZ::EntityId canvasEntityId, bool forContextMenu = false);
    QAction* CreateCloseAllOtherCanvasesAction(AZ::EntityId canvasEntityId, bool forContextMenu = false);
    QAction* CreateCloseAllCanvasesAction(bool forContextMenu = false);

    void SaveModeSettings(UiEditorMode mode, bool syncSettings);
    void RestoreModeSettings(UiEditorMode mode);

    int GetCanvasMaxHierarchyDepth(const LyShine::EntityArray& childElements);

    void DeleteSliceLibraryTree();

    void DestroyCanvas(const UiCanvasMetadata& canvasMetadata);

    bool IsCanvasTabMetadataValidForTabIndex(int index);
    AZ::EntityId GetCanvasEntityIdForTabIndex(int index);
    int GetTabIndexForCanvasEntityId(AZ::EntityId canvasEntityId);
    UiCanvasMetadata* GetCanvasMetadataForTabIndex(int index);
    UiCanvasMetadata* GetCanvasMetadata(AZ::EntityId canvasEntityId);
    UiCanvasMetadata* GetActiveCanvasMetadata();

    AZStd::string GetCanvasDisplayNameFromAssetPath(const AZStd::string& canvasAssetPathname);

    void HandleCanvasDisplayNameChanged(const UiCanvasMetadata& canvasMetadata);

    void SetupCentralWidget();
    void SetupTabbedViewportWidget(QWidget* parent);

    void CheckForOrphanedChildren(AZ::EntityId canvasEntityId);

    void AddTraceMessage(const char *message, AZStd::list<QString>& messageList);
    void ShowTraceMessages(const AZStd::string& canvasName);
    void ClearTraceMessages();

private slots:
    // Called when the clean state of the active undo stack changes
    void CleanChanged(bool clean);

private: // data

    QUndoGroup* m_undoGroup;

    UiSliceManager* m_sliceManager;

    AzQtComponents::TabWidget* m_canvasTabWidget;
    QWidget* m_canvasTabSectionWidget;
    HierarchyWidget* m_hierarchy;
    PropertiesWrapper* m_properties;
    ViewportWidget* m_viewport;
    CUiAnimViewDialog* m_animationWidget;
    PreviewActionLog* m_previewActionLog;
    PreviewAnimationList* m_previewAnimationList;

    MainToolbar* m_mainToolbar;
    ModeToolbar* m_modeToolbar;
    EnterPreviewToolbar* m_enterPreviewToolbar;
    PreviewToolbar* m_previewToolbar;

    QDockWidget* m_hierarchyDockWidget;
    QDockWidget* m_propertiesDockWidget;
    QDockWidget* m_animationDockWidget;
    QDockWidget* m_previewActionLogDockWidget;
    QDockWidget* m_previewAnimationListDockWidget;

    UiEditorMode m_editorMode;

    //! This tree caches the folder view of all the slice assets under the slice library path
    AssetTreeEntry* m_sliceLibraryTree = nullptr;

    //! Values for setting up undoable canvas/entity changes
    SerializeHelpers::SerializedEntryList m_preChangeState;
    bool m_haveValidEntitiesPreChangeState = false;
    AZStd::string m_canvasUndoXml;
    bool m_haveValidCanvasPreChangeState = false;

    //! This is used to change the enabled state
    //! of these actions as the selection changes.
    QList<QAction*> m_actionsEnabledWithSelection;
    QAction* m_pasteAsSiblingAction;
    QAction* m_pasteAsChildAction;
    QList<QAction*> m_actionsEnabledWithAlignAllowed;

    AZ::EntityId m_previewModeCanvasEntityId;

    AZ::Vector2 m_previewModeCanvasSize;

    QMetaObject::Connection m_clipboardConnection;

    //! Local copy of QSetting value of startup location of localization folder
    QString m_startupLocFolderName;

    std::map< AZ::EntityId, UiCanvasMetadata* > m_canvasMetadataMap;
    AZ::EntityId m_activeCanvasEntityId;

    int m_newCanvasCount;

    AZStd::list<QString> m_errors; // the list of errors that occured while loading a canvas
    AZStd::list<QString> m_warnings; // the list of warnings that occured while loading a canvas

    // Cursor used when picking an element in the hierarchy or viewport during object pick mode
    QCursor m_entityPickerCursor;
};

Q_DECLARE_METATYPE(EditorWindow::UiCanvasTabMetadata);
