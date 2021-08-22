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
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QWindow>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    AtomToolsDocumentMainWindow::AtomToolsDocumentMainWindow(QWidget* parent /* = 0 */)
        : AtomToolsMainWindow(parent)
    {
        setObjectName("AtomToolsDocumentMainWindow");

        AtomToolsDocumentNotificationBus::Handler::BusConnect();
    }

    AtomToolsDocumentMainWindow::~AtomToolsDocumentMainWindow()
    {
        AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
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
            AddTabForDocumentId(documentId, filename, absolutePath, [this]{
                // The tab widget requires a dummy page per tab
                auto contentWidget = new QWidget(centralWidget());
                contentWidget->setContentsMargins(0, 0, 0, 0);
                contentWidget->setFixedSize(0, 0);
                return contentWidget;
            });

            UpdateTabForDocumentId(documentId, filename, absolutePath, isModified);
        }

        const bool hasTabs = m_tabWidget->count() > 0;

        // Update menu options
        m_actionNew->setEnabled(true);
        m_actionOpen->setEnabled(true);
        m_actionOpenRecent->setEnabled(false);
        m_actionClose->setEnabled(hasTabs);
        m_actionCloseAll->setEnabled(hasTabs);
        m_actionCloseOthers->setEnabled(hasTabs);

        m_actionSave->setEnabled(isOpen && isSavable);
        m_actionSaveAsCopy->setEnabled(isOpen && isSavable);
        m_actionSaveAsChild->setEnabled(isOpen);
        m_actionSaveAll->setEnabled(hasTabs);

        m_actionExit->setEnabled(true);

        m_actionUndo->setEnabled(canUndo);
        m_actionRedo->setEnabled(canRedo);
        m_actionSettings->setEnabled(true);

        m_actionPreviousTab->setEnabled(m_tabWidget->count() > 1);
        m_actionNextTab->setEnabled(m_tabWidget->count() > 1);

        m_actionAbout->setEnabled(false);

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
        RemoveTabForDocumentId(documentId);
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
        UpdateTabForDocumentId(documentId, filename, absolutePath, isModified);
    }

    void AtomToolsDocumentMainWindow::OnDocumentUndoStateChanged(const AZ::Uuid& documentId)
    {
        if (documentId == GetDocumentIdFromTab(m_tabWidget->currentIndex()))
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
        UpdateTabForDocumentId(documentId, filename, absolutePath, isModified);
        SetStatusMessage(tr("Document saved: %1").arg(GetDocumentPath(documentId)));
    }

    void AtomToolsDocumentMainWindow::CreateMenu()
    {
        Base::CreateMenu();

        // Generating the main menu manually because it's easier and we will have some dynamic or data driven entries
        m_menuFile = menuBar()->addMenu("&File");

        m_actionNew = m_menuFile->addAction("&New...", [this]() {
            //CreateMaterialDialog createDialog(this);
            //createDialog.adjustSize();

            //if (createDialog.exec() == QDialog::Accepted &&
            //    !createDialog.m_materialFileInfo.absoluteFilePath().isEmpty() &&
            //    !createDialog.m_materialTypeFileInfo.absoluteFilePath().isEmpty())
            //{
            //    AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile,
            //        createDialog.m_materialTypeFileInfo.absoluteFilePath().toUtf8().constData(),
            //        createDialog.m_materialFileInfo.absoluteFilePath().toUtf8().constData());
            //}
        }, QKeySequence::New);

        m_actionOpen = m_menuFile->addAction("&Open...", [this]() {
            //const AZStd::vector<AZ::Data::AssetType> assetTypes = { azrtti_typeid<AZ::RPI::MaterialAsset>() };
            //const AZStd::string filePath = GetOpenFileInfo(assetTypes).absoluteFilePath().toUtf8().constData();
            //if (!filePath.empty())
            //{
            //    AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::OpenDocument, filePath);
            //}
        }, QKeySequence::Open);

        m_actionOpenRecent = m_menuFile->addAction("Open &Recent");

        m_menuFile->addSeparator();

        m_actionSave = m_menuFile->addAction("&Save", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            bool result = false;
            AtomToolsDocumentSystemRequestBus::BroadcastResult(result, &AtomToolsDocumentSystemRequestBus::Events::SaveDocument, documentId);
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Save);

        m_actionSaveAsCopy = m_menuFile->addAction("Save &As...", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            AtomToolsDocumentSystemRequestBus::BroadcastResult(result, &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsCopy,
                documentId, GetSaveFileInfo(documentPath).absoluteFilePath().toUtf8().constData());
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::SaveAs);

        m_actionSaveAsChild = m_menuFile->addAction("Save As &Child...", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            AtomToolsDocumentSystemRequestBus::BroadcastResult(result, &AtomToolsDocumentSystemRequestBus::Events::SaveDocumentAsChild,
                documentId, GetSaveFileInfo(documentPath).absoluteFilePath().toUtf8().constData());
            if (!result)
            {
                SetStatusError(tr("Document save failed: %1").arg(GetDocumentPath(documentId)));
            }
        });

        m_actionSaveAll = m_menuFile->addAction("Save A&ll", [this]() {
            bool result = false;
            AtomToolsDocumentSystemRequestBus::BroadcastResult(result, &AtomToolsDocumentSystemRequestBus::Events::SaveAllDocuments);
            if (!result)
            {
                SetStatusError(tr("Document save all failed"));
            }
        });

        m_menuFile->addSeparator();

        m_actionClose = m_menuFile->addAction("&Close", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
        }, QKeySequence::Close);

        m_actionCloseAll = m_menuFile->addAction("Close All", [this]() {
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments);
        });

        m_actionCloseOthers = m_menuFile->addAction("Close Others", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
        });

        m_menuFile->addSeparator();

        m_menuFile->addAction("Run &Python...", [this]() {
            const QString script = QFileDialog::getOpenFileName(this, "Run Script", QString(), QString("*.py"));
            if (!script.isEmpty())
            {
                AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilename, script.toUtf8().constData());
            }
        });

        m_menuFile->addSeparator();

        m_actionExit = m_menuFile->addAction("E&xit", [this]() {
            close();
        }, QKeySequence::Quit);

        m_menuEdit = menuBar()->addMenu("&Edit");

        m_actionUndo = m_menuEdit->addAction("&Undo", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            bool result = false;
            AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::Undo);
            if (!result)
            {
                SetStatusError(tr("Document undo failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Undo);

        m_actionRedo = m_menuEdit->addAction("&Redo", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            bool result = false;
            AtomToolsDocumentRequestBus::EventResult(result, documentId, &AtomToolsDocumentRequestBus::Events::Redo);
            if (!result)
            {
                SetStatusError(tr("Document redo failed: %1").arg(GetDocumentPath(documentId)));
            }
        }, QKeySequence::Redo);

        m_menuEdit->addSeparator();

        m_actionSettings = m_menuEdit->addAction("&Settings...", [this]() {
            //SettingsDialog dialog(this);
            //dialog.exec();
        }, QKeySequence::Preferences);
        m_actionSettings->setEnabled(true);

        m_menuView = menuBar()->addMenu("&View");

        m_menuView->addSeparator();

        m_actionPreviousTab = m_menuView->addAction("&Previous Tab", [this]() {
            SelectPreviousTab();
        }, Qt::CTRL | Qt::SHIFT | Qt::Key_Tab); //QKeySequence::PreviousChild is mapped incorrectly in Qt

        m_actionNextTab = m_menuView->addAction("&Next Tab", [this]() {
            SelectNextTab();
        }, Qt::CTRL | Qt::Key_Tab); //QKeySequence::NextChild works as expected but mirroring Previous

        m_menuHelp = menuBar()->addMenu("&Help");

        m_actionHelp = m_menuHelp->addAction("&Help...", [this]() {
            //HelpDialog dialog(this);
            //dialog.exec();
        });

        m_actionAbout = m_menuHelp->addAction("&About...", [this]() {
        });
    }

    void AtomToolsDocumentMainWindow::CreateTabBar()
    {
        Base::CreateTabBar();

        // This signal will be triggered whenever a tab is added, removed, selected, clicked, dragged
        // When the last tab is removed tabIndex will be -1 and the document ID will be null
        // This should automatically clear the active document
        connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int tabIndex) {
            const AZ::Uuid documentId = GetDocumentIdFromTab(tabIndex);
            AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        });

        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int tabIndex) {
            const AZ::Uuid documentId = GetDocumentIdFromTab(tabIndex);
            AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
        });
    }

    QString AtomToolsDocumentMainWindow::GetDocumentPath(const AZ::Uuid& documentId) const
    {
        AZStd::string absolutePath;
        AtomToolsDocumentRequestBus::EventResult(absolutePath, documentId, &AtomToolsDocumentRequestBus::Handler::GetAbsolutePath);
        return absolutePath.c_str();
    }

    void AtomToolsDocumentMainWindow::OpenTabContextMenu()
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
                const AZ::Uuid documentId = GetDocumentIdFromTab(clickedTabIndex);
                AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsDocumentNotificationBus::Events::OnDocumentOpened, documentId);
            });
            tabMenu.addAction("Close", [this, clickedTabIndex]() {
                const AZ::Uuid documentId = GetDocumentIdFromTab(clickedTabIndex);
                AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseDocument, documentId);
            });
            auto closeOthersAction = tabMenu.addAction("Close Others", [this, clickedTabIndex]() {
                const AZ::Uuid documentId = GetDocumentIdFromTab(clickedTabIndex);
                AtomToolsDocumentSystemRequestBus::Broadcast(&AtomToolsDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
            });
            closeOthersAction->setEnabled(tabBar->count() > 1);
            tabMenu.exec(QCursor::pos());
        }
    }
} // namespace AtomToolsFramework

//#include <Document/moc_AtomToolsDocumentMainWindow.cpp>
