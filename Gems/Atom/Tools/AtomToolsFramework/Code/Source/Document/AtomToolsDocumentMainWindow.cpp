/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentMainWindow.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AtomToolsFramework/SettingsDialog/SettingsDialog.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QInputDialog>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QTimer>
#include <QWindow>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    AtomToolsDocumentMainWindow::AtomToolsDocumentMainWindow(const AZ::Crc32& toolId, const QString& objectName, QWidget* parent)
        : Base(toolId, objectName, parent)
    {
        AddDocumentTabBar();

        m_assetBrowser->SetOpenHandler([this](const AZStd::string& absolutePath) {
            DocumentTypeInfoVector documentTypes;
            AtomToolsDocumentSystemRequestBus::EventResult(
                documentTypes, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::GetRegisteredDocumentTypes);
            for (const auto& documentType : documentTypes)
            {
                if (documentType.IsSupportedExtensionToOpen(absolutePath))
                {
                    AtomToolsDocumentSystemRequestBus::Event(
                        m_toolId, &AtomToolsDocumentSystemRequestBus::Events::OpenDocument, absolutePath);
                    return;
                }
            }

            QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath.c_str()));
        });

        setAcceptDrops(true);

        AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    AtomToolsDocumentMainWindow::~AtomToolsDocumentMainWindow()
    {
        AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    void AtomToolsDocumentMainWindow::CreateMenus(QMenuBar* menuBar)
    {
        Base::CreateMenus(menuBar);

        QAction* insertPostion = !m_menuFile->actions().empty() ? m_menuFile->actions().front() : nullptr;

        // Generating the main menu manually because it's easier and we will have some dynamic or data driven entries
        m_actionNew = CreateActionAtPosition(m_menuFile, insertPostion, "&New...", [this]() {
            AZStd::string openPath;
            AZStd::string savePath;
            if (GetCreateDocumentParams(openPath, savePath))
            {
                AtomToolsDocumentSystemRequestBus::Event(
                    m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFilePath, openPath, savePath);
            }
        }, QKeySequence::New);

        m_actionOpen = CreateActionAtPosition(m_menuFile, insertPostion, "&Open...", [this]() {
            for (const auto& path : GetOpenDocumentParams())
            {
                AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::OpenDocument, path);
            }
        }, QKeySequence::Open);

        m_menuOpenRecent = new QMenu("Open Recent", this);
        connect(m_menuOpenRecent, &QMenu::aboutToShow, this, [this]() {
            UpdateRecentFileMenu();
        });
        m_menuFile->insertMenu(insertPostion, m_menuOpenRecent);
        m_menuFile->insertSeparator(insertPostion);

        m_actionSave = CreateActionAtPosition(m_menuFile, insertPostion, "&Save", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            bool result = false;
            AtomToolsDocumentSystemRequestBus::EventResult(
                result, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::SaveDocument, documentId);
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Save);

        m_actionSaveAsCopy = CreateActionAtPosition(m_menuFile, insertPostion, "Save &As...", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            AtomToolsDocumentSystemRequestBus::EventResult(
                result, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsCopy, documentId,
                GetSaveDocumentParams(documentPath.toUtf8().constData()));
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(documentPath));
            }
        }, QKeySequence::SaveAs);

        m_actionSaveAsChild = CreateActionAtPosition(m_menuFile, insertPostion, "Save As &Child...", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            AtomToolsDocumentSystemRequestBus::EventResult(
                result, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsChild, documentId,
                GetSaveDocumentParams(documentPath.toUtf8().constData()));
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(documentPath));
            }
        });

        m_actionSaveAll = CreateActionAtPosition(m_menuFile, insertPostion, "Save A&ll", [this]() {
            bool result = false;
            AtomToolsDocumentSystemRequestBus::EventResult(
                result, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::SaveAllDocuments);
            if (!result)
            {
                SetStatusError(tr("Document save all failed"));
            }
        });
        m_menuFile->insertSeparator(insertPostion);

        m_actionClose = CreateActionAtPosition(m_menuFile, insertPostion, "&Close", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
        }, QKeySequence::Close);

        m_actionCloseAll = CreateActionAtPosition(m_menuFile, insertPostion, "Close All", [this]() {
            AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments);
        });

        m_actionCloseOthers = CreateActionAtPosition(m_menuFile, insertPostion, "Close Others", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            AtomToolsDocumentSystemRequestBus::Event(
                m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
        });
        m_menuFile->insertSeparator(insertPostion);

        insertPostion = !m_menuEdit->actions().empty() ? m_menuEdit->actions().front() : nullptr;

        m_actionUndo = CreateActionAtPosition(m_menuEdit, insertPostion, "&Undo", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            bool result = false;
            AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::Undo);
            if (!result)
            {
                SetStatusError(tr("Document undo failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Undo);

        m_actionRedo = CreateActionAtPosition(m_menuEdit, insertPostion, "&Redo", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            bool result = false;
            AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::Redo);
            if (!result)
            {
                SetStatusError(tr("Document redo failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Redo);
        m_menuEdit->insertSeparator(insertPostion);

        insertPostion = !m_menuView->actions().empty() ? m_menuView->actions().front() : nullptr;

        m_actionPreviousTab = CreateActionAtPosition(m_menuView, insertPostion, "&Previous Tab", [this]() {
            SelectPrevDocumentTab();
        }, Qt::CTRL | Qt::SHIFT | Qt::Key_Tab); //QKeySequence::PreviousChild is mapped incorrectly in Qt

        m_actionNextTab = CreateActionAtPosition(m_menuView, insertPostion, "&Next Tab", [this]() {
            SelectNextDocumentTab();
        }, Qt::CTRL | Qt::Key_Tab); //QKeySequence::NextChild works as expected but mirroring Previous
        m_menuView->insertSeparator(insertPostion);
    }

    void AtomToolsDocumentMainWindow::UpdateMenus(QMenuBar* menuBar)
    {
        Base::UpdateMenus(menuBar);

        const AZ::Uuid documentId = GetCurrentDocumentId();

        bool isOpen = false;
        AtomToolsDocumentRequestBus::EventResult(isOpen, documentId, &AtomToolsDocumentRequestBus::Events::IsOpen);
        bool canSave = false;
        AtomToolsDocumentRequestBus::EventResult(canSave, documentId, &AtomToolsDocumentRequestBus::Events::CanSave);
        bool canUndo = false;
        AtomToolsDocumentRequestBus::EventResult(canUndo, documentId, &AtomToolsDocumentRequestBus::Events::CanUndo);
        bool canRedo = false;
        AtomToolsDocumentRequestBus::EventResult(canRedo, documentId, &AtomToolsDocumentRequestBus::Events::CanRedo);

        const bool hasTabs = m_tabWidget->count() > 0;

        // Update menu options
        m_actionNew->setEnabled(true);
        m_actionOpen->setEnabled(true);
        m_actionClose->setEnabled(hasTabs);
        m_actionCloseAll->setEnabled(hasTabs);
        m_actionCloseOthers->setEnabled(hasTabs);

        m_actionSave->setEnabled(canSave);
        m_actionSaveAsCopy->setEnabled(canSave);
        m_actionSaveAsChild->setEnabled(isOpen);
        m_actionSaveAll->setEnabled(hasTabs);

        m_actionUndo->setEnabled(canUndo);
        m_actionRedo->setEnabled(canRedo);

        m_actionPreviousTab->setEnabled(m_tabWidget->count() > 1);
        m_actionNextTab->setEnabled(m_tabWidget->count() > 1);
    }

    AZStd::vector<AZStd::shared_ptr<DynamicPropertyGroup>> AtomToolsDocumentMainWindow::GetSettingsDialogGroups() const
    {
        AZStd::vector<AZStd::shared_ptr<DynamicPropertyGroup>> groups = Base::GetSettingsDialogGroups();
        groups.push_back(CreateSettingsGroup(
            "Document System Settings",
            "Document System Settings",
            {
                CreatePropertyFromSetting(
                    "/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/DisplayWarningMessageDialogs",
                    "Display Warning Message Dialogs",
                    "Display message boxes for warnings opening documents",
                    true),
                CreatePropertyFromSetting(
                    "/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/DisplayErrorMessageDialogs",
                    "Display Error Message Dialogs",
                    "Display message boxes for errors opening documents",
                    true),
                CreatePropertyFromSetting(
                    "/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/EnableAutomaticReload",
                    "Enable Automatic Reload",
                    "Automatically reload documents after external modifications",
                    true),
                CreatePropertyFromSetting(
                    "/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/EnableAutomaticReloadPrompts",
                    "Enable Automatic Reload Prompts",
                    "Confirm before automatically reloading modified documents",
                    true),
                CreatePropertyFromSetting(
                    "/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/AutoSaveEnabled",
                    "Enable Auto Save",
                    "Automatically save documents after they are modified",
                    false),
                CreatePropertyFromSetting(
                    "/O3DE/AtomToolsFramework/AtomToolsDocumentSystem/AutoSaveInterval",
                    "Auto Save Interval",
                    "How often (in milliseconds) auto save occurs",
                    aznumeric_cast<AZ::s64>(250)),
            }));
        return groups;
    }

    void AtomToolsDocumentMainWindow::AddDocumentTabBar()
    {
        m_tabWidget = new AzQtComponents::TabWidget(centralWidget());
        m_tabWidget->setObjectName("TabWidget");
        m_tabWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_tabWidget->setContentsMargins(0, 0, 0, 0);

        // The tab bar should only be visible if it has active documents
        m_tabWidget->setVisible(false);
        m_tabWidget->setTabBarAutoHide(false);
        m_tabWidget->setMovable(true);
        m_tabWidget->setTabsClosable(true);
        m_tabWidget->setUsesScrollButtons(true);

        // This signal will be triggered whenever a tab is added, removed, selected, clicked, dragged
        // When the last tab is removed tabIndex will be -1 and the document ID will be null
        // This should automatically clear the active document
        connect(m_tabWidget, &QTabWidget::currentChanged, this, [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            AtomToolsDocumentNotificationBus::Event(m_toolId,&AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        });

        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
        });

        // Add context menu for right-clicking on tabs
        m_tabWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
        connect(m_tabWidget, &QWidget::customContextMenuRequested, this, [this]() {
            OpenDocumentTabContextMenu();
        });

        centralWidget()->layout()->addWidget(m_tabWidget);
    }

    void AtomToolsDocumentMainWindow::UpdateRecentFileMenu()
    {
        m_menuOpenRecent->clear();

        AZStd::vector<AZStd::string> absolutePaths;
        AtomToolsDocumentSystemRequestBus::EventResult(
            absolutePaths, m_toolId, &AtomToolsDocumentSystemRequestBus::Handler::GetRecentFilePaths);
        for (const AZStd::string& path : absolutePaths)
        {
            if (QFile::exists(path.c_str()))
            {
                m_menuOpenRecent->addAction(tr("&%1: %2").arg(m_menuOpenRecent->actions().size()).arg(path.c_str()), [this, path]() {
                    AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::OpenDocument, path);
                });
            }
        }

        m_menuOpenRecent->addAction(tr("Clear Recent Files"), [this]() {
            QTimer::singleShot(0, this, [this]() {
                AtomToolsDocumentSystemRequestBus::Event(
                    m_toolId, &AtomToolsDocumentSystemRequestBus::Handler::ClearRecentFilePaths);
            });
        });
    }

    QString AtomToolsDocumentMainWindow::GetDocumentPath(const AZ::Uuid& documentId) const
    {
        AZStd::string absolutePath;
        AtomToolsDocumentRequestBus::EventResult(absolutePath, documentId, &AtomToolsDocumentRequestBus::Handler::GetAbsolutePath);
        return absolutePath.c_str();
    }

    AZ::Uuid AtomToolsDocumentMainWindow::GetDocumentTabId(const int tabIndex) const
    {
        const QVariant tabData = m_tabWidget->tabBar()->tabData(tabIndex);
        if (!tabData.isNull())
        {
            // We need to be able to convert between a UUID and a string to store and retrieve a document ID from the tab bar
            const QString documentIdString = tabData.toString();
            const QByteArray documentIdBytes = documentIdString.toUtf8();
            const AZ::Uuid documentId(documentIdBytes.data(), documentIdBytes.size());
            return documentId;
        }
        return AZ::Uuid::CreateNull();
    }

    AZ::Uuid AtomToolsDocumentMainWindow::GetCurrentDocumentId() const
    {
        return GetDocumentTabId(m_tabWidget->currentIndex());
    }

    int AtomToolsDocumentMainWindow::GetDocumentTabIndex(const AZ::Uuid& documentId) const
    {
        for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
        {
            if (documentId == GetDocumentTabId(tabIndex))
            {
                return tabIndex;
            }
        }
        return -1;
    }

    bool AtomToolsDocumentMainWindow::HasDocumentTab(const AZ::Uuid& documentId) const
    {
        return GetDocumentTabIndex(documentId) >= 0;
    }

    bool AtomToolsDocumentMainWindow::AddDocumentTab(const AZ::Uuid& documentId, QWidget* viewWidget)
    {
        if (!documentId.IsNull() && viewWidget)
        {
            // Blocking signals from the tab bar so the currentChanged signal is not sent while a document is already being opened.
            // This prevents the OnDocumentOpened notification from being sent recursively.
            const QSignalBlocker blocker(m_tabWidget);

            // If a tab for this document already exists then select it instead of creating a new one
            if (const int tabIndex = GetDocumentTabIndex(documentId); tabIndex >= 0)
            {
                m_tabWidget->setVisible(true);
                m_tabWidget->setCurrentIndex(tabIndex);
                UpdateDocumentTab(documentId);
                delete viewWidget;
                return false;
            }

            // The user can manually reorder tabs which will invalidate any association by index.
            // We need to store the document ID with the tab using the tab instead of a separate mapping.
            const int tabIndex = m_tabWidget->addTab(viewWidget, QString());
            m_tabWidget->tabBar()->setTabData(tabIndex, QVariant(documentId.ToString<QString>()));
            m_tabWidget->setVisible(true);
            m_tabWidget->setCurrentIndex(tabIndex);
            UpdateDocumentTab(documentId);
            QueueUpdateMenus(true);
            return true;
        }

        delete viewWidget;
        return false;
    }

    void AtomToolsDocumentMainWindow::RemoveDocumentTab(const AZ::Uuid& documentId)
    {
        // We are not blocking signals here because we want closing tabs to close the document and automatically select the next document.
        if (const int tabIndex = GetDocumentTabIndex(documentId); tabIndex >= 0)
        {
            m_tabWidget->removeTab(tabIndex);
            m_tabWidget->setVisible(m_tabWidget->count() > 0);
            m_tabWidget->repaint();
            QueueUpdateMenus(true);
        }
    }

    void AtomToolsDocumentMainWindow::UpdateDocumentTab(const AZ::Uuid& documentId)
    {
        // Whenever a document is opened, saved, or modified we need to update the tab label
        if (const int tabIndex = GetDocumentTabIndex(documentId); tabIndex >= 0)
        {
            bool isModified = false;
            AtomToolsDocumentRequestBus::EventResult(isModified, documentId, &AtomToolsDocumentRequestBus::Events::IsModified);
            AZStd::string absolutePath;
            AtomToolsDocumentRequestBus::EventResult(absolutePath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);
            AZStd::string filename;
            AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);
            if (filename.empty())
            {
                filename = "(untitled)";
            }

            // We use an asterisk prepended to the file name to denote modified document.
            // Appending is standard and preferred but the tabs elide from the end (instead of middle) and cut it off.
            const AZStd::string label = isModified ? "* " + filename : filename;
            m_tabWidget->setTabText(tabIndex, label.c_str());
            m_tabWidget->setTabToolTip(tabIndex, absolutePath.c_str());
            m_tabWidget->repaint();
        }
    }

    void AtomToolsDocumentMainWindow::SelectPrevDocumentTab()
    {
        if (m_tabWidget->count() > 1)
        {
            // Adding count to wrap around when index <= 0
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + m_tabWidget->count() - 1) % m_tabWidget->count());
        }
    }

    void AtomToolsDocumentMainWindow::SelectNextDocumentTab()
    {
        if (m_tabWidget->count() > 1)
        {
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + 1) % m_tabWidget->count());
        }
    }

    void AtomToolsDocumentMainWindow::OpenDocumentTabContextMenu()
    {
        const QTabBar* tabBar = m_tabWidget->tabBar();
        const QPoint position = tabBar->mapFromGlobal(QCursor::pos());
        const int clickedTabIndex = tabBar->tabAt(position);
        if (const AZ::Uuid documentId = GetDocumentTabId(clickedTabIndex); !documentId.IsNull())
        {
            QMenu menu;
            PopulateTabContextMenu(documentId, menu);
            menu.exec(QCursor::pos());
        }
    }

    void AtomToolsDocumentMainWindow::PopulateTabContextMenu(const AZ::Uuid& documentId, QMenu& menu)
    {
        menu.addAction("Select", [this, documentId]() {
            AtomToolsDocumentNotificationBus::Event(m_toolId, &AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        });
        menu.addAction("Close", [this, documentId]() {
            AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
        });
        menu.addAction("Close Others", [this, documentId]() {
            AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
        })->setEnabled(m_tabWidget->tabBar()->count() > 1);
    }

    AZStd::string AtomToolsDocumentMainWindow::GetSaveDocumentParams(const AZStd::string& initialPath) const
    {
        return GetSaveFilePath(initialPath);
    }

    bool AtomToolsDocumentMainWindow::GetCreateDocumentParams(AZStd::string& openPath, AZStd::string& savePath) const
    {
        DocumentTypeInfoVector documentTypes;
        AtomToolsDocumentSystemRequestBus::EventResult(
            documentTypes, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::GetRegisteredDocumentTypes);

        if (documentTypes.empty())
        {
            return false;
        }

        int documentTypeIndex = 0;
        if (documentTypes.size() > 1)
        {
            QStringList items;
            for (const auto& documentType : documentTypes)
            {
                items.append(documentType.m_documentTypeName.c_str());
            }

            bool result = false;
            QString item = QInputDialog::getItem(
                const_cast<AtomToolsDocumentMainWindow*>(this),
                tr("Select Document Type"),
                tr("Select document type to create"),
                items,
                0,
                false,
                &result);
            if (!result || item.isEmpty())
            {
                return false;
            }

            documentTypeIndex = items.indexOf(item);
        }

        const auto& documentType = documentTypes[documentTypeIndex];
        CreateDocumentDialog dialog(
            documentType,
            AZStd::string::format("%s/Assets", AZ::Utils::GetProjectPath().c_str()).c_str(),
            const_cast<AtomToolsDocumentMainWindow*>(this));
        dialog.adjustSize();

        if (dialog.exec() == QDialog::Accepted && !dialog.m_sourcePath.isEmpty() && !dialog.m_targetPath.isEmpty())
        {
            savePath = dialog.m_targetPath.toUtf8().constData();
            openPath = dialog.m_sourcePath.toUtf8().constData();
            return true;
        }
        return false;
    }

    AZStd::vector<AZStd::string> AtomToolsDocumentMainWindow::GetOpenDocumentParams() const
    {
        DocumentTypeInfoVector documentTypes;
        AtomToolsDocumentSystemRequestBus::EventResult(
            documentTypes, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::GetRegisteredDocumentTypes);

        QStringList extensionList;
        for (const auto& documentType : documentTypes)
        {
            for (const auto& extensionInfo : documentType.m_supportedExtensionsToOpen)
            {
                extensionList.append(extensionInfo.second.c_str());
            }
        }

        if (!extensionList.empty())
        {
            // Generate a regular expression that combines all of the supported extensions to use as a filter for the asset picker
            QString expression = QString("[\\w\\-.]+\\.(%1)").arg(extensionList.join("|"));
            return GetOpenFilePaths(QRegExp(expression, Qt::CaseInsensitive));
        }

        return {};
    }

    void AtomToolsDocumentMainWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        AtomToolsDocumentRequestBus::EventResult(isOpen, documentId, &AtomToolsDocumentRequestBus::Events::IsOpen);
        AZStd::string absolutePath;
        AtomToolsDocumentRequestBus::EventResult(absolutePath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

        UpdateDocumentTab(documentId);
        ActivateWindow();
        QueueUpdateMenus(true);

        if (isOpen && !absolutePath.empty())
        {
            // Whenever a document is opened or selected select the corresponding tab
            m_tabWidget->setCurrentIndex(GetDocumentTabIndex(documentId));

            // Find and select the file path in the asset browser
            m_assetBrowser->SelectEntries(absolutePath);

            SetStatusMessage(tr("Document opened: %1").arg(absolutePath.c_str()));
        }
    }

    void AtomToolsDocumentMainWindow::OnDocumentClosed(const AZ::Uuid& documentId)
    {
        RemoveDocumentTab(documentId);
        SetStatusMessage(tr("Document closed: %1").arg(GetDocumentPath(documentId)));
    }

    void AtomToolsDocumentMainWindow::OnDocumentCleared(const AZ::Uuid& documentId)
    {
        UpdateDocumentTab(documentId);
        QueueUpdateMenus(true);
        SetStatusMessage(tr("Document cleared: %1").arg(GetDocumentPath(documentId)));
    }

    void AtomToolsDocumentMainWindow::OnDocumentError(const AZ::Uuid& documentId)
    {
        UpdateDocumentTab(documentId);
        QueueUpdateMenus(true);
        SetStatusError(tr("Document error: %1").arg(GetDocumentPath(documentId)));
    }

    void AtomToolsDocumentMainWindow::OnDocumentDestroyed(const AZ::Uuid& documentId)
    {
        RemoveDocumentTab(documentId);
    }

    void AtomToolsDocumentMainWindow::OnDocumentModified(const AZ::Uuid& documentId)
    {
        UpdateDocumentTab(documentId);
    }

    void AtomToolsDocumentMainWindow::OnDocumentUndoStateChanged(const AZ::Uuid& documentId)
    {
        if (documentId == GetCurrentDocumentId())
        {
            QueueUpdateMenus(false);
        }
    }

    void AtomToolsDocumentMainWindow::OnDocumentSaved(const AZ::Uuid& documentId)
    {
        UpdateDocumentTab(documentId);
        SetStatusMessage(tr("Document saved: %1").arg(GetDocumentPath(documentId)));
    }

    void AtomToolsDocumentMainWindow::closeEvent(QCloseEvent* closeEvent)
    {
        bool canClose = true;
        AtomToolsDocumentSystemRequestBus::EventResult(canClose, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments);
        closeEvent->setAccepted(canClose);
        Base::closeEvent(closeEvent);
    }

    void AtomToolsDocumentMainWindow::dragEnterEvent(QDragEnterEvent* event)
    {
        // Check for files matching supported document types being dragged into the main window
        for (const AZStd::string& path : GetPathsFromMimeData(event->mimeData()))
        {
            DocumentTypeInfoVector documentTypes;
            AtomToolsDocumentSystemRequestBus::EventResult(
                documentTypes, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::GetRegisteredDocumentTypes);
            for (const auto& documentType : documentTypes)
            {
                if (documentType.IsSupportedExtensionToOpen(path))
                {
                    event->setAccepted(true);
                    event->acceptProposedAction();
                    Base::dragEnterEvent(event);
                    return;
                }
            }
        }

        event->setAccepted(false);
        Base::dragEnterEvent(event);
    }

    void AtomToolsDocumentMainWindow::dragMoveEvent(QDragMoveEvent* event)
    {
        // Files dragged into the main window must only be accepted if they are within the client area
        event->setAccepted(centralWidget() && centralWidget()->geometry().contains(event->pos()));
        Base::dragMoveEvent(event);
    }

    void AtomToolsDocumentMainWindow::dragLeaveEvent(QDragLeaveEvent* event)
    {
        Base::dragLeaveEvent(event);
    }

    void AtomToolsDocumentMainWindow::dropEvent(QDropEvent* event)
    {
        // If supported document files are dragged into the main window client area attempt to open them
        if (centralWidget() && centralWidget()->geometry().contains(event->pos()))
        {
            for (const AZStd::string& path : GetPathsFromMimeData(event->mimeData()))
            {
                DocumentTypeInfoVector documentTypes;
                AtomToolsDocumentSystemRequestBus::EventResult(
                    documentTypes, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::GetRegisteredDocumentTypes);
                for (const auto& documentType : documentTypes)
                {
                    if (documentType.IsSupportedExtensionToOpen(path))
                    {
                        AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::OpenDocument, path);
                        event->acceptProposedAction();
                    }
                }
            }
        }

        Base::dropEvent(event);
    }

    template<typename Functor>
    QAction* AtomToolsDocumentMainWindow::CreateActionAtPosition(
        QMenu* parent, QAction* position, const QString& text, Functor functor, const QKeySequence& shortcut)
    {
        QAction* action = new QAction(text, parent);
        action->setShortcut(shortcut);
        connect(action, &QAction::triggered, parent, functor);
        parent->insertAction(position, action);
        return action;
    }
} // namespace AtomToolsFramework

//#include <AtomToolsFramework/Document/moc_AtomToolsDocumentMainWindow.cpp>
