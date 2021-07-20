/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Name/Name.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Application/Application.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/PythonTerminal/ScriptTermDialog.h>
#include <AtomToolsFramework/Util/Util.h>

#include <Atom/Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>
#include <Atom/Window/ShaderManagementConsoleWindowNotificationBus.h>

#include <Source/Window/ShaderManagementConsoleWindow.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QCloseEvent>
#include <QVariant>
#include <QFileDialog>
#include <QWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    ShaderManagementConsoleWindow::ShaderManagementConsoleWindow(QWidget* parent /* = 0 */)
        : AzQtComponents::DockMainWindow(parent)
    {
        setWindowTitle("Shader Management Console");

        m_advancedDockManager = new AzQtComponents::FancyDocking(this);

        setDockNestingEnabled(true);
        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        m_menuBar = new QMenuBar(this);
        setMenuBar(m_menuBar);

        m_toolBar = new ShaderManagementConsoleToolBar(this);
        addToolBar(m_toolBar);

        m_centralWidget = new QWidget(this);
        m_tabWidget = new AzQtComponents::TabWidget(m_centralWidget);
        m_tabWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_tabWidget->setContentsMargins(0, 0, 0, 0);

        QVBoxLayout* vl = new QVBoxLayout(m_centralWidget);
        vl->setMargin(0);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->addWidget(m_tabWidget);
        m_centralWidget->setLayout(vl);
        setCentralWidget(m_centralWidget);

        SetupMenu();
        SetupTabs();

        m_assetBrowserDockWidget = new AzQtComponents::StyledDockWidget("Asset Browser");
        m_assetBrowserDockWidget->setObjectName(m_assetBrowserDockWidget->windowTitle());
        m_assetBrowserDockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        m_assetBrowser = new ShaderManagementConsoleBrowserWidget(m_assetBrowserDockWidget);
        m_assetBrowser->setMinimumSize(QSize(300, 300));
        m_assetBrowserDockWidget->setWidget(m_assetBrowser);
        addDockWidget(Qt::BottomDockWidgetArea, m_assetBrowserDockWidget);
        resizeDocks({ m_assetBrowserDockWidget }, { 400 }, Qt::Vertical);

        m_pythonTerminalDockWidget = new AzQtComponents::StyledDockWidget("Python Terminal");
        m_pythonTerminalDockWidget->setObjectName(m_pythonTerminalDockWidget->windowTitle());
        m_pythonTerminalDockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        m_pythonTerminal = new AzToolsFramework::CScriptTermDialog(m_pythonTerminalDockWidget);
        m_pythonTerminal->setMinimumSize(QSize(300, 300));
        m_pythonTerminalDockWidget->setWidget(m_pythonTerminal);
        addDockWidget(Qt::BottomDockWidgetArea, m_pythonTerminalDockWidget);
        resizeDocks({ m_pythonTerminalDockWidget }, { 400 }, Qt::Vertical);
        m_pythonTerminalDockWidget->setVisible(false);

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

        ShaderManagementConsoleWindowNotificationBus::Broadcast(&ShaderManagementConsoleWindowNotifications::OnShaderManagementConsoleWindowClosing);
    }

    void ShaderManagementConsoleWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isOpen, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsOpen);
        bool isSavable = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isSavable, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsSavable);
        bool canUndo = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(canUndo, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::CanUndo);
        bool canRedo = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(canRedo, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::CanRedo);

        // Update UI to display the new document
        AddTabForDocumentId(documentId);
        UpdateTabForDocumentId(documentId);

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
        m_actionPreferences->setEnabled(false);

        m_actionAssetBrowser->setEnabled(true);
        m_actionPythonTerminal->setEnabled(true);
        m_actionPreviousTab->setEnabled(m_tabWidget->count() > 1);
        m_actionNextTab->setEnabled(m_tabWidget->count() > 1);

        m_actionHelp->setEnabled(false);
        m_actionAbout->setEnabled(false);

        activateWindow();
        raise();
    }

    void ShaderManagementConsoleWindow::OnDocumentClosed(const AZ::Uuid& documentId)
    {
        RemoveTabForDocumentId(documentId);
    }

    void ShaderManagementConsoleWindow::OnDocumentModified(const AZ::Uuid& documentId)
    {
        UpdateTabForDocumentId(documentId);
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
        UpdateTabForDocumentId(documentId);
    }

    void ShaderManagementConsoleWindow::SetupMenu()
    {
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

        m_actionClose = m_menuFile->addAction("&Close", [this]() {
            CloseDocumentForTab(m_tabWidget->currentIndex());
        }, QKeySequence::Close);

        m_actionCloseAll = m_menuFile->addAction("Close All", [this]() {
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseAllDocuments);
        });

        m_actionCloseOthers = m_menuFile->addAction("Close Others", [this]() {
            CloseAllExceptDocumentForTab(m_tabWidget->currentIndex());
        });

        m_menuFile->addSeparator();

        m_actionSave = m_menuFile->addAction("&Save", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::SaveDocument, documentId);
        }, QKeySequence::Save);

        m_actionSaveAsCopy = m_menuFile->addAction("Save &As...", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::SaveDocumentAsCopy, documentId);
        }, QKeySequence::SaveAs);

        m_actionSaveAll = m_menuFile->addAction("Save A&ll", [this]() {
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::SaveAllDocuments);
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
            ShaderManagementConsoleDocumentRequestBus::Event(documentId, &ShaderManagementConsoleDocumentRequestBus::Events::Undo);
        }, QKeySequence::Undo);

        m_actionRedo = m_menuEdit->addAction("&Redo", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            ShaderManagementConsoleDocumentRequestBus::Event(documentId, &ShaderManagementConsoleDocumentRequestBus::Events::Redo);
        }, QKeySequence::Redo);

        m_menuEdit->addSeparator();

        m_actionPreferences = m_menuEdit->addAction("&Preferences...", [this]() {
        }, QKeySequence::Preferences);
        m_actionPreferences->setEnabled(false);

        m_menuView = m_menuBar->addMenu("&View");

        m_actionAssetBrowser = m_menuView->addAction("&Asset Browser", [this]() {
            m_assetBrowserDockWidget->setVisible(!m_assetBrowserDockWidget->isVisible());
        });

        m_actionPythonTerminal = m_menuView->addAction("Python &Terminal", [this]() {
            m_pythonTerminalDockWidget->setVisible(!m_pythonTerminalDockWidget->isVisible());
            if (m_pythonTerminalDockWidget->isVisible())
            {
                // reposition console window on the bottom, otherwise it gets docked in some weird spot...
                addDockWidget(Qt::BottomDockWidgetArea, m_pythonTerminalDockWidget);
            }
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

    void ShaderManagementConsoleWindow::SetupTabs()
    {
        // The tab bar should only be visible if it has active documents
        m_tabWidget->setVisible(false);
        m_tabWidget->setTabBarAutoHide(false);
        m_tabWidget->setMovable(true);
        m_tabWidget->setTabsClosable(true);
        m_tabWidget->setUsesScrollButtons(true);

        // Add context menu for right-clicking on tabs
        m_tabWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
        connect(m_tabWidget, &QWidget::customContextMenuRequested, this, [this]() {
            OpenTabContextMenu();
        });

        // This signal will be triggered whenever a tab is added, removed, selected, clicked, dragged
        // When the last tab is removed tabIndex will be -1 and the document ID will be null
        // This should automatically clear the active document
        connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int tabIndex) {
            SelectDocumentForTab(tabIndex);
        });

        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int tabIndex) {
            CloseDocumentForTab(tabIndex);
        });
    }

    void ShaderManagementConsoleWindow::AddTabForDocumentId(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        ShaderManagementConsoleDocumentRequestBus::EventResult(isOpen, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsOpen);

        if (documentId.IsNull() || !isOpen)
        {
            return;
        }

        // Blocking signals from the tab bar so the currentChanged signal is not sent while a document is already being opened.
        // This prevents the OnDocumentOpened notification from being sent recursively.
        const QSignalBlocker blocker(m_tabWidget);

        // If a tab for this document already exists then select it instead of creating a new one
        for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
        {
            if (documentId == GetDocumentIdFromTab(tabIndex))
            {
                m_tabWidget->setCurrentIndex(tabIndex);
                m_tabWidget->repaint();
                return;
            }
        }

        // Create a new tab for the document ID and assign it's label to the file name of the document.
        AZStd::string absolutePath;
        ShaderManagementConsoleDocumentRequestBus::EventResult(absolutePath, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);

        AZStd::string filename;
        AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);

        // The document tab contains a table view.
        auto tableView = new QTableView(m_centralWidget);
        tableView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

        auto model = new QStandardItemModel();
        tableView->setModel(model);

        const int tabIndex = m_tabWidget->addTab(tableView, filename.c_str());

        // The user can manually reorder tabs which will invalidate any association by index.
        // We need to store the document ID with the tab using the tab instead of a separate mapping.
        m_tabWidget->tabBar()->setTabData(tabIndex, QVariant(documentId.ToString<QString>()));
        m_tabWidget->setTabToolTip(tabIndex, absolutePath.c_str());
        m_tabWidget->setCurrentIndex(tabIndex);
        m_tabWidget->setVisible(true);
        m_tabWidget->repaint();

        CreateDocumentContent(documentId, model);
    }

    void ShaderManagementConsoleWindow::RemoveTabForDocumentId(const AZ::Uuid& documentId)
    {
        // We are not blocking signals here because we want closing tabs to close the associated document
        // and automatically select the next document. 
        for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
        {
            if (documentId == GetDocumentIdFromTab(tabIndex))
            {
                m_tabWidget->removeTab(tabIndex);
                m_tabWidget->setVisible(m_tabWidget->count() > 0);
                m_tabWidget->repaint();
                break;
            }
        }
    }

    void ShaderManagementConsoleWindow::UpdateTabForDocumentId(const AZ::Uuid& documentId)
    {
        // Whenever a document is opened, saved, or modified we need to update the tab label
        if (!documentId.IsNull())
        {
            // Because tab order and indexes can change from user interactions, we cannot store a map
            // between a tab index and document ID.
            // We must iterate over all of the tabs to find the one associated with this document.
            for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
            {
                if (documentId == GetDocumentIdFromTab(tabIndex))
                {
                    AZStd::string absolutePath;
                    ShaderManagementConsoleDocumentRequestBus::EventResult(absolutePath, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::GetAbsolutePath);

                    AZStd::string filename;
                    AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);

                    bool isModified = false;
                    ShaderManagementConsoleDocumentRequestBus::EventResult(isModified, documentId, &ShaderManagementConsoleDocumentRequestBus::Events::IsModified);

                    // We use an asterisk appended to the file name to denote modified document
                    if (isModified)
                    {
                        filename += " *";
                    }

                    m_tabWidget->setTabText(tabIndex, filename.c_str());
                    m_tabWidget->setTabToolTip(tabIndex, absolutePath.c_str());
                    m_tabWidget->repaint();
                    break;
                }
            }
        }
    }

    AZ::Uuid ShaderManagementConsoleWindow::GetDocumentIdFromTab(const int tabIndex) const
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
                SelectDocumentForTab(clickedTabIndex);
            });
            tabMenu.addAction("Close", [this, clickedTabIndex]() {
                CloseDocumentForTab(clickedTabIndex);
            });
            auto closeOthersAction = tabMenu.addAction("Close Others", [this, clickedTabIndex]() {
                CloseAllExceptDocumentForTab(clickedTabIndex);
            });
            closeOthersAction->setEnabled(tabBar->count() > 1);
            tabMenu.exec(QCursor::pos());
        }
    }

    void ShaderManagementConsoleWindow::SelectPreviousTab()
    {
        if (m_tabWidget->count() > 1)
        {
            // Adding count to wrap around when index <= 0
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + m_tabWidget->count() - 1) % m_tabWidget->count());
        }
    }

    void ShaderManagementConsoleWindow::SelectNextTab()
    {
        if (m_tabWidget->count() > 1)
        {
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + 1) % m_tabWidget->count());
        }
    }

    void ShaderManagementConsoleWindow::SelectDocumentForTab(const int tabIndex)
    {
        const AZ::Uuid documentId = GetDocumentIdFromTab(tabIndex);
        ShaderManagementConsoleDocumentNotificationBus::Broadcast(&ShaderManagementConsoleDocumentNotificationBus::Events::OnDocumentOpened, documentId);
    }

    void ShaderManagementConsoleWindow::CloseDocumentForTab(const int tabIndex)
    {
        const AZ::Uuid documentId = GetDocumentIdFromTab(tabIndex);
        ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseDocument, documentId);
    }

    void ShaderManagementConsoleWindow::CloseAllExceptDocumentForTab(const int tabIndex)
    {
        AZStd::vector<AZ::Uuid> documentIdsToClose;
        documentIdsToClose.reserve(m_tabWidget->count());
        const AZ::Uuid documentIdToKeepOpen = GetDocumentIdFromTab(tabIndex);
        for (int tabI = 0; tabI < m_tabWidget->count(); ++tabI)
        {
            const AZ::Uuid documentId = GetDocumentIdFromTab(tabI);
            if (documentId != documentIdToKeepOpen)
            {
                documentIdsToClose.push_back(documentId);
            }
        }

        for (const AZ::Uuid& documentId : documentIdsToClose)
        {
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::CloseDocument, documentId);
        }
    }

    void ShaderManagementConsoleWindow::CreateDocumentContent(const AZ::Uuid& documentId, QStandardItemModel* model)
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
    }
} // namespace ShaderManagementConsole

#include <Source/Window/moc_ShaderManagementConsoleWindow.cpp>
