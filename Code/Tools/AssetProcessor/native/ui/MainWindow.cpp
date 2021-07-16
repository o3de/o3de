/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "MainWindow.h"

#include "AssetTreeFilterModel.h"
#include "AssetTreeItem.h"
#include "ConnectionEditDialog.h"
#include "ProductAssetTreeItemData.h"
#include "ProductAssetTreeModel.h"
#include "SourceAssetTreeItemData.h"
#include "SourceAssetTreeModel.h"

#include <AzFramework/Asset/AssetSystemBus.h>


#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/LineEdit.h>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>

#include <native/resourcecompiler/JobsModel.h>
#include "native/ui/ui_MainWindow.h"
#include "native/ui/JobTreeViewItemDelegate.h"


#include "../utilities/GUIApplicationManager.h"
#include "../utilities/ApplicationServer.h"
#include "../connection/connectionManager.h"
#include "../connection/connection.h"
#include "../resourcecompiler/rccontroller.h"
#include "../resourcecompiler/RCJobSortFilterProxyModel.h"
#include "../shadercompiler/shadercompilerModel.h"


#include <QClipboard>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include <QKeyEvent>

static const char* g_showContextDetailsKey = "ShowContextDetailsTable";
static const QString g_jobFilteredSearchWidgetState = QStringLiteral("jobFilteredSearchWidget");
static const qint64 AssetTabFilterUpdateIntervalMs = 5000;

MainWindow::Config MainWindow::loadConfig(QSettings& settings)
{
    using namespace AzQtComponents;

    Config config = defaultConfig();

    // Asset Status
    {
        ConfigHelpers::GroupGuard assetStatus(&settings, QStringLiteral("AssetStatus"));
        ConfigHelpers::read<int>(settings, QStringLiteral("JobStatusColumnWidth"), config.jobStatusColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobSourceColumnWidth"), config.jobSourceColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobPlatformColumnWidth"), config.jobPlatformColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobKeyColumnWidth"), config.jobKeyColumnWidth);
        ConfigHelpers::read<int>(settings, QStringLiteral("JobCompletedColumnWidth"), config.jobCompletedColumnWidth);
    }

    // Event Log Details
    {
        ConfigHelpers::GroupGuard eventLogDetails(&settings, QStringLiteral("EventLogDetails"));
        ConfigHelpers::read<int>(settings, QStringLiteral("LogTypeColumnWidth"), config.logTypeColumnWidth);
    }

    return config;
}

MainWindow::Config MainWindow::defaultConfig()
{
    // These are used if the values can't be read from AssetProcessorConfig.ini.
    Config config;

    config.jobStatusColumnWidth = 100;
    config.jobSourceColumnWidth = 160;
    config.jobPlatformColumnWidth = 100;
    config.jobKeyColumnWidth = 120;
    config.jobCompletedColumnWidth = 160;

    config.logTypeColumnWidth = 150;

    return config;
}

MainWindow::MainWindow(GUIApplicationManager* guiApplicationManager, QWidget* parent)
    : QMainWindow(parent)
    , m_guiApplicationManager(guiApplicationManager)
    , m_jobSortFilterProxy(new AssetProcessor::JobSortFilterProxyModel(this))
    , m_logSortFilterProxy(new LogSortFilterProxy(this))
    , m_jobsModel(new AssetProcessor::JobsModel(this))
    , m_logsModel(new AzToolsFramework::Logging::LogTableModel(this))
    , ui(new Ui::MainWindow)
    , m_loggingPanel(nullptr)
    , m_fileSystemWatcher(new QFileSystemWatcher(this))
{
    ui->setupUi(this);

    // Don't show the "Filter by:" text on this filter widget
    ui->jobFilteredSearchWidget->clearLabelText();
    ui->detailsFilterWidget->clearLabelText();
    ui->timerContainerWidget->setVisible(false);
}

bool MainWindow::eventFilter(QObject* /*obj*/, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key::Key_Space)
        {
            // Stop space key from opening filter list.
            return true;
        }
    }
    return false;
}

void MainWindow::Activate()
{
    using namespace AssetProcessor;

    m_sharedDbConnection = AZStd::shared_ptr<AzToolsFramework::AssetDatabase::AssetDatabaseConnection>(aznew AzToolsFramework::AssetDatabase::AssetDatabaseConnection());
    m_sharedDbConnection->OpenDatabase();

    ui->projectLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Project"))
        .arg(QDir{m_guiApplicationManager->GetProjectPath()}.absolutePath()));

    ui->rootLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Root"))
        .arg(m_guiApplicationManager->GetSystemRoot().absolutePath()));

    ui->portLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Processor port"))
        .arg(m_guiApplicationManager->GetApplicationServer()->GetServerListeningPort()));

    connect(ui->supportButton, &QPushButton::clicked, this, &MainWindow::OnSupportClicked);

    ui->buttonList->addTab(QStringLiteral("Jobs"));
    ui->buttonList->addTab(QStringLiteral("Assets"));
    ui->buttonList->addTab(QStringLiteral("Logs"));
    ui->buttonList->addTab(QStringLiteral("Shaders"));
    ui->buttonList->addTab(QStringLiteral("Connections"));
    ui->buttonList->addTab(QStringLiteral("Tools"));

    connect(ui->buttonList, &AzQtComponents::SegmentBar::currentChanged, ui->dialogStack, &QStackedWidget::setCurrentIndex);
    const int startIndex = static_cast<int>(DialogStackIndex::Jobs);
    ui->dialogStack->setCurrentIndex(startIndex);
    ui->buttonList->setCurrentIndex(startIndex);

    //Connection view
    ui->connectionTreeView->setModel(m_guiApplicationManager->GetConnectionManager());
    ui->connectionTreeView->setEditTriggers(QAbstractItemView::CurrentChanged);
    ui->connectionTreeView->header()->setSectionResizeMode(ConnectionManager::IdColumn, QHeaderView::Stretch);
    ui->connectionTreeView->header()->setSectionResizeMode(ConnectionManager::AutoConnectColumn, QHeaderView::Fixed);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::StatusColumn, 160);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::IpColumn, 150);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::PortColumn, 60);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::PlatformColumn, 60);
    ui->connectionTreeView->header()->resizeSection(ConnectionManager::AutoConnectColumn, 60);
    
    ui->connectionTreeView->header()->setStretchLastSection(false);
    connect(ui->connectionTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::OnConnectionSelectionChanged);


    ui->editConnectionButton->setEnabled(false);
    ui->removeConnectionButton->setEnabled(false);
    connect(ui->editConnectionButton, &QPushButton::clicked, this, &MainWindow::OnEditConnection);
    connect(ui->addConnectionButton, &QPushButton::clicked, this, &MainWindow::OnAddConnection);
    connect(ui->removeConnectionButton, &QPushButton::clicked, this, &MainWindow::OnRemoveConnection);
    connect(ui->connectionTreeView, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        EditConnection(index);
    });

    ui->connectionTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->connectionTreeView, &QTreeView::customContextMenuRequested, this, &MainWindow::OnConnectionContextMenu);

    //allowed list connections
    connect(m_guiApplicationManager->GetConnectionManager(), &ConnectionManager::FirstTimeAddedToRejctedList, this, &MainWindow::FirstTimeAddedToRejctedList);
    connect(m_guiApplicationManager->GetConnectionManager(), &ConnectionManager::SyncAllowedListAndRejectedList, this, &MainWindow::SyncAllowedListAndRejectedList);
    connect(ui->allowListAllowedListConnectionsListView, &QListView::clicked, this, &MainWindow::OnAllowedListConnectionsListViewClicked);
    ui->allowListAllowedListConnectionsListView->setModel(&m_allowedListAddresses);
    connect(ui->allowedListRejectedConnectionsListView, &QListView::clicked, this, &MainWindow::OnRejectedConnectionsListViewClicked);
    ui->allowedListRejectedConnectionsListView->setModel(&m_rejectedAddresses);
    
    connect(ui->allowedListEnableCheckBox, &QCheckBox::toggled, this, &MainWindow::OnAllowedListCheckBoxToggled);
    
    connect(ui->allowedListAddHostNameToolButton, &QToolButton::clicked, this, &MainWindow::OnAddHostNameAllowedListButtonClicked);
    connect(ui->allowedListAddIPToolButton, &QPushButton::clicked, this, &MainWindow::OnAddIPAllowedListButtonClicked);
    
    connect(ui->allowedListToAllowedListToolButton, &QPushButton::clicked, this, &MainWindow::OnToAllowedListButtonClicked);
    connect(ui->allowedListToRejectedListToolButton, &QToolButton::clicked, this, &MainWindow::OnToRejectedListButtonClicked);

    //set the input validator for ip addresses on the add address line edit
    QRegExp validHostName("^((?=.{1,255}$)[0-9A-Za-z](?:(?:[0-9A-Za-z]|\\b-){0,61}[0-9A-Za-z])?(?:\\.[0-9A-Za-z](?:(?:[0-9A-Za-z]|\\b-){0,61}[0-9A-Za-z])?)*\\.?)$");
    QRegExp validIP("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])(\\/([0-9]|[1-2][0-9]|3[0-2]))?$|^((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]d|1dd|[1-9]?d)(.(25[0-5]|2[0-4]d|1dd|[1-9]?d)){3}))|:)))(%.+)?s*(\\/([0-9]|[1-9][0-9]|1[0-1][0-9]|12[0-8]))?$");

    QRegExpValidator* hostNameValidator = new QRegExpValidator(validHostName, this);
    ui->allowedListAddHostNameLineEdit->setValidator(hostNameValidator);
    
    QRegExpValidator* ipValidator = new QRegExpValidator(validIP, this);
    ui->allowedListAddIPLineEdit->setValidator(ipValidator);

    //Job view
    m_jobSortFilterProxy->setSourceModel(m_jobsModel);
    m_jobSortFilterProxy->setDynamicSortFilter(true);
    m_jobSortFilterProxy->setFilterKeyColumn(JobsModel::ColumnSource);

    ui->jobTreeView->setModel(m_jobSortFilterProxy);
    ui->jobTreeView->setSortingEnabled(true);
    ui->jobTreeView->header()->setDefaultAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    ui->jobTreeView->setItemDelegate(new AssetProcessor::JobTreeViewItemDelegate(ui->jobTreeView));

    ui->jobTreeView->setToolTip(tr("Click to view Job Log"));

    ui->detailsFilterWidget->SetTypeFilterVisible(true);
    connect(ui->detailsFilterWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, m_logSortFilterProxy,
        static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&LogSortFilterProxy::setFilterRegExp));
    connect(ui->detailsFilterWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged, m_logSortFilterProxy, &LogSortFilterProxy::onTypeFilterChanged);

    // add filters for each logging type
    ui->detailsFilterWidget->AddTypeFilter("Status", "Debug", AzToolsFramework::Logging::LogLine::TYPE_DEBUG);
    ui->detailsFilterWidget->AddTypeFilter("Status", "Message", AzToolsFramework::Logging::LogLine::TYPE_MESSAGE);
    ui->detailsFilterWidget->AddTypeFilter("Status", "Warning", AzToolsFramework::Logging::LogLine::TYPE_WARNING);
    ui->detailsFilterWidget->AddTypeFilter("Status", "Error", AzToolsFramework::Logging::LogLine::TYPE_ERROR);

    m_logSortFilterProxy->setDynamicSortFilter(true);
    m_logSortFilterProxy->setSourceModel(m_logsModel);
    m_logSortFilterProxy->setFilterKeyColumn(AzToolsFramework::Logging::LogTableModel::ColumnMessage);
    m_logSortFilterProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    
    ui->jobLogTableView->setModel(m_logSortFilterProxy);
    ui->jobLogTableView->setItemDelegate(new AzToolsFramework::Logging::LogTableItemDelegate(ui->jobLogTableView));
    ui->jobLogTableView->setExpandOnSelection();

    connect(ui->jobTreeView->header(), &QHeaderView::sortIndicatorChanged, m_jobSortFilterProxy, &AssetProcessor::JobSortFilterProxyModel::sort);
    connect(ui->jobTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::JobSelectionChanged);
    ui->jobFilteredSearchWidget->SetTypeFilterVisible(true);
    ui->jobFilteredSearchWidget->assetTypeSelectorButton()->installEventFilter(this);

    // listen for job status changes in order to update the log view with the latest log data
    connect(m_guiApplicationManager->GetRCController(), &AssetProcessor::RCController::JobStatusChanged, this, &MainWindow::JobStatusChanged);

    ui->jobContextLogTableView->setModel(new AzToolsFramework::Logging::ContextDetailsLogTableModel(ui->jobContextLogTableView));
    ui->jobContextLogTableView->setItemDelegate(new AzQtComponents::TableViewItemDelegate(ui->jobContextLogTableView));
    ui->jobContextLogTableView->setExpandOnSelection();

    // Don't collapse the jobContextContainer
    ui->jobDialogSplitter->setCollapsible(2, false);

    // Note: the settings can't be used in ::MainWindow(), because the application name
    // hasn't been set up and therefore the settings will load from somewhere different than later
    // on.
    QSettings settings;
    bool showContextDetails = settings.value(g_showContextDetailsKey, false).toBool();
    ui->jobContextLogDetails->setChecked(showContextDetails);

    // The context log details are shown by default, so only do anything with them on startup
    // if they should be hidden based on the loaded settings
    if (!showContextDetails)
    {
        SetContextLogDetailsVisible(showContextDetails);
    }

    connect(ui->jobContextLogDetails, &QCheckBox::toggled, this, [this](bool visible) {
        SetContextLogDetailsVisible(visible);

        QSettings settingsObj;
        settingsObj.setValue(g_showContextDetailsKey, visible);
    });

    connect(ui->jobLogTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::JobLogSelectionChanged);

    const auto statuses =
    {
        AzToolsFramework::AssetSystem::JobStatus::Failed,
        AzToolsFramework::AssetSystem::JobStatus::Completed,
        AzToolsFramework::AssetSystem::JobStatus::Queued,
        AzToolsFramework::AssetSystem::JobStatus::InProgress
    };

    const auto category = tr("Status");
    for (const auto& status : statuses)
    {
        ui->jobFilteredSearchWidget->AddTypeFilter(category, JobsModel::GetStatusInString(status, 0, 0),
            QVariant::fromValue(status));
    }

    AssetProcessor::CustomJobStatusFilter customFilter{ true };
    ui->jobFilteredSearchWidget->AddTypeFilter(category, "Completed w/ Warnings", QVariant::fromValue(customFilter));

    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged,
        m_jobSortFilterProxy, &AssetProcessor::JobSortFilterProxyModel::OnJobStatusFilterChanged);
    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        m_jobSortFilterProxy,
        static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetProcessor::JobSortFilterProxyModel::setFilterRegExp));
    {
        QSettings settingsObj(this);
        ui->jobFilteredSearchWidget->readSettings(settingsObj, g_jobFilteredSearchWidgetState);
    }
    auto writeJobFilterSettings = [this]()
    {
        QSettings settingsObj(this);
        ui->jobFilteredSearchWidget->writeSettings(settingsObj, g_jobFilteredSearchWidgetState);
    };
    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged,
        this, writeJobFilterSettings);
    connect(ui->jobFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        this, writeJobFilterSettings);

    //Shader view
    ui->shaderTreeView->setModel(m_guiApplicationManager->GetShaderCompilerModel());
    ui->shaderTreeView->header()->resizeSection(ShaderCompilerModel::ColumnTimeStamp, 80);
    ui->shaderTreeView->header()->resizeSection(ShaderCompilerModel::ColumnServer, 40);
    ui->shaderTreeView->header()->resizeSection(ShaderCompilerModel::ColumnError, 220);
    ui->shaderTreeView->header()->setSectionResizeMode(ShaderCompilerModel::ColumnError, QHeaderView::Stretch);
    ui->shaderTreeView->header()->setStretchLastSection(false);

    // Asset view
    m_sourceAssetTreeFilterModel = new AssetProcessor::AssetTreeFilterModel(this);
    m_sourceModel = new AssetProcessor::SourceAssetTreeModel(m_sharedDbConnection, this);
    m_sourceModel->Reset();
    m_sourceAssetTreeFilterModel->setSourceModel(m_sourceModel);
    ui->SourceAssetsTreeView->setModel(m_sourceAssetTreeFilterModel);
    connect(ui->assetDataFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        m_sourceAssetTreeFilterModel, static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetTreeFilterModel::FilterChanged));

    m_productAssetTreeFilterModel = new AssetProcessor::AssetTreeFilterModel(this);
    m_productModel = new AssetProcessor::ProductAssetTreeModel(m_sharedDbConnection, this);
    m_productModel->Reset();
    m_productAssetTreeFilterModel->setSourceModel(m_productModel);
    ui->ProductAssetsTreeView->setModel(m_productAssetTreeFilterModel);
    connect(ui->assetDataFilteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        m_productAssetTreeFilterModel, static_cast<void (QSortFilterProxyModel::*)(const QString&)>(&AssetTreeFilterModel::FilterChanged));

    AzQtComponents::StyleManager::setStyleSheet(ui->sourceAssetDetailsPanel, QStringLiteral("style:AssetProcessor.qss"));
    AzQtComponents::StyleManager::setStyleSheet(ui->productAssetDetailsPanel, QStringLiteral("style:AssetProcessor.qss"));

    ui->sourceAssetDetailsPanel->RegisterAssociatedWidgets(
        ui->SourceAssetsTreeView,
        m_sourceModel,
        m_sourceAssetTreeFilterModel,
        ui->ProductAssetsTreeView,
        m_productModel,
        m_productAssetTreeFilterModel,
        ui->assetsTabWidget);
    ui->productAssetDetailsPanel->RegisterAssociatedWidgets(
        ui->SourceAssetsTreeView,
        m_sourceModel,
        m_sourceAssetTreeFilterModel,
        ui->ProductAssetsTreeView,
        m_productModel,
        m_productAssetTreeFilterModel,
        ui->assetsTabWidget);
    ui->productAssetDetailsPanel->SetScannerInformation(ui->missingDependencyScanResults, m_guiApplicationManager->GetAssetProcessorManager()->GetDatabaseConnection());
    ui->productAssetDetailsPanel->SetScanQueueEnabled(false);

    connect(ui->SourceAssetsTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, ui->sourceAssetDetailsPanel, &SourceAssetDetailsPanel::AssetDataSelectionChanged);
    connect(ui->ProductAssetsTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, ui->productAssetDetailsPanel, &ProductAssetDetailsPanel::AssetDataSelectionChanged);
    connect(ui->assetsTabWidget, &QTabWidget::currentChanged, this, &MainWindow::OnAssetTabChange);

    ui->ProductAssetsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->ProductAssetsTreeView, &QWidget::customContextMenuRequested, this, &MainWindow::ShowProductAssetContextMenu);

    ui->SourceAssetsTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->SourceAssetsTreeView, &QWidget::customContextMenuRequested, this, &MainWindow::ShowSourceAssetContextMenu);

    SetupAssetSelectionCaching();

    //Log View
    m_loggingPanel = ui->LoggingPanel;
    m_loggingPanel->SetStorageID(AZ_CRC("AssetProcessor::LogPanel", 0x75baa468));

    connect(ui->logButton, &QPushButton::clicked, this, &MainWindow::DesktopOpenJobLogs);

    if (!m_loggingPanel->LoadState())
    {
        // if unable to load state then show the default tabs
        ResetLoggingPanel();
    }

    AzQtComponents::ConfigHelpers::loadConfig<Config, MainWindow>(m_fileSystemWatcher, &m_config, QStringLiteral("style:AssetProcessorConfig.ini"), this, std::bind(&MainWindow::ApplyConfig, this));
    ApplyConfig();

    connect(m_loggingPanel, &AzToolsFramework::LogPanel::StyledLogPanel::TabsReset, this, &MainWindow::ResetLoggingPanel);
    connect(m_guiApplicationManager->GetRCController(), &AssetProcessor::RCController::JobStatusChanged, m_jobsModel, &AssetProcessor::JobsModel::OnJobStatusChanged);
    connect(m_guiApplicationManager->GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::JobRemoved, m_jobsModel, &AssetProcessor::JobsModel::OnJobRemoved);
    connect(m_guiApplicationManager->GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::SourceDeleted, m_jobsModel, &AssetProcessor::JobsModel::OnSourceRemoved);
    connect(m_guiApplicationManager->GetAssetProcessorManager(), &AssetProcessor::AssetProcessorManager::SourceFolderDeleted, m_jobsModel, &AssetProcessor::JobsModel::OnFolderRemoved);

    connect(ui->jobTreeView, &AzQtComponents::TableView::customContextMenuRequested, this, &MainWindow::ShowJobViewContextMenu);
    connect(ui->jobContextLogTableView, &AzQtComponents::TableView::customContextMenuRequested, this, &MainWindow::ShowLogLineContextMenu);
    connect(ui->jobLogTableView, &AzQtComponents::TableView::customContextMenuRequested, this, &MainWindow::ShowJobLogContextMenu);

    m_jobsModel->PopulateJobsFromDatabase();

    // Tools tab:
    connect(ui->fullScanButton, &QPushButton::clicked, this, &MainWindow::OnRescanButtonClicked);

    settings.beginGroup("Options");
    bool zeroAnalysisModeFromSettings = settings.value("EnableZeroAnalysis", QVariant(true)).toBool();
    settings.endGroup();

    QObject::connect(ui->modtimeSkippingCheckBox, &QCheckBox::stateChanged, this, 
        [this](int newCheckState)
    {
        bool newOption = newCheckState == Qt::Checked ? true : false;
        m_guiApplicationManager->GetAssetProcessorManager()->SetEnableModtimeSkippingFeature(newOption);
        QSettings settingsInCallback;
        settingsInCallback.beginGroup("Options");
        settingsInCallback.setValue("EnableZeroAnalysis", QVariant(newOption));
        settingsInCallback.endGroup();
    });

    m_guiApplicationManager->GetAssetProcessorManager()->SetEnableModtimeSkippingFeature(zeroAnalysisModeFromSettings);
    ui->modtimeSkippingCheckBox->setCheckState(zeroAnalysisModeFromSettings ? Qt::Checked : Qt::Unchecked);
}

void MainWindow::SetupAssetSelectionCaching()
{
    // Connect the source model resetting to preserve selection and restore it after the model is reset.
    connect(m_sourceModel, &QAbstractItemModel::modelAboutToBeReset, [&]()
    {
        QItemSelection sourceSelection = m_sourceAssetTreeFilterModel->mapSelectionToSource(ui->SourceAssetsTreeView->selectionModel()->selection());

        if (sourceSelection.indexes().count() == 0 || !sourceSelection.indexes()[0].isValid())
        {
            return;
        }
        QModelIndex sourceModelIndex = sourceSelection.indexes()[0];
        AssetProcessor::AssetTreeItem* childItem = static_cast<AssetProcessor::AssetTreeItem*>(sourceModelIndex.internalPointer());
        m_cachedSourceAssetSelection = childItem->GetData()->m_assetDbName;
    });

    connect(m_sourceModel, &QAbstractItemModel::modelReset, [&]()
    {
        if (m_cachedSourceAssetSelection.empty())
        {
            return;
        }
        QModelIndex goToIndex = m_sourceModel->GetIndexForSource(m_cachedSourceAssetSelection);
        // If the cached selection was deleted or is no longer available, clear the selection.
        if (!goToIndex.isValid())
        {
            m_cachedSourceAssetSelection.clear();
            ui->ProductAssetsTreeView->selectionModel()->clearSelection();
            // ClearSelection says in the Qt docs that the selectionChange signal will be sent, but that wasn't happening,
            // so force the details panel to refresh.
            ui->sourceAssetDetailsPanel->AssetDataSelectionChanged(QItemSelection(), QItemSelection());
            return;
        }
        m_sourceAssetTreeFilterModel->ForceModelIndexVisible(goToIndex);
        QModelIndex filterIndex = m_sourceAssetTreeFilterModel->mapFromSource(goToIndex);
        ui->SourceAssetsTreeView->scrollTo(filterIndex, QAbstractItemView::ScrollHint::EnsureVisible);
        ui->SourceAssetsTreeView->selectionModel()->select(filterIndex, AssetProcessor::AssetTreeModel::GetAssetTreeSelectionFlags());
    });

    // Connect the product model resetting to preserve selection and restore it after the model is reset.
    connect(m_productModel, &QAbstractItemModel::modelAboutToBeReset, [&]()
    {
        QItemSelection productSelection = m_productAssetTreeFilterModel->mapSelectionToSource(ui->ProductAssetsTreeView->selectionModel()->selection());

        if (productSelection.indexes().count() == 0 || !productSelection.indexes()[0].isValid())
        {
            return;
        }
        QModelIndex productModelIndex = productSelection.indexes()[0];
        AssetProcessor::AssetTreeItem* childItem = static_cast<AssetProcessor::AssetTreeItem*>(productModelIndex.internalPointer());
        m_cachedProductAssetSelection = childItem->GetData()->m_assetDbName;
    });

    connect(m_productModel, &QAbstractItemModel::modelReset, [&]()
    {
        if (m_cachedProductAssetSelection.empty())
        {
            return;
        }
        QModelIndex goToIndex = m_productModel->GetIndexForProduct(m_cachedProductAssetSelection);
        // If the cached selection was deleted or is no longer available, clear the selection.
        if (!goToIndex.isValid())
        {
            m_cachedProductAssetSelection.clear();
            ui->ProductAssetsTreeView->selectionModel()->clearSelection();
            // ClearSelection says in the Qt docs that the selectionChange signal will be sent, but that wasn't happening,
            // so force the details panel to refresh.
            ui->productAssetDetailsPanel->AssetDataSelectionChanged(QItemSelection(), QItemSelection());
            return;
        }
        m_productAssetTreeFilterModel->ForceModelIndexVisible(goToIndex);
        QModelIndex filterIndex = m_productAssetTreeFilterModel->mapFromSource(goToIndex);
        ui->ProductAssetsTreeView->scrollTo(filterIndex, QAbstractItemView::ScrollHint::EnsureVisible);
        ui->ProductAssetsTreeView->selectionModel()->select(filterIndex, AssetProcessor::AssetTreeModel::GetAssetTreeSelectionFlags());
    });
}

void MainWindow::OnRescanButtonClicked()
{
    m_guiApplicationManager->Rescan();
}

void MainWindow::OnSupportClicked(bool /*checked*/)
{
    QDesktopServices::openUrl(
        QStringLiteral("https://o3de.org/docs/user-guide/assets/pipeline/"));
}

void MainWindow::EditConnection(const QModelIndex& index)
{
    if (index.data(ConnectionManager::UserConnectionRole).toBool())
    {
        ConnectionEditDialog dialog(m_guiApplicationManager->GetConnectionManager(), index, this);

        dialog.exec();
    }
}

void MainWindow::OnConnectionContextMenu(const QPoint& point)
{
    QPersistentModelIndex index = ui->connectionTreeView->indexAt(point);

    bool isUserConnection = index.isValid() && index.data(ConnectionManager::UserConnectionRole).toBool();
    QMenu menu(this);

    QAction* editConnectionAction = menu.addAction("&Edit connection...");
    editConnectionAction->setEnabled(isUserConnection);
    connect(editConnectionAction, &QAction::triggered, this, [index, this] {
        EditConnection(index);
    });

    menu.exec(ui->connectionTreeView->viewport()->mapToGlobal(point));
}

void MainWindow::OnEditConnection(bool /*checked*/)
{
    auto selectedIndices = ui->connectionTreeView->selectionModel()->selectedRows();
    Q_ASSERT(selectedIndices.count() > 0);

    // Only edit the first connection. Guaranteed above by the edit connection button only being enabled if one connection is selected
    EditConnection(selectedIndices[0]);
}

void MainWindow::OnAddConnection(bool /*checked*/)
{
    m_guiApplicationManager->GetConnectionManager()->addUserConnection();
}

void MainWindow::OnAllowedListConnectionsListViewClicked() 
{
    ui->allowedListRejectedConnectionsListView->clearSelection();
}

void MainWindow::OnRejectedConnectionsListViewClicked()
{
    ui->allowListAllowedListConnectionsListView->clearSelection();
}

void MainWindow::OnAllowedListCheckBoxToggled() 
{
    if (!ui->allowedListEnableCheckBox->isChecked())
    {
        //warn this is not safe
        if(QMessageBox::Ok == QMessageBox::warning(this, tr("!!!WARNING!!!"), tr("Turning off allowed listing poses a significant security risk as it would allow any device to connect to your asset processor and that device will have READ/WRITE access to the Asset Processors file system. Only do this if you sure you know what you are doing and accept the risks."),
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel))
        {
            ui->allowedListRejectedConnectionsListView->clearSelection();
            ui->allowListAllowedListConnectionsListView->clearSelection();
            ui->allowedListAddHostNameLineEdit->setEnabled(false);
            ui->allowedListAddHostNameToolButton->setEnabled(false);
            ui->allowedListAddIPLineEdit->setEnabled(false);
            ui->allowedListAddIPToolButton->setEnabled(false);
            ui->allowListAllowedListConnectionsListView->setEnabled(false);
            ui->allowedListRejectedConnectionsListView->setEnabled(false);
            ui->allowedListToAllowedListToolButton->setEnabled(false);
            ui->allowedListToRejectedListToolButton->setEnabled(false);
        }
        else
        {
            ui->allowedListEnableCheckBox->setChecked(true);
        }
    }
    else
    {
        ui->allowedListAddHostNameLineEdit->setEnabled(true);
        ui->allowedListAddHostNameToolButton->setEnabled(true);
        ui->allowedListAddIPLineEdit->setEnabled(true);
        ui->allowedListAddIPToolButton->setEnabled(true);
        ui->allowListAllowedListConnectionsListView->setEnabled(true);
        ui->allowedListRejectedConnectionsListView->setEnabled(true);
        ui->allowedListToAllowedListToolButton->setEnabled(true);
        ui->allowedListToRejectedListToolButton->setEnabled(true);
    }
    
    m_guiApplicationManager->GetConnectionManager()->AllowedListingEnabled(ui->allowedListEnableCheckBox->isChecked());
}

void MainWindow::OnAddHostNameAllowedListButtonClicked()
{
    QString text = ui->allowedListAddHostNameLineEdit->text();
    const QRegExpValidator *hostnameValidator = static_cast<const QRegExpValidator *>(ui->allowedListAddHostNameLineEdit->validator());
    int pos;
    QValidator::State state = hostnameValidator->validate(text, pos);
    if (state == QValidator::Acceptable)
    {
        auto lineEdit = ui->allowedListAddHostNameLineEdit;
        m_guiApplicationManager->GetConnectionManager()->AddAddressToAllowedList(text);
        lineEdit->clear();
        // Clear error state set in LineEdit.
        lineEdit->setProperty(AzQtComponents::HasError, false);
        auto errorToolButton = lineEdit->findChild<QToolButton*>(AzQtComponents::ErrorToolButton);
        if (errorToolButton && AzQtComponents::LineEdit::errorIconEnabled(lineEdit))
        {
            errorToolButton->setVisible(false);
        }
    }
}

void MainWindow::OnAddIPAllowedListButtonClicked()
{
    QString text = ui->allowedListAddIPLineEdit->text();
    const QRegExpValidator *ipValidator = static_cast<const QRegExpValidator *>(ui->allowedListAddIPLineEdit->validator());
    int pos;
    QValidator::State state = ipValidator->validate(text, pos);
    if (state== QValidator::Acceptable)
    {
        auto lineEdit = ui->allowedListAddIPLineEdit;
        m_guiApplicationManager->GetConnectionManager()->AddAddressToAllowedList(text);
        lineEdit->clear();
        // Clear error state set in LineEdit.
        lineEdit->setProperty(AzQtComponents::HasError, false);
        auto errorToolButton = lineEdit->findChild<QToolButton*>(AzQtComponents::ErrorToolButton);
        if (errorToolButton && AzQtComponents::LineEdit::errorIconEnabled(lineEdit))
        {
            errorToolButton->setVisible(false);
        }
    }
}

void MainWindow::OnToRejectedListButtonClicked()
{
    QModelIndexList indices = ui->allowListAllowedListConnectionsListView->selectionModel()->selectedIndexes();
    if(!indices.isEmpty() && indices.first().isValid())
    {
        QString itemText = indices.first().data(Qt::DisplayRole).toString();
        m_guiApplicationManager->GetConnectionManager()->RemoveAddressFromAllowedList(itemText);
        m_guiApplicationManager->GetConnectionManager()->AddRejectedAddress(itemText, true);
    }
}

void MainWindow::OnToAllowedListButtonClicked()
{
    QModelIndexList indices = ui->allowedListRejectedConnectionsListView->selectionModel()->selectedIndexes();
    if (!indices.isEmpty() && indices.first().isValid())
    {
        QString itemText = indices.front().data(Qt::DisplayRole).toString();
        m_guiApplicationManager->GetConnectionManager()->RemoveRejectedAddress(itemText);
        m_guiApplicationManager->GetConnectionManager()->AddAddressToAllowedList(itemText);
    }
}

void MainWindow::OnRemoveConnection(bool /*checked*/)
{
    ConnectionManager* manager = m_guiApplicationManager->GetConnectionManager();

    QModelIndexList list = ui->connectionTreeView->selectionModel()->selectedRows();
    for (QModelIndex index: list)
    {
        manager->removeConnection(index);
    }
}

void MainWindow::OnConnectionSelectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/)
{
    auto selectedIndices = ui->connectionTreeView->selectionModel()->selectedRows();
    int selectionCount = selectedIndices.count();

    bool anyUserConnectionsSelected = false;
    for (int i = 0; i < selectionCount; i++)
    {
        QModelIndex selectedIndex = selectedIndices[i];
        if (selectedIndex.data(ConnectionManager::UserConnectionRole).toBool())
        {
            anyUserConnectionsSelected = true;
            break;
        }
    }

    ui->removeConnectionButton->setEnabled(anyUserConnectionsSelected);
    ui->editConnectionButton->setEnabled(anyUserConnectionsSelected && (selectionCount == 1));
}


MainWindow::~MainWindow()
{
    m_guiApplicationManager = nullptr;
    delete ui;
}



void MainWindow::ShowWindow()
{
    show();
    AzQtComponents::bringWindowToTop(this);
}


void MainWindow::SyncAllowedListAndRejectedList(QStringList allowedList, QStringList rejectedList)
{
    m_allowedListAddresses.setStringList(allowedList);
    m_rejectedAddresses.setStringList(rejectedList);
}

void MainWindow::FirstTimeAddedToRejctedList(QString ipAddress)
{
    QMessageBox* msgBox = new QMessageBox(this);
    msgBox->setText(tr("!!!Rejected Connection!!!"));
    msgBox->setInformativeText(ipAddress + tr(" tried to connect and was rejected because it was not on the allowed list. If you want this connection to be allowed go to connections tab and add it to allowed list."));
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setDefaultButton(QMessageBox::Ok);
    msgBox->setWindowModality(Qt::NonModal);
    msgBox->setModal(false);
    msgBox->show();
}

void MainWindow::SaveLogPanelState()
{
    if (m_loggingPanel)
    {
        m_loggingPanel->SaveState();
    }
}

void MainWindow::ResetTimers()
{
    m_scanTime = m_analysisTime = m_processTime = 0;
    m_scanTimer.restart();
    m_analysisTimer.invalidate();
    m_processTimer.invalidate();
}

void MainWindow::CheckStartAnalysisTimers()
{
    if (m_scanTimer.isValid())
    {
        m_scanTime = m_scanTimer.elapsed();
        m_scanTimer.invalidate();
    }
    if (!m_analysisTimer.isValid() && !m_analysisTime)
    {
        m_analysisTimer.start();
    }
}

void MainWindow::CheckStartProcessTimers()
{
    if (m_analysisTimer.isValid())
    {
        m_analysisTime = m_analysisTimer.restart();
        m_analysisTimer.invalidate();
    }

    if (!m_processTimer.isValid() && !m_processTime)
    {
        m_processTimer.start();
    }
}

void MainWindow::CheckEndAnalysisTimer()
{
    if (m_analysisTimer.isValid() && !m_analysisTime)
    {
        m_analysisTime = m_analysisTimer.elapsed();
        m_analysisTimer.invalidate();
    }
}

void MainWindow::CheckEndProcessTimer()
{
    if (m_processTimer.isValid() && !m_processTime)
    {
        m_processTime = m_processTimer.elapsed();
        m_processTimer.invalidate();
    }
}

QString MainWindow::FormatStringTime(qint64 msTime) const
{
    int msecInt{ static_cast<int>(msTime) };
    int timeHrs{ msecInt / (1000 * 60 * 60) };
    msecInt = msecInt % (1000 * 60 * 60);

    int timeMins{ msecInt / (1000 * 60) };
    msecInt = msecInt % (1000 * 60 );

    int timeSecs{ msecInt / 1000 };
    int timeMsec{ msecInt % 1000 };

    QTime timeVal{ timeHrs, timeMins, timeSecs, timeMsec };

    if (timeHrs)
    {
        return timeVal.toString("h:mm:ss.z");
    }
    return timeVal.toString("mm:ss.z");
}

void MainWindow::IntervalAssetTabFilterRefresh()
{
    if (ui->buttonList->currentIndex() != static_cast<int>(DialogStackIndex::Assets)
        || !ui->assetDataFilteredSearchWidget->hasStringFilter())
    {
        return;
    }

    if (!m_filterRefreshTimer.isValid())
    {
        m_filterRefreshTimer.start();
    }

    if (m_filterRefreshTimer.elapsed() >= AssetTabFilterUpdateIntervalMs)
    {
        emit ui->assetDataFilteredSearchWidget->TextFilterChanged(ui->assetDataFilteredSearchWidget->textFilter());
        m_filterRefreshTimer.restart();
    }
}

void MainWindow::ShutdownAssetTabFilterRefresh()
{
    if (m_filterRefreshTimer.isValid())
    {
        emit ui->assetDataFilteredSearchWidget->TextFilterChanged(ui->assetDataFilteredSearchWidget->textFilter());
    }

    m_filterRefreshTimer.invalidate();
}

void MainWindow::OnAssetProcessorStatusChanged(const AssetProcessor::AssetProcessorStatusEntry entry)
{
    using namespace AssetProcessor;
    QString text;
    switch (entry.m_status)
    {
    case AssetProcessorStatus::Initializing_Gems:
        text = tr("Initializing Gem...%1").arg(entry.m_extraInfo);
        break;
    case AssetProcessorStatus::Initializing_Builders:
        text = tr("Initializing Builders...");
        break;
    case AssetProcessorStatus::Scanning_Started:
        ResetTimers();
        text = tr("Scanning...");
        break;
    case AssetProcessorStatus::Analyzing_Jobs:
        CheckStartAnalysisTimers();
        m_createJobCount = entry.m_count;

        if (m_processJobsCount + m_createJobCount > 0)
        {
            text = tr("Working, analyzing jobs remaining %1, processing jobs remaining %2...").arg(m_createJobCount).arg(m_processJobsCount);
            ui->timerContainerWidget->setVisible(false);
            ui->productAssetDetailsPanel->SetScanQueueEnabled(false);
            
            IntervalAssetTabFilterRefresh();
        }
        else
        {
            CheckEndAnalysisTimer();
            text = tr("Idle...");
            ui->timerContainerWidget->setVisible(true);
            m_guiApplicationManager->RemoveOldTempFolders();
            // Once the asset processor goes idle, enable the scan queue.
            // This minimizes the potential for over-reporting missing dependencies (if a queued job would resolve them)
            // and prevents running too many threads with too much work (scanning + processing jobs both take time).
            ui->productAssetDetailsPanel->SetScanQueueEnabled(true);

            ShutdownAssetTabFilterRefresh();
        }
        break;
    case AssetProcessorStatus::Processing_Jobs:
        CheckStartProcessTimers();
        m_processJobsCount = entry.m_count;  

        if (m_processJobsCount + m_createJobCount > 0)
        {
            text = tr("Working, analyzing jobs remaining %1, processing jobs remaining %2...").arg(m_createJobCount).arg(m_processJobsCount);
            ui->timerContainerWidget->setVisible(false);
            ui->productAssetDetailsPanel->SetScanQueueEnabled(false);

            IntervalAssetTabFilterRefresh();
        }
        else
        {
            CheckEndProcessTimer();
            text = tr("Idle...");
            ui->timerContainerWidget->setVisible(true);
            m_guiApplicationManager->RemoveOldTempFolders();
            // Once the asset processor goes idle, enable the scan queue.
            // This minimizes the potential for over-reporting missing dependencies (if a queued job would resolve them)
            // and prevents running too many threads with too much work (scanning + processing jobs both take time).
            ui->productAssetDetailsPanel->SetScanQueueEnabled(true);
            AZ_TracePrintf(
                AssetProcessor::ConsoleChannel,
                "Job processing completed. Asset Processor is currently idle. Process time: %s\n",
                FormatStringTime(m_processTime).toUtf8().constData());
            ShutdownAssetTabFilterRefresh();
        }
        break;
    default:
        text = QString();
        break;
    }

    ui->APStatusValueLabel->setText(QStringLiteral("%1: %2")
        .arg(tr("Status"))
        .arg(text));

    ui->lastScanTimer->setText(FormatStringTime(m_scanTime));
    ui->analysisTimer->setText(FormatStringTime(m_analysisTime));
    ui->processingTimer->setText(FormatStringTime(m_processTime));
}

void MainWindow::HighlightAsset(QString assetPath)
{
    // Make sure that the currently active tab is the job list
    ui->buttonList->setCurrentIndex(static_cast<int>(DialogStackIndex::Jobs));

    // clear all filters
    ui->jobFilteredSearchWidget->ClearTextFilter();
    ui->jobFilteredSearchWidget->ClearTypeFilter();

    // jobs are listed with relative source asset paths, so we need to remove
    // the scan folder prefix from the absolute path
    bool success = true;
    AZStd::vector<AZStd::string> scanFolders;
    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetScanFolders, scanFolders);
    if (success)
    {
        for (auto& scanFolder : scanFolders)
        {
            if (assetPath.startsWith(scanFolder.c_str(), Qt::CaseInsensitive))
            {
                // + 1 for the path separator
                assetPath = assetPath.mid(static_cast<int>(scanFolder.size() + 1));
                break;
            }
        }
    }

    // apply the filter for our asset path
    ui->jobFilteredSearchWidget->SetTextFilter(assetPath);

    // select the first item in the list
    ui->jobTreeView->setCurrentIndex(m_jobSortFilterProxy->index(0, 0));
}

void MainWindow::OnAssetTabChange(int index)
{
    AssetTabIndex tabIndex = static_cast<AssetTabIndex>(index);
    switch (tabIndex)
    {
    case AssetTabIndex::Source:
        ui->sourceAssetDetailsPanel->setVisible(true);
        ui->productAssetDetailsPanel->setVisible(false);
        break;
    case AssetTabIndex::Product:
        ui->sourceAssetDetailsPanel->setVisible(false);
        ui->productAssetDetailsPanel->setVisible(true);
        break;
    }
}

void MainWindow::ApplyConfig()
{
    using namespace AssetProcessor;

    // Asset Status
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnStatus, m_config.jobStatusColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnSource, m_config.jobSourceColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnPlatform, m_config.jobPlatformColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnJobKey, m_config.jobKeyColumnWidth);
    ui->jobTreeView->header()->resizeSection(JobsModel::ColumnCompleted, m_config.jobCompletedColumnWidth);

    // Event Log Details
    ui->jobLogTableView->header()->resizeSection(AzToolsFramework::Logging::LogTableModel::ColumnType, m_config.logTypeColumnWidth);
}

MainWindow::LogSortFilterProxy::LogSortFilterProxy(QObject* parentOjbect) : QSortFilterProxyModel(parentOjbect) 
{
}

bool MainWindow::LogSortFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!m_logTypes.empty())
    {
        QModelIndex testIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        Q_ASSERT(testIndex.isValid());
        auto indexLogType = static_cast<AzToolsFramework::Logging::LogLine::LogType>(testIndex.data(AzToolsFramework::Logging::LogTableModel::LogTypeRole).toInt());
        if (!m_logTypes.contains(indexLogType))
        {
            return false;
        }
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void MainWindow::LogSortFilterProxy::onTypeFilterChanged(const AzQtComponents::SearchTypeFilterList& activeTypeFilters)
{
    beginResetModel();
    m_logTypes.clear();

    for (auto typeIter = activeTypeFilters.begin(), endIter = activeTypeFilters.end(); typeIter != endIter; ++typeIter)
    {
        m_logTypes.insert(static_cast<AzToolsFramework::Logging::LogLine::LogType>(typeIter->metadata.toInt()));
    }
    endResetModel();
}

void MainWindow::SetContextLogDetailsVisible(bool visible)
{
    using namespace AzQtComponents;

    const char* soloClass = "solo"; // see AssetsTab.qss; this is what provides the right margin around the widgets for the context details

    if (visible)
    {
        Style::removeClass(ui->jobContextLogDetails, soloClass);

        ui->jobLogLayout->removeWidget(ui->jobContextLogBar);
        ui->jobContextLayout->insertWidget(0, ui->jobContextLogBar);
    }
    else
    {
        Style::addClass(ui->jobContextLogDetails, soloClass);

        ui->jobContextLayout->removeWidget(ui->jobContextLogBar);
        ui->jobLogLayout->addWidget(ui->jobContextLogBar);
    }
    ui->jobContextContainer->setVisible(visible);
    ui->jobContextLogLabel->setVisible(visible);
}

void MainWindow::SetContextLogDetails(const QMap<QString, QString>& details)
{
    auto model = qobject_cast<AzToolsFramework::Logging::ContextDetailsLogTableModel*>(ui->jobContextLogTableView->model());

    if (details.isEmpty())
    {
        ui->jobContextLogStackedWidget->setCurrentWidget(ui->jobContextLogPlaceholderLabel);
    }
    else
    {
        ui->jobContextLogStackedWidget->setCurrentWidget(ui->jobContextLogTableView);
    }

    model->SetDetails(details);
}

void MainWindow::ClearContextLogDetails()
{
    SetContextLogDetails({});
}

void MainWindow::UpdateJobLogView(QModelIndex selectedIndex)
{
    if (!m_logsModel)
    {
        return;
    }

    // SelectionMode is set to QAbstractItemView::SingleSelection so there is only one selected item at a time
    AZStd::string jobLog = m_jobSortFilterProxy->data(selectedIndex, AssetProcessor::JobsModel::DataRoles::logRole).toString().toUtf8().data();

    m_logsModel->Clear();
    AzToolsFramework::Logging::LogLine::ParseLog(jobLog.c_str(), jobLog.length() + 1,
        AZStd::bind(&AzToolsFramework::Logging::LogTableModel::AppendLineAsync, m_logsModel, AZStd::placeholders::_1));
    m_logsModel->CommitLines();
    ui->jobLogTableView->scrollToBottom();
    ui->jobLogStackedWidget->setCurrentWidget(ui->jobLogTableView);
}

void MainWindow::JobSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
{
    if (selected.indexes().length() != 0)
    {
        UpdateJobLogView(selected.indexes()[0]);
    }
    // The only alternative is that there has been only a deselection, as otherwise both selected and deselected would be empty
    else
    {
        ui->jobLogStackedWidget->setCurrentWidget(ui->jobLogPlaceholderLabel);
    }

    ClearContextLogDetails();
}

void MainWindow::JobStatusChanged([[maybe_unused]] AssetProcessor::JobEntry entry, [[maybe_unused]] AzToolsFramework::AssetSystem::JobStatus status)
{
    QModelIndexList selectedIndexList = ui->jobTreeView->selectionModel()->selectedIndexes();
    if (selectedIndexList.empty())
    {
        return;
    }

    QModelIndex selectedIndex = selectedIndexList.at(0);

    // retrieve cachedJobInfo for the selected entry
    const QModelIndex sourceIndex = m_jobSortFilterProxy->mapToSource(selectedIndex);
    AssetProcessor::CachedJobInfo* cachedJobInfo = m_jobsModel->getItem(sourceIndex.row());
    AZ_Assert(cachedJobInfo, "Failed to find cached job info");
    if (!cachedJobInfo)
    {
        return;
    }

    // ignore the notification if it's not for the selected entry
    if (cachedJobInfo->m_elementId.GetInputAssetName() != entry.m_databaseSourceName)
    {
        return;
    }

    UpdateJobLogView(selectedIndex);
}

void MainWindow::JobLogSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
{
    if (selected.count() == 1)
    {
        const QMap<QString, QString> details = selected.indexes().first().data(AzToolsFramework::Logging::LogTableModel::DetailsRole).value<QMap<QString, QString>>();
        SetContextLogDetails(details);
    }
    else
    {
        ClearContextLogDetails();
    }
}

void MainWindow::DesktopOpenJobLogs()
{
    char resolvedDir[AZ_MAX_PATH_LEN * 2];
    QString currentDir;
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(AssetUtilities::ComputeJobLogFolder().c_str(), resolvedDir, sizeof(resolvedDir));

    currentDir = QString::fromUtf8(resolvedDir);

    if (QFile::exists(currentDir))
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(currentDir));
    }
    else
    {
        AZ_TracePrintf(AssetProcessor::ConsoleChannel, "[Error] Logs folder (%s) does not exists.\n", currentDir.toUtf8().constData());
    }
}

void MainWindow::ResetLoggingPanel()
{
    if (m_loggingPanel)
    {
        m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Debug", "", ""));
        m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Messages", "", "", true, true, true, false));
        m_loggingPanel->AddLogTab(AzToolsFramework::LogPanel::TabSettings("Warnings/Errors Only", "", "", false, true, true, false));
    }
}

void MainWindow::ShowJobLogContextMenu(const QPoint& pos)
{
    using namespace AzToolsFramework::Logging;
    QModelIndex sourceIndex = ui->jobLogTableView->indexAt(pos);

    // If there is no index under the mouse cursor, let check the selected index of the view
    if (!sourceIndex.isValid())
    {
        const auto indexes = ui->jobLogTableView->selectionModel()->selectedIndexes();

        if (!indexes.isEmpty())
        {
            sourceIndex = indexes.first();
        }
    }

    QMenu menu;
    QAction* line = menu.addAction(tr("Copy line"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.data(LogTableModel::LogLineTextRole).toString());
    });
    QAction* lineDetails = menu.addAction(tr("Copy line with details"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.data(LogTableModel::CompleteLogLineTextRole).toString());
    });
    menu.addAction(tr("Copy all"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(m_logsModel->toString(true));
    });

    if (!sourceIndex.isValid())
    {
        line->setEnabled(false);
        lineDetails->setEnabled(false);
    }

    menu.exec(ui->jobLogTableView->viewport()->mapToGlobal(pos));
}

static QString FindAbsoluteFilePath(const AssetProcessor::CachedJobInfo* cachedJobInfo)
{
    using namespace AzToolsFramework;

    bool result = false;
    AZ::Data::AssetInfo info;
    AZStd::string watchFolder;
    QByteArray assetNameUtf8 = cachedJobInfo->m_elementId.GetInputAssetName().toUtf8();
    AssetSystemRequestBus::BroadcastResult(result, &AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, assetNameUtf8.constData(), info, watchFolder);
    if (!result)
    {
        AZ_Error("AssetProvider", false, "Failed to locate asset info for '%s'.", assetNameUtf8.constData());
    }

    return result
        ? QDir(watchFolder.c_str()).absoluteFilePath(info.m_relativePath.c_str())
        : QString();
};

static void SendShowInAssetBrowserResponse(const QString& filePath, ConnectionManager* connectionManager, unsigned int connectionId, QByteArray data)
{
    Connection* connection = connectionManager->getConnection(connectionId);
    if (!connection)
    {
        return;
    }

    AzToolsFramework::AssetSystem::WantAssetBrowserShowResponse response;
    bool readFromStream = AZ::Utils::LoadObjectFromBufferInPlace(data.data(), data.size(), response);
    AZ_Assert(readFromStream, "AssetProcessor failed to deserialize from stream");
    if (!readFromStream)
    {
        return;
    }

#ifdef AZ_PLATFORM_WINDOWS
    // Required on Windows to allow the other process to come to the foreground
    AllowSetForegroundWindow(response.m_processId);
#endif // #ifdef AZ_PLATFORM_WINDOWS

    AzToolsFramework::AssetSystem::AssetBrowserShowRequest message;
    message.m_filePath = filePath.toUtf8().data();
    connection->Send(AzFramework::AssetSystem::DEFAULT_SERIAL, message);
}

void MainWindow::ShowJobViewContextMenu(const QPoint& pos)
{
    using namespace AssetProcessor;

    auto cachedJobInfoAt = [this](const QPoint& pos)
    {
        const QModelIndex proxyIndex = ui->jobTreeView->indexAt(pos);
        const QModelIndex sourceIndex = m_jobSortFilterProxy->mapToSource(proxyIndex);
        return m_jobsModel->getItem(sourceIndex.row());
    };

    const CachedJobInfo* item = cachedJobInfoAt(pos);

    if (!item)
    {
        return;
    }
    QMenu menu;
    menu.setToolTipsVisible(true);

    menu.addAction("Show in Asset Browser", this, [&]()
    {
        ConnectionManager* connectionManager = m_guiApplicationManager->GetConnectionManager();

        QString filePath = FindAbsoluteFilePath(item);

        AzToolsFramework::AssetSystem::WantAssetBrowserShowRequest requestMessage;

        auto& connectionMap = connectionManager->getConnectionMap();
        auto connections = connectionMap.values();
        for (auto connection : connections)
        {
            using namespace AzFramework::AssetSystem;

            // Ask the Editor, and only the Editor, if it wants to receive
            // the message for showing an asset in the AssetBrowser.
            // This also allows the Editor to send back it's Process ID, which
            // allows the Windows platform to call AllowSetForegroundWindow()
            // which is required to bring the Editor window to the foreground
            if (connection->Identifier() == ConnectionIdentifiers::Editor)
            {
                unsigned int connectionId = connection->ConnectionId();
                connection->SendRequest(requestMessage, [connectionManager, connectionId, filePath](AZ::u32 /*type*/, QByteArray callbackData) {
                    SendShowInAssetBrowserResponse(filePath, connectionManager, connectionId, callbackData);
                });
            }
        }
    });

    // Only completed items will be available in the assets tab.
    QAction* assetTabSourceAction = menu.addAction(tr("View source asset"), this, [&]()
    {
        ui->dialogStack->setCurrentIndex(static_cast<int>(DialogStackIndex::Assets));
        ui->buttonList->setCurrentIndex(static_cast<int>(DialogStackIndex::Assets));
        ui->sourceAssetDetailsPanel->GoToSource(item->m_elementId.GetInputAssetName().toUtf8().constData());
    });

    QString productMenuTitle(tr("View product asset..."));        
    if (item->m_jobState != AzToolsFramework::AssetSystem::JobStatus::Completed)
    {
        QString disabledActionTooltip(tr("Only completed jobs are available in the Assets tab."));
        assetTabSourceAction->setToolTip(disabledActionTooltip);
        assetTabSourceAction->setDisabled(true);

        // Disabled menus don't support tooltips, so add it as an action, instead.
        QAction* productMenuAction = menu.addAction(productMenuTitle);
        productMenuAction->setToolTip(disabledActionTooltip);
        productMenuAction->setDisabled(true);
    }
    else
    {
        assetTabSourceAction->setToolTip(tr("Show the source asset for this job in the Assets tab."));

        QMenu* productMenu = menu.addMenu(productMenuTitle);
        productMenu->setToolTipsVisible(true);

        bool anyProductsAvailableForJob = false;
        m_sharedDbConnection->QueryJobByJobRunKey(
            item->m_jobRunKey,
            [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
        {
            m_sharedDbConnection->QueryProductByJobID(
                jobEntry.m_jobID,
                [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry)
            {
                if (productEntry.m_productName.empty())
                {
                    return true;
                }
                anyProductsAvailableForJob = true;
                QAction* assetTabProductAction = productMenu->addAction(productEntry.m_productName.c_str(), this, [&, productEntry]()
                {
                    ui->dialogStack->setCurrentIndex(static_cast<int>(DialogStackIndex::Assets));
                    ui->buttonList->setCurrentIndex(static_cast<int>(DialogStackIndex::Assets));
                    ui->sourceAssetDetailsPanel->GoToProduct(productEntry.m_productName);
                });
                assetTabProductAction->setToolTip("Shows this product asset in the Product Assets tab.");
                return true; // Keep iterating, add all products.
            });
            return false; // Stop iterating, there should only be one job with this run key.
        });

        if (!anyProductsAvailableForJob)
        {
            // If there were no products, then show a disabled action with a tooltip.
            // Disabled menus don't support tooltips, so remove the menu first.
            menu.removeAction(productMenu->menuAction());
            productMenu->deleteLater();
            productMenu = nullptr;

            QAction* productMenuAction = menu.addAction(productMenuTitle);
            productMenuAction->setToolTip(tr("This job created no products."));
            productMenuAction->setDisabled(true);
        }
    }

    QAction* fileBrowserAction = menu.addAction(AzQtComponents::fileBrowserActionName(), this, [&]()
    {
        AzQtComponents::ShowFileOnDesktop(FindAbsoluteFilePath(item));
    });
    fileBrowserAction->setToolTip(tr("Opens a window in your operating system's file explorer to view the source asset for this job."));

    menu.addAction(tr("Open"), this, [&]()
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(FindAbsoluteFilePath(item)));
    });

    menu.addAction(tr("Copy"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(FindAbsoluteFilePath(item));
    });

    // Get the internal path to the log file
    const QModelIndex proxyIndex = ui->jobTreeView->indexAt(pos);
    const QModelIndex sourceIndex = m_jobSortFilterProxy->mapToSource(proxyIndex);
    QVariant pathVariant = m_jobsModel->data(sourceIndex, JobsModel::logFileRole);

    // Get the absolute path of the log file
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    char resolvedPath[AZ_MAX_PATH_LEN];
    fileIO->ResolvePath(pathVariant.toByteArray().constData(), resolvedPath, AZ_MAX_PATH_LEN);

    QFileInfo fileInfo(resolvedPath);
    auto openLogFile = menu.addAction(tr("Open log file"), this, [&]()
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
    });
    openLogFile->setEnabled(fileInfo.exists());

    auto logDir = fileInfo.absoluteDir();
    auto openLogFolder = menu.addAction(tr("Open folder with log file"), this, [&]()
    {
        if (fileInfo.exists())
        {
            AzQtComponents::ShowFileOnDesktop(fileInfo.absoluteFilePath());
        }
        else
        {
            // If the file doesn't exist, but the directory does, just open the directory
            AzQtComponents::ShowFileOnDesktop(logDir.absolutePath());
        }
    });
    // Only open and show the folder if the file actually exists, otherwise it's confusing
    openLogFolder->setEnabled(fileInfo.exists());

    menu.exec(ui->jobTreeView->viewport()->mapToGlobal(pos));
}

void MainWindow::SelectJobAndMakeVisible(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }
    // Make sure the job is visible, clear any existing filters.
    // This has to be done before getting the filter index, because it will change.
    ui->jobFilteredSearchWidget->ClearTextFilter();
    ui->jobFilteredSearchWidget->ClearTypeFilter();

    ui->dialogStack->setCurrentIndex(static_cast<int>(DialogStackIndex::Jobs));
    ui->buttonList->setCurrentIndex(static_cast<int>(DialogStackIndex::Jobs));
    QModelIndex proxyIndex = m_jobSortFilterProxy->mapFromSource(index);
    ui->jobTreeView->scrollTo(proxyIndex, QAbstractItemView::ScrollHint::EnsureVisible);
    // This isn't an asset tree, but use the same selection mode when selecting this row.
    // Setting the current index works a bit better than just selecting, because the item will be treated as
    // active for purposes of keyboard navigation and additional row highlighting (if the tree view itself gains focus)
    ui->jobTreeView->selectionModel()->setCurrentIndex(proxyIndex, AssetProcessor::AssetTreeModel::GetAssetTreeSelectionFlags());
}

void MainWindow::ShowSourceAssetContextMenu(const QPoint& pos)
{
    using namespace AssetProcessor;
    auto sourceAt = [this](const QPoint& pos)
    {
        const QModelIndex proxyIndex = ui->SourceAssetsTreeView->indexAt(pos);
        const QModelIndex sourceIndex = m_sourceAssetTreeFilterModel->mapToSource(proxyIndex);
        return static_cast<AssetTreeItem*>(sourceIndex.internalPointer());
    };

    const AssetTreeItem* cachedAsset = sourceAt(pos);

    if (!cachedAsset)
    {
        return;
    }
    QMenu menu(this);
    menu.setToolTipsVisible(true);
    const AZStd::shared_ptr<const SourceAssetTreeItemData> sourceItemData = AZStd::rtti_pointer_cast<const SourceAssetTreeItemData>(cachedAsset->GetData());


    QString jobMenuText(tr("View job..."));
    QString productMenuText(tr("View product asset..."));
    if (cachedAsset->getChildCount() > 0)
    {
        // Tooltips don't appear for disabled menus, so if this is a folder, create it as an action instead.
        QAction* jobAction = menu.addAction(jobMenuText);
        jobAction->setDisabled(true);
        jobAction->setToolTip(tr("Folders do not have associated jobs."));

        QAction* productAction = menu.addAction(productMenuText);
        productAction->setDisabled(true);
        productAction->setToolTip(tr("Folders do not have associated products."));
    }
    else
    {
        QMenu* jobMenu = menu.addMenu(jobMenuText);
        jobMenu->setToolTipsVisible(true);
        QMenu* productMenu = menu.addMenu(productMenuText);
        productMenu->setToolTipsVisible(true);

        m_sharedDbConnection->QueryJobBySourceID(sourceItemData->m_sourceInfo.m_sourceID,
            [&](AzToolsFramework::AssetDatabase::JobDatabaseEntry& jobEntry)
        {
            QAction* jobAction = jobMenu->addAction(tr("with key %1 for platform %2").arg(jobEntry.m_jobKey.c_str(), jobEntry.m_platform.c_str()), this, [&, jobEntry]()
            {
                QModelIndex jobIndex = m_jobsModel->GetJobFromSourceAndJobInfo(sourceItemData->m_assetDbName, jobEntry.m_platform, jobEntry.m_jobKey);
                SelectJobAndMakeVisible(jobIndex);
            });
            jobAction->setToolTip(tr("Show this job in the Jobs tab."));

            m_sharedDbConnection->QueryProductByJobID(
                jobEntry.m_jobID,
                [&](AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry)
            {
                if (productEntry.m_productName.empty())
                {
                    return true;
                }
                QAction* productAction = productMenu->addAction(productEntry.m_productName.c_str(), this, [&, productEntry]()
                {
                    ui->sourceAssetDetailsPanel->GoToProduct(productEntry.m_productName);
                });
                productAction->setToolTip("Show this product in the product assets tab.");
                return true; // Keep iterating, add all products.
            });
            return true; // Stop iterating, there should only be one job with this run key.
        });
    }

    QAction* fileBrowserAction = menu.addAction(AzQtComponents::fileBrowserActionName(), this, [&]()
    {
        AZ::Outcome<QString> pathToSource = GetAbsolutePathToSource(*cachedAsset);
        if (pathToSource.IsSuccess())
        {
            AzQtComponents::ShowFileOnDesktop(pathToSource.GetValue());
        }
    });
    QString fileOrFolder(cachedAsset->getChildCount() > 0 ? tr("folder") : tr("file"));
    fileBrowserAction->setToolTip(tr("Opens a window in your operating system's file explorer to view this %1.").arg(fileOrFolder));

    QAction* copyFullPathAction = menu.addAction(tr("Copy full path"), this, [&]()
    {
        AZ::Outcome<QString> pathToSource = GetAbsolutePathToSource(*cachedAsset);
        if (pathToSource.IsSuccess())
        {
            QGuiApplication::clipboard()->setText(pathToSource.GetValue());
        }
    });

    copyFullPathAction->setToolTip(tr("Copies the full path to this file to your clipboard."));

    QString reprocessFolder{ tr("Reprocess Folder") };
    QString reprocessFile{ tr("Reprocess File") };

    QAction* reprocessAssetAction = menu.addAction(cachedAsset->getChildCount() ? reprocessFolder : reprocessFile, this, [&]()
    {
        AZ::Outcome<QString> pathToSource = GetAbsolutePathToSource(*cachedAsset);
        m_guiApplicationManager->GetAssetProcessorManager()->RequestReprocess(pathToSource.GetValue());
    });

    QString reprocessFolderTip{ tr("Put the source assets in the selected folder back in the processing queue") };
    QString reprocessFileTip{ tr("Put the source asset back in the processing queue") };

    reprocessAssetAction->setToolTip(cachedAsset->getChildCount() ? reprocessFolderTip : reprocessFileTip);

    menu.exec(ui->SourceAssetsTreeView->viewport()->mapToGlobal(pos));
}

void MainWindow::ShowProductAssetContextMenu(const QPoint& pos)
{
    using namespace AssetProcessor;
    auto productAt = [this](const QPoint& pos)
    {
        const QModelIndex proxyIndex = ui->ProductAssetsTreeView->indexAt(pos);
        const QModelIndex sourceIndex = m_productAssetTreeFilterModel->mapToSource(proxyIndex);
        return static_cast<AssetTreeItem*>(sourceIndex.internalPointer());
    };

    const AssetTreeItem* cachedAsset = productAt(pos);

    if (!cachedAsset)
    {
        return;
    }

    QMenu menu(this);
    menu.setToolTipsVisible(true);
    const AZStd::shared_ptr<const ProductAssetTreeItemData> productItemData = AZStd::rtti_pointer_cast<const ProductAssetTreeItemData>(cachedAsset->GetData());

    QAction* jobAction = menu.addAction("View job", this, [&]()
    {
        if (!productItemData)
        {
            return;
        }

        QModelIndex jobIndex = m_jobsModel->GetJobFromProduct(productItemData->m_databaseInfo, *m_sharedDbConnection);
        SelectJobAndMakeVisible(jobIndex);
    });

    QAction* sourceAssetAction = menu.addAction("View source asset", this, [&]()
    {
        if (!productItemData)
        {
            return;
        }
        m_sharedDbConnection->QuerySourceByProductID(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
        {
            ui->sourceAssetDetailsPanel->GoToSource(sourceEntry.m_sourceName);
            return false; // Don't keep iterating
        });
    });

    if (cachedAsset->getChildCount() > 0)
    {
        sourceAssetAction->setDisabled(true);
        sourceAssetAction->setToolTip(tr("Folders do not have source assets."));
        jobAction->setDisabled(true);
        jobAction->setToolTip(tr("Folders do not have associated jobs."));
    }
    else
    {
        sourceAssetAction->setToolTip(tr("Selects the source asset associated with this product asset."));
        jobAction->setToolTip(tr("Selects the job that created this product asset in the Jobs tab."));
    }

    QAction* fileBrowserAction = menu.addAction(AzQtComponents::fileBrowserActionName(), this, [&]()
    {
        AZ::Outcome<QString> pathToProduct = GetAbsolutePathToProduct(*cachedAsset);
        if (pathToProduct.IsSuccess())
        {
            AzQtComponents::ShowFileOnDesktop(pathToProduct.GetValue());
        }
        
    });

    QString fileOrFolder(cachedAsset->getChildCount() > 0 ? tr("folder") : tr("file"));
    fileBrowserAction->setToolTip(tr("Opens a window in your operating system's file explorer to view this %1.").arg(fileOrFolder));

    QAction* copyFullPathAction = menu.addAction(tr("Copy full path"), this, [&]()
    {
        AZ::Outcome<QString> pathToProduct = GetAbsolutePathToProduct(*cachedAsset);
        if (pathToProduct.IsSuccess())
        {
            QGuiApplication::clipboard()->setText(pathToProduct.GetValue());
        }
    });

    copyFullPathAction->setToolTip(tr("Copies the full path for this %1 to your clipboard.").arg(fileOrFolder));

    QAction* sourceAssetReprocessAction = menu.addAction("Reprocess source asset", this, [&]()
    {
        if (!productItemData )
        {
            return;
        }
        m_sharedDbConnection->QuerySourceByProductID(
            productItemData->m_databaseInfo.m_productID,
            [&](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
        {
            m_sharedDbConnection->QueryScanFolderByScanFolderID(sourceEntry.m_scanFolderPK,[&] (AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry scanfolder)
            {
                QString reprocessSource{ scanfolder.m_scanFolder.c_str() };
                reprocessSource.append("/");
                reprocessSource.append(sourceEntry.m_sourceName.c_str());
                m_guiApplicationManager->GetAssetProcessorManager()->RequestReprocess(reprocessSource);
                return false; // Don't keep iterating
            });
            return false; // Don't keep iterating
        });
    });
    if (cachedAsset->getChildCount() > 0)
    {
        sourceAssetReprocessAction->setDisabled(true);
    }
    sourceAssetReprocessAction->setToolTip(tr("Reprocess the source asset which created this product"));

    menu.exec(ui->ProductAssetsTreeView->viewport()->mapToGlobal(pos));
}

void MainWindow::ShowLogLineContextMenu(const QPoint& pos)
{
    using namespace AzToolsFramework::Logging;
    QModelIndex sourceIndex = ui->jobContextLogTableView->indexAt(pos);

    // If there is no index under the mouse cursor, let check the selected index of the view
    if (!sourceIndex.isValid())
    {
        const auto indexes = ui->jobContextLogTableView->selectionModel()->selectedIndexes();

        if (!indexes.isEmpty())
        {
            sourceIndex = indexes.first();
        }
    }

    QMenu menu;
    QAction* key = menu.addAction(tr("Copy selected key"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.sibling(sourceIndex.row(), ContextDetailsLogTableModel::ColumnKey).data().toString());
    });
    QAction* value = menu.addAction(tr("Copy selected value"), this, [&]()
    {
        QGuiApplication::clipboard()->setText(sourceIndex.sibling(sourceIndex.row(), ContextDetailsLogTableModel::ColumnValue).data().toString());
    });
    menu.addAction(tr("Copy all values"), this, [&]()
    {
        auto model = qobject_cast<ContextDetailsLogTableModel*>(ui->jobContextLogTableView->model());
        QGuiApplication::clipboard()->setText(model->toString());
    });

    if (!sourceIndex.isValid())
    {
        key->setEnabled(false);
        value->setEnabled(false);
    }

    menu.exec(ui->jobContextLogTableView->viewport()->mapToGlobal(pos));
}

