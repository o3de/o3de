/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentTypeInfo.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportInputController.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportScene.h>
#include <AtomToolsFramework/Graph/GraphDocumentRequestBus.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/SettingsDialog/SettingsDialog.h>
#include <AtomToolsFramework/Util/Util.h>
#include <Document/MaterialGraphCompiler.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/sort.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <Editor/MaterialCanvasEditorSystemComponent.h>
#include <Editor/MaterialCanvasPaneWindow.h>
#include <Window/MaterialCanvasViewportContent.h>

// NodePaletteDockWidget.h only forward declares NodePaletteConfig; the definition is in NodePaletteWidget.h. Constructing
// one by value needs the complete type, which is why MaterialCanvasMainWindow.h includes both headers as well.
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

#include <QByteArray>
#include <QCloseEvent>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace MaterialCanvas
{
    namespace
    {
        static constexpr const char* PaneLayoutSettingsKey = "/O3DE/Editor/MaterialCanvasPane/PaneWindowState";

        //! Resolves the shared graph view settings, bringing the tool systems up on first use.
        //!
        //! Free rather than a member because it is called from the constructor's initializer list, before the object exists.
        AtomToolsFramework::GraphViewSettingsPtr GetGraphViewSettingsForPane()
        {
            if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
            {
                return systemComponent->GetGraphViewSettings();
            }

            AZ_Error("MaterialCanvasPaneWindow", false, "Material Canvas pane opened before its system component activated.");
            return {};
        }

        //! THIS MUST STAY A FREE FUNCTION, and so must anything else that names AtomToolsDocumentNotifications.
        //!
        //! MaterialCanvasPaneWindow inherits AtomToolsDocumentNotificationBus::Handler privately, which makes the injected
        //! name AtomToolsDocumentNotifications an inaccessible member of this class. Forming
        //! &AtomToolsDocumentNotifications::OnDocumentOpened inside a member function is therefore ill-formed (MSVC
        //! C2247/C2248) no matter whether it is spelled through ::Handler:: or ::Events:: -- it is the name lookup that is
        //! rejected, not the member. A namespace-scope function is not part of the hierarchy, so the check does not apply.
        //!
        //! This does NOT affect AtomToolsDocumentSystemRequestBus or AtomToolsDocumentRequestBus, which are not bases of
        //! this window and are used from members normally.
        void NotifyDocumentOpened(const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        {
            AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                toolId, &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        }

        AtomToolsFramework::DocumentTypeInfoVector GetRegisteredDocumentTypes(const AZ::Crc32& toolId)
        {
            AtomToolsFramework::DocumentTypeInfoVector documentTypes;
            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
                documentTypes, toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::GetRegisteredDocumentTypes);
            return documentTypes;
        }

        AZStd::string GetDocumentPath(const AZ::Uuid& documentId)
        {
            AZStd::string absolutePath;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                absolutePath, documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetAbsolutePath);
            return absolutePath;
        }

        AZStd::string GetDocumentDisplayName(const AZ::Uuid& documentId)
        {
            const AZStd::string absolutePath = GetDocumentPath(documentId);

            AZStd::string fileName;
            if (!absolutePath.empty() && AZ::StringFunc::Path::GetFullFileName(absolutePath.c_str(), fileName))
            {
                return fileName;
            }
            return "untitled";
        }

        //! Collects dropped or dragged paths that any registered document type can open.
        AZStd::vector<AZStd::string> GetOpenableDroppedPaths(const AZ::Crc32& toolId, const QMimeData* mimeData)
        {
            AZStd::vector<AZStd::string> acceptedPaths;
            const auto documentTypes = GetRegisteredDocumentTypes(toolId);
            for (const AZStd::string& path : AtomToolsFramework::GetPathsFromMimeData(mimeData))
            {
                for (const auto& documentType : documentTypes)
                {
                    if (documentType.IsSupportedExtensionToOpen(path))
                    {
                        acceptedPaths.push_back(path);
                        break;
                    }
                }
            }
            return acceptedPaths;
        }
    } // namespace

    MaterialCanvasPaneWindow::MaterialCanvasPaneWindow(QWidget* parent)
        : Base(parent)
        , m_toolId(MaterialCanvasEditorSystemComponent::ToolId)
        , m_graphViewSettingsPtr(GetGraphViewSettingsForPane())
        // Registers the Graph Canvas style sheet for this tool id. Without it nodes render unstyled.
        , m_styleManager(
              MaterialCanvasEditorSystemComponent::ToolId,
              GetGraphViewSettingsForPane() ? GetGraphViewSettingsForPane()->m_styleManagerPath : AZStd::string())
    {
        setObjectName("MaterialCanvasPaneWindow");

        // Plain Qt docking. The Editor's own FancyDocking instance manages this window from the outside, so this window must
        // not create one of its own -- that is the arrangement Script Canvas uses.
        setDockNestingEnabled(true);
        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        // Enable dropping graph files onto the window.
        setAcceptDrops(true);

        CreateDocumentTabs();
        CreateNodePaletteDock();
        CreateInspectorDock();
        CreateViewportDock();
        CreateMenus();

        QTimer::singleShot(
            0,
            this,
            [this]()
            {
                if (auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get())
                {
                    hotKeyManagerInterface->AssignWidgetToActionContext("o3de.context.editor.materialcanvas", this);
                }
            });

        // Starting widths, used only when there is no saved layout to restore. Without these the node palette can collapse
        // to nothing, because Qt has no size hint worth using before any of these widgets have content.
        if (m_nodePalette && m_inspectorDock)
        {
            resizeDocks({ m_nodePalette, m_inspectorDock }, { 280, 380 }, Qt::Horizontal);
        }
        if (m_inspectorDock && m_viewportDock)
        {
            resizeDocks({ m_inspectorDock, m_viewportDock }, { 500, 400 }, Qt::Vertical);
        }

        RestoreLayout();

        if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
        {
            systemComponent->SetPaneWindow(this);
        }

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
        AtomToolsFramework::GraphDocumentNotificationBus::Handler::BusConnect(m_toolId);

        UpdateMenuState();
    }

    MaterialCanvasPaneWindow::~MaterialCanvasPaneWindow()
    {
        if (auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get())
        {
            hotKeyManagerInterface->RemoveWidgetFromActionContext("o3de.context.editor.materialcanvas", this);
        }

        SaveLayout();

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        AtomToolsFramework::GraphDocumentNotificationBus::Handler::BusDisconnect();

        // Last resort. closeEvent normally closes documents with prompts before this point; by the time the destructor runs
        // there is nobody left to ask, and the document views are children of a tab widget that is about to be destroyed.
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments);

        if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
        {
            systemComponent->SetPaneWindow(nullptr);
        }
    }

    void MaterialCanvasPaneWindow::closeEvent(QCloseEvent* event)
    {
        // Give the user a chance to save before the pane goes away. Cancelling any prompt aborts the close, which is why
        // this cannot simply live in the destructor.
        if (!CloseDocuments(GetOpenDocumentIds()))
        {
            event->ignore();
            return;
        }

        SaveLayout();

        // AtomToolsMainWindow::closeEvent writes this window's dock state to a tool agnostic settings key that the standalone Material
        // Canvas application also reads on startup. The two are not interchangeable: the pane and the application host different sets of
        // dock widgets, so restoring the pane's state into the application leaves it without a realised viewport, and the first tick then
        // dereferences a null ViewGroup inside PostFxLayerComponentController. That is the crash that made the standalone tool unusable
        // until usersettings.materialcanvas.setreg was deleted by hand, and it survived closing the Editor because the bad value was
        // already on disk.
        //
        // The pane keeps its own layout under PaneLayoutSettingsKey, base64 encoded, so nothing here is lost by leaving the shared key
        // exactly as the standalone application left it.
        static constexpr const char* SharedWindowStateKey = "/O3DE/AtomToolsFramework/MainWindow/WindowState";
        const AZStd::string windowStateBeforeClose = AtomToolsFramework::GetSettingsObject(SharedWindowStateKey, AZStd::string());

        Base::closeEvent(event);

        AtomToolsFramework::SetSettingsObject(SharedWindowStateKey, windowStateBeforeClose);
    }

    void MaterialCanvasPaneWindow::SaveCurrentDocument()
    {
        SaveDocument(GetCurrentDocumentId());
    }

    void MaterialCanvasPaneWindow::SaveLayout() const
    {
        // QMainWindow::saveState returns arbitrary binary. Copying it straight into a settings registry string, as this did before,
        // produces a settings file that is not valid UTF-8 and therefore cannot be read back reliably. The same defect is visible in
        // the standalone tool's own AtomToolsFramework/MainWindow/Layouts entry, which is where the invalid bytes in
        // usersettings.materialcanvas.setreg come from. StringFunc::Base64 exists for exactly this, per its own documentation:
        // "allows it to be stored safely in json or xml data".
        const QByteArray windowState = saveState();
        const AZStd::string encodedState = AZ::StringFunc::Base64::Encode(
            reinterpret_cast<const AZ::u8*>(windowState.constData()), aznumeric_cast<size_t>(windowState.size()));

        AtomToolsFramework::SetSettingsObject(PaneLayoutSettingsKey, encodedState);
    }

    void MaterialCanvasPaneWindow::RestoreLayout()
    {
        const AZStd::string encodedState = AtomToolsFramework::GetSettingsObject(PaneLayoutSettingsKey, AZStd::string());
        if (encodedState.empty())
        {
            return;
        }

        AZStd::vector<AZ::u8> windowState;
        if (!AZ::StringFunc::Base64::Decode(windowState, encodedState.c_str(), encodedState.size()))
        {
            // A layout written by a build from before the encoding change will not decode. Ignoring it costs one default layout.
            AZ_Warning("MaterialCanvasPaneWindow", false, "Saved pane layout could not be decoded and has been ignored.");
            return;
        }

        restoreState(QByteArray(reinterpret_cast<const char*>(windowState.data()), aznumeric_cast<int>(windowState.size())));
    }

    void MaterialCanvasPaneWindow::CreateMenus()
    {
        QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

        // New and Open are built from the registered document types, exactly as AtomToolsDocumentMainWindow does, so all
        // three types (Material Graph, Material Graph Node Config, Shader Source Data) get entries and each dialog is
        // filtered to its own extensions.
        const auto documentTypes = GetRegisteredDocumentTypes(m_toolId);

        QMenu* newMenu = documentTypes.size() > 1 ? fileMenu->addMenu(tr("&New")) : fileMenu;
        bool isFirst = true;
        for (const auto& documentType : documentTypes)
        {
            auto action = newMenu->addAction(tr("New %1 Document...").arg(documentType.m_documentTypeName.c_str()));
            if (isFirst)
            {
                action->setShortcut(QKeySequence::New);
            }
            connect(
                action,
                &QAction::triggered,
                this,
                [this, documentType]()
                {
                    // CreateDocumentFromTypeName alone does not seed a new graph from its template, which is why creating
                    // one that way produced nothing usable. The dialog collects a source template and a target path, and
                    // CreateDocumentFromFilePath copies one to the other.
                    AtomToolsFramework::CreateDocumentDialog dialog(
                        documentType, AZStd::string::format("%s/Assets", AZ::Utils::GetProjectPath().c_str()).c_str(), this);
                    dialog.adjustSize();

                    if (dialog.exec() == QDialog::Accepted)
                    {
                        AZ::Uuid documentId = AZ::Uuid::CreateNull();
                        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
                            documentId,
                            m_toolId,
                            &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFilePath,
                            dialog.m_sourcePath.toUtf8().constData(),
                            dialog.m_targetPath.toUtf8().constData());
                        NotifyDocumentOpened(m_toolId, documentId);
                    }
                });
            isFirst = false;
        }

        QMenu* openMenu = documentTypes.size() > 1 ? fileMenu->addMenu(tr("&Open")) : fileMenu;
        isFirst = true;
        for (const auto& documentType : documentTypes)
        {
            if (documentType.m_supportedExtensionsToOpen.empty())
            {
                continue;
            }

            auto action = openMenu->addAction(tr("Open %1 Document...").arg(documentType.m_documentTypeName.c_str()));
            if (isFirst)
            {
                action->setShortcut(QKeySequence::Open);
            }
            connect(
                action,
                &QAction::triggered,
                this,
                [toolId = m_toolId, documentType]()
                {
                    const auto paths = AtomToolsFramework::GetOpenFilePathsFromDialog(
                        {}, documentType.m_supportedExtensionsToOpen, documentType.m_documentTypeName, true);

                    // Queued so the modal dialog is fully torn down before documents start opening, matching the shell.
                    AZ::SystemTickBus::QueueFunction(
                        [toolId, paths]()
                        {
                            for (const auto& path : paths)
                            {
                                AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                                    toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument, path);
                            }
                        });
                });
            isFirst = false;
        }

        m_menuOpenRecent = fileMenu->addMenu(tr("Open Recent"));
        connect(
            m_menuOpenRecent,
            &QMenu::aboutToShow,
            this,
            [this]()
            {
                UpdateRecentFileMenu();
            });

        fileMenu->addSeparator();

        // Apply sits next to Save because the two are halves of one decision. Save writes the graph; Apply publishes the material the
        // graph describes. An edit refreshes only the reduced preview build, so without Apply there is no way to try a change in a level
        // without committing it to the source file first.
        m_actionApply = fileMenu->addAction(tr("A&pply"));
        m_actionApply->setShortcut(QKeySequence("Ctrl+Shift+A"));
        connect(
            m_actionApply,
            &QAction::triggered,
            this,
            [this]()
            {
                AtomToolsFramework::GraphDocumentRequestBus::Event(
                    GetCurrentDocumentId(), &AtomToolsFramework::GraphDocumentRequestBus::Events::QueueApplyGraph);
            });

        m_actionSave = fileMenu->addAction(tr("&Save"));
        connect(m_actionSave, &QAction::triggered, this, [this]() { SaveDocument(GetCurrentDocumentId()); });

        m_actionSaveAs = fileMenu->addAction(tr("Save &As..."));
        m_actionSaveAs->setShortcut(QKeySequence::SaveAs);
        connect(
            m_actionSaveAs,
            &QAction::triggered,
            this,
            [this]()
            {
                const AZ::Uuid documentId = GetCurrentDocumentId();
                if (documentId.IsNull())
                {
                    return;
                }

                // Filters come from the document's own type info, the same as AtomToolsDocumentMainWindow::GetSaveDocumentParams.
                AtomToolsFramework::DocumentTypeInfo documentType;
                AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                    documentType, documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetDocumentTypeInfo);

                const AZStd::string savePath = AtomToolsFramework::GetSaveFilePathFromDialog(
                    GetDocumentPath(documentId), documentType.m_supportedExtensionsToSave, documentType.m_documentTypeName);
                if (savePath.empty())
                {
                    return;
                }

                bool result = false;
                AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
                    result,
                    m_toolId,
                    &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsCopy,
                    documentId,
                    savePath);
                AZ_Warning("MaterialCanvasPaneWindow", result, "Document save failed: %s", savePath.c_str());
            });

        m_actionSaveAll = fileMenu->addAction(tr("Save A&ll"));
        connect(
            m_actionSaveAll,
            &QAction::triggered,
            this,
            [this]()
            {
                for (const auto& documentId : GetOpenDocumentIds())
                {
                    if (!SaveDocument(documentId))
                    {
                        break;
                    }
                }
            });

        fileMenu->addSeparator();

        m_actionClose = fileMenu->addAction(tr("&Close"));
        m_actionClose->setShortcut(QKeySequence::Close);
        connect(m_actionClose, &QAction::triggered, this, [this]() { CloseDocuments({ GetCurrentDocumentId() }); });

        m_actionCloseAll = fileMenu->addAction(tr("Close All"));
        connect(m_actionCloseAll, &QAction::triggered, this, [this]() { CloseDocuments(GetOpenDocumentIds()); });

        fileMenu->addSeparator();

        // Without this, none of the Material Canvas options are reachable from the pane -- including Enable Faster Shader
        // Builds and Use Preview-Only Material Pipeline, which are the two that matter most for compile times.
        auto actionSettings = fileMenu->addAction(tr("Settings..."));
        connect(actionSettings, &QAction::triggered, this, [this]() { OpenSettingsDialog(); });

        // Built from this window's own docks rather than QMainWindow::createPopupMenu. That helper returns a menu whose
        // actions are owned elsewhere, and copying them into another menu that is then rebuilt on every show does not
        // survive reliably -- which is why the previous View menu came up empty.
        QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
        connect(
            viewMenu,
            &QMenu::aboutToShow,
            this,
            [this, viewMenu]()
            {
                viewMenu->clear();

                auto dockWidgets = findChildren<QDockWidget*>();
                AZStd::sort(
                    dockWidgets.begin(),
                    dockWidgets.end(),
                    [](QDockWidget* a, QDockWidget* b)
                    {
                        return a->windowTitle() < b->windowTitle();
                    });

                for (auto dockWidget : dockWidgets)
                {
                    // toggleViewAction is owned by the dock widget itself, so it stays valid across rebuilds of this menu.
                    viewMenu->addAction(dockWidget->toggleViewAction());
                }

                viewMenu->addSeparator();
                viewMenu->addAction(
                    tr("Reset Layout"),
                    this,
                    [this]()
                    {
                        for (auto dockWidget : findChildren<QDockWidget*>())
                        {
                            dockWidget->setFloating(false);
                            dockWidget->setVisible(dockWidget->objectName() != "MaterialCanvasPane_ViewportSettings");
                        }
                        if (m_nodePalette && m_inspectorDock)
                        {
                            resizeDocks({ m_nodePalette, m_inspectorDock }, { 280, 380 }, Qt::Horizontal);
                        }
                        if (m_inspectorDock && m_viewportDock)
                        {
                            resizeDocks({ m_inspectorDock, m_viewportDock }, { 500, 400 }, Qt::Vertical);
                        }
                    });
            });
    }

    void MaterialCanvasPaneWindow::UpdateRecentFileMenu()
    {
        m_menuOpenRecent->clear();

        AZStd::vector<AZStd::string> absolutePaths;
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
            absolutePaths, m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::GetRecentFilePaths);

        for (const AZStd::string& path : absolutePaths)
        {
            if (QFile::exists(path.c_str()))
            {
                m_menuOpenRecent->addAction(
                    tr("&%1: %2").arg(m_menuOpenRecent->actions().size()).arg(path.c_str()),
                    this,
                    [toolId = m_toolId, path]()
                    {
                        // Deferred so the menu is not destroyed underneath itself while the document opens.
                        AZ::SystemTickBus::QueueFunction(
                            [toolId, path]()
                            {
                                AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                                    toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument, path);
                            });
                    });
            }
        }

        m_menuOpenRecent->addAction(
            tr("Clear Recent Files"),
            this,
            [toolId = m_toolId]()
            {
                AZ::SystemTickBus::QueueFunction(
                    [toolId]()
                    {
                        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                            toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::ClearRecentFilePaths);
                    });
            });
    }

    void MaterialCanvasPaneWindow::OpenSettingsDialog()
    {
        AtomToolsFramework::SettingsDialog dialog(this);

        dialog.GetInspector()->AddGroupsBegin();
        PopulateSettingsInspector(dialog.GetInspector());
        dialog.GetInspector()->AddGroupsEnd();

        // Temporarily forcing fixed size to prevent the dialog size from being overridden after being shown
        dialog.setFixedSize(800, 400);
        dialog.show();
        dialog.setMinimumSize(0, 0);
        dialog.setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        dialog.exec();

        // Persist the graph view settings the way MaterialCanvasMainWindow::OnSettingsDialogClosed does.
        if (m_graphViewSettingsPtr)
        {
            AtomToolsFramework::SetSettingsObject("/O3DE/Atom/GraphView/ViewSettings", m_graphViewSettingsPtr);
            GraphCanvas::AssetEditorSettingsNotificationBus::Event(
                m_toolId, &GraphCanvas::AssetEditorSettingsNotifications::OnSettingsChanged);

            if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
            {
                systemComponent->SaveSettings();
            }
        }
    }

    void MaterialCanvasPaneWindow::PopulateSettingsInspector(AtomToolsFramework::InspectorWidget* inspector) const
    {
        // Kept identical to MaterialCanvasMainWindow::PopulateSettingsInspector. If a setting is added there it must be
        // added here too, or it will be reachable from the standalone tool and invisible in the pane.
        m_materialCanvasCompileSettingsGroup = AtomToolsFramework::CreateSettingsPropertyGroup(
            tr("Material Canvas Settings").toUtf8().constData(),
            tr("Material Canvas Settings").toUtf8().constData(),
            { AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/EnableFasterShaderBuilds",
                  tr("Enable Faster Shader Builds").toUtf8().constData(),
                  tr("By default, some platforms perform an exhaustive compilation of shaders for multiple RHI. For example, the default "
                     "Windows shader builder settings automatically compiles shaders for DX12, Vulkan, and the Null renderer.\n\nThis option "
                     "overrides those registry settings and makes compilation and preview times much faster by only compiling shaders for the "
                     "currently active platform and RHI.\n\nThis also disables automatic shader variant generation.\n\nChanging this setting "
                     "requires restarting the Editor and the Asset Processor.\n\nChanging the active RHI with this setting enabled may "
                     "require clearing the cache to regenerate shaders for the new RHI.\n\nThe settings files containing the overrides will be "
                     "placed in the user/Registry folder for the current project.")
                      .toUtf8()
                      .constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/EnablePreviewOnlyMaterialPipeline",
                  tr("Use Preview-Only Material Pipeline").toUtf8().constData(),
                  tr("An abstract material type is expanded into one shader per render pass, for every enabled material pipeline. With "
                     "the default MainPipeline and LowEndPipeline, a Standard lighting model produces 21 shaders, and every one of them is "
                     "rebuilt whenever the graph changes.\n\nThis option replaces both with a single trimmed pipeline that builds only the "
                     "shaders the Material Canvas viewport actually draws with: depth, shadow, forward, and transparent. Four shaders "
                     "instead of 21.\n\nThe cost is reduced fidelity for a few features while it is enabled. Per-pixel depth offset and "
                     "alpha cutout fall back to un-offset depth and shadow silhouettes, tinted transparent materials do not draw, light "
                     "culling gets less precise depth bounds for transparent surfaces, and there are no motion vectors for TAA or motion "
                     "blur.\n\nThe pipeline is declared on each material type Material Canvas generates, so it applies to the graphs "
                     "being edited and to nothing else in the project. It takes effect on the next compile; no restart is needed. Note "
                     "that the generated material type is still the one other systems load, so while this is enabled that material "
                     "carries preview shaders rather than production ones.")
                      .toUtf8()
                      .constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/EnableInMemoryPreviewMaterial",
                  tr("Build Preview Material In Memory").toUtf8().constData(),
                  tr("Builds the preview material inside Material Canvas instead of waiting for the Asset Processor to build it.\n\n"
                  "Two of the four jobs an edit currently triggers, the material type final stage and the material builder, were "
                  "measured at 300 ms and 268 ms of Asset Processor time for 16 ms and roughly 20 ms of actual work. The remainder "
                  "is hashing, dependency fingerprinting, product copies and catalog updates around builders that barely do "
                  "anything.\n\nNothing is reimplemented to skip them: CreateMaterialTypeAsset and MaterialAssetCreator are the same "
                  "calls those builders make, so the preview material is built the same way, just here. The shader is still built by "
                  "the Asset Processor and this depends on it, so whenever the shader is not ready yet the viewport falls back to "
                  "the normal path and waits, exactly as before.\n\nRequires preview output to be enabled. Production materials are "
                  "unaffected: this only changes how the viewport gets its preview.").toUtf8().constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/ProductionMaterialPipelines",
                  tr("Production Material Pipelines").toUtf8().constData(),
                  tr("Comma separated list of material pipelines the production material type is built through. Leave this empty to use "
                  "every pipeline the project registers, which by default is MainPipeline and LowEndPipeline.\n\nThose two produce nearly "
                  "identical shaders: for the same transparent Standard PBR material they measure 13,201 and 13,181 preprocessed lines, "
                  "and 1,287 ms and 1,286 ms of azslc. A project with no low end target therefore pays for a second complete set of "
                  "shaders on every save and never loads them. Setting this to \"MainPipeline\" removes that.\n\nThis affects the "
                  "production output only; the preview set is unaffected. An unrecognised name is reported as a warning and the default "
                  "list is used, so a mistake here costs a log line rather than a material that does not render.").toUtf8().constData(),
                  AZStd::string("")),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/ForceDeleteGeneratedFiles",
                  tr("Delete Files On Compile").toUtf8().constData(),
                  tr("This option forces files previously generated from the current graph to be deleted before creating new ones.")
                      .toUtf8()
                      .constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/ForceClearAssetFingerprints",
                  tr("Clear Asset Fingerprints On Compile").toUtf8().constData(),
                  tr("This option forces the AP to reprocess generated files even if no differences were detected since last generated. This "
                     "guarantees that notifications are sent for assets like materials that may not be changed even if their dependent "
                     "material types or shaders are. Enabling this setting may affect the time it takes for the viewport to reflect shader "
                     "and material changes.")
                      .toUtf8()
                      .constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnOpen",
                  tr("Enable Compile On Open").toUtf8().constData(),
                  tr("If enabled, shaders and materials will automatically be generated whenever a material graph is opened.")
                      .toUtf8()
                      .constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave",
                  tr("Enable Compile On Save").toUtf8().constData(),
                  tr("If enabled, shaders and materials will automatically be generated whenever a material graph is saved.")
                      .toUtf8()
                      .constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit",
                  tr("Enable Compile On Edit").toUtf8().constData(),
                  tr("If enabled, shaders and materials will automatically be generated whenever a material graph is edited.")
                      .toUtf8()
                      .constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphStarted",
                  tr("Clear Viewport Material When Compiling Starts").toUtf8().constData(),
                  tr("Clear the viewport model's material whenever compiling shaders and materials starts.").toUtf8().constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/Atom/MaterialCanvas/Viewport/ClearMaterialOnCompileGraphFailed",
                  tr("Clear Viewport Material When Compiling Fails").toUtf8().constData(),
                  tr("Clear the viewport model's material whenever compiling shaders and materials fails.").toUtf8().constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/GraphCompiler/EnableLogging",
                  tr("Enable Compiler Logging").toUtf8().constData(),
                  tr("Toggle verbose logging for material graph generation.").toUtf8().constData(),
                  false),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/DynamicNode/EnablePropertyEditingOnNodeUI",
                  tr("Enable Property Editing On Nodes").toUtf8().constData(),
                  tr("Toggle settings to display properties and allow them to be added directly on graph nodes.").toUtf8().constData(),
                  true),
              AtomToolsFramework::CreateSettingsPropertyValue(
                  "/O3DE/AtomToolsFramework/GraphCompiler/QueueGraphCompileIntervalMs",
                  tr("Queue Graph Compile Interval Ms").toUtf8().constData(),
                  tr("The delay (in milliseconds) before the graph is recompiled after changes.").toUtf8().constData(),
                  aznumeric_cast<AZ::s64>(500),
                  aznumeric_cast<AZ::s64>(0),
                  aznumeric_cast<AZ::s64>(1000)) });

        inspector->AddGroup(
            m_materialCanvasCompileSettingsGroup->m_name,
            m_materialCanvasCompileSettingsGroup->m_displayName,
            m_materialCanvasCompileSettingsGroup->m_description,
            new AtomToolsFramework::InspectorPropertyGroupWidget(
                m_materialCanvasCompileSettingsGroup.get(),
                m_materialCanvasCompileSettingsGroup.get(),
                azrtti_typeid<AtomToolsFramework::DynamicPropertyGroup>()));

        if (m_graphViewSettingsPtr)
        {
            inspector->AddGroup(
                tr("Graph View Settings").toUtf8().constData(),
                tr("Graph View Settings").toUtf8().constData(),
                tr("Configuration settings for the graph view interaction, animation, and other behavior.").toUtf8().constData(),
                new AtomToolsFramework::InspectorPropertyGroupWidget(
                    m_graphViewSettingsPtr.get(), m_graphViewSettingsPtr.get(), m_graphViewSettingsPtr->RTTI_Type()));
        }

        // The standalone tool would call Base::PopulateSettingsInspector here to append AtomToolsMainWindow's Application
        // Settings group. Those are process-level options (log clearing on start, source control, tick intervals) that
        // belong to a standalone application, not to a pane inside the Editor, so they are deliberately not shown.
    }

    bool MaterialCanvasPaneWindow::SaveDocument(const AZ::Uuid& documentId)
    {
        if (documentId.IsNull())
        {
            return false;
        }

        bool result = false;
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
            result, m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::SaveDocument, documentId);

        AZ_Warning("MaterialCanvasPaneWindow", result, "Document save failed: %s", GetDocumentPath(documentId).c_str());
        return result;
    }

    bool MaterialCanvasPaneWindow::CloseDocumentCheck(const AZ::Uuid& documentId)
    {
        const AZStd::string documentPath = GetDocumentPath(documentId);

        bool isModified = false;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            isModified, documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::IsModified);

        if (isModified)
        {
            const auto selection = QMessageBox::question(
                this,
                tr("Document has unsaved changes"),
                tr("Do you want to save changes to\n%1?").arg(documentPath.c_str()),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if (selection == QMessageBox::Cancel)
            {
                return false;
            }

            if (selection == QMessageBox::Yes && !SaveDocument(documentId))
            {
                QMessageBox::critical(
                    this,
                    tr("Document could not be closed"),
                    tr("Close document failed because document was not saved: \n%1").arg(documentPath.c_str()));
                return false;
            }
        }

        return true;
    }

    bool MaterialCanvasPaneWindow::CloseDocuments(const AZStd::vector<AZ::Uuid>& documentIds)
    {
        for (const auto& documentId : documentIds)
        {
            if (documentId.IsNull())
            {
                continue;
            }

            if (!CloseDocumentCheck(documentId))
            {
                return false;
            }

            bool result = false;
            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
                result, m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);

            if (!result)
            {
                return false;
            }
        }
        return true;
    }

    AZStd::vector<AZ::Uuid> MaterialCanvasPaneWindow::GetOpenDocumentIds() const
    {
        AZStd::vector<AZ::Uuid> documentIds;
        if (m_tabWidget)
        {
            documentIds.reserve(m_tabWidget->count());
            for (int index = 0; index < m_tabWidget->count(); ++index)
            {
                const QVariant tabData = m_tabWidget->tabBar()->tabData(index);
                if (tabData.isValid())
                {
                    documentIds.push_back(tabData.value<AZ::Uuid>());
                }
            }
        }
        return documentIds;
    }

    void MaterialCanvasPaneWindow::dragEnterEvent(QDragEnterEvent* event)
    {
        event->setAccepted(!GetOpenableDroppedPaths(m_toolId, event->mimeData()).empty());
        Base::dragEnterEvent(event);
    }

    void MaterialCanvasPaneWindow::dragMoveEvent(QDragMoveEvent* event)
    {
        // Only accept drops over the document area, so dropping onto a dock does not silently open a graph.
        event->setAccepted(centralWidget() && centralWidget()->geometry().contains(event->position().toPoint()));
        Base::dragMoveEvent(event);
    }

    void MaterialCanvasPaneWindow::dropEvent(QDropEvent* event)
    {
        if (centralWidget() && centralWidget()->geometry().contains(event->position().toPoint()))
        {
            const auto acceptedPaths = GetOpenableDroppedPaths(m_toolId, event->mimeData());
            if (!acceptedPaths.empty())
            {
                AZ::SystemTickBus::QueueFunction(
                    [toolId = m_toolId, acceptedPaths]()
                    {
                        for (const AZStd::string& path : acceptedPaths)
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                                toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument, path);
                        }
                    });
                event->acceptProposedAction();
            }
        }

        Base::dropEvent(event);
    }

    void MaterialCanvasPaneWindow::CreateDocumentTabs()
    {
        m_tabWidget = new QTabWidget(this);
        m_tabWidget->setObjectName("MaterialCanvasPaneTabWidget");
        m_tabWidget->setContentsMargins(0, 0, 0, 0);
        m_tabWidget->setDocumentMode(true);
        m_tabWidget->setMovable(true);
        m_tabWidget->setTabsClosable(true);
        m_tabWidget->setUsesScrollButtons(true);

        connect(
            m_tabWidget,
            &QTabWidget::tabCloseRequested,
            this,
            [this](int index)
            {
                const QVariant tabData = m_tabWidget->tabBar()->tabData(index);
                if (tabData.isValid())
                {
                    // Goes through CloseDocuments so closing a tab prompts to save, rather than discarding silently.
                    CloseDocuments({ tabData.value<AZ::Uuid>() });
                }
            });

        // Keep the inspector following whichever graph is in front.
        connect(
            m_tabWidget,
            &QTabWidget::currentChanged,
            this,
            [this]()
            {
                if (m_documentInspector)
                {
                    m_documentInspector->SetDocumentId(GetCurrentDocumentId());
                }
                UpdateMenuState();
            });

        setCentralWidget(m_tabWidget);
    }

    void MaterialCanvasPaneWindow::CreateNodePaletteDock()
    {
        if (!m_graphViewSettingsPtr)
        {
            return;
        }

        GraphCanvas::NodePaletteConfig nodePaletteConfig;
        nodePaletteConfig.m_rootTreeItem = m_graphViewSettingsPtr->m_createNodeTreeItemsFn(m_toolId);
        nodePaletteConfig.m_editorId = m_toolId;
        nodePaletteConfig.m_mimeType = m_graphViewSettingsPtr->m_nodeMimeType.c_str();
        nodePaletteConfig.m_isInContextMenu = false;
        nodePaletteConfig.m_saveIdentifier = m_graphViewSettingsPtr->m_nodeSaveIdentifier;

        // NodePaletteDockWidget is already a QDockWidget, so it is docked directly rather than going through AddDock.
        m_nodePalette = aznew GraphCanvas::NodePaletteDockWidget(this, tr("Node Palette"), nodePaletteConfig);
        m_nodePalette->setObjectName("MaterialCanvasPane_NodePalette");
        m_nodePalette->setMinimumWidth(200);
        addDockWidget(Qt::LeftDockWidgetArea, m_nodePalette);
        m_nodePalette->setVisible(true);
    }

    void MaterialCanvasPaneWindow::CreateInspectorDock()
    {
        m_documentInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_documentInspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialCanvas/DocumentInspector");
        m_inspectorDock = AddDock(tr("Inspector"), m_documentInspector, Qt::RightDockWidgetArea);
    }

    void MaterialCanvasPaneWindow::CreateViewportDock()
    {
        // Identical construction to MaterialCanvasMainWindow. The viewport, its scene, its content and its input controller
        // are the same classes; only the surrounding window differs.
        m_toolBar = new AtomToolsFramework::EntityPreviewViewportToolBar(m_toolId, this);
        m_materialViewport = new AtomToolsFramework::EntityPreviewViewportWidget(m_toolId, this);

        auto entityContext = AZStd::make_shared<AzFramework::EntityContext>();
        entityContext->InitContext();

        auto viewportScene = AZStd::make_shared<AtomToolsFramework::EntityPreviewViewportScene>(
            m_toolId, m_materialViewport, entityContext, "MaterialCanvasPaneViewportWidget", "passes/mainrenderpipeline.azasset");

        auto viewportContent = AZStd::make_shared<MaterialCanvasViewportContent>(m_toolId, m_materialViewport, entityContext);

        auto viewportController =
            AZStd::make_shared<AtomToolsFramework::EntityPreviewViewportInputController>(m_toolId, m_materialViewport, viewportContent);

        m_materialViewport->Init(entityContext, viewportScene, viewportContent, viewportController);

        auto viewportAndToolBar = new QWidget(this);
        auto viewportLayout = new QVBoxLayout(viewportAndToolBar);
        viewportLayout->setContentsMargins(0, 0, 0, 0);
        viewportLayout->setSpacing(0);
        viewportLayout->addWidget(m_toolBar);
        viewportLayout->addWidget(m_materialViewport);
        viewportAndToolBar->setLayout(viewportLayout);

        m_viewportDock = AddDock(tr("Viewport"), viewportAndToolBar, Qt::RightDockWidgetArea);

        m_viewportSettingsInspector = new AtomToolsFramework::EntityPreviewViewportSettingsInspector(m_toolId, this);
        AddDock(tr("Viewport Settings"), m_viewportSettingsInspector, Qt::LeftDockWidgetArea, false);
    }

    QDockWidget* MaterialCanvasPaneWindow::AddDock(const QString& name, QWidget* widget, Qt::DockWidgetArea area, bool visible)
    {
        auto dockWidget = new QDockWidget(name, this);

        // The object name is what QMainWindow::saveState keys dock positions on, so it must be stable across sessions or the
        // saved layout will not restore.
        dockWidget->setObjectName(QString("MaterialCanvasPane_%1").arg(QString(name).remove(' ')));
        dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        widget->setParent(dockWidget);
        dockWidget->setWidget(widget);
        addDockWidget(area, dockWidget);
        dockWidget->setVisible(visible);
        return dockWidget;
    }

    bool MaterialCanvasPaneWindow::AddDocumentView(const AZ::Uuid& documentId, QWidget* viewWidget)
    {
        if (!viewWidget)
        {
            return false;
        }

        if (const int existingIndex = GetTabIndexForDocument(documentId); existingIndex >= 0)
        {
            m_tabWidget->setCurrentIndex(existingIndex);
            return true;
        }

        const int index = m_tabWidget->addTab(viewWidget, GetDocumentDisplayName(documentId).c_str());
        m_tabWidget->tabBar()->setTabData(index, QVariant::fromValue(documentId));
        m_tabWidget->setCurrentIndex(index);
        return true;
    }

    AZ::Uuid MaterialCanvasPaneWindow::GetCurrentDocumentId() const
    {
        if (m_tabWidget)
        {
            const QVariant tabData = m_tabWidget->tabBar()->tabData(m_tabWidget->currentIndex());
            if (tabData.isValid())
            {
                return tabData.value<AZ::Uuid>();
            }
        }
        return AZ::Uuid::CreateNull();
    }

    int MaterialCanvasPaneWindow::GetTabIndexForDocument(const AZ::Uuid& documentId) const
    {
        for (int index = 0; index < m_tabWidget->count(); ++index)
        {
            const QVariant tabData = m_tabWidget->tabBar()->tabData(index);
            if (tabData.isValid() && tabData.value<AZ::Uuid>() == documentId)
            {
                return index;
            }
        }
        return -1;
    }

    void MaterialCanvasPaneWindow::UpdateDocumentTab(const AZ::Uuid& documentId)
    {
        if (const int index = GetTabIndexForDocument(documentId); index >= 0)
        {
            bool isModified = false;
            AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
                isModified, documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::IsModified);

            const AZStd::string name = GetDocumentDisplayName(documentId);
            m_tabWidget->setTabText(index, isModified ? QString("* %1").arg(name.c_str()) : QString(name.c_str()));
            m_tabWidget->setTabToolTip(index, GetDocumentPath(documentId).c_str());
        }
    }

    void MaterialCanvasPaneWindow::UpdateMenuState()
    {
        const bool isOpen = !GetCurrentDocumentId().IsNull();
        const bool hasTabs = m_tabWidget && m_tabWidget->count() > 0;

        for (auto action : { m_actionSave, m_actionSaveAs, m_actionClose })
        {
            if (action)
            {
                action->setEnabled(isOpen);
            }
        }

        for (auto action : { m_actionSaveAll, m_actionCloseAll })
        {
            if (action)
            {
                action->setEnabled(hasTabs);
            }
        }

        if (m_actionApply)
        {
            bool applyNeeded = false;
            AtomToolsFramework::GraphDocumentRequestBus::EventResult(
                applyNeeded, GetCurrentDocumentId(), &AtomToolsFramework::GraphDocumentRequestBus::Events::IsApplyGraphNeeded);

            // Hidden rather than disabled when preview output is off. There is no second output to publish then, so every compile has
            // already produced the real material.
            m_actionApply->setVisible(MaterialGraphCompiler::IsPreviewOutputEnabled());
            m_actionApply->setEnabled(isOpen && applyNeeded);

            // Greying out is the indicator: enabled means the material outside Material Canvas is behind the graph. The tooltip says
            // which state this is, because a disabled item on its own reads as broken rather than as done.
            m_actionApply->setToolTip(
                applyNeeded ? tr("Rebuild the material for use outside Material Canvas. It is currently behind this graph.")
                            : tr("The material outside Material Canvas is up to date with this graph."));
        }
    }

    void MaterialCanvasPaneWindow::OnCompileGraphCompleted(const AZ::Uuid& documentId)
    {
        if (documentId == GetCurrentDocumentId())
        {
            UpdateMenuState();
        }
    }

    void MaterialCanvasPaneWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        m_documentInspector->SetDocumentId(documentId);
        UpdateDocumentTab(documentId);
        UpdateMenuState();
    }

    void MaterialCanvasPaneWindow::OnDocumentClosed(const AZ::Uuid& documentId)
    {
        if (const int index = GetTabIndexForDocument(documentId); index >= 0)
        {
            // Removing the tab detaches the view widget; deleteLater keeps it alive until the current event has finished
            // unwinding, since this can be reached from the view's own signal handlers.
            QWidget* viewWidget = m_tabWidget->widget(index);
            m_tabWidget->removeTab(index);
            if (viewWidget)
            {
                viewWidget->deleteLater();
            }
        }

        if (m_tabWidget->count() == 0)
        {
            m_documentInspector->SetDocumentId(AZ::Uuid::CreateNull());
        }

        UpdateMenuState();
    }

    void MaterialCanvasPaneWindow::OnDocumentSaved(const AZ::Uuid& documentId)
    {
        // Without this the modified marker never clears after a save.
        UpdateDocumentTab(documentId);
    }

    void MaterialCanvasPaneWindow::OnDocumentModified(const AZ::Uuid& documentId)
    {
        UpdateDocumentTab(documentId);
    }

    void MaterialCanvasPaneWindow::OnDocumentUndoStateChanged(const AZ::Uuid& documentId)
    {
        UpdateDocumentTab(documentId);
    }

    void MaterialCanvasPaneWindow::OnDocumentCleared(const AZ::Uuid& documentId)
    {
        UpdateDocumentTab(documentId);
    }

    void MaterialCanvasPaneWindow::OnDocumentError(const AZ::Uuid& documentId)
    {
        UpdateDocumentTab(documentId);
    }
} // namespace MaterialCanvas
