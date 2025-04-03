/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include <AzQtComponents/Buses/ShortcutDispatch.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>

#include "AlignToolbarSection.h"
#include "ViewportAlign.h"
#include "CanvasHelpers.h"
#include "GuideHelpers.h"
#include <LyShine/Bus/UiEditorCanvasBus.h>
#include <Util/PathUtil.h>

#include <QFileDialog>
#include <QMenuBar>
#include <QUndoGroup>
#include <QUndoView>
#include <QDesktopServices>
#include <QMessageBox>
#include <QDockWidget>

static const bool debugViewUndoStack = false;

QAction* EditorWindow::AddMenuAction(const QString& text, bool enabled, QMenu* menu,  AZStd::function<void (bool)> function)
{
    QAction* action = new QAction(text, this);
    action->setEnabled(enabled);
    QObject::connect(action,
        &QAction::triggered,
        function);
    menu->addAction(action);
    addAction(action);  // Also add the action to the window until the shortcut dispatcher can find the menu action
    m_actionsEnabledWithSelection.push_back(action);
    return action;
}

void EditorWindow::EditorMenu_Open(QString optional_selectedFile)
{
    if (optional_selectedFile.isEmpty())
    {
        QString dir;
        QStringList recentFiles = ReadRecentFiles();

        // If we had recently opened canvases, open the most recent one's directory
        if (recentFiles.size() > 0)
        {
            dir = Path::GetPath(recentFiles.front());
        }
        // Else go to the default canvas directory
        else
        {
            dir = FileHelpers::GetAbsoluteDir(UICANVASEDITOR_CANVAS_DIRECTORY);
        }

        AssetSelectionModel selection;

        StringFilter* stringFilter = new StringFilter();
        const QString& filterString = QString(".") + UICANVASEDITOR_CANVAS_EXTENSION;
        stringFilter->SetName("UI Canvas files (*.uicanvas)");
        stringFilter->SetFilterString(filterString);
        stringFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);
        auto stringFilterPtr = FilterConstType(stringFilter);

        selection.SetDisplayFilter(stringFilterPtr);
        selection.SetSelectionFilter(stringFilterPtr);
        selection.SetMultiselect(true);

        AssetBrowserComponentRequestBus::Broadcast(&AssetBrowserComponentRequests::PickAssets, selection, AzToolsFramework::GetActiveWindow());

        if (!selection.IsValid())
        {
            return;
        }

        QStringList list;
        for (const auto& result : selection.GetResults())
        {
            list.push_back(result->GetFullPath().c_str());
        }

        OpenCanvases(list);
    }
    else
    {
        OpenCanvas(optional_selectedFile);
    }
}

void EditorWindow::AddMenu_File()
{
    QMenu* menu = menuBar()->addMenu("&File");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    // Create a new canvas.
    {
        QAction* action = new QAction("&New Canvas", this);
        action->setShortcut(QKeySequence::New);
        QObject::connect(action,
            &QAction::triggered,
            this,
            &EditorWindow::NewCanvas);
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    // Load a canvas.
    {
        QAction* action = new QAction("&Open Canvas...", this);
        action->setShortcut(QKeySequence::Open);
        QObject::connect(action,
            &QAction::triggered,
            this,
            [ this ]([[maybe_unused]] bool checked)
            {
                EditorMenu_Open("");
            });
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    bool canvasLoaded = GetCanvas().IsValid();

    menu->addSeparator();

    UiCanvasMetadata *canvasMetadata = canvasLoaded ? GetCanvasMetadata(GetCanvas()) : nullptr;

    if (canvasMetadata && canvasMetadata->m_isSliceEditing)
    {
        // Save the slice
        {
            QAction *action = CreateSaveSliceAction(canvasMetadata);
            menu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }
    }
    else
    {
        // Save the canvas
        {
            QAction *action = CreateSaveCanvasAction(GetCanvas());
            menu->addAction(action);
            addAction(action);
        }

        // Save the canvas with new file name
        {
            QAction* action = CreateSaveCanvasAsAction(GetCanvas());
            menu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }
    }

    // Save all the canvases
    {
        QAction* action = CreateSaveAllCanvasesAction();
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    menu->addSeparator();

    // Close the active canvas
    {
        QAction* action = CreateCloseCanvasAction(GetCanvas());
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    // Close all canvases
    {
        QAction* action = CreateCloseAllCanvasesAction();
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    // Close all but the active canvas
    {
        QAction* action = CreateCloseAllOtherCanvasesAction(GetCanvas());
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    menu->addSeparator();

    // Recent Files.
    {
        QStringList recentFiles = ReadRecentFiles();

        // List of recent files.
        {
            QMenu* recentMenu = menu->addMenu("&Recent Files");
            recentMenu->setEnabled(!recentFiles.isEmpty());

            // QStringList -> QMenu.
            for (auto && fileName : recentFiles)
            {
                QAction* action = new QAction(fileName, this);
                QObject::connect(action,
                    &QAction::triggered,
                    this,
                    [ this, fileName ]([[maybe_unused]] bool checked)
                    {
                        EditorMenu_Open(fileName);
                    });
                recentMenu->addAction(action);
                addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
            }
        }

        // Clear Recent Files.
        {
            QAction* action = new QAction("Clear Recent Files", this);
            action->setEnabled(!recentFiles.isEmpty());

            QObject::connect(action,
                &QAction::triggered,
                this,
                [ this ]([[maybe_unused]] bool checked)
                {
                    ClearRecentFile();

                    RefreshEditorMenu();
                });
            menu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }
    }
}

void EditorWindow::AddMenuItems_Edit(QMenu* menu)
{
    // Undo.
    {
        QAction* action = GetUndoGroup()->createUndoAction(this);
        action->setShortcut(QKeySequence::Undo);
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    // Redo.
    {
        QAction* action = GetUndoGroup()->createRedoAction(this);

        // IMPORTANT: We CAN'T just provide QKeySequence::Redo as a
        // shortcut because the menu will show CTRL+Y as the shortcut.
        // To display CTRL+SHIFT+Z by default, we have to provide the
        // list of shortcuts explicitly.
        {
            action->setShortcuts(QList<QKeySequence>{AzQtComponents::RedoKeySequence,
                                                     QKeySequence(Qt::META + Qt::SHIFT + Qt::Key_Z),
                                                     QKeySequence(QKeySequence::Redo)});
        }

        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    bool canvasLoaded = GetCanvas().IsValid();

    menu->addSeparator();

    // Select All.
    {
        QAction* action = new QAction("Select &All", this);
        action->setShortcut(QKeySequence::SelectAll);
        action->setEnabled(canvasLoaded);
        QObject::connect(action,
            &QAction::triggered,
            GetHierarchy(),
            &HierarchyWidget::selectAll);
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    menu->addSeparator();

    bool itemsAreSelected = !GetHierarchy()->selectedItems().isEmpty();
    bool thereIsContentInTheClipboard = ClipboardContainsOurDataType();

    // Cut.
    {
        QAction* action = new QAction("Cu&t", this);
        action->setShortcut(QKeySequence::Cut);
        action->setEnabled(itemsAreSelected);
        QObject::connect(action,
            &QAction::triggered,
            GetHierarchy(),
            &HierarchyWidget::Cut);
        menu->addAction(action);
        addAction(action);  // Also add the action to the window until the shortcut dispatcher can find the menu action

        m_actionsEnabledWithSelection.push_back(action);
    }

    // Copy.
    {
        QAction* action = new QAction("&Copy", this);
        action->setShortcut(QKeySequence::Copy);
        action->setEnabled(itemsAreSelected);
        QObject::connect(action,
            &QAction::triggered,
            GetHierarchy(),
            &HierarchyWidget::Copy);
        menu->addAction(action);
        addAction(action);  // Also add the action to the window until the shortcut dispatcher can find the menu action

        m_actionsEnabledWithSelection.push_back(action);
    }

    // Paste.
    {
        // Paste as silbing.
        {
            QAction* action = new QAction(itemsAreSelected ? "&Paste as sibling" : "&Paste", this);
            action->setShortcut(QKeySequence::Paste);
            action->setEnabled(canvasLoaded && thereIsContentInTheClipboard);
            QObject::connect(action,
                &QAction::triggered,
                GetHierarchy(),
                &HierarchyWidget::PasteAsSibling);
            menu->addAction(action);
            addAction(action);  // Also add the action to the window until the shortcut dispatcher can find the menu action

            m_pasteAsSiblingAction = action;
        }

        // Paste as child.
        {
            QAction* action = new QAction("Paste as c&hild", this);
            {
                action->setShortcuts(QList<QKeySequence>{QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_V),
                                                         QKeySequence(Qt::META + Qt::SHIFT + Qt::Key_V)});
            }
            action->setEnabled(canvasLoaded && thereIsContentInTheClipboard && itemsAreSelected);
            QObject::connect(action,
                &QAction::triggered,
                GetHierarchy(),
                &HierarchyWidget::PasteAsChild);
            menu->addAction(action);
            addAction(action);  // Also add the action to the window until the shortcut dispatcher can find the menu action

            m_pasteAsChildAction = action;
        }
    }

    if (debugViewUndoStack)
    {
        QAction* action = new QAction("[DEBUG] View undo stack", this);
        action->setEnabled(canvasLoaded);
        QObject::connect(action,
            &QAction::triggered,
            this,
            [this]([[maybe_unused]] bool checked)
            {
                static QUndoView* undoView = nullptr;
                if (undoView)
                {
                    undoView->setGroup(GetUndoGroup());
                }
                else
                {
                    undoView = new QUndoView(GetUndoGroup());
                    undoView->setWindowTitle("[DEBUG] Undo stack");
                    undoView->setAttribute(Qt::WA_QuitOnClose, false);
                }
                undoView->show();
            });
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    menu->addSeparator();

    // Find elements
    {
        QAction* action = new QAction("&Find Elements...", this);
        action->setShortcut(QKeySequence::Find);
        action->setEnabled(canvasLoaded);
        QObject::connect(action,
            &QAction::triggered,
            [this]([[maybe_unused]] bool checked)
        {
            ShowEntitySearchModal();
        });
        menu->addAction(action);
        addAction(action);  // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    menu->addSeparator();

    // Delete.
    {
        QAction* action = new QAction("Delete", this);
        action->setShortcut(QKeySequence::Delete);
        action->setEnabled(itemsAreSelected);
        QObject::connect(action,
            &QAction::triggered,
            GetHierarchy(),
            [this]() { GetHierarchy()->DeleteSelectedItems(); });
        menu->addAction(action);
        addAction(action);  // Also add the action to the window until the shortcut dispatcher can find the menu action

        m_actionsEnabledWithSelection.push_back(action);
    }

    // Add Align sub-menu
    {
        QMenu* alignMenu = menu->addMenu("Align");

        auto viewport = canvasLoaded ? GetViewport() : nullptr;
        bool enabled = viewport && itemsAreSelected && ViewportAlign::IsAlignAllowed(this);

        // Add each sub-menu item. Store the QActions so that we can enable/disable them when align is allowed or not.
        {
            m_actionsEnabledWithAlignAllowed.push_back(
                AddMenuAction("Top Edges", enabled, alignMenu,
                    [this]([[maybe_unused]] bool checked) { ViewportAlign::AlignSelectedElements(this, ViewportAlign::AlignType::VerticalTop); } ));
            m_actionsEnabledWithAlignAllowed.push_back(
                AddMenuAction("Vertical Centers", enabled, alignMenu,
                    [this]([[maybe_unused]] bool checked) { ViewportAlign::AlignSelectedElements(this, ViewportAlign::AlignType::VerticalCenter); } ));
            m_actionsEnabledWithAlignAllowed.push_back(
                AddMenuAction("Bottom Edges", enabled, alignMenu,
                    [this]([[maybe_unused]] bool checked) { ViewportAlign::AlignSelectedElements(this, ViewportAlign::AlignType::VerticalBottom); } ));
            m_actionsEnabledWithAlignAllowed.push_back(
                AddMenuAction("Left Edges", enabled, alignMenu,
                    [this]([[maybe_unused]] bool checked) { ViewportAlign::AlignSelectedElements(this, ViewportAlign::AlignType::HorizontalLeft); } ));
            m_actionsEnabledWithAlignAllowed.push_back(
                AddMenuAction("Horizontal Centers", enabled, alignMenu,
                    [this]([[maybe_unused]] bool checked) { ViewportAlign::AlignSelectedElements(this, ViewportAlign::AlignType::HorizontalCenter); } ));
            m_actionsEnabledWithAlignAllowed.push_back(
                AddMenuAction("Right Edges", enabled, alignMenu,
                    [this]([[maybe_unused]] bool checked) { ViewportAlign::AlignSelectedElements(this, ViewportAlign::AlignType::HorizontalRight); } ));
        }
    }
}

void EditorWindow::AddMenu_Edit()
{
    QMenu* menu = menuBar()->addMenu("&Edit");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    AddMenuItems_Edit(menu);
}

void EditorWindow::AddMenu_View()
{
    QMenu* menu = menuBar()->addMenu("&View");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    bool canvasLoaded = GetCanvas().IsValid();

    // Zoom options
    {
        // Zoom in
        {
            QAction* action = new QAction("Zoom &In", this);
            action->setShortcut(QKeySequence::ZoomIn);
            action->setEnabled(canvasLoaded);
            QObject::connect(action,
                &QAction::triggered,
                this,
                [this]([[maybe_unused]] bool checked)
                {
                    GetViewport()->GetViewportInteraction()->IncreaseCanvasToViewportScale();
                });
            menu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }

        // Zoom out
        {
            QAction* action = new QAction("Zoom &Out", this);
            action->setShortcut(QKeySequence::ZoomOut);
            action->setEnabled(canvasLoaded);
            QObject::connect(action,
                &QAction::triggered,
                this,
                [this]([[maybe_unused]] bool checked)
                {
                    GetViewport()->GetViewportInteraction()->DecreaseCanvasToViewportScale();
                });
            menu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }

        // Zoom to fit
        {
            QAction* action = new QAction("&Fit Canvas", this);
            {
                action->setShortcuts(QList<QKeySequence>{QKeySequence(Qt::CTRL + Qt::Key_0),
                                                         QKeySequence(Qt::META + Qt::Key_0)});
            }
            action->setEnabled(canvasLoaded);
            QObject::connect(action,
                &QAction::triggered,
                this,
                [this]([[maybe_unused]] bool checked)
                {
                    GetViewport()->GetViewportInteraction()->CenterCanvasInViewport();
                });
            menu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }

        // Actual size
        {
            QAction* action = new QAction("Actual &Size", this);
            {
                action->setShortcuts(QList<QKeySequence>{QKeySequence(Qt::CTRL + Qt::Key_1),
                                                         QKeySequence(Qt::META + Qt::Key_1)});
            }
            action->setEnabled(canvasLoaded);
            QObject::connect(action,
                &QAction::triggered,
                this,
                [this]([[maybe_unused]] bool checked)
                {
                    // Center the canvas then update scale
                    GetViewport()->GetViewportInteraction()->CenterCanvasInViewport();
                    GetViewport()->GetViewportInteraction()->ResetCanvasToViewportScale();
                });
            menu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }
    }

    menu->addSeparator();

    // Add all Edit mode QDockWidget panes.
    {
        QList<QDockWidget*> list = findChildren<QDockWidget*>();

        for (auto p : list)
        {
            // findChildren is recursive. But we only want dock widgets that are immediate
            // children since the Animation pane has some dock widgets of its own
            if (p->parent() == this && !IsPreviewModeDockWidget(p))
            {
                menu->addAction(p->toggleViewAction());
            }
        }
    }

    // Add all Edit mode QToolBar panes.
    {
        QList<QToolBar*> list = findChildren<QToolBar*>();
        for (auto p : list)
        {
            if (p->parent() == this && !IsPreviewModeToolbar(p))
            {
                menu->addAction(p->toggleViewAction());
            }
        }
    }

    menu->addSeparator();

    // Add menu item to hide/show the rulers
    {
        QAction* action = new QAction("&Rulers", this);
        action->setCheckable(true);
        action->setChecked(GetViewport() ? GetViewport()->AreRulersShown() : false);
        action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
        action->setEnabled(canvasLoaded);
        QObject::connect(action,
            &QAction::triggered,
            [this](bool checked)
            {
                // Set the visibility of the rulers
                GetViewport()->ShowRulers(checked);
            });
        menu->addAction(action);
        addAction(action);
    }

    // Add menu item to hide/show the guides
    {
        QAction* action = new QAction("&Guides", this);
        action->setCheckable(true);
        action->setChecked(GetViewport() ? GetViewport()->AreGuidesShown() : false);
        action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Semicolon));
        action->setEnabled(canvasLoaded);
        QObject::connect(action,
            &QAction::triggered,
            [this](bool checked)
            {
                // Set the visibility of the guides
                GetViewport()->ShowGuides(checked);
            });
        menu->addAction(action);
        addAction(action);
    }

    // Add menu item to lock the guides
    {
        QAction* action = new QAction("Lock Guides", this);
        action->setCheckable(true);
        action->setChecked(GuideHelpers::AreGuidesLocked(GetCanvas()));
        action->setEnabled(canvasLoaded);
        QObject::connect(action,
            &QAction::triggered,
            [this](bool checked)
            {
                // Set whether the guides are locked
                AZStd::string canvasUndoXml = CanvasHelpers::BeginUndoableCanvasChange(GetCanvas());
                GuideHelpers::SetGuidesAreLocked(GetCanvas(), checked);
                CanvasHelpers::EndUndoableCanvasChange(this, "toggle guides locked", canvasUndoXml);
            });
        menu->addAction(action);
        addAction(action);
    }

    // Add menu item to clear the guides
    {
        QAction* action = new QAction("Clear Guides", this);
        action->setEnabled(canvasLoaded);
        QObject::connect(action,
            &QAction::triggered,
            [this]([[maybe_unused]] bool checked)
            {
                // Clear guides
                AZStd::string canvasUndoXml = CanvasHelpers::BeginUndoableCanvasChange(GetCanvas());
                UiEditorCanvasBus::Event(GetCanvas(), &UiEditorCanvasBus::Events::RemoveAllGuides);
                CanvasHelpers::EndUndoableCanvasChange(this, "clear guides", canvasUndoXml);
            });
        menu->addAction(action);
        addAction(action);
    }

    menu->addSeparator();

    // Add sub-menu to control which elements have borders drawn on them
    {
        QMenu* drawElementBordersMenu = menu->addMenu("Draw &Borders on Unselected Elements");

        auto viewport = GetViewport();

        // Add option to draw borders on all unselected elements (subject to "Include" options below)
        {
            QAction* action = new QAction("&Draw Borders", this);
            action->setCheckable(true);
            action->setChecked(canvasLoaded ? viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Unselected) : false);
            action->setEnabled(canvasLoaded);
            QObject::connect(action,
                &QAction::triggered,
                viewport,
                [viewport, this]([[maybe_unused]] bool checked)
                {
                    viewport->ToggleDrawElementBorders(ViewportWidget::DrawElementBorders_Unselected);
                    RefreshEditorMenu();
                });
            drawElementBordersMenu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }

        // Add option to include visual elements.
        {
            QAction* action = new QAction("Include &Visual Elements", this);
            action->setCheckable(true);
            action->setChecked(canvasLoaded ? viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Visual) : false);
            action->setEnabled(canvasLoaded ? viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Unselected) : false);
            QObject::connect(action,
                &QAction::triggered,
                viewport,
                [viewport]([[maybe_unused]] bool checked)
                {
                    viewport->ToggleDrawElementBorders(ViewportWidget::DrawElementBorders_Visual);
                });
            drawElementBordersMenu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }

        // Add option to include parent elements.
        {
            QAction* action = new QAction("Include &Parent Elements", this);
            action->setCheckable(true);
            action->setChecked(canvasLoaded ? viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Parent) : false);
            action->setEnabled(canvasLoaded ? viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Unselected) : false);
            QObject::connect(action,
                &QAction::triggered,
                viewport,
                [viewport]([[maybe_unused]] bool checked)
                {
                    viewport->ToggleDrawElementBorders(ViewportWidget::DrawElementBorders_Parent);
                });
            drawElementBordersMenu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }

        // Add option to include hidden elements.
        {
            QAction* action = new QAction("Include &Hidden Elements", this);
            action->setCheckable(true);
            action->setChecked(viewport ? viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Hidden) : false);
            action->setEnabled(viewport ? viewport->IsDrawingElementBorders(ViewportWidget::DrawElementBorders_Unselected) : false);
            QObject::connect(action,
                &QAction::triggered,
                viewport,
                [viewport]([[maybe_unused]] bool checked)
                {
                    viewport->ToggleDrawElementBorders(ViewportWidget::DrawElementBorders_Hidden);
                });
            drawElementBordersMenu->addAction(action);
            addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
        }
    }

    AddMenu_View_LanguageSetting(menu);

    // Reload all fonts
    {
        QAction* action = new QAction("Reload All Fonts", this);
        {
            action->setShortcuts(QList<QKeySequence>{QKeySequence(Qt::CTRL + Qt::Key_L),
                QKeySequence(Qt::META + Qt::Key_L)});
        }
        action->setEnabled(canvasLoaded);
        QObject::connect(action,
            &QAction::triggered,
            []([[maybe_unused]] bool checked)
        {
            gEnv->pCryFont->ReloadAllFonts();
        });
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }
}

void EditorWindow::AddMenu_View_LanguageSetting(QMenu* viewMenu)
{
    QMenu* setCurrentLanguageMenu = viewMenu->addMenu("Set Current &Language");

    // Group language settings together by action group to only allow one
    // selection/language to be active at a time
    QActionGroup* actionGroup = new QActionGroup(setCurrentLanguageMenu);

    // Iterate through the subdirectories of the localization folder. Each
    // directory corresponds to a different language containing localization
    // translations for that language.
    AZStd::string fullLocPath(AZStd::string(gEnv->pFileIO->GetAlias("@products@")) + "/" + AZStd::string(m_startupLocFolderName.toUtf8().constData()));
    QDir locDir(fullLocPath.c_str());
    locDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    locDir.setSorting(QDir::Name);
    QFileInfoList infoList(locDir.entryInfoList());
    for (auto subDirectory : infoList)
    {
        QString directoryName(subDirectory.fileName().toLower());

        // The loc system expects XML assets stored in a language-specific
        // folder with an "_xml" suffix in the name. Truncate the displayed
        // name so the user just sees the language name (this isn't required
        // though).
        const QString xmlPostFix("_xml");
        if (directoryName.endsWith(xmlPostFix))
        {
            directoryName.chop(xmlPostFix.length());
        }

        QAction* action = setCurrentLanguageMenu->addAction(directoryName);
        action->setCheckable(true);

        // When a language is selected, update the localization folder CVar
        QObject::connect(action,
            &QAction::triggered,
            this,
            [this, directoryName]([[maybe_unused]] bool checked)
        {
            // First try to locate the directory by name, without the "_xml"
            // suffix (in case it actually exists by this name).
            QString fullLocPath(QString(gEnv->pFileIO->GetAlias("@products@")) + "/" + m_startupLocFolderName + "/" + directoryName);
            QDir locDir(fullLocPath);

            // Try the directory with the expected suffix
            if (!locDir.exists())
            {
                locDir.setPath(locDir.path() + "_xml");
            }

            // Once the new CVar value is set, the loc system will auto-parse
            // the folder contents. See CSystem::OnLocalizationFolderCVarChanged.
            ICVar* locFolderCvar = gEnv->pConsole->GetCVar("sys_localization_folder");
            AZ_Assert(locFolderCvar,
                "sys_localization_folder no longer defined! This should be created in CSystem::CreateSystemVars().");

            if (locFolderCvar)
            {
                locFolderCvar->Set(locDir.path().toUtf8().constData());

                // Might as well throw a message if our dependencies change
                AZ_Assert(
                    locFolderCvar->GetOnChangeCallback(),
                    "sys_localization_folder CVar callback missing! "
                    "This used to be set to CSystem::OnLocalizationFolderCVarChanged but is now missing. "
                    "UI Editor language-switching features are no longer working.");
            }

            // Update the language setting; this will allow font families to
            // load language-specific font assets
            ICVar* languageCvar = gEnv->pConsole->GetCVar("g_language");
            AZ_Assert(languageCvar,
                "g_language no longer defined! This should be created in CSystem::CreateSystemVars().");

            if (languageCvar)
            {
                languageCvar->Set(directoryName.toUtf8().constData());

                // Make sure that our callback pipeline is setup properly
                AZ_Assert(
                    languageCvar->GetOnChangeCallback(),
                    "g_language CVar callback missing! "
                    "This used to be set to CSystem::OnLangaugeCVarChanged but is now missing. "
                    "UI Editor language-switching features are no longer working.");
            }
        });

        actionGroup->addAction(action);
    }
}

void EditorWindow::AddMenu_Preview()
{
    QMenu* menu = menuBar()->addMenu("&Preview");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    // Toggle preview.
    {
        QString menuItemName;
        if (m_editorMode == UiEditorMode::Edit)
        {
            menuItemName = "&Preview";
        }
        else
        {
            menuItemName = "End &Preview";
        }

        QAction* action = new QAction(menuItemName, this);
        action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_P));
        action->setEnabled(GetCanvas().IsValid());
        QObject::connect(action,
            &QAction::triggered,
            this,
            &EditorWindow::ToggleEditorMode);
        menu->addAction(action);
        addAction(action);  // Also add the action to the window until the shortcut dispatcher can find the menu action
    }
}

void EditorWindow::AddMenu_PreviewView()
{
    QMenu* menu = menuBar()->addMenu("&View");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    // Add all Preview mode QDockWidget panes.
    {
        QList<QDockWidget*> list = findChildren<QDockWidget*>();

        for (auto p : list)
        {
            // findChildren is recursive. But we only want dock widgets that are immediate
            // children since the Animation pane has some dock widgets of its own
            if (p->parent() == this && IsPreviewModeDockWidget(p))
            {
                menu->addAction(p->toggleViewAction());
            }
        }
    }

    // Add all Preview mode QToolBar panes.
    {
        QList<QToolBar*> list = findChildren<QToolBar*>();

        for (auto p : list)
        {
            if (p->parent() == this && IsPreviewModeToolbar(p))
            {
                menu->addAction(p->toggleViewAction());
            }
        }
    }
}

void EditorWindow::AddMenu_Help()
{
    const char* documentationUrl = "https://o3de.org/docs/user-guide/interactivity/user-interface/";
    const char* tutorialsUrl = "https://o3de.org/docs/learning-guide/tutorials/";
    const char* forumUrl = "https://o3de.org/community/";

    QMenu* menu = menuBar()->addMenu("&Help");
    menu->setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    // Documentation
    {
        QAction* action = new QAction("&Documentation", this);

        QObject::connect(action,
            &QAction::triggered,
            this,
            [documentationUrl]([[maybe_unused]] bool checked)
            {
                QDesktopServices::openUrl(QUrl(documentationUrl));
            });
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    // Tutorials
    {
        QAction* action = new QAction("&Tutorials", this);

        QObject::connect(action,
            &QAction::triggered,
            this,
            [tutorialsUrl]([[maybe_unused]] bool checked)
            {
                QDesktopServices::openUrl(QUrl(tutorialsUrl));
            });
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }

    // Forum
    {
        QAction* action = new QAction("&Forum", this);

        QObject::connect(action,
            &QAction::triggered,
            this,
            [forumUrl]([[maybe_unused]] bool checked)
            {
                QDesktopServices::openUrl(QUrl(forumUrl));
            });
        menu->addAction(action);
        addAction(action); // Also add the action to the window until the shortcut dispatcher can find the menu action
    }
}

void EditorWindow::UpdateActionsEnabledState()
{
    bool itemsAreSelected = !GetHierarchy()->selectedItems().isEmpty();
    bool thereIsContentInTheClipboard = ClipboardContainsOurDataType();

    for (QAction* action : m_actionsEnabledWithSelection)
    {
        action->setEnabled(itemsAreSelected);
    }

    if (m_pasteAsSiblingAction)
    {
        m_pasteAsSiblingAction->setEnabled(thereIsContentInTheClipboard);
    }
    if (m_pasteAsChildAction)
    {
        m_pasteAsChildAction->setEnabled(thereIsContentInTheClipboard && itemsAreSelected);
    }

    bool alignAllowed = ViewportAlign::IsAlignAllowed(this);
    for (QAction* action : m_actionsEnabledWithAlignAllowed)
    {
        action->setEnabled(alignAllowed);
    }
    GetModeToolbar()->GetAlignToolbarSection()->SetIsEnabled(alignAllowed);
}

void EditorWindow::RefreshEditorMenu()
{
    m_actionsEnabledWithSelection.clear();
    m_pasteAsSiblingAction = nullptr;
    m_pasteAsChildAction = nullptr;
    m_actionsEnabledWithAlignAllowed.clear();

    auto actionList = actions();
    for (QAction* action : actionList)
    {
        removeAction(action);
        action->deleteLater();
    }

    menuBar()->clear();

    if (GetEditorMode() == UiEditorMode::Edit)
    {
        AddMenu_File();
        AddMenu_Edit();
        AddMenu_View();
        AddMenu_Preview();
        AddMenu_Help();
    }
    else
    {
        AddMenu_PreviewView();
        AddMenu_Preview();
        AddMenu_Help();
    }

    // Lastly, set up shortcuts that aren't on the menu since all actions were removed above
    SetupShortcuts();
}

void EditorWindow::SetupShortcuts()
{
    // Actions with shortcuts are created instead of direct shortcuts because the shortcut dispatcher only looks for matching actions

    // Cycle coordinate system
    {
        QAction* action = new QAction("Coordinate System Cycle", this);
        action->setShortcut(QKeySequence(UICANVASEDITOR_COORDINATE_SYSTEM_CYCLE_SHORTCUT_KEY_SEQUENCE));
        QObject::connect(action,
            &QAction::triggered,
            [this]()
        {
            SignalCoordinateSystemCycle();
        });
        addAction(action);
    }

    // Toggle Snap to Grid
    {
        QAction* action = new QAction("Snap to Grid Toggle", this);
        action->setShortcut(QKeySequence(UICANVASEDITOR_SNAP_TO_GRID_TOGGLE_SHORTCUT_KEY_SEQUENCE));
        QObject::connect(action,
            &QAction::triggered,
            [this]()
        {
            SignalSnapToGridToggle();
        });
        addAction(action);
    }
}

void DisplayNullMetadataMessage(EditorWindow* editorWindow)
{
    QMessageBox(QMessageBox::Critical,
        "Error",
        QString::fromUtf8("Unable to save: canvas metadata is null. Please try reopening the canvas."),
        QMessageBox::Ok, editorWindow).exec();
}

QAction* EditorWindow::CreateSaveCanvasAction(AZ::EntityId canvasEntityId, bool forContextMenu)
{
    UiCanvasMetadata *canvasMetadata = canvasEntityId.IsValid() ? GetCanvasMetadata(canvasEntityId) : nullptr;

    AZStd::string canvasSourcePathname;
    AZStd::string canvasFilename;
    if (canvasMetadata)
    {
        canvasSourcePathname = canvasMetadata->m_canvasSourceAssetPathname;
        UiCanvasBus::EventResult(canvasFilename, canvasEntityId, &UiCanvasBus::Events::GetPathname);
    }

    QFileInfo fileInfo(canvasSourcePathname.c_str());
    QAction* action = new QAction(QString("&Save " + (fileInfo.fileName().isEmpty() ? "Canvas" : fileInfo.fileName())), this);
    if (!forContextMenu && !canvasFilename.empty())
    {
        action->setShortcut(QKeySequence::Save);
    }
    // If there's no filename,
    // we want the menu to be visible, but disabled.
    action->setEnabled(!canvasFilename.empty());

    QObject::connect(action,
        &QAction::triggered,
        this,
        [this, canvasEntityId]([[maybe_unused]] bool checked)
    {
        UiCanvasMetadata *canvasMetadata = GetCanvasMetadata(canvasEntityId);
        AZ_Assert(canvasMetadata, "Canvas metadata not found");
        if (canvasMetadata)
        {
            bool ok = SaveCanvasToXml(*canvasMetadata, false);
            if (!ok)
            {
                return;
            }

            // Refresh the File menu to update the
            // "Recent Files" and "Save".
            RefreshEditorMenu();
        }
        else
        {
            DisplayNullMetadataMessage(this);
        }
    });

    return action;
}

QAction* EditorWindow::CreateSaveCanvasAsAction(AZ::EntityId canvasEntityId, bool forContextMenu)
{
    UiCanvasMetadata *canvasMetadata = canvasEntityId.IsValid() ? GetCanvasMetadata(canvasEntityId) : nullptr;

    AZStd::string canvasSourcePathname;
    AZStd::string canvasFilename;
    if (canvasMetadata)
    {
        canvasSourcePathname = canvasMetadata->m_canvasSourceAssetPathname;
        UiCanvasBus::EventResult(canvasFilename, canvasEntityId, &UiCanvasBus::Events::GetPathname);
    }

    QAction* action = new QAction("Save Canvas &As...", this);

    if (!forContextMenu && canvasFilename.empty())
    {
        action->setShortcut(QKeySequence::Save);
    }
    action->setEnabled(canvasMetadata);

    QObject::connect(action,
        &QAction::triggered,
        this,
        [this, canvasEntityId]([[maybe_unused]] bool checked)
    {
        UiCanvasMetadata *canvasMetadata = GetCanvasMetadata(canvasEntityId);
        AZ_Assert(canvasMetadata, "Canvas metadata not found");
        if (canvasMetadata)
        {
            bool ok = SaveCanvasToXml(*canvasMetadata, true);
            if (!ok)
            {
                return;
            }

            // Refresh the File menu to update the
            // "Recent Files" and "Save".
            RefreshEditorMenu();
        }
        else
        {
            DisplayNullMetadataMessage(this);
        }
    });

    return action;
}

QAction* EditorWindow::CreateSaveSliceAction(UiCanvasMetadata *canvasMetadata, bool forContextMenu)
{
    // We will never call this function unless canvasMetadata is non null and m_isSliceEditing is true
    AZ_Assert(canvasMetadata && canvasMetadata->m_isSliceEditing, "CreateSaveSliceAction requires valid canvas metadata and to be in slice editing mode");

    // as a safeguard check that the entity still exists
    AZ::EntityId sliceEntityId = canvasMetadata->m_sliceEntityId;
    AZ::Entity* sliceEntity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(sliceEntity, &AZ::ComponentApplicationBus::Events::FindEntity, sliceEntityId);
    if (!sliceEntity)
    {
        // Slice entity not found, disable the menu item but also change it to indicate the error
        QAction* action = new QAction(QString("&Save Slice (slice entity not found)"), this);
        action->setEnabled(false);
        return action;
    }

    // get the slice address
    AZ::SliceComponent::SliceInstanceAddress sliceAddress;
    AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, canvasMetadata->m_sliceEntityId,
        &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

    // if isSliceEntity is false then something is wrong. The user could have done a detach slice for example
    if (!sliceAddress.IsValid() || !sliceAddress.GetReference()->GetSliceAsset())
    {
        // Slice entity is no longer a slice instance, disable the menu item but also change it to indicate the error
        QAction* action = new QAction(QString("&Save Slice (slice entity is no longer an instance)"), this);
        action->setEnabled(false);
        return action;
    }

    AZStd::string canvasDisplayName = canvasMetadata->m_canvasDisplayName;

    QAction* action = new QAction(QString("&Save ") + canvasDisplayName.c_str(), this);
    if (!forContextMenu)
    {
        action->setShortcut(QKeySequence::Save);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    }

    // There should always be a valid path for the slice but if there is not we disable the menu item.
    action->setEnabled(!canvasDisplayName.empty());

    QObject::connect(action,
        &QAction::triggered,
        [this, canvasMetadata]([[maybe_unused]] bool checked)
    {
        SaveSlice(*canvasMetadata);
    });

    return action;
}

QAction* EditorWindow::CreateSaveAllCanvasesAction([[maybe_unused]] bool forContextMenu)
{
    QAction* action = new QAction(QString("Save All Canvases"), this);
    action->setEnabled(m_canvasMetadataMap.size() > 0);
    QObject::connect(action,
        &QAction::triggered,
        this,
        [this]([[maybe_unused]] bool checked)
    {
        bool saved = false;
        for (auto mapItem : m_canvasMetadataMap)
        {
            auto canvasMetadata = mapItem.second;
            if (canvasMetadata->m_isSliceEditing)
            {
                saved |= SaveSlice(*canvasMetadata);
            }
            else
            {
                saved |= SaveCanvasToXml(*canvasMetadata, false);
            }
        }

        if (saved)
        {
            // Refresh the File menu to update the
            // "Recent Files" and "Save".
            RefreshEditorMenu();
        }
    });

    return action;
}

QAction* EditorWindow::CreateCloseCanvasAction(AZ::EntityId canvasEntityId, bool forContextMenu)
{
    QAction* action = new QAction("&Close Canvas", this);
    if (!forContextMenu)
    {
        action->setShortcut(QKeySequence::Close);
    }
    action->setEnabled(canvasEntityId.IsValid());
    QObject::connect(action,
        &QAction::triggered,
        this,
        [this, canvasEntityId]([[maybe_unused]] bool checked)
    {
        CloseCanvas(canvasEntityId);
    });

    return action;
}

QAction* EditorWindow::CreateCloseAllOtherCanvasesAction(AZ::EntityId canvasEntityId, bool forContextMenu)
{
    QAction* action = new QAction(forContextMenu ? "Close All but This Canvas" : "Close All but Active Canvas", this);
    action->setEnabled(m_canvasMetadataMap.size() > 1);
    QObject::connect(action,
        &QAction::triggered,
        this,
        [this, canvasEntityId]([[maybe_unused]] bool checked)
    {
        CloseAllOtherCanvases(canvasEntityId);
    });

    return action;
}

QAction* EditorWindow::CreateCloseAllCanvasesAction([[maybe_unused]] bool forContextMenu)
{
    QAction* action = new QAction("Close All Canvases", this);
    action->setEnabled(m_canvasMetadataMap.size() > 0);
    QObject::connect(action,
        &QAction::triggered,
        this,
        &EditorWindow::CloseAllCanvases);

    return action;
}
