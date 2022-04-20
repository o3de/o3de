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
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QInputDialog>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
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
        m_actionNew = CreateAction("&New...", [this]() {
            AZStd::string openPath;
            AZStd::string savePath;
            if (GetCreateDocumentParams(openPath, savePath))
            {
                AtomToolsDocumentSystemRequestBus::Event(
                    m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFilePath, openPath, savePath);
            }
        }, QKeySequence::New);
        m_menuFile->insertAction(insertPostion, m_actionNew);

        m_actionOpen = CreateAction("&Open...", [this]() {
            for (const auto& path : GetOpenDocumentParams())
            {
                AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::OpenDocument, path);
            }
        }, QKeySequence::Open);
        m_menuFile->insertAction(insertPostion, m_actionOpen);

        m_menuOpenRecent = new QMenu("Open Recent", this);
        connect(m_menuOpenRecent, &QMenu::aboutToShow, this, [this]() {
            UpdateRecentFileMenu();
        });
        m_menuFile->insertMenu(insertPostion, m_menuOpenRecent);
        m_menuFile->insertSeparator(insertPostion);

        m_actionSave = CreateAction("&Save", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            bool result = false;
            AtomToolsDocumentSystemRequestBus::EventResult(
                result, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::SaveDocument, documentId);
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Save);
        m_menuFile->insertAction(insertPostion, m_actionSave);

        m_actionSaveAsCopy = CreateAction("Save &As...", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            AtomToolsDocumentSystemRequestBus::EventResult(
                result, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsCopy, documentId,
                GetSaveFilePath(documentPath.toUtf8().constData()));
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(documentPath));
            }
        }, QKeySequence::SaveAs);
        m_menuFile->insertAction(insertPostion, m_actionSaveAsCopy);

        m_actionSaveAsChild = CreateAction("Save As &Child...", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            AtomToolsDocumentSystemRequestBus::EventResult(
                result, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsChild, documentId,
                GetSaveFilePath(documentPath.toUtf8().constData()));
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(documentPath));
            }
        });
        m_menuFile->insertAction(insertPostion, m_actionSaveAsChild);

        m_actionSaveAll = CreateAction("Save A&ll", [this]() {
            bool result = false;
            AtomToolsDocumentSystemRequestBus::EventResult(
                result, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::SaveAllDocuments);
            if (!result)
            {
                SetStatusError(tr("Document save all failed"));
            }
        });
        m_menuFile->insertAction(insertPostion, m_actionSaveAll);
        m_menuFile->insertSeparator(insertPostion);

        m_actionClose = CreateAction("&Close", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
        }, QKeySequence::Close);
        m_menuFile->insertAction(insertPostion, m_actionClose);

        m_actionCloseAll = CreateAction("Close All", [this]() {
            AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments);
        });
        m_menuFile->insertAction(insertPostion, m_actionCloseAll);

        m_actionCloseOthers = CreateAction("Close Others", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            AtomToolsDocumentSystemRequestBus::Event(
                m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
        });
        m_menuFile->insertAction(insertPostion, m_actionCloseOthers);
        m_menuFile->insertSeparator(insertPostion);

        insertPostion = !m_menuEdit->actions().empty() ? m_menuEdit->actions().front() : nullptr;

        m_actionUndo = CreateAction("&Undo", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            bool result = false;
            AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::Undo);
            if (!result)
            {
                SetStatusError(tr("Document undo failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Undo);
        m_menuEdit->insertAction(insertPostion, m_actionUndo);

        m_actionRedo = CreateAction("&Redo", [this]() {
            const AZ::Uuid documentId = GetCurrentDocumentId();
            bool result = false;
            AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::Redo);
            if (!result)
            {
                SetStatusError(tr("Document redo failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Redo);
        m_menuEdit->insertAction(insertPostion, m_actionRedo);
        m_menuEdit->insertSeparator(insertPostion);

        insertPostion = !m_menuView->actions().empty() ? m_menuView->actions().front() : nullptr;

        m_actionPreviousTab = CreateAction("&Previous Tab", [this]() {
            SelectPrevDocumentTab();
        }, Qt::CTRL | Qt::SHIFT | Qt::Key_Tab); //QKeySequence::PreviousChild is mapped incorrectly in Qt
        m_menuView->insertAction(insertPostion, m_actionPreviousTab);

        m_actionNextTab = CreateAction("&Next Tab", [this]() {
            SelectNextDocumentTab();
        }, Qt::CTRL | Qt::Key_Tab); //QKeySequence::NextChild works as expected but mirroring Previous
        m_menuView->insertAction(insertPostion, m_actionNextTab);
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
        connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int tabIndex) {
            const AZ::Uuid documentId = GetDocumentTabId(tabIndex);
            AtomToolsDocumentNotificationBus::Event(m_toolId,&AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        });

        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int tabIndex) {
            const AZ::Uuid documentId = GetDocumentTabId(tabIndex);
            AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
        });

        // Add context menu for right-clicking on tabs
        m_tabWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
        connect(m_tabWidget, &QWidget::customContextMenuRequested, this, [this]() {
            OpenDocumentTabContextMenu();
        });

        centralWidget()->layout()->addWidget(m_tabWidget);
    }

    void AtomToolsDocumentMainWindow::AddRecentFilePath(const AZStd::string& absolutePath)
    {
        if (!absolutePath.empty())
        {
            // Get the list of previously stored recent file paths from the settings registry
            AZStd::vector<AZStd::string> paths = GetSettingsObject(RecentFilePathsKey, AZStd::vector<AZStd::string>());

            // If the new path is already in the list then remove it Because it will be moved to the front of the list
            AZStd::erase_if(paths, [&absolutePath](const AZStd::string& currentPath) {
                return AZ::StringFunc::Equal(currentPath, absolutePath);
            });

            paths.insert(paths.begin(), absolutePath);

            constexpr const size_t recentFilePathsMax = 10;
            if (paths.size() > recentFilePathsMax)
            {
                paths.resize(recentFilePathsMax);
            }

            SetSettingsObject(RecentFilePathsKey, paths);
        }
    }

    void AtomToolsDocumentMainWindow::ClearRecentFilePaths()
    {
        SetSettingsObject(RecentFilePathsKey, AZStd::vector<AZStd::string>());
    }

    void AtomToolsDocumentMainWindow::UpdateRecentFileMenu()
    {
        m_menuOpenRecent->clear();
        for (const AZStd::string& path : GetSettingsObject(RecentFilePathsKey, AZStd::vector<AZStd::string>()))
        {
            if (QFile::exists(path.c_str()))
            {
                m_menuOpenRecent->addAction(tr("&%1: %2").arg(m_menuOpenRecent->actions().size()).arg(path.c_str()), [this, path]() {
                    AtomToolsDocumentSystemRequestBus::Event(m_toolId, &AtomToolsDocumentSystemRequestBus::Events::OpenDocument, path);
                });
            }
        }

        m_menuOpenRecent->addAction(tr("Clear Recent Files"), [this]() {
            QTimer::singleShot(0, this, &AtomToolsDocumentMainWindow::ClearRecentFilePaths);
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

    bool AtomToolsDocumentMainWindow::GetCreateDocumentParams(AZStd::string& openPath, AZStd::string& savePath)
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
            QString item = QInputDialog::getItem(this, tr("Select Document Type"), tr("Select document type to create"), items, 0, false, &result);
            if (!result || item.isEmpty())
            {
                return false;
            }

            documentTypeIndex = items.indexOf(item);
        }

        const auto& documentType = documentTypes[documentTypeIndex];
        CreateDocumentDialog dialog(documentType, AZStd::string::format("%s/Assets", AZ::Utils::GetProjectPath().c_str()).c_str(), this);
        dialog.adjustSize();

        if (dialog.exec() == QDialog::Accepted && !dialog.m_sourcePath.isEmpty() && !dialog.m_targetPath.isEmpty())
        {
            savePath = dialog.m_targetPath.toUtf8().constData();
            openPath = dialog.m_sourcePath.toUtf8().constData();
            return true;
        }
        return false;
    }

    AZStd::vector<AZStd::string> AtomToolsDocumentMainWindow::GetOpenDocumentParams()
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

        AddRecentFilePath(absolutePath);
        UpdateDocumentTab(documentId);
        ActivateWindow();
        QueueUpdateMenus(true);

        m_assetBrowser->SelectEntries(absolutePath);

        if (isOpen && !absolutePath.empty())
        {
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

    template<typename Functor>
    QAction* AtomToolsDocumentMainWindow::CreateAction(const QString& text, Functor functor, const QKeySequence& shortcut)
    {
        QAction* action = new QAction(text, this);
        action->setShortcut(shortcut);
        connect(action, &QAction::triggered, this, functor);
        return action;
    }
} // namespace AtomToolsFramework

//#include <AtomToolsFramework/Document/moc_AtomToolsDocumentMainWindow.cpp>
