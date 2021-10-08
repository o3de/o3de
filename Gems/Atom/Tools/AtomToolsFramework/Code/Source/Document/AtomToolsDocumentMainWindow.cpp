/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentMainWindow.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowNotificationBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QLayout>
#include <QMenu>
#include <QMenuBar>
#include <QWindow>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    AtomToolsDocumentMainWindow::AtomToolsDocumentMainWindow(QWidget* parent /* = 0 */)
        : AtomToolsMainWindow(parent)
    {
        setObjectName("AtomToolsDocumentMainWindow");
        AddDocumentMenus();
        AddDocumentTabBar();
        AtomToolsDocumentNotificationBus::Handler::BusConnect();
    }

    AtomToolsDocumentMainWindow::~AtomToolsDocumentMainWindow()
    {
        AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    void AtomToolsDocumentMainWindow::AddDocumentMenus()
    {
        QAction* insertPostion = !m_menuFile->actions().empty() ? m_menuFile->actions().front() : nullptr;

        // Generating the main menu manually because it's easier and we will have some dynamic or data driven entries
        m_actionNew = CreateAction("&New...", [this]() {
            AZStd::string openPath;
            AZStd::string savePath;
            if (GetCreateDocumentParams(openPath, savePath))
            {
                AtomToolsDocumentSystemRequestBus::Broadcast(
                    &AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile, openPath, savePath);
            }
        }, QKeySequence::New);
        m_menuFile->insertAction(insertPostion, m_actionNew);

        m_actionOpen = CreateAction("&Open...", [this]() {
            AZStd::string openPath;
            if (GetOpenDocumentParams(openPath))
            {
                AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::OpenDocument, openPath);
            }
        }, QKeySequence::Open);
        m_menuFile->insertAction(insertPostion, m_actionOpen);
        m_menuFile->insertSeparator(insertPostion);

        m_actionSave = CreateAction("&Save", [this]() {
            const AZ::Uuid documentId = GetDocumentTabId(m_tabWidget->currentIndex());
            bool result = false;
            AtomToolsDocumentSystemRequestBus::BroadcastResult(result, &AtomToolsDocumentSystemRequestBus::Events::SaveDocument, documentId);
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Save);
        m_menuFile->insertAction(insertPostion, m_actionSave);

        m_actionSaveAsCopy = CreateAction("Save &As...", [this]() {
            const AZ::Uuid documentId = GetDocumentTabId(m_tabWidget->currentIndex());
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            AtomToolsDocumentSystemRequestBus::BroadcastResult(result, &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsCopy,
                documentId, GetSaveFileInfo(documentPath).absoluteFilePath().toUtf8().constData());
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::SaveAs);
        m_menuFile->insertAction(insertPostion, m_actionSaveAsCopy);

        m_actionSaveAsChild = CreateAction("Save As &Child...", [this]() {
            const AZ::Uuid documentId = GetDocumentTabId(m_tabWidget->currentIndex());
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            AtomToolsDocumentSystemRequestBus::BroadcastResult(result, &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsChild,
                documentId, GetSaveFileInfo(documentPath).absoluteFilePath().toUtf8().constData());
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(GetDocumentPath(documentId)));
            }
        });
        m_menuFile->insertAction(insertPostion, m_actionSaveAsChild);

        m_actionSaveAll = CreateAction("Save A&ll", [this]() {
            bool result = false;
            AtomToolsDocumentSystemRequestBus::BroadcastResult(result, &AtomToolsDocumentSystemRequestBus::Events::SaveAllDocuments);
            if (!result)
            {
                SetStatusError(tr("Document save all failed"));
            }
        });
        m_menuFile->insertAction(insertPostion, m_actionSaveAll);
        m_menuFile->insertSeparator(insertPostion);

        m_actionClose = CreateAction("&Close", [this]() {
            const AZ::Uuid documentId = GetDocumentTabId(m_tabWidget->currentIndex());
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
        }, QKeySequence::Close);
        m_menuFile->insertAction(insertPostion, m_actionClose);

        m_actionCloseAll = CreateAction("Close All", []() {
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments);
        });
        m_menuFile->insertAction(insertPostion, m_actionCloseAll);

        m_actionCloseOthers = CreateAction("Close Others", [this]() {
            const AZ::Uuid documentId = GetDocumentTabId(m_tabWidget->currentIndex());
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
        });
        m_menuFile->insertAction(insertPostion, m_actionCloseOthers);
        m_menuFile->insertSeparator(insertPostion);

        insertPostion = !m_menuEdit->actions().empty() ? m_menuEdit->actions().front() : nullptr;

        m_actionUndo = CreateAction("&Undo", [this]() {
            const AZ::Uuid documentId = GetDocumentTabId(m_tabWidget->currentIndex());
            bool result = false;
            AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::Undo);
            if (!result)
            {
                SetStatusError(tr("Document undo failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Undo);
        m_menuEdit->insertAction(insertPostion, m_actionUndo);

        m_actionRedo = CreateAction("&Redo", [this]() {
            const AZ::Uuid documentId = GetDocumentTabId(m_tabWidget->currentIndex());
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

        m_actionPreviousTab = CreateAction(
            "&Previous Tab",
            [this]()
            {
            SelectPrevDocumentTab();
        }, Qt::CTRL | Qt::SHIFT | Qt::Key_Tab); //QKeySequence::PreviousChild is mapped incorrectly in Qt
        m_menuView->insertAction(insertPostion, m_actionPreviousTab);

        m_actionNextTab = CreateAction("&Next Tab", [this]() {
            SelectNextDocumentTab();
        }, Qt::CTRL | Qt::Key_Tab); //QKeySequence::NextChild works as expected but mirroring Previous
        m_menuView->insertAction(insertPostion, m_actionNextTab);
        m_menuView->insertSeparator(insertPostion);
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
            AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        });

        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int tabIndex) {
            const AZ::Uuid documentId = GetDocumentTabId(tabIndex);
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
        });

        // Add context menu for right-clicking on tabs
        m_tabWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
        connect(m_tabWidget, &QWidget::customContextMenuRequested, this, [this]() {
            OpenDocumentTabContextMenu();
        });

        centralWidget()->layout()->addWidget(m_tabWidget);
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

    void AtomToolsDocumentMainWindow::AddDocumentTab(
        const AZ::Uuid& documentId, const AZStd::string& label, const AZStd::string& toolTip)
    {
        // Blocking signals from the tab bar so the currentChanged signal is not sent while a document is already being opened.
        // This prevents the OnDocumentOpened notification from being sent recursively.
        const QSignalBlocker blocker(m_tabWidget);

        // If a tab for this document already exists then select it instead of creating a new one
        for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
        {
            if (documentId == GetDocumentTabId(tabIndex))
            {
                m_tabWidget->setCurrentIndex(tabIndex);
                m_tabWidget->repaint();
                return;
            }
        }

        const int tabIndex = m_tabWidget->addTab(CreateDocumentTabView(documentId), label.c_str());

        // The user can manually reorder tabs which will invalidate any association by index.
        // We need to store the document ID with the tab using the tab instead of a separate mapping.
        m_tabWidget->tabBar()->setTabData(tabIndex, QVariant(documentId.ToString<QString>()));
        m_tabWidget->setTabToolTip(tabIndex, toolTip.c_str());
        m_tabWidget->setCurrentIndex(tabIndex);
        m_tabWidget->setVisible(true);
        m_tabWidget->repaint();
    }

    void AtomToolsDocumentMainWindow::RemoveDocumentTab(const AZ::Uuid& documentId)
    {
        // We are not blocking signals here because we want closing tabs to close the associated document
        // and automatically select the next document.
        for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
        {
            if (documentId == GetDocumentTabId(tabIndex))
            {
                m_tabWidget->removeTab(tabIndex);
                m_tabWidget->setVisible(m_tabWidget->count() > 0);
                m_tabWidget->repaint();
                break;
            }
        }
    }

    void AtomToolsDocumentMainWindow::UpdateDocumentTab(
        const AZ::Uuid& documentId, const AZStd::string& label, const AZStd::string& toolTip, bool isModified)
    {
        // Whenever a document is opened, saved, or modified we need to update the tab label
        if (!documentId.IsNull())
        {
            // Because tab order and indexes can change from user interactions, we cannot store a map
            // between a tab index and document ID.
            // We must iterate over all of the tabs to find the one associated with this document.
            for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
            {
                if (documentId == GetDocumentTabId(tabIndex))
                {
                    // We use an asterisk prepended to the file name to denote modified document
                    // Appending is standard and preferred but the tabs elide from the
                    // end (instead of middle) and cut it off
                    const AZStd::string modifiedLabel = isModified ? "* " + label : label;
                    m_tabWidget->setTabText(tabIndex, modifiedLabel.c_str());
                    m_tabWidget->setTabToolTip(tabIndex, toolTip.c_str());
                    m_tabWidget->repaint();
                    break;
                }
            }
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

    QWidget* AtomToolsDocumentMainWindow::CreateDocumentTabView(const AZ::Uuid& documentId)
    {
        AZ_UNUSED(documentId);
        auto contentWidget = new QWidget(centralWidget());
        contentWidget->setContentsMargins(0, 0, 0, 0);
        contentWidget->setFixedSize(0, 0);
        return contentWidget;
    }

    void AtomToolsDocumentMainWindow::OpenDocumentTabContextMenu()
    {
        const QTabBar* tabBar = m_tabWidget->tabBar();
        const QPoint position = tabBar->mapFromGlobal(QCursor::pos());
        const int clickedTabIndex = tabBar->tabAt(position);
        const int currentTabIndex = tabBar->currentIndex();
        if (clickedTabIndex >= 0)
        {
            QMenu tabMenu;
            const QString selectActionName = (currentTabIndex == clickedTabIndex) ? "Select in Browser" : "Select";
            tabMenu.addAction(selectActionName, [this, clickedTabIndex]() {
                const AZ::Uuid documentId = GetDocumentTabId(clickedTabIndex);
                AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
            });
            tabMenu.addAction("Close", [this, clickedTabIndex]() {
                const AZ::Uuid documentId = GetDocumentTabId(clickedTabIndex);
                AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
            });
            auto closeOthersAction = tabMenu.addAction("Close Others", [this, clickedTabIndex]() {
                const AZ::Uuid documentId = GetDocumentTabId(clickedTabIndex);
                AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
            });
            closeOthersAction->setEnabled(tabBar->count() > 1);
            tabMenu.exec(QCursor::pos());
        }
    }

    bool AtomToolsDocumentMainWindow::GetCreateDocumentParams(AZStd::string& openPath, AZStd::string& savePath)
    {
        AZ_UNUSED(openPath);
        AZ_UNUSED(savePath);
        return false;
    }

    bool AtomToolsDocumentMainWindow::GetOpenDocumentParams(AZStd::string& openPath)
    {
        AZ_UNUSED(openPath);
        return false;
    }

    void AtomToolsDocumentMainWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        AtomToolsDocumentRequestBus::EventResult(isOpen, documentId, &AtomToolsDocumentRequestBus::Events::IsOpen);
        bool isSavable = false;
        AtomToolsDocumentRequestBus::EventResult(isSavable, documentId, &AtomToolsDocumentRequestBus::Events::IsSavable);
        bool isModified = false;
        AtomToolsDocumentRequestBus::EventResult(isModified, documentId, &AtomToolsDocumentRequestBus::Events::IsModified);
        bool canUndo = false;
        AtomToolsDocumentRequestBus::EventResult(canUndo, documentId, &AtomToolsDocumentRequestBus::Events::CanUndo);
        bool canRedo = false;
        AtomToolsDocumentRequestBus::EventResult(canRedo, documentId, &AtomToolsDocumentRequestBus::Events::CanRedo);
        AZStd::string absolutePath;
        AtomToolsDocumentRequestBus::EventResult(absolutePath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);
        AZStd::string filename;
        AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);

        // Update UI to display the new document
        if (!documentId.IsNull() && isOpen)
        {
            // Create a new tab for the document ID and assign it's label to the file name of the document.
            AddDocumentTab(documentId, filename, absolutePath);
            UpdateDocumentTab(documentId, filename, absolutePath, isModified);
        }

        const bool hasTabs = m_tabWidget->count() > 0;

        // Update menu options
        m_actionNew->setEnabled(true);
        m_actionOpen->setEnabled(true);
        m_actionClose->setEnabled(hasTabs);
        m_actionCloseAll->setEnabled(hasTabs);
        m_actionCloseOthers->setEnabled(hasTabs);

        m_actionSave->setEnabled(isOpen && isSavable);
        m_actionSaveAsCopy->setEnabled(isOpen && isSavable);
        m_actionSaveAsChild->setEnabled(isOpen);
        m_actionSaveAll->setEnabled(hasTabs);

        m_actionUndo->setEnabled(canUndo);
        m_actionRedo->setEnabled(canRedo);

        m_actionPreviousTab->setEnabled(m_tabWidget->count() > 1);
        m_actionNextTab->setEnabled(m_tabWidget->count() > 1);

        activateWindow();
        raise();

        const QString documentPath = GetDocumentPath(documentId);
        if (!documentPath.isEmpty())
        {
            SetStatusMessage(tr("Document opened: %1").arg(documentPath));
        }
    }

    void AtomToolsDocumentMainWindow::OnDocumentClosed(const AZ::Uuid& documentId)
    {
        RemoveDocumentTab(documentId);
        SetStatusMessage(tr("Document closed: %1").arg(GetDocumentPath(documentId)));
    }

    void AtomToolsDocumentMainWindow::OnDocumentModified(const AZ::Uuid& documentId)
    {
        bool isModified = false;
        AtomToolsDocumentRequestBus::EventResult(isModified, documentId, &AtomToolsDocumentRequestBus::Events::IsModified);
        AZStd::string absolutePath;
        AtomToolsDocumentRequestBus::EventResult(absolutePath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);
        AZStd::string filename;
        AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);
        UpdateDocumentTab(documentId, filename, absolutePath, isModified);
    }

    void AtomToolsDocumentMainWindow::OnDocumentUndoStateChanged(const AZ::Uuid& documentId)
    {
        if (documentId == GetDocumentTabId(m_tabWidget->currentIndex()))
        {
            bool canUndo = false;
            AtomToolsDocumentRequestBus::EventResult(canUndo, documentId, &AtomToolsDocumentRequestBus::Events::CanUndo);
            bool canRedo = false;
            AtomToolsDocumentRequestBus::EventResult(canRedo, documentId, &AtomToolsDocumentRequestBus::Events::CanRedo);
            m_actionUndo->setEnabled(canUndo);
            m_actionRedo->setEnabled(canRedo);
        }
    }

    void AtomToolsDocumentMainWindow::OnDocumentSaved(const AZ::Uuid& documentId)
    {
        bool isModified = false;
        AtomToolsDocumentRequestBus::EventResult(isModified, documentId, &AtomToolsDocumentRequestBus::Events::IsModified);
        AZStd::string absolutePath;
        AtomToolsDocumentRequestBus::EventResult(absolutePath, documentId, &AtomToolsDocumentRequestBus::Events::GetAbsolutePath);
        AZStd::string filename;
        AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);
        UpdateDocumentTab(documentId, filename, absolutePath, isModified);
        SetStatusMessage(tr("Document saved: %1").arg(GetDocumentPath(documentId)));
    }


    void AtomToolsDocumentMainWindow::closeEvent(QCloseEvent* closeEvent)
    {
        bool didClose = true;
        AtomToolsDocumentSystemRequestBus::BroadcastResult(didClose, &AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments);
        if (!didClose)
        {
            closeEvent->ignore();
            return;
        }

        AtomToolsMainWindowNotificationBus::Broadcast(&AtomToolsMainWindowNotifications::OnMainWindowClosing);
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

//#include <Document/moc_AtomToolsDocumentMainWindow.cpp>
