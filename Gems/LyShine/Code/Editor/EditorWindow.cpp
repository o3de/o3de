/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "CanvasHelpers.h"
#include "AssetDropHelpers.h"
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/Bus/UiEditorCanvasBus.h>
#include <Util/PathUtil.h>
#include "EditorDefs.h"
#include "ErrorDialog.h"
#include "Settings.h"
#include "AnchorPresets.h"
#include "PivotPresets.h"
#include "Animation/UiAnimViewDialog.h"
#include "AssetTreeEntry.h"
#include "FindEntityWidget.h"

#include <QMessageBox>
#include <QBoxLayout>
#include <QClipboard>
#include <QUndoGroup>
#include <QScrollBar>

#define UICANVASEDITOR_SETTINGS_EDIT_MODE_STATE_KEY     (QString("Edit Mode State") + " " + FileHelpers::GetAbsoluteGameDir())
#define UICANVASEDITOR_SETTINGS_EDIT_MODE_GEOM_KEY      (QString("Edit Mode Geometry") + " " + FileHelpers::GetAbsoluteGameDir())
#define UICANVASEDITOR_SETTINGS_PREVIEW_MODE_STATE_KEY  (QString("Preview Mode State") + " " + FileHelpers::GetAbsoluteGameDir())
#define UICANVASEDITOR_SETTINGS_PREVIEW_MODE_GEOM_KEY   (QString("Preview Mode Geometry") + " " + FileHelpers::GetAbsoluteGameDir())
#define UICANVASEDITOR_SETTINGS_WINDOW_STATE_VERSION    (1)

#define UICANVASEDITOR_ENTITY_PICKER_CURSOR             ":/Icons/EntityPickerCursor.png"

// This has to live outside of any namespaces due to issues on Linux with calls to Q_INIT_RESOURCE if they are inside a namespace
void initUiCanvasEditorResources()
{
    Q_INIT_RESOURCE(UiCanvasEditor);
    Q_INIT_RESOURCE(UiAnimViewDialog);
}

namespace
{
    //! \brief Writes the current value of the sys_localization_folder CVar to the editor settings file (Amazon.ini)
    void SaveStartupLocalizationFolderSetting()
    {
        if (gEnv && gEnv->pConsole)
        {
            ICVar* locFolderCvar = gEnv->pConsole->GetCVar("sys_localization_folder");

            QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);
            settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

            settings.setValue(UICANVASEDITOR_SETTINGS_STARTUP_LOC_FOLDER_KEY, locFolderCvar->GetString());

            settings.endGroup();
            settings.sync();
        }
    }

    //! \brief Reads loc folder value from Amazon.ini and re-sets the CVar accordingly
    void RestoreStartupLocalizationFolderSetting()
    {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);
        settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

        QString startupLocFolder(settings.value(UICANVASEDITOR_SETTINGS_STARTUP_LOC_FOLDER_KEY).toString());
        if (!startupLocFolder.isEmpty() && gEnv && gEnv->pConsole)
        {
            ICVar* locFolderCvar = gEnv->pConsole->GetCVar("sys_localization_folder");
            locFolderCvar->Set(startupLocFolder.toUtf8().constData());
        }

        settings.endGroup();
        settings.sync();
    }
}

EditorWindow::UiCanvasEditState::UiCanvasEditState()
    : m_inited(false)
{
}

EditorWindow::UiCanvasMetadata::UiCanvasMetadata()
    : m_entityContext(nullptr)
    , m_undoStack(nullptr)
    , m_canvasChangedAndSaved(false)
    , m_isSliceEditing(false)
{
}

EditorWindow::UiCanvasMetadata::~UiCanvasMetadata()
{
    delete m_entityContext;
    delete m_undoStack;
}

EditorWindow::EditorWindow(QWidget* parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , IEditorNotifyListener()
    , m_undoGroup(new QUndoGroup(this))
    , m_sliceManager(new UiSliceManager(AzFramework::EntityContextId::CreateNull()))
    , m_hierarchy(new HierarchyWidget(this))
    , m_properties(new PropertiesWrapper(m_hierarchy, this))
    , m_canvasTabWidget(nullptr)
    , m_canvasTabSectionWidget(nullptr)
    , m_viewport(nullptr)
    , m_animationWidget(new CUiAnimViewDialog(this))
    , m_previewActionLog(new PreviewActionLog(this))
    , m_previewAnimationList(new PreviewAnimationList(this))
    , m_mainToolbar(new MainToolbar(this))
    , m_modeToolbar(new ModeToolbar(this))
    , m_enterPreviewToolbar(new EnterPreviewToolbar(this))
    , m_previewToolbar(new PreviewToolbar(this))
    , m_hierarchyDockWidget(nullptr)
    , m_propertiesDockWidget(nullptr)
    , m_animationDockWidget(nullptr)
    , m_previewActionLogDockWidget(nullptr)
    , m_previewAnimationListDockWidget(nullptr)
    , m_editorMode(UiEditorMode::Edit)
    , m_actionsEnabledWithSelection()
    , m_pasteAsSiblingAction(nullptr)
    , m_pasteAsChildAction(nullptr)
    , m_actionsEnabledWithAlignAllowed()
    , m_previewModeCanvasEntityId()
    , m_previewModeCanvasSize(0.0f, 0.0f)
    , m_clipboardConnection()
    , m_newCanvasCount(1)
{
    initUiCanvasEditorResources();

    // Since the lifetime of EditorWindow and the UI Editor itself aren't the
    // same, we use the initial opening of the UI Editor to save the current
    // value of the loc folder CVar since the user can temporarily change its
    // value while using the UI Editor.
    SaveStartupLocalizationFolderSetting();

    PropertyHandlers::Register();

    setAcceptDrops(true);

    // Store local copy of startup localization value
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);
    settings.beginGroup(UICANVASEDITOR_NAME_SHORT);
    m_startupLocFolderName = settings.value(UICANVASEDITOR_SETTINGS_STARTUP_LOC_FOLDER_KEY).toString();
    settings.endGroup();

    // update menus when the selection changes
    connect(m_hierarchy, &HierarchyWidget::SetUserSelection, this, &EditorWindow::UpdateActionsEnabledState);
    m_clipboardConnection = connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &EditorWindow::UpdateActionsEnabledState);

    // Create the cursor to be used when picking an element in the hierarchy or viewport during object pick mode.
    // Uses the default hot spot which is the center of the image
    m_entityPickerCursor = QCursor(QPixmap(UICANVASEDITOR_ENTITY_PICKER_CURSOR));

    // disable rendering of the editor window until we have restored the window state
    setUpdatesEnabled(false);

    // Create the viewport widget
    m_viewport = new ViewportWidget(this);
    m_viewport->GetViewportInteraction()->UpdateZoomFactorLabel();
    m_viewport->setFocusPolicy(Qt::StrongFocus);

    // Create the central widget
    SetupCentralWidget();

    // Signal: Hierarchical tree -> Properties pane.
    QObject::connect(m_hierarchy,
        SIGNAL(SetUserSelection(HierarchyItemRawPtrList*)),
        m_properties->GetProperties(),
        SLOT(UserSelectionChanged(HierarchyItemRawPtrList*)));

    // Signal: Hierarchical tree -> Viewport pane.
    QObject::connect(m_hierarchy,
        SIGNAL(SetUserSelection(HierarchyItemRawPtrList*)),
        GetViewport(),
        SLOT(UserSelectionChanged(HierarchyItemRawPtrList*)));


    QObject::connect(m_undoGroup, &QUndoGroup::cleanChanged, this, &EditorWindow::CleanChanged);

    // by default the BottomDockWidgetArea will be the full width of the main window
    // and will make the Hierarchy and Properties panes less tall. This makes the
    // Hierarchy and Properties panes occupy the corners and makes the animation pane less wide.
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Hierarchy pane.
    {
        m_hierarchyDockWidget = new AzQtComponents::StyledDockWidget("Hierarchy");
        m_hierarchyDockWidget->setObjectName("HierarchyDockWidget");    // needed to save state
        m_hierarchyDockWidget->setWidget(m_hierarchy);
        // needed to get keyboard shortcuts properly
        m_hierarchy->setFocusPolicy(Qt::StrongFocus);
        addDockWidget(Qt::LeftDockWidgetArea, m_hierarchyDockWidget, Qt::Vertical);
    }

    // Properties pane.
    {
        m_propertiesDockWidget = new AzQtComponents::StyledDockWidget("Properties");
        m_propertiesDockWidget->setObjectName("PropertiesDockWidget");    // needed to save state
        m_propertiesDockWidget->setWidget(m_properties);
        m_properties->setFocusPolicy(Qt::StrongFocus);
        addDockWidget(Qt::RightDockWidgetArea, m_propertiesDockWidget, Qt::Vertical);
    }

    // Animation pane.
    {
        m_animationDockWidget = new AzQtComponents::StyledDockWidget("Animation Editor");
        m_animationDockWidget->setObjectName("AnimationDockWidget");    // needed to save state
        m_animationDockWidget->setWidget(m_animationWidget);
        m_animationWidget->setFocusPolicy(Qt::StrongFocus);
        addDockWidget(Qt::BottomDockWidgetArea, m_animationDockWidget, Qt::Horizontal);
    }

    // Preview action log pane (only shown in preview mode)
    {
        m_previewActionLogDockWidget = new AzQtComponents::StyledDockWidget("Action Log");
        m_previewActionLogDockWidget->setObjectName("PreviewActionLog");    // needed to save state
        m_previewActionLogDockWidget->setWidget(m_previewActionLog);
        m_previewActionLog->setFocusPolicy(Qt::StrongFocus);
        addDockWidget(Qt::BottomDockWidgetArea, m_previewActionLogDockWidget, Qt::Horizontal);
    }

    // Preview animation list pane (only shown in preview mode)
    {
        m_previewAnimationListDockWidget = new AzQtComponents::StyledDockWidget("Animation List");
        m_previewAnimationListDockWidget->setObjectName("PreviewAnimationList");    // needed to save state
        m_previewAnimationListDockWidget->setWidget(m_previewAnimationList);
        m_previewAnimationList->setFocusPolicy(Qt::StrongFocus);
        addDockWidget(Qt::LeftDockWidgetArea, m_previewAnimationListDockWidget, Qt::Vertical);
    }

    // We start out in edit mode so hide the preview mode widgets
    m_previewActionLogDockWidget->hide();
    m_previewAnimationListDockWidget->hide();
    m_previewToolbar->hide();

    // Initialize the menus
    RefreshEditorMenu();

    GetIEditor()->RegisterNotifyListener(this);

    // Initialize the toolbars
    m_viewport->GetViewportInteraction()->InitializeToolbars();

    // Start listening for any queries on the UiEditorDLLBus
    UiEditorDLLBus::Handler::BusConnect();

    // Start listening for any queries on the UiEditorChangeNotificationBus
    UiEditorChangeNotificationBus::Handler::BusConnect();

    // Start listening for any internal requests and notifications in the UI Editor
    UiEditorInternalRequestBus::Handler::BusConnect();
    UiEditorInternalNotificationBus::Handler::BusConnect();

    AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler::BusConnect();

    AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    FontNotificationBus::Handler::BusConnect();

    // Don't draw the viewport until the window is shown
    m_viewport->SetRedrawEnabled(false);

    // Create an empty canvas
    LoadCanvas("", true);

    QTimer::singleShot(0, this, SLOT(RestoreEditorWindowSettings()));
}

EditorWindow::~EditorWindow()
{
    AzToolsFramework::AssetBrowser::AssetBrowserModelNotificationBus::Handler::BusDisconnect();

    AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    FontNotificationBus::Handler::BusDisconnect();

    QObject::disconnect(m_clipboardConnection);

    GetIEditor()->UnregisterNotifyListener(this);

    UiEditorDLLBus::Handler::BusDisconnect();
    UiEditorChangeNotificationBus::Handler::BusDisconnect();

    UiEditorInternalRequestBus::Handler::BusDisconnect();
    UiEditorInternalNotificationBus::Handler::BusDisconnect();

    // This has to be disconnected, or we'll get some weird feedback loop
    // where the cleanChanged signal propagates back up to the EditorWindow's
    // tab control, which is possibly already deleted, and everything explodes
    QObject::disconnect(m_undoGroup, &QUndoGroup::cleanChanged, this, &EditorWindow::CleanChanged);

    // Destroy all loaded canvases
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        DestroyCanvas(*canvasMetadata);
        delete canvasMetadata;
    }

    m_activeCanvasEntityId.SetInvalid();
    // Tell the UI animation system that the active canvas has changed
    UiEditorAnimationBus::Broadcast(&UiEditorAnimationBus::Events::ActiveCanvasChanged);

    // unload the preview mode canvas if it exists (e.g. if we close the editor window while in preview mode)
    if (m_previewModeCanvasEntityId.IsValid())
    {
        AZ::Interface<ILyShine>::Get()->ReleaseCanvas(m_previewModeCanvasEntityId, false);
    }

    delete m_sliceLibraryTree;

    delete m_sliceManager;

    // We must restore the original loc folder CVar value otherwise we will
    // have no way of obtaining the original loc folder location (in case
    // the user chooses to open the UI Editor once more).
    RestoreStartupLocalizationFolderSetting();
}

LyShine::EntityArray EditorWindow::GetSelectedElements()
{
    LyShine::EntityArray elements = SelectionHelpers::GetSelectedElements(
        m_hierarchy,
        m_hierarchy->selectedItems());

    return elements;
}

AZ::EntityId EditorWindow::GetActiveCanvasId()
{
    return GetCanvas();
}

UndoStack* EditorWindow::GetActiveUndoStack()
{
    return GetActiveStack();
}

void EditorWindow::OnEditorTransformPropertiesNeedRefresh()
{
    AZ::Uuid transformComponentUuid = LyShine::UiTransform2dComponentUuid;
    GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues, &transformComponentUuid);
}

void EditorWindow::OnEditorPropertiesRefreshEntireTree()
{
    GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
}

void EditorWindow::OpenSourceCanvasFile(QString absolutePathToFile)
{
    // If in Preview mode, exit back to Edit mode
    if (m_editorMode == UiEditorMode::Preview)
    {
        ToggleEditorMode();
    }

    OpenCanvas(absolutePathToFile);
}

AzToolsFramework::EntityIdList EditorWindow::GetSelectedEntityIds()
{
    EntityHelpers::EntityIdList entityIds;

    if (m_hierarchy)
    {
        entityIds = SelectionHelpers::GetSelectedElementIds(
            m_hierarchy,
            m_hierarchy->selectedItems(),
            false);
    }

    return entityIds;
}

AZ::Entity::ComponentArrayType EditorWindow::GetSelectedComponents()
{
    AZ::Entity::ComponentArrayType selectedComponents;

    if (m_properties)
    {
        selectedComponents = m_properties->GetProperties()->GetSelectedComponents();
    }

    return selectedComponents;
}

AZ::EntityId EditorWindow::GetActiveCanvasEntityId()
{
    return GetCanvas();
}

void EditorWindow::OnSelectedEntitiesPropertyChanged()
{
    // This is necessary to update the PropertiesWidget.
    m_hierarchy->SignalUserSelectionHasChanged(m_hierarchy->selectedItems());
}

void EditorWindow::OnBeginUndoableEntitiesChange()
{
    AZ_Assert(!m_haveValidCanvasPreChangeState && !m_haveValidEntitiesPreChangeState, "Calling BeginUndoableEntitiesChange before EndUndoableEntitiesChange");

    // Check if the canvas is selected to set up the correct undo command
    if (m_hierarchy->selectedItems().empty())
    {
        m_canvasUndoXml = CanvasHelpers::BeginUndoableCanvasChange(m_activeCanvasEntityId);
        m_haveValidCanvasPreChangeState = true;
    }
    else
    {
        HierarchyClipboard::BeginUndoableEntitiesChange(this, m_preChangeState);
        m_haveValidEntitiesPreChangeState = true;
    }
}

void EditorWindow::OnEndUndoableEntitiesChange(const AZStd::string& commandText)
{
    // Check if the canvas is selected to set up the correct undo command
    if (m_hierarchy->selectedItems().empty())
    {
        AZ_Assert(m_haveValidCanvasPreChangeState, "Calling EndUndoableEntitiesChange without calling BeginUndoableEntitiesChange first");
        if (m_haveValidCanvasPreChangeState)
        {
            CanvasHelpers::EndUndoableCanvasChange(this, commandText.c_str(), m_canvasUndoXml);
            m_haveValidCanvasPreChangeState = false;
        }
    }
    else
    {
        AZ_Assert(m_haveValidEntitiesPreChangeState, "Calling EndUndoableEntitiesChange without calling BeginUndoableEntitiesChange first");
        if (m_haveValidEntitiesPreChangeState)
        {
            HierarchyClipboard::EndUndoableEntitiesChange(this, commandText.c_str(), m_preChangeState);
            m_haveValidEntitiesPreChangeState = false;
        }
    }
}

void EditorWindow::EntryAdded(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* /*entry*/)
{
    DeleteSliceLibraryTree();
}

void EditorWindow::EntryRemoved(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* /*entry*/)
{
    DeleteSliceLibraryTree();
}

void EditorWindow::OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, [[maybe_unused]] const AZ::SliceComponent::SliceInstanceAddress& sliceAddress, [[maybe_unused]] const AzFramework::SliceInstantiationTicket& ticket)
{
    // We are only interested in the first tab that is waiting for this slice asset to be instantiated.
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        if (canvasMetadata->m_isSliceEditing && canvasMetadata->m_sliceAssetId == sliceAssetId && !canvasMetadata->m_sliceEntityId.IsValid())
        {
            // This is the slice instantiation that we do automatically when a slice is opened for edit in a new tab

            // Get the entityId of the top level element we have instantiated into the canvas and store it
            AZ::EntityId sliceEntityId;
            UiCanvasBus::EventResult(sliceEntityId, canvasMetadata->m_canvasEntityId, &UiCanvasBus::Events::GetChildElementEntityId, 0);
            canvasMetadata->m_sliceEntityId = sliceEntityId;
        
            // we don't want an asterisk to show as we haven't made any changes to the slice yet
            canvasMetadata->m_undoStack->setClean();

            // Update the menus for file/save/close - the file menu will show the slice name
            RefreshEditorMenu();

            // only do this for one slice (in case of the edge case where two slice edit tabs could have been opened before either slice is instantiated)
            break;
        }
    }

    // Check if we have any more tabs waiting for their slice to be instantiated for edit (highly unlikely, it would be an edge case)
    bool waitingForMoreSliceEditInstantiates = false;
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        if (canvasMetadata->m_isSliceEditing && !canvasMetadata->m_sliceEntityId.IsValid())
        {
            waitingForMoreSliceEditInstantiates = true;
        }
    }

    if (!waitingForMoreSliceEditInstantiates)
    {
        UiEditorEntityContextNotificationBus::Handler::BusDisconnect();
    }
}

void EditorWindow::OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, [[maybe_unused]] const AzFramework::SliceInstantiationTicket& ticket)
{
    // We are only interested in the first tab that is waiting for this slice asset to be instantiated.
    // It may be impossible to get this error because, in the case of Edit Slice in New Tab, we already have the slice asset loaded
    // so it is hard for the instantiate to fail.
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        if (canvasMetadata->m_isSliceEditing && canvasMetadata->m_sliceAssetId == sliceAssetId && !canvasMetadata->m_sliceEntityId.IsValid())
        {
            // The slice instantiation that failed is an instantiation that we do automatically when a slice is opened for edit in a new tab

            // Instantiate failed so close the tab and delete this metadata
            UnloadCanvas(canvasMetadata->m_canvasEntityId);

            // only do this for one slice (in case of the edge case where two slice edit tabs could have been opened before either slice is instantiated)
            break;
        }
    }

    // Check if we have any more tabs waiting for their slice to be instantiated for edit (highly unlikely, it would be an edge case)
    bool waitingForMoreSliceEditInstantiates = false;
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        if (canvasMetadata->m_isSliceEditing && !canvasMetadata->m_sliceEntityId.IsValid())
        {
            waitingForMoreSliceEditInstantiates = true;
        }
    }

    if (!waitingForMoreSliceEditInstantiates)
    {
        UiEditorEntityContextNotificationBus::Handler::BusDisconnect();
    }
}

void EditorWindow::OnEscape()
{
    if (GetEditorMode() == UiEditorMode::Preview && isActiveWindow())
    {
        ToggleEditorMode();
    }
}

void EditorWindow::OnFontsReloaded()
{
    OnEditorPropertiesRefreshEntireTree();
}

bool EditorWindow::OnPreError(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
{
    AddTraceMessage(message, m_errors);
    return true;
}

bool EditorWindow::OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
{
    AddTraceMessage(message, m_warnings);
    return true;
}

void EditorWindow::DestroyCanvas(const UiCanvasMetadata& canvasMetadata)
{
    AZ::Interface<ILyShine>::Get()->ReleaseCanvas(canvasMetadata.m_canvasEntityId, true);
}

bool EditorWindow::IsCanvasTabMetadataValidForTabIndex(int index)
{
    QVariant tabData = m_canvasTabWidget->tabBar()->tabData(index);
    return tabData.isValid();
}

AZ::EntityId EditorWindow::GetCanvasEntityIdForTabIndex(int index)
{
    QVariant tabData = m_canvasTabWidget->tabBar()->tabData(index);
    AZ_Assert(tabData.isValid(), "Canvas tab metadata is not valid");
    if (tabData.isValid())
    {
        auto canvasTabMetadata = tabData.value<UiCanvasTabMetadata>();
        return canvasTabMetadata.m_canvasEntityId;
    }

    return AZ::EntityId();
}

int EditorWindow::GetTabIndexForCanvasEntityId(AZ::EntityId canvasEntityId)
{
    for (int i = 0; i < m_canvasTabWidget->count(); i++)
    {
        if (GetCanvasEntityIdForTabIndex(i) == canvasEntityId)
        {
            return i;
        }
    }

    return -1;
}

EditorWindow::UiCanvasMetadata* EditorWindow::GetCanvasMetadataForTabIndex(int index)
{
    return GetCanvasMetadata(GetCanvasEntityIdForTabIndex(index));
}

EditorWindow::UiCanvasMetadata* EditorWindow::GetCanvasMetadata(AZ::EntityId canvasEntityId)
{
    auto canvasMetadataMapIt = m_canvasMetadataMap.find(canvasEntityId);
    return (canvasMetadataMapIt != m_canvasMetadataMap.end() ? canvasMetadataMapIt->second : nullptr);
}

EditorWindow::UiCanvasMetadata* EditorWindow::GetActiveCanvasMetadata()
{
    return GetCanvasMetadata(m_activeCanvasEntityId);
}

AZStd::string EditorWindow::GetCanvasDisplayNameFromAssetPath(const AZStd::string& canvasAssetPathname)
{
    QFileInfo fileInfo(canvasAssetPathname.c_str());
    QString canvasDisplayName(fileInfo.baseName());
    if (canvasDisplayName.isEmpty())
    {
        canvasDisplayName = QString("Canvas%1").arg(m_newCanvasCount++);
    }

    return AZStd::string(canvasDisplayName.toLatin1().data());
}

void EditorWindow::HandleCanvasDisplayNameChanged(const UiCanvasMetadata& canvasMetadata)
{
    // Update the tab label for the canvas
    AZStd::string tabText(canvasMetadata.m_canvasDisplayName);
    if (GetChangesHaveBeenMade(canvasMetadata))
    {
        tabText.append("*");
    }
    int tabIndex = GetTabIndexForCanvasEntityId(canvasMetadata.m_canvasEntityId);
    if (m_canvasTabWidget->tabText(tabIndex) != QString(tabText.c_str()))
    {
        m_canvasTabWidget->setTabText(tabIndex, tabText.c_str());
    }
    m_canvasTabWidget->setTabToolTip(tabIndex, canvasMetadata.m_canvasSourceAssetPathname.empty() ? canvasMetadata.m_canvasDisplayName.c_str() : canvasMetadata.m_canvasSourceAssetPathname.c_str());
}

void EditorWindow::CleanChanged([[maybe_unused]] bool clean)
{
    UiCanvasMetadata *canvasMetadata = GetActiveCanvasMetadata();
    if (canvasMetadata)
    {
        HandleCanvasDisplayNameChanged(*canvasMetadata);
    }
}

bool EditorWindow::SaveCanvasToXml(UiCanvasMetadata& canvasMetadata, bool forceAskingForFilename)
{
    AZStd::string sourceAssetPathName = canvasMetadata.m_canvasSourceAssetPathname;
    AZStd::string assetIdPathname;

    if (canvasMetadata.m_errorsOnLoad)
    {
        bool saveWithErrors = CanSaveWithErrors(canvasMetadata);
        if (!saveWithErrors)
        {
            return false;
        }
    }

    if (!forceAskingForFilename)
    {
        // Before saving, make sure the file contains an extension we're expecting
        QString filename = sourceAssetPathName.c_str();
        if ((!filename.isEmpty()) &&
            (!FileHelpers::FilenameHasExtension(filename, UICANVASEDITOR_CANVAS_EXTENSION)))
        {
            QMessageBox::warning(this, tr("Warning"), tr("Please save with the expected extension: *.%1").arg(UICANVASEDITOR_CANVAS_EXTENSION));
            forceAskingForFilename = true;
        }
    }

    if (sourceAssetPathName.empty() || forceAskingForFilename)
    {
        // Default the pathname to where the current canvas was loaded from or last saved to

        QString dir;
        QStringList recentFiles = ReadRecentFiles();

        // If the canvas we are saving already has a name
        if (!sourceAssetPathName.empty())
        {
            // Default to where it was loaded from or last saved to
            // Also notice that we directly assign dir to the filename
            // This allows us to have its existing name already entered in
            // the File Name field.
            dir = sourceAssetPathName.c_str();
        }
        // Else if we had recently opened canvases, open the most recent one's directory
        else if (recentFiles.size() > 0)
        {
            dir = Path::GetPath(recentFiles.front());
        }
        // Else go to the default canvas directory
        else
        {
            dir = FileHelpers::GetAbsoluteDir(UICANVASEDITOR_CANVAS_DIRECTORY);
        }

        // Make sure the directory exists. If not, walk up the directory path until we find one that does
        // so that we will have a consistent 'starting folder' in the 'AzQtComponents::FileDialog::GetSaveFileName' call
        // across different platforms. 
        AZ::IO::FixedMaxPath dirPath(dir.toUtf8().constData());

        while (!AZ::IO::SystemFile::IsDirectory(dirPath.c_str()))
        {
            AZ::IO::PathView parentPath = dirPath.ParentPath();
            if (parentPath == dirPath)
            {
                // We've reach the root path, need to break out whether or not
                // the root path exists
                break;
            }
            else
            {
                dirPath = parentPath;
            }
        }
        // Append the default filename
        dirPath /= canvasMetadata.m_canvasDisplayName;
        dir = QString::fromUtf8(dirPath.c_str(), static_cast<int>(dirPath.Native().size()));

        QString filename = AzQtComponents::FileDialog::GetSaveFileName(nullptr,
            QString(),
            dir,
            "*." UICANVASEDITOR_CANVAS_EXTENSION,
            nullptr);
        if (filename.isEmpty())
        {
            return false;
        }

        // Append extension if not present
        FileHelpers::AppendExtensionIfNotPresent(filename, UICANVASEDITOR_CANVAS_EXTENSION);

        sourceAssetPathName = filename.toUtf8().data();

        // Check if the canvas is being saved in the product path
        bool foundRelativePath = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundRelativePath, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, sourceAssetPathName, assetIdPathname);
        if (!foundRelativePath)
        {
            // Warn that canvas is being saved outside the product path
            int result = QMessageBox::warning(this,
                tr("Warning"),
                tr("UI canvas %1 is being saved outside the source folder for the project (or the Asset Processor is not running).\n\nSaving to this location will result in not being able to re-open the UI Canvas in the UI Editor from this location.\n\nWould you still like to save to this location?").arg(filename),
                (QMessageBox::Save | QMessageBox::Cancel),
                QMessageBox::Cancel);

            if (result == QMessageBox::Save)
            {
                assetIdPathname = Path::FullPathToGamePath(sourceAssetPathName.c_str()); // Relative path.
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        sourceAssetPathName = canvasMetadata.m_canvasSourceAssetPathname;
        UiCanvasBus::EventResult(assetIdPathname, canvasMetadata.m_canvasEntityId, &UiCanvasBus::Events::GetPathname);
    }

    FileHelpers::SourceControlAddOrEdit(sourceAssetPathName.c_str(), this);

    bool saveSuccessful = false;
    UiCanvasBus::EventResult(
        saveSuccessful,
        canvasMetadata.m_canvasEntityId,
        &UiCanvasBus::Events::SaveToXml,
        assetIdPathname.c_str(),
        sourceAssetPathName.c_str());

    if (saveSuccessful)
    {
        AddRecentFile(sourceAssetPathName.c_str());

        canvasMetadata.m_errorsOnLoad = false;

        if (!canvasMetadata.m_canvasChangedAndSaved)
        {
            canvasMetadata.m_canvasChangedAndSaved = GetChangesHaveBeenMade(canvasMetadata);
        }
        canvasMetadata.m_canvasSourceAssetPathname = sourceAssetPathName;

        AZStd::string newDisplayName = GetCanvasDisplayNameFromAssetPath(canvasMetadata.m_canvasSourceAssetPathname);
        if (canvasMetadata.m_canvasDisplayName != newDisplayName)
        {
            canvasMetadata.m_canvasDisplayName = newDisplayName;
        }

        canvasMetadata.m_undoStack->setClean();

        // Although the line above will call this if the clean state changed we could be doing a "Save As" of a canvas
        // that has no unsaved changes, so the clean state would not change but we want to change the display name.
        HandleCanvasDisplayNameChanged(canvasMetadata);

        return true;
    }

    QMessageBox(QMessageBox::Critical,
        "Error",
        tr("Unable to save %1. Is the file read-only?").arg(sourceAssetPathName.empty() ? "file" : sourceAssetPathName.c_str()),
        QMessageBox::Ok, this).exec();

    return false;
}

bool EditorWindow::SaveSlice(UiCanvasMetadata& canvasMetadata)
{
    // as a safeguard check that the entity still exists
    AZ::EntityId sliceEntityId = canvasMetadata.m_sliceEntityId;
    AZ::Entity* sliceEntity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(sliceEntity, &AZ::ComponentApplicationBus::Events::FindEntity, sliceEntityId);
    if (!sliceEntity)
    {
        QMessageBox::critical(this, QObject::tr("Slice Push Failed"), "Slice entity not found in canvas.");
        return false;
    }

    AZ::SliceComponent::SliceInstanceAddress sliceAddress;
    AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, sliceEntityId,
        &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

    // if false then something is wrong. The user could have done a detach slice for example
    if (!sliceAddress.IsValid() || !sliceAddress.GetReference()->GetSliceAsset())
    {
        QMessageBox::critical(this, QObject::tr("Slice Push Failed"), "Slice entity no longer appears to be a slice instance.");
        return false;
    }

    // make a list that contains the top-level instanced entity plus all of its descendants
    AzToolsFramework::EntityIdList allEntitiesInLocalInstance;
    allEntitiesInLocalInstance.push_back(sliceEntityId);
    UiElementBus::Event(
        sliceEntityId,
        &UiElementBus::Events::CallOnDescendantElements,
        [&allEntitiesInLocalInstance](const AZ::EntityId id)
        {
            allEntitiesInLocalInstance.push_back(id);
        });

    const AZ::Outcome<void, AZStd::string> outcome = GetSliceManager()->QuickPushSliceInstance(sliceAddress, allEntitiesInLocalInstance);

    if (!outcome)
    {
        QMessageBox::critical(
            this,
            QObject::tr("Slice Push Failed"), 
            outcome.GetError().c_str());

        return false;
    }

    return true;
}

bool EditorWindow::CanSaveWithErrors(const UiCanvasMetadata& canvasMetadata)
{
    // Prompt the user that saving may result in data loss. Most of the time this is not desired
    // (which is why 'cancel' is the default interaction), but this does provide users a way to still
    // save their canvas if this is the only way they can solve the erroneous data
    QMessageBox msgBox(this);
    msgBox.setText(tr("Canvas %1 loaded with errors. You may lose work if you save.").arg(canvasMetadata.m_canvasDisplayName.c_str()));
    msgBox.setInformativeText(tr("Do you want to save your changes?"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int result = msgBox.exec();

    return result == QMessageBox::Save;
}

bool EditorWindow::LoadCanvas(const QString& canvasFilename, bool autoLoad, bool changeActiveCanvasToThis)
{
    // Don't allow a new canvas to load if there is a context menu up since loading doesn't
    // delete the context menu. Another option is to close the context menu on canvas load,
    // but the main editor's behavior seems to be to ignore the main keyboard shortcuts if
    // a context menu is up
    QWidget* widget = QApplication::activePopupWidget();
    if (widget)
    {
        return false;
    }

    AZStd::string assetIdPathname;
    AZStd::string sourceAssetPathName;
    if (!canvasFilename.isEmpty())
    {
        // Get the relative product path of the canvas to load
        bool foundRelativePath = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundRelativePath, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetRelativeProductPathFromFullSourceOrProductPath, canvasFilename.toUtf8().data(), assetIdPathname);
        if (!foundRelativePath)
        {
            // Canvas to load is not in a project source folder. Report an error
            QMessageBox::critical(this, tr("Error"), tr("Failed to open %1. Please ensure the file resides in a valid source folder for the project and that the Asset Processor is running.").arg(canvasFilename));
            return false;
        }

        // Get the path to the source UI Canvas from the relative product path
        // This is done because a canvas could be loaded from the cache folder. In this case, we want to find the path to the source file
        bool fullPathfound = false;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fullPathfound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, assetIdPathname, sourceAssetPathName);
        if (!fullPathfound)
        {
            // Couldn't find the source file. Report an error
            QMessageBox::critical(this, tr("Error"), tr("Failed to find the source file for UI canvas %1. Please ensure that the Asset Processor is running and that the source file exists").arg(canvasFilename));
            return false;
        }
    }

    // Check if canvas is already loaded
    AZ::EntityId alreadyLoadedCanvas;
    if (!canvasFilename.isEmpty())
    {
        for (auto mapItem : m_canvasMetadataMap)
        {
            auto canvasMetadata = mapItem.second;
            if (canvasMetadata->m_canvasSourceAssetPathname == sourceAssetPathName)
            {
                alreadyLoadedCanvas = canvasMetadata->m_canvasEntityId;
                break;
            }
        }
    }

    if (alreadyLoadedCanvas.IsValid())
    {
        // Canvas is already loaded
        if (changeActiveCanvasToThis)
        {
            if (CanChangeActiveCanvas())
            {
                SetActiveCanvas(alreadyLoadedCanvas);
            }
        }
        return true;
    }

    AZ::EntityId canvasEntityId;
    UiEditorEntityContext* entityContext = new UiEditorEntityContext(this);

    // Load the canvas
    bool errorsOnLoad = false;
    if (canvasFilename.isEmpty())
    {
        canvasEntityId = AZ::Interface<ILyShine>::Get()->CreateCanvasInEditor(entityContext);
    }
    else
    {
        // Collect errors and warnings during the canvas load
        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        canvasEntityId = AZ::Interface<ILyShine>::Get()->LoadCanvasInEditor(assetIdPathname.c_str(), sourceAssetPathName.c_str(), entityContext);

        // Stop receiving error and warning events
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

        if (canvasEntityId.IsValid())
        {
            AddRecentFile(sourceAssetPathName.c_str());

            CheckForOrphanedChildren(canvasEntityId);

            // Show any errors and warnings that occurred during the canvas load
            ShowTraceMessages(GetCanvasDisplayNameFromAssetPath(sourceAssetPathName));

            errorsOnLoad = !m_errors.empty();
        }
        else
        {
            // There was an error loading the file. Report an error
            QMessageBox::critical(this, tr("Error"), tr("Failed to load UI canvas %1. See log for details").arg(sourceAssetPathName.c_str()));
        }

        // Clear any trace messages from the canvas load
        ClearTraceMessages();
    }

    if (!canvasEntityId.IsValid())
    {
        delete entityContext;
        return false;
    }

    // Add a canvas tab
    AZStd::string canvasDisplayName = GetCanvasDisplayNameFromAssetPath(sourceAssetPathName);

    int newTabIndex = m_canvasTabWidget->addTab(new QWidget(m_canvasTabWidget), canvasDisplayName.c_str()); // this will call OnCurrentCanvasTabChanged if first tab, but nothing will happen because the metadata won't be set yet
    UiCanvasTabMetadata tabMetadata;
    tabMetadata.m_canvasEntityId = canvasEntityId;
    m_canvasTabWidget->tabBar()->setTabData(newTabIndex, QVariant::fromValue(tabMetadata));
    m_canvasTabWidget->setTabToolTip(newTabIndex, sourceAssetPathName.empty() ? canvasDisplayName.c_str() : sourceAssetPathName.c_str());

    UiCanvasMetadata* canvasMetadata = new UiCanvasMetadata;
    canvasMetadata->m_canvasEntityId = canvasEntityId;
    canvasMetadata->m_canvasSourceAssetPathname = sourceAssetPathName;
    canvasMetadata->m_canvasDisplayName = canvasDisplayName;
    canvasMetadata->m_entityContext = entityContext;
    canvasMetadata->m_undoStack = new UndoStack(m_undoGroup);
    canvasMetadata->m_autoLoaded = autoLoad;
    canvasMetadata->m_errorsOnLoad = errorsOnLoad;
    canvasMetadata->m_canvasChangedAndSaved = false;

    // Check if there is an automatically created canvas that should be unloaded.
    // Unload an automatically created canvas if:
    // 1. it's the only loaded canvas
    // 2. changes have not been made to it
    // 3. the newly loaded canvas is not a new canvas
    AZ::EntityId unloadCanvasEntityId;
    if (!canvasMetadata->m_canvasSourceAssetPathname.empty())
    {
        if (m_canvasMetadataMap.size() == 1)
        {
            UiCanvasMetadata *unloadCanvasMetadata = GetActiveCanvasMetadata();
            if (unloadCanvasMetadata && unloadCanvasMetadata->m_autoLoaded)
            {
                // Check if there are changes to this canvas
                if (unloadCanvasMetadata->m_canvasSourceAssetPathname.empty() && !GetChangesHaveBeenMade(*unloadCanvasMetadata))
                {
                    unloadCanvasEntityId = unloadCanvasMetadata->m_canvasEntityId;
                }
            }
        }
    }

    // Add the newly loaded canvas to the map
    m_canvasMetadataMap[canvasEntityId] = canvasMetadata;

    // Make the newly loaded canvas the active canvas
    if (changeActiveCanvasToThis || !m_activeCanvasEntityId.IsValid())
    {
        if (CanChangeActiveCanvas())
        {
            SetActiveCanvas(canvasEntityId);
        }
    }

    // If there was an automatically created empty canvas, unload it
    if (unloadCanvasEntityId.IsValid())
    {
        UnloadCanvas(unloadCanvasEntityId);
    }

    return true;
}

void EditorWindow::UnloadCanvas(AZ::EntityId canvasEntityId)
{
    UiCanvasMetadata* canvasMetadata = GetCanvasMetadata(canvasEntityId);
    if (canvasMetadata)
    {
        // Stop object pick mode so that the hierarchy and viewport states are
        // set back to normal before saving the canvas edit state
        AzToolsFramework::EditorPickModeRequestBus::Broadcast(
            &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);

        // Delete the canvas
        DestroyCanvas(*canvasMetadata);

        // Remove the undo stack from the undo group
        m_undoGroup->removeStack(canvasMetadata->m_undoStack);

        // Remove the canvas metadata from the list of loaded canvases
        m_canvasMetadataMap.erase(canvasMetadata->m_canvasEntityId);
        delete canvasMetadata;

        // Remove the tab associated with this canvas
        // OnCurrentCanvasTabChanged will be called, and the active canvas will be updated
        int tabIndex = GetTabIndexForCanvasEntityId(canvasEntityId);
        m_canvasTabWidget->removeTab(tabIndex);

        // Ensure the active canvas is valid in case removeTab didn't cause it to change or the implementation changed
        if (!GetCanvasMetadata(m_activeCanvasEntityId))
        {
            if (IsCanvasTabMetadataValidForTabIndex(m_canvasTabWidget->currentIndex()))
            {
                SetActiveCanvas(GetCanvasEntityIdForTabIndex(m_canvasTabWidget->currentIndex()));
            }
            else
            {
                SetActiveCanvas(AZ::EntityId());
            }
        }
    }
}

void EditorWindow::NewCanvas()
{
    LoadCanvas("", false);
}

void EditorWindow::OpenCanvas(const QString& canvasFilename)
{
    LoadCanvas(canvasFilename, false);
}

void EditorWindow::OpenCanvases(const QStringList& canvasFilenames)
{
    for (int i = 0; i < canvasFilenames.size(); i++)
    {
        LoadCanvas(canvasFilenames.at(i), false, (i == 0));
    }
}

void EditorWindow::CloseCanvas(AZ::EntityId canvasEntityId)
{
    UiCanvasMetadata* canvasMetadata = GetCanvasMetadata(canvasEntityId);
    if (canvasMetadata)
    {
        if (CanUnloadCanvas(*canvasMetadata))
        {
            UnloadCanvas(canvasMetadata->m_canvasEntityId);
        }
    }
}

void EditorWindow::CloseAllCanvases()
{
    if (!m_activeCanvasEntityId.IsValid())
    {
        return;
    }

    // Check if all canvases can be unloaded
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        if (!CanUnloadCanvas(*canvasMetadata))
        {
            return;
        }
    }

    // Make a list of canvases to unload. Unload the active canvas last so that the
    // active canvas doesn't keep changing when the canvases are unloaded one by one
    AZStd::vector<AZ::EntityId> canvasEntityIds;
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        if (canvasMetadata->m_canvasEntityId != m_activeCanvasEntityId)
        {
            canvasEntityIds.push_back(canvasMetadata->m_canvasEntityId);
        }
    }
    canvasEntityIds.push_back(m_activeCanvasEntityId);

    UnloadCanvases(canvasEntityIds);
}

void EditorWindow::CloseAllOtherCanvases(AZ::EntityId canvasEntityId)
{
    if (m_canvasMetadataMap.size() < 2)
    {
        return;
    }

    // Check if all but the specified canvas can be unloaded
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        if (canvasMetadata->m_canvasEntityId != canvasEntityId)
        {
            if (!CanUnloadCanvas(*canvasMetadata))
            {
                return;
            }
        }
    }

    // Make a list of canvases to unload
    AZStd::vector<AZ::EntityId> canvasEntityIds;
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        if (canvasMetadata->m_canvasEntityId != canvasEntityId)
        {
            canvasEntityIds.push_back(canvasMetadata->m_canvasEntityId);
        }
    }

    UnloadCanvases(canvasEntityIds);

    // Update the menus for file/save/close
    RefreshEditorMenu();
}

bool EditorWindow::CanChangeActiveCanvas()
{
    UiCanvasMetadata *canvasMetadata = GetActiveCanvasMetadata();
    if (canvasMetadata)
    {
        if (canvasMetadata->m_entityContext->HasPendingRequests() || canvasMetadata->m_entityContext->IsInstantiatingSlices())
        {
            return false;
        }
    }

    return true;
}

void EditorWindow::SetActiveCanvas(AZ::EntityId canvasEntityId)
{
    // This function is called explicitly to set the current active canvas (when a new canvas is loaded).
    // This function is also called from the OnCurrentCanvasTabChanged event handler that is triggered by a user action
    // that changes the tab index (closing a tab or clicking on a different tab)

    if (canvasEntityId == m_activeCanvasEntityId)
    {
        return;
    }

    // Don't redraw the viewport until the active tab has visually changed
    m_viewport->SetRedrawEnabled(false);

    // Disable previous active canvas
    if (m_activeCanvasEntityId.IsValid())
    {
        UiCanvasMetadata* canvasMetadata = GetActiveCanvasMetadata();

        // If the active canvas hasn't been unloaded, stop object pick mode so that the hierarchy 
        // and viewport states are set back to normal before saving the canvas edit state
        AzToolsFramework::EditorPickModeRequestBus::Broadcast(
            &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);

        // Disable undo stack
        if (canvasMetadata)
        {
            canvasMetadata->m_undoStack->setActive(false);
        }

        // Save canvas edit state
        SaveActiveCanvasEditState();
    }

    // Update the active canvas Id
    m_activeCanvasEntityId = canvasEntityId;

    // Set the current tab index to that of the active canvas.
    // If this function was called explicitly (when a new canvas is loaded), setCurrentIndex will trigger OnCurrentCanvasTabChanged, and OnCurrentCanvasTabChanged
    // will call us back, but we will early out because the new active canvas will be the same as the current active canvas.
    // If this function was called from the OnCurrentCanvasTabChanged event handler (triggered by a user clicking on a tab or a user closing a tab),
    // the new tab index will be the same as the current tab index so no more events will be triggered by calling setCurrentIndex here
    m_canvasTabWidget->setCurrentIndex(GetTabIndexForCanvasEntityId(m_activeCanvasEntityId));

    // Get the new active canvas's metadata
    UiCanvasMetadata* canvasMetadata = m_activeCanvasEntityId.IsValid() ? GetCanvasMetadata(m_activeCanvasEntityId) : nullptr;

    // Enable new active canvas
    if (canvasMetadata)
    {
        canvasMetadata->m_undoStack->setActive(true);
    }

    // Update the slice manager 
    m_sliceManager->SetEntityContextId(canvasMetadata ? canvasMetadata->m_entityContext->GetContextId() : AzFramework::EntityContextId::CreateNull());

    // Tell the UI animation system that the active canvas has changed
    UiEditorAnimationBus::Broadcast(&UiEditorAnimationBus::Events::ActiveCanvasChanged);

    // Clear the hierarchy pane
    m_hierarchy->ClearItems();

    if (m_activeCanvasEntityId.IsValid())
    {
        // create the hierarchy tree from the loaded canvas
        LyShine::EntityArray childElements;
        UiCanvasBus::EventResult(childElements, m_activeCanvasEntityId, &UiCanvasBus::Events::GetChildElements);
        m_hierarchy->CreateItems(childElements);
    }

    m_hierarchy->clearSelection();
    m_hierarchy->SetUserSelection(nullptr); // trigger a selection change so the properties updates

    m_hierarchy->ActiveCanvasChanged();

    m_viewport->ActiveCanvasChanged();

    RefreshEditorMenu();

    // Restore Canvas edit state
    RestoreActiveCanvasEditState();

    m_properties->ActiveCanvasChanged();

    // Do the rest of the restore after all other events have had a chance to process because
    // The hierarchy and properties scrollbars have not been set up yet
    QTimer::singleShot(0, this, &EditorWindow::RestoreActiveCanvasEditStatePostEvents);
}

void EditorWindow::SaveActiveCanvasEditState()
{
    UiCanvasMetadata* canvasMetadata = GetActiveCanvasMetadata();
    if (canvasMetadata)
    {
        UiCanvasEditState& canvasEditState = canvasMetadata->m_canvasEditState;

        // Save viewport state
        canvasEditState.m_canvasViewportMatrixProps = m_viewport->GetViewportInteraction()->GetCanvasViewportMatrixProps();
        canvasEditState.m_shouldScaleToFitOnViewportResize = m_viewport->GetViewportInteraction()->ShouldScaleToFitOnViewportResize();
        canvasEditState.m_viewportInteractionMode = m_viewport->GetViewportInteraction()->GetMode();
        canvasEditState.m_viewportCoordinateSystem = m_viewport->GetViewportInteraction()->GetCoordinateSystem();

        // Save hierarchy state
        const QTreeWidgetItemRawPtrQList& selection = m_hierarchy->selectedItems();
        canvasEditState.m_selectedElements = SelectionHelpers::GetSelectedElementIds(m_hierarchy, selection, false);
        canvasEditState.m_hierarchyScrollValue = m_hierarchy->verticalScrollBar() ? m_hierarchy->verticalScrollBar()->value() : 0;

        // Save properties state
        canvasEditState.m_propertiesScrollValue = m_properties->GetProperties()->GetScrollValue();

        // Save animation state
        canvasEditState.m_uiAnimationEditState.m_time = 0.0f;
        canvasEditState.m_uiAnimationEditState.m_timelineScale = 1.0f;
        canvasEditState.m_uiAnimationEditState.m_timelineScrollOffset = 0;
        UiEditorAnimationStateBus::BroadcastResult(
            canvasEditState.m_uiAnimationEditState, &UiEditorAnimationStateBus::Events::GetCurrentEditState);

        canvasEditState.m_inited = true;
    }
}

void EditorWindow::RestoreActiveCanvasEditState()
{
    UiCanvasMetadata* canvasMetadata = GetActiveCanvasMetadata();
    if (canvasMetadata)
    {
        const UiCanvasEditState& canvasEditState = canvasMetadata->m_canvasEditState;
        if (canvasEditState.m_inited)
        {
            // Restore viewport state
            m_viewport->GetViewportInteraction()->SetCanvasViewportMatrixProps(canvasEditState.m_canvasViewportMatrixProps);
            if (canvasEditState.m_shouldScaleToFitOnViewportResize)
            {
                m_viewport->GetViewportInteraction()->CenterCanvasInViewport();
            }
            m_viewport->GetViewportInteraction()->SetCoordinateSystem(canvasEditState.m_viewportCoordinateSystem);
            m_viewport->GetViewportInteraction()->SetMode(canvasEditState.m_viewportInteractionMode);

            // Restore hierarchy state
            HierarchyHelpers::SetSelectedItems(m_hierarchy, &canvasEditState.m_selectedElements);

            // Restore animation state
            UiEditorAnimationStateBus::Broadcast(
                &UiEditorAnimationStateBus::Events::RestoreCurrentEditState, canvasEditState.m_uiAnimationEditState);
        }
    }
}

void EditorWindow::RestoreActiveCanvasEditStatePostEvents()
{
    UiCanvasMetadata* canvasMetadata = GetActiveCanvasMetadata();
    if (canvasMetadata)
    {
        const UiCanvasEditState& canvasEditState = canvasMetadata->m_canvasEditState;
        if (canvasEditState.m_inited)
        {
            // Restore hierarchy state
            if (m_hierarchy->verticalScrollBar())
            {
                m_hierarchy->verticalScrollBar()->setValue(canvasEditState.m_hierarchyScrollValue);
            }

            // Restore properties state
            m_properties->GetProperties()->SetScrollValue(canvasEditState.m_propertiesScrollValue);
        }
    }

    m_viewport->SetRedrawEnabled(true);
    m_viewport->setFocus();
}

void EditorWindow::UnloadCanvases(const AZStd::vector<AZ::EntityId>& canvasEntityIds)
{
    for (int i = 0; i < canvasEntityIds.size(); i++)
    {
        UnloadCanvas(canvasEntityIds[i]);
    }
}

AZ::EntityId EditorWindow::GetCanvas()
{
    return m_activeCanvasEntityId;
}

HierarchyWidget* EditorWindow::GetHierarchy()
{
    AZ_Assert(m_hierarchy, "Missing hierarchy widget");
    return m_hierarchy;
}

ViewportWidget* EditorWindow::GetViewport()
{
    AZ_Assert(m_viewport, "Missing viewport widget");
    return m_viewport;
}

PropertiesWidget* EditorWindow::GetProperties()
{
    AZ_Assert(m_properties, "Missing properties wrapper");
    AZ_Assert(m_properties->GetProperties(), "Missing properties widget");
    return m_properties->GetProperties();
}

MainToolbar* EditorWindow::GetMainToolbar()
{
    AZ_Assert(m_mainToolbar, "Missing main toolbar");
    return m_mainToolbar;
}

ModeToolbar* EditorWindow::GetModeToolbar()
{
    AZ_Assert(m_modeToolbar, "Missing mode toolbar");
    return m_modeToolbar;
}

EnterPreviewToolbar* EditorWindow::GetEnterPreviewToolbar()
{
    AZ_Assert(m_enterPreviewToolbar, "Missing enter preview toolbar");
    return m_enterPreviewToolbar;
}

PreviewToolbar* EditorWindow::GetPreviewToolbar()
{
    AZ_Assert(m_previewToolbar, "Missing preview toolbar");
    return m_previewToolbar;
}

NewElementToolbarSection* EditorWindow::GetNewElementToolbarSection()
{
    AZ_Assert(m_mainToolbar, "Missing main toolbar");
    return m_mainToolbar->GetNewElementToolbarSection();
}

CoordinateSystemToolbarSection* EditorWindow::GetCoordinateSystemToolbarSection()
{
    AZ_Assert(m_mainToolbar, "Missing main toolbar");
    return m_mainToolbar->GetCoordinateSystemToolbarSection();
}

CanvasSizeToolbarSection* EditorWindow::GetCanvasSizeToolbarSection()
{
    AZ_Assert(m_mainToolbar, "Missing main toolbar");
    return m_mainToolbar->GetCanvasSizeToolbarSection();
}

const QCursor& EditorWindow::GetEntityPickerCursor()
{
    return m_entityPickerCursor;
}

void EditorWindow::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
    switch (ev)
    {
    case eNotify_OnIdleUpdate:
        m_viewport->Refresh();
        break;
    case eNotify_OnStyleChanged:
    {
        // change skin
        RefreshEditorMenu();
        break;
    }
    case eNotify_OnUpdateViewports:
    {
        // provides a way for the animation editor to force updates of the properties dialog during
        // an animation
        GetProperties()->TriggerRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values);
        break;
    }
    }
}

bool EditorWindow::CanExitNow()
{
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        if (!CanUnloadCanvas(*canvasMetadata))
        {
            return false;
        }
    }

    return true;
}

bool EditorWindow::CanUnloadCanvas(UiCanvasMetadata& canvasMetadata)
{
    if (GetChangesHaveBeenMade(canvasMetadata))
    {
        QString name;
        if (canvasMetadata.m_isSliceEditing)
        {
            // This already has "Slice:" prepended to the slice name
            name = canvasMetadata.m_canvasDisplayName.c_str();
        }
        else
        {
            name = tr("UI canvas \"%1\"").arg(canvasMetadata.m_canvasDisplayName.c_str());
        }

        const auto defaultButton = QMessageBox::Save;
        int result = QMessageBox::question(this,
            tr("Save UI Canvas Changes?"),
            tr("Would you like to save changes to %1 before closing?").arg(name),
            (QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel),
            defaultButton);

        if (result == QMessageBox::Save)
        {
            bool ok = false;
            if (canvasMetadata.m_isSliceEditing)
            {
                ok = SaveSlice(canvasMetadata);
            }
            else
            {
                ok = SaveCanvasToXml(canvasMetadata, false);
            }
            if (!ok)
            {
                return false;
            }
        }
        else if (result == QMessageBox::Discard)
        {
            // Nothing to do
        }
        else // if( result == QMessageBox::Cancel )
        {
            return false;
        }
    }

    return true;
}

bool EditorWindow::GetChangesHaveBeenMade(const UiCanvasMetadata& canvasMetadata)
{
    return !canvasMetadata.m_undoStack->isClean();
}

QUndoGroup* EditorWindow::GetUndoGroup()
{
    return m_undoGroup;
}

UndoStack* EditorWindow::GetActiveStack()
{
    return qobject_cast<UndoStack*>(m_undoGroup->activeStack());
}

AssetTreeEntry* EditorWindow::GetSliceLibraryTree()
{
    if (!m_sliceLibraryTree)
    {
        const AZStd::string pathToSearch("ui/slices/library/");
        const AZ::Data::AssetType sliceAssetType(AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());

        m_sliceLibraryTree = AssetTreeEntry::BuildAssetTree(sliceAssetType, pathToSearch);
    }

    return m_sliceLibraryTree;
}

AZ::EntityId EditorWindow::GetCanvasForCurrentEditorMode()
{
    AZ::EntityId canvasEntityId;
    if (GetEditorMode() == UiEditorMode::Edit)
    {
        canvasEntityId = GetCanvas();
    }
    else
    {
        canvasEntityId = GetPreviewModeCanvas();
    }
    return canvasEntityId;
}

void EditorWindow::ToggleEditorMode()
{
    m_editorMode = (m_editorMode == UiEditorMode::Edit) ? UiEditorMode::Preview : UiEditorMode::Edit;

    emit EditorModeChanged(m_editorMode);

    m_viewport->ClearUntilSafeToRedraw();

    if (m_editorMode == UiEditorMode::Edit)
    {
        // unload the preview mode canvas
        if (m_previewModeCanvasEntityId.IsValid())
        {
            m_previewActionLog->Deactivate();
            m_previewAnimationList->Deactivate();

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationBus::Events::FindEntity, m_previewModeCanvasEntityId);
            if (entity)
            {
                AZ::Interface<ILyShine>::Get()->ReleaseCanvas(m_previewModeCanvasEntityId, false);
            }
            m_previewModeCanvasEntityId.SetInvalid();
        }

        m_canvasTabSectionWidget->show();

        SaveModeSettings(UiEditorMode::Preview, false);
        RestoreModeSettings(UiEditorMode::Edit);
    }
    else
    {
        // Stop object pick mode
        AzToolsFramework::EditorPickModeRequestBus::Broadcast(
            &AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);

        m_canvasTabSectionWidget->hide();

        SaveModeSettings(UiEditorMode::Edit, false);
        RestoreModeSettings(UiEditorMode::Preview);

        GetPreviewToolbar()->UpdatePreviewCanvasScale(m_viewport->GetPreviewCanvasScale());

        // clone the editor canvas to create a temporary preview mode canvas
        if (m_activeCanvasEntityId.IsValid())
        {
            AZ_Assert(!m_previewModeCanvasEntityId.IsValid(), "There is an existing preview mode canvas");

            // Get the canvas size
            AZ::Vector2 canvasSize = GetPreviewCanvasSize();
            if (canvasSize.GetX() == 0.0f && canvasSize.GetY() == 0.0f)
            {
                // special value of (0,0) means use the viewport size
                canvasSize = AZ::Vector2(aznumeric_cast<float>(m_viewport->size().width()), aznumeric_cast<float>(m_viewport->size().height()));
            }

            AZ::Entity* clonedCanvas = nullptr;
            UiCanvasBus::EventResult(clonedCanvas, m_activeCanvasEntityId, &UiCanvasBus::Events::CloneCanvas, canvasSize);

            if (clonedCanvas)
            {
                m_previewModeCanvasEntityId = clonedCanvas->GetId();
            }
            else
            {
                QMessageBox::critical(this, "Preview Mode Error", GetEntityContext()->GetErrorMessage().c_str());

                // A zero-msec timeout will cause the single-shot timer to execute
                // once all events currently in the queue have processed. This allows
                // the current "preview mode toggle" to finish and then immediately 
                // toggle back to edit mode.
                const int queueForImmediateExecution = 0;
                QTimer::singleShot(queueForImmediateExecution, this,
                    [this]()
                    {
                        ToggleEditorMode();
                    });
            }
        }

        m_previewActionLog->Activate(m_previewModeCanvasEntityId);

        m_previewAnimationList->Activate(m_previewModeCanvasEntityId);

        // In Preview mode we want keyboard input to go to to the ViewportWidget so set the
        // it to be focused
        m_viewport->setFocus();
    }

    // Update the menus for this mode
    RefreshEditorMenu();
}

AZ::Vector2 EditorWindow::GetPreviewCanvasSize()
{
    return m_previewModeCanvasSize;
}

void EditorWindow::SetPreviewCanvasSize(AZ::Vector2 previewCanvasSize)
{
    m_previewModeCanvasSize = previewCanvasSize;
}

bool EditorWindow::IsPreviewModeToolbar(const QToolBar* toolBar)
{
    bool result = false;
    if (toolBar == m_previewToolbar)
    {
        result = true;
    }
    return result;
}

bool EditorWindow::IsPreviewModeDockWidget(const QDockWidget* dockWidget)
{
    bool result = false;
    if (dockWidget == m_previewActionLogDockWidget ||
        dockWidget == m_previewAnimationListDockWidget)
    {
        result = true;
    }
    return result;
}

void EditorWindow::RestoreEditorWindowSettings()
{
    // Allow the editor window to draw now that we are ready to restore state.
    // Do this before restoring state, otherwise an undocked widget will not be affected by the call
    setUpdatesEnabled(true);

    RestoreModeSettings(m_editorMode);

    m_viewport->SetRedrawEnabled(true);
}

void EditorWindow::SaveEditorWindowSettings()
{
    // This saves the dock position, size and visibility of all the dock widgets and tool bars
    // for the current mode (it also syncs the settings for the other mode that have already been saved to settings)
    SaveModeSettings(m_editorMode, true);
}

UiSliceManager* EditorWindow::GetSliceManager()
{
    return m_sliceManager;
}

UiEditorEntityContext* EditorWindow::GetEntityContext()
{
    if (GetCanvas().IsValid())
    {
        auto canvasMetadata = GetActiveCanvasMetadata();
        AZ_Assert(canvasMetadata, "Canvas metadata not found");
        return canvasMetadata ? canvasMetadata->m_entityContext : nullptr;
    }

    return nullptr;
}

void EditorWindow::ReplaceEntityContext(UiEditorEntityContext* entityContext)
{
    UiCanvasMetadata* canvasMetadata = GetActiveCanvasMetadata();
    if (canvasMetadata)
    {
        delete canvasMetadata->m_entityContext;
        canvasMetadata->m_entityContext = entityContext;

        m_sliceManager->SetEntityContextId(entityContext->GetContextId());

        m_hierarchy->EntityContextChanged();
        m_viewport->EntityContextChanged();
    }
}

QMenu* EditorWindow::createPopupMenu()
{
    QMenu* menu = new QMenu(this);

    // Add all QDockWidget panes for the current editor mode
    {
        QList<QDockWidget*> list = findChildren<QDockWidget*>();

        for (auto p : list)
        {
            // findChildren is recursive, but we only want dock widgets that are immediate children
            if (p->parent() == this)
            {
                bool isPreviewModeDockWidget = IsPreviewModeDockWidget(p);
                if (m_editorMode == UiEditorMode::Edit && !isPreviewModeDockWidget ||
                    m_editorMode == UiEditorMode::Preview && isPreviewModeDockWidget)
                {
                    menu->addAction(p->toggleViewAction());
                }
            }
        }
    }

    // Add all QToolBar panes for the current editor mode
    {
        QList<QToolBar*> list = findChildren<QToolBar*>();
        for (auto p : list)
        {
            if (p->parent() == this)
            {
                bool isPreviewModeToolbar = IsPreviewModeToolbar(p);
                if (m_editorMode == UiEditorMode::Edit && !isPreviewModeToolbar ||
                    m_editorMode == UiEditorMode::Preview && isPreviewModeToolbar)
                {
                    menu->addAction(p->toggleViewAction());
                }
            }
        }
    }

    return menu;
}

AZ::EntityId EditorWindow::GetCanvasForEntityContext(const AzFramework::EntityContextId& contextId)
{
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        if (canvasMetadata->m_entityContext->GetContextId() == contextId)
        {
            return canvasMetadata->m_canvasEntityId;
        }
    }

    return AZ::EntityId();
}

void EditorWindow::EditSliceInNewTab(AZ::Data::AssetId sliceAssetId)
{
    if (!LoadCanvas("", false))
    {
        return;
    }

    AZStd::string assetIdPathname;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
        assetIdPathname, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, sliceAssetId);

    AZStd::string sourceAssetPathName;
    bool fullPathfound = false;
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fullPathfound,
        &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, assetIdPathname, sourceAssetPathName);
    if (!fullPathfound)
    {
        sourceAssetPathName = assetIdPathname;
    }

    AZStd::string canvasDisplayName = "Slice:";
    canvasDisplayName += GetCanvasDisplayNameFromAssetPath(sourceAssetPathName);

    UiCanvasMetadata* canvasMetadata = GetActiveCanvasMetadata();
    canvasMetadata->m_sliceAssetId = sliceAssetId;
    canvasMetadata->m_canvasSourceAssetPathname = sourceAssetPathName;
    canvasMetadata->m_canvasDisplayName = canvasDisplayName;
    canvasMetadata->m_isSliceEditing = true;

    HandleCanvasDisplayNameChanged(*canvasMetadata);

    // instantiate the slice in the new canvas
    AZ::Vector2 viewportPosition(-1.0f,-1.0f); // indicates no viewport position specified

    AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
    sliceAsset.Create(sliceAssetId, true);

    const AzFramework::EntityContextId& entityContextId = canvasMetadata->m_entityContext->GetContextId();

    AzFramework::SliceInstantiationTicket ticket;
    UiEditorEntityContextRequestBus::EventResult(
        ticket, entityContextId, &UiEditorEntityContextRequestBus::Events::InstantiateEditorSlice, sliceAsset, viewportPosition);

    if (ticket.IsValid())
    {
        // Normally we are only ever waiting for one slice to instantiate for Edit Slice, but there could be an edge case where
        // the Instantiate notification is delayed and the user does Edit Slice again.
        if (!UiEditorEntityContextNotificationBus::Handler::BusIsConnected())
        {
            UiEditorEntityContextNotificationBus::Handler::BusConnect();
        }
    }
}

void EditorWindow::UpdateChangedStatusOnAssetChange(const AzFramework::EntityContextId& contextId, const AZ::Data::Asset<AZ::Data::AssetData>& asset)
{
    AZ::EntityId canvasToUpdate = GetCanvasForEntityContext(contextId);
    UiCanvasMetadata* canvasMetadata = GetCanvasMetadata(canvasToUpdate);
    if (canvasMetadata->m_isSliceEditing && asset.GetType() == AZ::AzTypeInfo<AZ::SliceAsset>::Uuid())
    {
        // we are in slice edit mode and a slice asset has changed. This could be because we just did a save (push to slice) and the asset
        // has been reloaded. Or it could have been pushed to in a different tab.
        // Time to do a check to see if there are any remaining overrides on the slice being edited

        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, canvasMetadata->m_sliceEntityId,
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

        // if false then something is wrong. The user could have done a detach slice for example
        if (!sliceAddress.IsValid())
        {
            return;
        }

        // as a safeguard check that the entity still exists
        AZ::EntityId sliceEntityId = canvasMetadata->m_sliceEntityId;
        AZ::Entity* sliceEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(sliceEntity, &AZ::ComponentApplicationBus::Events::FindEntity, sliceEntityId);
        if (!sliceEntity)
        {
            return;
        }

        // make a list that contains the top-level instanced entity plus all of its descendants
        // If entities have been removed they will not be in this list but we will spot the change because the
        // m_children member of the parent will have changed.
        AzToolsFramework::EntityIdList allEntitiesInLocalInstance;
        allEntitiesInLocalInstance.push_back(sliceEntityId);
        UiElementBus::Event(
            sliceEntityId,
            &UiElementBus::Events::CallOnDescendantElements,
            [&allEntitiesInLocalInstance](const AZ::EntityId id)
            {
                allEntitiesInLocalInstance.push_back(id);
            });

        // test if there are any overrides for the slice instance
        bool hasOverrides = AzToolsFramework::SliceUtilities::DoEntitiesHaveOverrides( allEntitiesInLocalInstance );

        if (!hasOverrides)
        {
            // if there are no overrides then call setClean on the stack
            canvasMetadata->m_undoStack->setClean();
        }
    }
}

void EditorWindow::EntitiesAddedOrRemoved()
{
    // entities have been added or removed to/from the active canvas

    UiCanvasMetadata* canvasMetadata = GetActiveCanvasMetadata();
    if (canvasMetadata->m_isSliceEditing)
    {
        // If we are slice editing then it is possible that the change has removed or recreated the slice entity.
        // The file menu changes depending on whether the slice entity is valid so update it.
        RefreshEditorMenu();
    }
}

void EditorWindow::FontTextureHasChanged()
{
    // A font texture has changed since we last rendered. Force a render graph update for each loaded canvas.
    // Only text components that actually use the affected font will actually regenerate their quads.
    for (auto mapItem : m_canvasMetadataMap)
    {
        auto canvasMetadata = mapItem.second;
        UiCanvasComponentImplementationBus::Event(
            canvasMetadata->m_canvasEntityId, &UiCanvasComponentImplementationBus::Events::MarkRenderGraphDirty);
    }

    if (GetEditorMode() == UiEditorMode::Preview)
    {
        UiCanvasComponentImplementationBus::Event(
            GetPreviewModeCanvas(), &UiCanvasComponentImplementationBus::Events::MarkRenderGraphDirty);
    }
}

void EditorWindow::OnCanvasTabCloseButtonPressed(int index)
{
    UiCanvasMetadata* canvasMetadata = GetCanvasMetadataForTabIndex(index);
    if (canvasMetadata)
    {
        if (CanUnloadCanvas(*canvasMetadata))
        {
            bool isActiveCanvas = (canvasMetadata->m_canvasEntityId == m_activeCanvasEntityId);
            UnloadCanvas(canvasMetadata->m_canvasEntityId);

            if (!isActiveCanvas)
            {
                // Update the menus for file/save/close
                RefreshEditorMenu();
            }
        }
    }
}

void EditorWindow::OnCurrentCanvasTabChanged(int index)
{
    // This is called when the first tab is added, when a tab is removed, or when a user clicks on a tab that's not the current tab

    // Get the canvas associated with this index
    AZ::EntityId canvasEntityId = IsCanvasTabMetadataValidForTabIndex(index) ? GetCanvasEntityIdForTabIndex(index) : AZ::EntityId();

    if (index >= 0 && !canvasEntityId.IsValid())
    {
        // This occurs when the first tab is added. Since the tab metadata is set after the tab is added, we don't handle this here.
        // Instead, SetActiveCanvas is called explicitly when a tab is added
        return;
    }

    if (canvasEntityId.IsValid() && canvasEntityId == m_activeCanvasEntityId)
    {
        // Nothing else to do. This occurs when a tab is clicked, but the active canvas cannot be changed so the current tab is reverted
        // back to the tab of the active canvas
        return;
    }

    if (!CanChangeActiveCanvas())
    {
        // Set the tab back to that of the active canvas
        int activeCanvasIndex = GetTabIndexForCanvasEntityId(m_activeCanvasEntityId);
        m_canvasTabWidget->setCurrentIndex(activeCanvasIndex);

        QMessageBox::information(this,
            tr("Running Slice Operations"),
            tr("The current UI canvas is still running slice operations. Please wait until complete before changing tabs."));

        return;
    }

    SetActiveCanvas(canvasEntityId);
}

void EditorWindow::OnCanvasTabContextMenuRequested(const QPoint &point)
{
    int tabIndex = m_canvasTabWidget->tabBar()->tabAt(point);

    if (tabIndex >= 0)
    {
        AZ::EntityId canvasEntityId = GetCanvasEntityIdForTabIndex(tabIndex);
        UiCanvasMetadata *canvasMetadata = GetCanvasMetadata(canvasEntityId);

        QMenu menu(this);
        if (canvasMetadata && canvasMetadata->m_isSliceEditing)
        {
            menu.addAction(CreateSaveSliceAction(canvasMetadata, true));
        }
        else
        {
            menu.addAction(CreateSaveCanvasAction(canvasEntityId, true));
            menu.addAction(CreateSaveCanvasAsAction(canvasEntityId, true));
        }

        menu.addAction(CreateSaveAllCanvasesAction(true));
        menu.addSeparator();
        menu.addAction(CreateCloseCanvasAction(canvasEntityId, true));
        menu.addAction(CreateCloseAllCanvasesAction(true));
        menu.addAction(CreateCloseAllOtherCanvasesAction(canvasEntityId, true));
        menu.addSeparator();

        QAction* action = new QAction("Copy Full Path", this);
        action->setEnabled(canvasMetadata && !canvasMetadata->m_canvasSourceAssetPathname.empty());
        QObject::connect(action,
            &QAction::triggered,
            this,
            [this, canvasEntityId]([[maybe_unused]] bool checked)
        {
            UiCanvasMetadata *canvasMetadata = GetCanvasMetadata(canvasEntityId);
            AZ_Assert(canvasMetadata, "Canvas metadata not found");
            if (canvasMetadata)
            {
                QApplication::clipboard()->setText(canvasMetadata->m_canvasSourceAssetPathname.c_str());
            }
        });
        menu.addAction(action);

        menu.exec(m_canvasTabWidget->mapToGlobal(point));
    }
    else
    {
        if (m_canvasMetadataMap.size() > 0)
        {
            QMenu menu(this);
            menu.addAction(CreateSaveAllCanvasesAction(true));
            menu.addSeparator();
            menu.addAction(CreateCloseAllCanvasesAction(true));

            menu.exec(m_canvasTabWidget->mapToGlobal(point));
        }
    }
}

void EditorWindow::SaveModeSettings(UiEditorMode mode, bool syncSettings)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);
    settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

    if (mode == UiEditorMode::Edit)
    {
        // save the edit mode state
        settings.setValue(UICANVASEDITOR_SETTINGS_EDIT_MODE_STATE_KEY, saveState(UICANVASEDITOR_SETTINGS_WINDOW_STATE_VERSION));
        settings.setValue(UICANVASEDITOR_SETTINGS_EDIT_MODE_GEOM_KEY, saveGeometry());
    }
    else
    {
        // save the preview mode state
        settings.setValue(UICANVASEDITOR_SETTINGS_PREVIEW_MODE_STATE_KEY, saveState(UICANVASEDITOR_SETTINGS_WINDOW_STATE_VERSION));
        settings.setValue(UICANVASEDITOR_SETTINGS_PREVIEW_MODE_GEOM_KEY, saveGeometry());
    }

    settings.endGroup();    // UI canvas editor

    if (syncSettings)
    {
        settings.sync();
    }
}

void EditorWindow::RestoreModeSettings(UiEditorMode mode)
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME);
    settings.beginGroup(UICANVASEDITOR_NAME_SHORT);

    if (mode == UiEditorMode::Edit)
    {
        // restore the edit mode state
        restoreState(settings.value(UICANVASEDITOR_SETTINGS_EDIT_MODE_STATE_KEY).toByteArray(), UICANVASEDITOR_SETTINGS_WINDOW_STATE_VERSION);
        restoreGeometry(settings.value(UICANVASEDITOR_SETTINGS_EDIT_MODE_GEOM_KEY).toByteArray());
    }
    else
    {
        // restore the preview mode state
        bool stateRestored = restoreState(settings.value(UICANVASEDITOR_SETTINGS_PREVIEW_MODE_STATE_KEY).toByteArray(), UICANVASEDITOR_SETTINGS_WINDOW_STATE_VERSION);
        bool geomRestored = restoreGeometry(settings.value(UICANVASEDITOR_SETTINGS_PREVIEW_MODE_GEOM_KEY).toByteArray());

        // if either of the above failed then manually hide and show widgets,
        // this will happen the first time someone uses preview mode
        if (!stateRestored || !geomRestored)
        {
            m_hierarchyDockWidget->hide();
            m_propertiesDockWidget->hide();
            m_animationDockWidget->hide();
            m_mainToolbar->hide();
            m_modeToolbar->hide();
            m_enterPreviewToolbar->hide();

            m_previewToolbar->show();
            m_previewActionLogDockWidget->show();
            m_previewAnimationListDockWidget->show();
        }
    }

    settings.endGroup();    // UI canvas editor
}

int EditorWindow::GetCanvasMaxHierarchyDepth(const LyShine::EntityArray& rootChildElements)
{
    int depth = 0;

    if (rootChildElements.empty())
    {
        return depth;
    }

    size_t numChildrenCurLevel = rootChildElements.size();
    size_t numChildrenNextLevel = 0;
    std::list<AZ::Entity*> elementList(rootChildElements.begin(), rootChildElements.end());
    while (!elementList.empty())
    {
        auto& entity = elementList.front();

        LyShine::EntityArray childElements;
        UiElementBus::EventResult(childElements, entity->GetId(), &UiElementBus::Events::GetChildElements);
        if (!childElements.empty())
        {
            elementList.insert(elementList.end(), childElements.begin(), childElements.end());
            numChildrenNextLevel += childElements.size();
        }

        elementList.pop_front();
        numChildrenCurLevel--;

        if (numChildrenCurLevel == 0)
        {
            depth++;
            numChildrenCurLevel = numChildrenNextLevel;
            numChildrenNextLevel = 0;
        }
    }

    return depth;
}

void EditorWindow::DeleteSliceLibraryTree()
{
    // this just deletes the tree so that we know it is dirty
    if (m_sliceLibraryTree)
    {
        delete m_sliceLibraryTree;
        m_sliceLibraryTree = nullptr;
    }
}

void EditorWindow::paintEvent(QPaintEvent* paintEvent)
{
    QMainWindow::paintEvent(paintEvent);

    if (m_viewport)
    {
        m_viewport->Refresh();
    }
}

void EditorWindow::closeEvent(QCloseEvent* closeEvent)
{
    if (!CanExitNow())
    {
        // Nothing to do.
        closeEvent->ignore();
        return;
    }

    // Save the current window state
    SaveEditorWindowSettings();

    m_animationWidget->EditorAboutToClose();

    QMainWindow::closeEvent(closeEvent);
}

void EditorWindow::dragEnterEvent(QDragEnterEvent* event)
{
    AssetDropHelpers::AssetList canvasAssets;
    if (AssetDropHelpers::AcceptsMimeType(event->mimeData()))
    {
        AssetDropHelpers::DecodeUiCanvasAssetsFromMimeData(event->mimeData(), canvasAssets);
    }

    if (!canvasAssets.empty())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void EditorWindow::dropEvent(QDropEvent* event)
{
    AssetDropHelpers::AssetList canvasAssets;
    if (AssetDropHelpers::AcceptsMimeType(event->mimeData()))
    {
        AssetDropHelpers::DecodeUiCanvasAssetsFromMimeData(event->mimeData(), canvasAssets);
    }

    QStringList canvasFilenames;
    for (const AZ::Data::AssetId& canvasAssetId : canvasAssets)
    {
        const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry* source = AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry::GetSourceByUuid(canvasAssetId.m_guid);
        if (!source)
        {
            continue;
        }

        AZStd::string fullEntryPath = source->GetFullPath();
        if (!fullEntryPath.empty())
        {
            canvasFilenames.push_back(QString(fullEntryPath.c_str()));
        }
    }

    // If in Preview mode, exit back to Edit mode
    if (m_editorMode == UiEditorMode::Preview)
    {
        ToggleEditorMode();
    }

    OpenCanvases(canvasFilenames);

    activateWindow();
    m_viewport->setFocus();
}

void EditorWindow::SetupCentralWidget()
{
    QWidget* centralWidget = new QWidget(this);

    // Create a vertical layout for the central widget that will lay out a tab section widget and a viewport widget
    SetupTabbedViewportWidget(centralWidget);

    setCentralWidget(centralWidget);
}

void EditorWindow::SetupTabbedViewportWidget(QWidget* parent)
{
    // Create a vertical layout for the central widget that will lay out a tab section widget and a viewport widget
    QVBoxLayout* tabbedViewportLayout = new QVBoxLayout(parent);
    tabbedViewportLayout->setContentsMargins(0, 0, 0, 0);
    tabbedViewportLayout->setSpacing(0);

    // Create a tab section widget that's a child of the central widget
    m_canvasTabSectionWidget = new QWidget(parent);
    m_canvasTabSectionWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    // Add the tab section widget to the layout of the central widget
    tabbedViewportLayout->addWidget(m_canvasTabSectionWidget);

    // Create a horizontal layout for the tab section widget that will lay out a tab bar and an add canvas button
    QHBoxLayout* canvasTabSectionWidgetLayout = new QHBoxLayout(m_canvasTabSectionWidget);
    canvasTabSectionWidgetLayout->setContentsMargins(0, 0, 0, 0);

    // Create a canvas tab bar that's a child of the tab section widget
    m_canvasTabWidget = new AzQtComponents::TabWidget(m_canvasTabSectionWidget);
    m_canvasTabWidget->tabBar()->setMovable(true);
    m_canvasTabWidget->tabBar()->setTabsClosable(true);
    m_canvasTabWidget->tabBar()->setExpanding(false);
    m_canvasTabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);

    // Add the canvas tab bar to the layout of the tab section widget
    canvasTabSectionWidgetLayout->addWidget(m_canvasTabWidget);
    
    QAction* addCanvasAction = new QAction(QIcon(QStringLiteral(":/stylesheet/img/logging/add-filter.svg")), "", this);
    QObject::connect(addCanvasAction, &QAction::triggered, this, [this] { NewCanvas(); });
    m_canvasTabWidget->setActionToolBarVisible();
    m_canvasTabWidget->addAction(addCanvasAction);
    
    connect(m_canvasTabWidget->tabBar(), &QTabBar::tabCloseRequested, this, &EditorWindow::OnCanvasTabCloseButtonPressed);
    connect(m_canvasTabWidget->tabBar(), &QTabBar::currentChanged, this, &EditorWindow::OnCurrentCanvasTabChanged);
    connect(m_canvasTabWidget->tabBar(), &QTabBar::customContextMenuRequested, this, &EditorWindow::OnCanvasTabContextMenuRequested);

    AzQtComponents::TabWidget::applySecondaryStyle(m_canvasTabWidget, false);

    QWidget* viewportWithRulers = m_viewport->CreateViewportWithRulersWidget(this);

    // Add the viewport widget to the layout of the central widget 
    tabbedViewportLayout->addWidget(viewportWithRulers);
}

void EditorWindow::CheckForOrphanedChildren(AZ::EntityId canvasEntityId)
{
    bool result = false;
    UiEditorCanvasBus::EventResult(result, canvasEntityId, &UiEditorCanvasBus::Events::CheckForOrphanedElements);

    if (result)
    {
        // There are orphaned elements. Ask the user whether to recover or remove them.
        int result2 = QMessageBox::warning(this,
            tr("Warning: Orphaned Elements"),
            tr("This UI canvas has orphaned UI elements that are no longer in the element hierarchy.\n\n"
               "They can either be recovered and placed under an element named RecoveredOrphans or they can be deleted.\n\n"
                "Do you wish to recover them?"),
            (QMessageBox::Yes | QMessageBox::No),
            QMessageBox::Yes);

        if (result2 == QMessageBox::Yes)
        {
            UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::RecoverOrphanedElements);
        }
        else
        {
            UiEditorCanvasBus::Event(canvasEntityId, &UiEditorCanvasBus::Events::RemoveOrphanedElements);
        }
    }
}

void EditorWindow::ShowEntitySearchModal()
{
    QDialog* dialog = new QDialog(this);
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(5, 5, 5, 5);
    FindEntityWidget* findEntityWidget = new FindEntityWidget(m_activeCanvasEntityId, dialog);
    mainLayout->addWidget(findEntityWidget);
    dialog->setWindowTitle(QObject::tr("Find Elements"));
    dialog->setMinimumSize(QSize(500, 500));
    dialog->resize(QSize(600, 600));
    dialog->setLayout(mainLayout);

    QWidget::connect(findEntityWidget, &FindEntityWidget::OnFinished, dialog,
        [this, dialog](AZStd::vector<AZ::EntityId> selectedEntities)
    {
        if (!selectedEntities.empty())
        {
            // Clear any selected entities in the hierarchy so that if an entity is already selected, it will still be scrolled to
            m_hierarchy->clearSelection();
            m_hierarchy->setCurrentItem(nullptr);

            // Expand the entities to be selected in the hierarchy
            HierarchyHelpers::ExpandItemsAndAncestors(m_hierarchy, selectedEntities);

            // Select the entities in the hierarchy
            HierarchyHelpers::SetSelectedItems(m_hierarchy, &selectedEntities);
        }

        dialog->accept();
    }
    );

    QWidget::connect(findEntityWidget, &FindEntityWidget::OnCanceled, dialog,
        [dialog]()
    {
        dialog->reject();
    }
    );

    dialog->exec();
    delete dialog;
}

void EditorWindow::AddTraceMessage(const char *message, AZStd::list<QString>& messageList)
{
    messageList.push_back(QString(message));
}

void EditorWindow::ShowTraceMessages(const AZStd::string& canvasName)
{
    // Display the errors and warnings in one dialog window
    if (m_errors.size() == 0 && m_warnings.size() == 0)
    {
        return;
    }

    SandboxEditor::ErrorDialog errorDialog(this);
    QString title = QString().asprintf("Error Log - %s", canvasName.c_str());
    errorDialog.setWindowTitle(title);
    errorDialog.AddMessages(SandboxEditor::ErrorDialog::MessageType::Error, m_errors);
    errorDialog.AddMessages(SandboxEditor::ErrorDialog::MessageType::Warning, m_warnings);
    errorDialog.exec();
}

void EditorWindow::ClearTraceMessages()
{
    m_errors.clear();
    m_warnings.clear();
}

#include <moc_EditorWindow.cpp>
