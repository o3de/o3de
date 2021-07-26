/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/MainWindow.h>
#include <source/ui/ui_MainWindow.h>
#include <source/utils/GUIApplicationManager.h>

#include <AzCore/std/utils.h>
#include <AzToolsFramework/UI/Logging/LogTableItemDelegate.h>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDateTime>
#include <QDesktopServices>

namespace AssetBundler
{

    MainWindow::MainWindow(AssetBundler::GUIApplicationManager* guiApplicationManager, QWidget* parent)
        : QMainWindow(parent)
        , m_guiApplicationManager(guiApplicationManager)
    {
        m_ui.reset(new Ui::MainWindow);
        m_ui->setupUi(this);

        m_ui->verticalLayout->setContentsMargins(0, 0, 0, 0);

        // Set up Log
        m_logModel.reset(new AzToolsFramework::Logging::LogTableModel(this));
        m_ui->logTableView->setModel(m_logModel.data());
        m_ui->logTableView->setIndentation(0);

        m_ui->logTableView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_ui->logTableView, &QWidget::customContextMenuRequested, this, &MainWindow::ShowLogContextMenu);

        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        // Create the Unsaved Changes Popup
        m_unsavedChangesMsgBox.reset(new QMessageBox(this));
        m_unsavedChangesMsgBox->setText(tr("There are unsaved changes."));
        m_unsavedChangesMsgBox->setInformativeText(tr("Would you like to save all changes before quitting?"));
        m_unsavedChangesMsgBox->setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        m_unsavedChangesMsgBox->setDefaultButton(QMessageBox::Save);

        // Set up Quit functionality
        m_ui->actionClose->setShortcut(Qt::Key_Q | Qt::CTRL);
        m_ui->actionClose->setMenuRole(QAction::QuitRole);
        connect(m_ui->actionClose, SIGNAL(triggered()), this, SLOT(close()));
        this->addAction(m_ui->actionClose);

        // Set up Tabs
        AssetBundlerTabWidget::InitAssetBundlerSettings(m_guiApplicationManager->GetCurrentProjectFolder().c_str());

        m_seedListTab.reset(new SeedTabWidget(
            this,
            m_guiApplicationManager,
            QString(m_guiApplicationManager->GetAssetBundlingFolder().c_str())));
        m_ui->tabWidget->addTab(m_seedListTab.data(), m_seedListTab->GetTabTitle());

        m_assetListTab.reset(new AssetListTabWidget(this, m_guiApplicationManager));
        m_ui->tabWidget->addTab(m_assetListTab.data(), m_assetListTab->GetTabTitle());

        m_rulesTab.reset(new RulesTabWidget(this, m_guiApplicationManager));
        m_ui->tabWidget->addTab(m_rulesTab.data(), m_rulesTab->GetTabTitle());

        m_bundleListTab.reset(new BundleListTabWidget(this, m_guiApplicationManager));
        m_ui->tabWidget->addTab(m_bundleListTab.data(), m_bundleListTab->GetTabTitle());

        // Set up link to documentation
        QAction* supportAction = new QAction(QIcon(":/stylesheet/img/help.svg"), "", this);
        connect(supportAction, &QAction::triggered, this, &MainWindow::OnSupportClicked);
        connect(m_ui->actionDocumentation, &QAction::triggered, this, &MainWindow::OnSupportClicked);
        m_ui->tabWidget->setActionToolBarVisible();
        m_ui->tabWidget->addAction(supportAction);

        // Set up Save functionality
        m_ui->actionSave->setShortcut(Qt::Key_S | Qt::CTRL);
        connect(m_ui->actionSave, &QAction::triggered, this, &MainWindow::SaveCurrentSelection);

        m_ui->actionSaveAll->setShortcut(Qt::Key_S | Qt::CTRL | Qt::SHIFT);
        connect(m_ui->actionSaveAll, &QAction::triggered, this, &MainWindow::SaveAll);
    }

    MainWindow::~MainWindow()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        m_guiApplicationManager = nullptr;
    }

    void MainWindow::Activate()
    {
        m_seedListTab->Activate();
        m_rulesTab->Activate();
        m_assetListTab->Activate();
        m_bundleListTab->Activate();
    }

    void MainWindow::ApplyConfig()
    {
        const GUIApplicationManager::Config& config = m_guiApplicationManager->GetConfig();
        // Event Log Details
        m_ui->logTableView->header()->resizeSection(AzToolsFramework::Logging::LogTableModel::ColumnType, config.logTypeColumnWidth);
        m_ui->logTableView->header()->resizeSection(AzToolsFramework::Logging::LogTableModel::ColumnWindow, config.logSourceColumnWidth);

        m_seedListTab->ApplyConfig();
        m_assetListTab->ApplyConfig();
        m_rulesTab->ApplyConfig();
        m_bundleListTab->ApplyConfig();
    }

    void MainWindow::WriteToLog(const AZStd::string& message, AzToolsFramework::Logging::LogLine::LogType logType)
    {
        WriteToLog(message.c_str(), logType);
    }

    void MainWindow::WriteToLog(const QString& message, AzToolsFramework::Logging::LogLine::LogType logType)
    {
        WriteToLog(message.toUtf8().data(), logType);
    }

    void MainWindow::WriteToLog(const char* message, AzToolsFramework::Logging::LogLine::LogType logType)
    {
        WriteToLog(message, "AssetBundler", logType);
    }

    void MainWindow::WriteToLog(const char* message, const char* window, AzToolsFramework::Logging::LogLine::LogType logType)
    {
        m_logModel->AppendLine(
            AzToolsFramework::Logging::LogLine(
                message,
                window,
                logType,
                QDateTime::currentMSecsSinceEpoch()));
        m_ui->logTableView->scrollToBottom();
    }

    bool MainWindow::OnPreError(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        WriteToLog(message, window, AzToolsFramework::Logging::LogLine::LogType::TYPE_ERROR);
        return true;
    }

    bool MainWindow::OnPreWarning(const char* window, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* message)
    {
        WriteToLog(message, window, AzToolsFramework::Logging::LogLine::LogType::TYPE_WARNING);
        return true;
    }

    bool MainWindow::OnPrintf(const char* /*window*/, const char* /*message*/)
    {
        return true;
    }

    void MainWindow::ShowWindow()
    {
        AzQtComponents::bringWindowToTop(this);
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        if (!HasUnsavedChanges())
        {
            // No need to ask the user if they want to quit when there are no unsaved changes
            event->accept();
            return;
        }

        int unsavedChangesResult = m_unsavedChangesMsgBox->exec();

        switch (unsavedChangesResult)
        {
        case QMessageBox::Save:
            // Save All was clicked
            SaveAll();
            event->accept();
            break;
        case QMessageBox::Discard:
            // Don't Save was clicked
            event->accept();
            break;
        case QMessageBox::Cancel:
            // Cancel was clicked
            event->ignore();
            break;
        default:
            // should never be reached
            AZ_Assert(false, "No result was returned by the Unsaved Changes Message Box!");
            break;
        }
    }

    void MainWindow::OnSupportClicked()
    {
        QDesktopServices::openUrl(
            QStringLiteral("https://o3de.org/docs/user-guide/packaging/asset-bundler/"));
    }

    void MainWindow::ShowLogContextMenu(const QPoint& pos)
    {
        QModelIndex index = m_ui->logTableView->indexAt(pos);

        QMenu menu;

        QAction* action = menu.addAction(tr("Copy line"), this, [&]()
            {
                QApplication::clipboard()->setText(index.data(AzToolsFramework::Logging::LogTableModel::LogLineTextRole).toString());
            });
        action->setEnabled(index.isValid());

        menu.exec(m_ui->logTableView->mapToGlobal(pos));
    }

    bool MainWindow::HasUnsavedChanges()
    {
        return m_seedListTab->HasUnsavedChanges() || m_rulesTab->HasUnsavedChanges();
    }

    void MainWindow::SaveCurrentSelection()
    {
        switch (m_ui->tabWidget->currentIndex())
        {
        case TabIndex::Seeds:
            m_seedListTab->SaveCurrentSelection();
            break;
        case TabIndex::Rules:
            m_rulesTab->SaveCurrentSelection();
            break;
        default:
            break;
        }
    }

    void MainWindow::SaveAll()
    {
        m_seedListTab->SaveAll();
        m_rulesTab->SaveAll();
    }
} // namespace AssetBundler

#include <source/ui/moc_MainWindow.cpp>
