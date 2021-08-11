/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <AzCore/Name/Name.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/PythonTerminal/ScriptTermDialog.h>

#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowNotificationBus.h>

#include <Atom/Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>
#include <Window/ShaderManagementConsoleWindow.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QCloseEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QTableView>
#include <QWindow>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    ShaderManagementConsoleWindow::ShaderManagementConsoleWindow(QWidget* parent /* = 0 */)
        : AtomToolsFramework::AtomToolsMainWindow(parent)
    {
        resize(1280, 1024);

        // Among other things, we need the window wrapper to save the main window size, position, and state
        auto mainWindowWrapper =
            new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons);
        mainWindowWrapper->setGuest(this);
        mainWindowWrapper->enableSaveRestoreGeometry("O3DE", "ShaderManagementConsole", "mainWindowGeometry");

        setWindowTitle("Shader Management Console");

        setObjectName("ShaderManagementConsoleWindow");

        m_toolBar = new ShaderManagementConsoleToolBar(this);
        m_toolBar->setObjectName("ToolBar");
        addToolBar(m_toolBar);

        CreateMenu();
        CreateTabBar();

        AddDockWidget("Asset Browser", new ShaderManagementConsoleBrowserWidget, Qt::BottomDockWidgetArea, Qt::Vertical);
        AddDockWidget("Python Terminal", new AzToolsFramework::CScriptTermDialog, Qt::BottomDockWidgetArea, Qt::Horizontal);

        SetDockWidgetVisible("Python Terminal", false);

        // Restore geometry and show the window
        mainWindowWrapper->showFromSettings();

        ShaderManagementConsoleDocumentNotificationBus::Handler::BusConnect();
        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    ShaderManagementConsoleWindow::~ShaderManagementConsoleWindow()
    {
        ShaderManagementConsoleDocumentNotificationBus::Handler::BusDisconnect();
    }

    void ShaderManagementConsoleWindow::closeEvent(QCloseEvent* closeEvent)
    {
        bool didClose = true;
        ShaderManagementConsoleDocumentSystemRequestBus::BroadcastResult(didClose, &ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseAllDocuments);
        if (!didClose)
        {
            closeEvent->ignore();
            return;
        }

        AtomToolsFramework::AtomToolsMainWindowNotificationBus::Broadcast(
            &AtomToolsFramework::AtomToolsMainWindowNotifications::OnMainWindowClosing);
    }

    void ShaderManagementConsoleWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isOpen, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsOpen);
        bool isSavable = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isSavable, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsSavable);
        bool isModified = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isModified, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsModified);
        bool canUndo = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(canUndo, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::CanUndo);
        bool canRedo = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(canRedo, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::CanRedo);
        AZStd::string absolutePath;
        ShaderManagementConsoleDocumentRequestBus::EventResult(absolutePath, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);
        AZStd::string filename;
        AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);

        // Update UI to display the new document
        if (!documentId.IsNull() && isOpen)
        {
            // Create a new tab for the document ID and assign it's label to the file name of the document.
            AddTabForDocumentId(documentId, filename, absolutePath, [this, documentId]{
                // The document tab contains a table view.
                auto contentWidget = new QTableView(centralWidget());
                contentWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
                contentWidget->setModel(CreateDocumentContent(documentId));
                return contentWidget;
            });
        }

        UpdateTabForDocumentId(documentId, filename, absolutePath, isModified);

        const bool hasTabs = m_tabWidget->count() > 0;

        // Update menu options
        m_actionOpen->setEnabled(true);
        m_actionOpenRecent->setEnabled(false);
        m_actionClose->setEnabled(hasTabs);
        m_actionCloseAll->setEnabled(hasTabs);
        m_actionCloseOthers->setEnabled(hasTabs);

        m_actionSave->setEnabled(isOpen && isSavable);
        m_actionSaveAsCopy->setEnabled(isOpen && isSavable);
        m_actionSaveAll->setEnabled(hasTabs);

        m_actionExit->setEnabled(true);

        m_actionUndo->setEnabled(canUndo);
        m_actionRedo->setEnabled(canRedo);
        m_actionSettings->setEnabled(false);

        m_actionAssetBrowser->setEnabled(true);
        m_actionPythonTerminal->setEnabled(true);
        m_actionPreviousTab->setEnabled(m_tabWidget->count() > 1);
        m_actionNextTab->setEnabled(m_tabWidget->count() > 1);

        m_actionHelp->setEnabled(false);
        m_actionAbout->setEnabled(false);

        activateWindow();
        raise();

        const QString documentPath = GetDocumentPath(documentId);
        if (!documentPath.isEmpty())
        {
            const QString status = QString("Document closed: %1").arg(documentPath);
            m_statusMessage->setText(QString("<font color=\"White\">%1</font>").arg(status));
        }
    }

    void ShaderManagementConsoleWindow::OnDocumentClosed(const AZ::Uuid& documentId)
    {
        RemoveTabForDocumentId(documentId);

        const QString documentPath = GetDocumentPath(documentId);
        const QString status = QString("Document closed: %1").arg(documentPath);
        m_statusMessage->setText(QString("<font color=\"White\">%1</font>").arg(status));
    }

    void ShaderManagementConsoleWindow::OnDocumentModified(const AZ::Uuid& documentId)
    {
        bool isModified = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isModified, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsModified);
        AZStd::string absolutePath;
        ShaderManagementConsoleDocumentRequestBus::EventResult(absolutePath, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);
        AZStd::string filename;
        AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);
        UpdateTabForDocumentId(documentId, filename, absolutePath, isModified);
    }

    void ShaderManagementConsoleWindow::OnDocumentUndoStateChanged(const AZ::Uuid& documentId)
    {
        if (documentId == GetDocumentIdFromTab(m_tabWidget->currentIndex()))
        {
            bool canUndo = false;
            ShaderManagementConsoleDocumentRequestBus::EventResult(canUndo, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::CanUndo);
            bool canRedo = false;
            ShaderManagementConsoleDocumentRequestBus::EventResult(canRedo, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::CanRedo);
            m_actionUndo->setEnabled(canUndo);
            m_actionRedo->setEnabled(canRedo);
        }
    }

    void ShaderManagementConsoleWindow::OnDocumentSaved(const AZ::Uuid& documentId)
    {
        bool isModified = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isModified, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsModified);
        AZStd::string absolutePath;
        ShaderManagementConsoleDocumentRequestBus::EventResult(absolutePath, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);
        AZStd::string filename;
        AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);
        UpdateTabForDocumentId(documentId, filename, absolutePath, isModified);

        const QString documentPath = GetDocumentPath(documentId);
        const QString status = QString("Document closed: %1").arg(documentPath);
        m_statusMessage->setText(QString("<font color=\"White\">%1</font>").arg(status));
    }

    void ShaderManagementConsoleWindow::CreateMenu()
    {
        Base::CreateMenu();

        // Generating the main menu manually because it's easier and we will have some dynamic or data driven entries
        m_menuFile = m_menuBar->addMenu("&File");

        m_actionOpen = m_menuFile->addAction("&Open...", [this]() {
            const AZStd::vector<AZ::Data::AssetType> assetTypes = {
            };

            const AZStd::string filePath = AtomToolsFramework::GetOpenFileInfo(assetTypes).absoluteFilePath().toUtf8().constData();
            if (!filePath.empty())
            {
                ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::OpenDocument, filePath);
            }
        }, QKeySequence::Open);

        m_actionOpenRecent = m_menuFile->addAction("Open &Recent");

        m_menuFile->addSeparator();

        m_actionSave = m_menuFile->addAction("&Save", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            bool result = false;
            ShaderManagementConsoleDocumentSystemRequestBus::BroadcastResult(result, &ShaderManagementConsoleDocumentSystemRequestBus::Events::SaveDocument, documentId);
            if (!result)
            {
                const QString documentPath = GetDocumentPath(documentId);
                const QString status = QString("Failed to save document: %1").arg(documentPath);
                m_statusMessage->setText(QString("<font color=\"Red\">%1</font>").arg(status));
            }
        }, QKeySequence::Save);

        m_actionSaveAsCopy = m_menuFile->addAction("Save &As...", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            ShaderManagementConsoleDocumentSystemRequestBus::BroadcastResult(result, &ShaderManagementConsoleDocumentSystemRequestBus::Events::SaveDocumentAsCopy,
                documentId, AtomToolsFramework::GetSaveFileInfo(documentPath).absoluteFilePath().toUtf8().constData());
            if (!result)
            {
                const QString status = QString("Failed to save document: %1").arg(documentPath);
                m_statusMessage->setText(QString("<font color=\"Red\">%1</font>").arg(status));
            }
        }, QKeySequence::SaveAs);

        m_actionSaveAll = m_menuFile->addAction("Save A&ll", [this]() {
            bool result = false;
            ShaderManagementConsoleDocumentSystemRequestBus::BroadcastResult(result, &ShaderManagementConsoleDocumentSystemRequestBus::Events::SaveAllDocuments);
            if (!result)
            {
                const QString status = QString("Failed to save documents.");
                m_statusMessage->setText(QString("<font color=\"Red\">%1</font>").arg(status));
            }
        });

        m_menuFile->addSeparator();

        m_actionClose = m_menuFile->addAction("&Close", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseDocument, documentId);
        }, QKeySequence::Close);

        m_actionCloseAll = m_menuFile->addAction("Close All", [this]() {
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseAllDocuments);
        });

        m_actionCloseOthers = m_menuFile->addAction("Close Others", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
        });

        m_menuFile->addSeparator();

        m_menuFile->addAction("Run Python...", [this]() {
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

        m_menuEdit = m_menuBar->addMenu("&Edit");

        m_actionUndo = m_menuEdit->addAction("&Undo", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            bool result = false;
            ShaderManagementConsoleDocumentRequestBus::EventResult(result, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::Undo);
            if (!result)
            {
                const QString documentPath = GetDocumentPath(documentId);
                const QString status = QString("Failed to perform Undo on document: %1").arg(documentPath);
                m_statusMessage->setText(QString("<font color=\"Red\">%1</font>").arg(status));
            }
        }, QKeySequence::Undo);

        m_actionRedo = m_menuEdit->addAction("&Redo", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            bool result = false;
            ShaderManagementConsoleDocumentRequestBus::EventResult(result, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::Redo);
            if (!result)
            {
                const QString documentPath = GetDocumentPath(documentId);
                const QString status = QString("Failed to perform Redo on document: %1").arg(documentPath);
                m_statusMessage->setText(QString("<font color=\"Red\">%1</font>").arg(status));
            }
        }, QKeySequence::Redo);

        m_menuEdit->addSeparator();

        m_actionSettings = m_menuEdit->addAction("&Settings...", [this]() {
        }, QKeySequence::Preferences);
        m_actionSettings->setEnabled(false);

        m_menuView = m_menuBar->addMenu("&View");

        m_actionAssetBrowser = m_menuView->addAction("&Asset Browser", [this]() {
            const AZStd::string label = "Asset Browser";
            SetDockWidgetVisible(label, !IsDockWidgetVisible(label));
        });

        m_actionPythonTerminal = m_menuView->addAction("Python &Terminal", [this]() {
            const AZStd::string label = "Python Terminal";
            SetDockWidgetVisible(label, !IsDockWidgetVisible(label));
        });


        m_menuView->addSeparator();

        m_actionPreviousTab = m_menuView->addAction("&Previous Tab", [this]() {
            SelectPreviousTab();
        }, Qt::CTRL | Qt::SHIFT | Qt::Key_Tab); //QKeySequence::PreviousChild is mapped incorrectly in Qt

        m_actionNextTab = m_menuView->addAction("&Next Tab", [this]() {
            SelectNextTab();
        }, Qt::CTRL | Qt::Key_Tab); //QKeySequence::NextChild works as expected but mirroring Previous

        m_menuHelp = m_menuBar->addMenu("&Help");

        m_actionHelp = m_menuHelp->addAction("&Help...", [this]() {
        });

        m_actionAbout = m_menuHelp->addAction("&About...", [this]() {
        });
    }

    void ShaderManagementConsoleWindow::CreateTabBar()
    {
        Base::CreateTabBar();

        // This signal will be triggered whenever a tab is added, removed, selected, clicked, dragged
        // When the last tab is removed tabIndex will be -1 and the document ID will be null
        // This should automatically clear the active document
        connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int tabIndex) {
            const AZ::Uuid documentId = GetDocumentIdFromTab(tabIndex);
            ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        });

        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int tabIndex) {
            const AZ::Uuid documentId = GetDocumentIdFromTab(tabIndex);
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseDocument, documentId);
        });
    }

    QString ShaderManagementConsoleWindow::GetDocumentPath(const AZ::Uuid& documentId) const
    {
        AZStd::string absolutePath;
        ShaderManagementConsoleDocumentRequestBus::EventResult(absolutePath, documentId, &ShaderManagementConsoleDocumentRequestBus::Handler::GetAbsolutePath);
        return absolutePath.c_str();
    }

    void ShaderManagementConsoleWindow::OpenTabContextMenu()
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
                ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentOpened, documentId);
            });
            tabMenu.addAction("Close", [this, clickedTabIndex]() {
                const AZ::Uuid documentId = GetDocumentIdFromTab(clickedTabIndex);
                ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseDocument, documentId);
            });
            auto closeOthersAction = tabMenu.addAction("Close Others", [this, clickedTabIndex]() {
                const AZ::Uuid documentId = GetDocumentIdFromTab(clickedTabIndex);
                ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
            });
            closeOthersAction->setEnabled(tabBar->count() > 1);
            tabMenu.exec(QCursor::pos());
        }
    }

    QStandardItemModel* ShaderManagementConsoleWindow::CreateDocumentContent(const AZ::Uuid& documentId)
    {
        AZStd::unordered_set<AZStd::string> optionNames;

        size_t shaderOptionCount = 0;
        ShaderManagementConsoleDocumentRequestBus::EventResult(shaderOptionCount, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionCount);

        for (size_t optionIndex = 0; optionIndex < shaderOptionCount; ++optionIndex)
        {
            AZ::RPI::ShaderOptionDescriptor shaderOptionDesc;
            ShaderManagementConsoleDocumentRequestBus::EventResult(shaderOptionDesc, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderOptionDescriptor, optionIndex);

            const char* optionName = shaderOptionDesc.GetName().GetCStr();
            optionNames.insert(optionName);
        }

        size_t shaderVariantCount = 0;
        ShaderManagementConsoleDocumentRequestBus::EventResult(shaderVariantCount, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantCount);

        auto model = new QStandardItemModel();
        model->setRowCount(static_cast<int>(shaderVariantCount));
        model->setColumnCount(static_cast<int>(optionNames.size()));

        int nameIndex = 0;
        for (const auto& optionName : optionNames)
        {
            model->setHeaderData(nameIndex++, Qt::Horizontal, optionName.c_str());
        }

        for (int variantIndex = 0; variantIndex < shaderVariantCount; ++variantIndex)
        {
            AZ::RPI::ShaderVariantListSourceData::VariantInfo shaderVariantInfo;
            ShaderManagementConsoleDocumentRequestBus::EventResult(shaderVariantInfo, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetShaderVariantInfo, variantIndex);

            model->setHeaderData(variantIndex, Qt::Vertical, QString::number(variantIndex));

            for (const auto& shaderOption : shaderVariantInfo.m_options)
            {
                AZ::Name optionName{ shaderOption.first };
                AZ::Name optionValue{ shaderOption.second };

                auto optionIt = optionNames.find(optionName.GetCStr());
                int optionIndex = static_cast<int>(AZStd::distance(optionNames.begin(), optionIt));

                QStandardItem* item = new QStandardItem(optionValue.GetCStr());
                model->setItem(variantIndex, optionIndex, item);
            }
        }

        return model;
    }
} // namespace ShaderManagementConsole

#include <Window/moc_ShaderManagementConsoleWindow.cpp>
